// Video playback for .bik files using FFmpeg + ge_* rendering API + OpenAL.
//
// Renders decoded frames via ge_video_* (backend-agnostic).
// Streams audio through OpenAL buffer queue.
// A/V sync: wall clock is master, video waits by pts, audio streams independently.

#include "engine/video/video_player.h"
#include "engine/graphics/graphics_engine/game_graphics_engine.h"
#include "engine/io/env.h"
#include "engine/platform/host.h"
#include "engine/platform/host_globals.h" // ShellActive
#include "engine/platform/sdl3_bridge.h"
#include "engine/platform/ds_bridge.h"
#include "engine/input/input_frame.h" // input_key_just_pressed / input_btn_just_pressed
#include "engine/input/mouse_capture.h" // mouse_capture_set_suppressed
#include "game/action_map/act_cinematic.h" // ACT_CINE_VIDEO_SKIP_*
#include "game/game_globals.h" // RENDER_FPS_DEFAULT_CAP, g_render_fps_cap
#include <SDL3/SDL.h>

#include <AL/al.h>
#include <AL/alc.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
}

#include <cstdio>
#include <cstring>

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------

static constexpr int AUDIO_QUEUE_BUFFERS = 4;
static constexpr int AUDIO_BUFFER_SAMPLES = 4096; // samples per buffer

// ---------------------------------------------------------------------------
// OpenAL audio streaming
// ---------------------------------------------------------------------------

struct AudioStream {
    ALuint source;
    ALuint buffers[AUDIO_QUEUE_BUFFERS];
    int sample_rate;
    int channels;
    ALenum format;
    bool playing;
    int next_buf; // next buffer slot to fill before playing

    // Accumulation buffer — never drops data
    int16_t* accum;
    int accum_count; // samples (per channel) stored
    int accum_cap; // capacity in samples (per channel)
};

static bool audio_init(AudioStream* a, int sample_rate, int channels)
{
    memset(a, 0, sizeof(*a));
    a->sample_rate = sample_rate;
    a->channels = channels;
    a->format = (channels == 1) ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;

    alGenSources(1, &a->source);
    if (!a->source)
        return false;

    alGenBuffers(AUDIO_QUEUE_BUFFERS, a->buffers);

    // Non-spatialized: direct stereo output
    alSourcei(a->source, AL_SOURCE_RELATIVE, AL_TRUE);
    alSource3f(a->source, AL_POSITION, 0.0f, 0.0f, 0.0f);
    // Volume follows the "Volume" (fx) slider from Sound menu.
    float gain = (float)ENV_get_value_number("fx_volume", 127, "Audio") / 127.0f;
    alSourcef(a->source, AL_GAIN, gain);

    // Initial accumulator
    a->accum_cap = AUDIO_BUFFER_SAMPLES * AUDIO_QUEUE_BUFFERS * 2;
    a->accum = (int16_t*)av_malloc(a->accum_cap * channels * sizeof(int16_t));

    return true;
}

// Append decoded samples to accumulation buffer.
static void audio_accumulate(AudioStream* a, const int16_t* samples, int count)
{
    // Grow if needed
    if (a->accum_count + count > a->accum_cap) {
        int new_cap = (a->accum_count + count) * 2;
        int16_t* new_buf = (int16_t*)av_malloc(new_cap * a->channels * sizeof(int16_t));
        if (a->accum_count > 0)
            memcpy(new_buf, a->accum, a->accum_count * a->channels * sizeof(int16_t));
        av_free(a->accum);
        a->accum = new_buf;
        a->accum_cap = new_cap;
    }
    memcpy(a->accum + a->accum_count * a->channels, samples, count * a->channels * sizeof(int16_t));
    a->accum_count += count;
}

// Fill one OpenAL buffer from the front of the accumulation buffer.
// Returns true if a buffer was filled.
static bool audio_fill_one_buffer(AudioStream* a, ALuint buf)
{
    if (a->accum_count <= 0)
        return false;

    int n = a->accum_count > AUDIO_BUFFER_SAMPLES ? AUDIO_BUFFER_SAMPLES : a->accum_count;
    alBufferData(buf, a->format, a->accum, n * a->channels * sizeof(int16_t), a->sample_rate);
    alSourceQueueBuffers(a->source, 1, &buf);

    // Shift remaining data forward
    a->accum_count -= n;
    if (a->accum_count > 0)
        memmove(a->accum, a->accum + n * a->channels, a->accum_count * a->channels * sizeof(int16_t));

    return true;
}

// Flush accumulated audio into OpenAL buffers. Call every frame.
static void audio_flush(AudioStream* a)
{
    if (!a->playing) {
        // Pre-start: fill initial buffer slots
        while (a->next_buf < AUDIO_QUEUE_BUFFERS && a->accum_count > 0) {
            if (!audio_fill_one_buffer(a, a->buffers[a->next_buf]))
                break;
            a->next_buf++;
        }
        // Start when we have enough buffers
        if (a->next_buf >= 2) {
            alSourcePlay(a->source);
            a->playing = true;
        }
        return;
    }

    // Post-start: recycle processed buffers
    ALint processed = 0;
    alGetSourcei(a->source, AL_BUFFERS_PROCESSED, &processed);
    while (processed > 0 && a->accum_count > 0) {
        ALuint buf;
        alSourceUnqueueBuffers(a->source, 1, &buf);
        audio_fill_one_buffer(a, buf);
        processed--;
    }

    // Recover from underrun
    ALint state;
    alGetSourcei(a->source, AL_SOURCE_STATE, &state);
    if (state != AL_PLAYING) {
        alSourcePlay(a->source);
    }
}

static void audio_destroy(AudioStream* a)
{
    if (a->source) {
        alSourceStop(a->source);
        ALint queued = 0;
        alGetSourcei(a->source, AL_BUFFERS_QUEUED, &queued);
        while (queued-- > 0) {
            ALuint buf;
            alSourceUnqueueBuffers(a->source, 1, &buf);
        }
        alDeleteBuffers(AUDIO_QUEUE_BUFFERS, a->buffers);
        alDeleteSources(1, &a->source);
        a->source = 0;
    }
    av_free(a->accum);
    a->accum = nullptr;
}

// ---------------------------------------------------------------------------
// Main playback
// ---------------------------------------------------------------------------

bool video_play(const char* filename, bool allow_skip)
{
    // Videos aren't gameplay — suppress mouse capture for the
    // playback's duration. set_suppressed releases any active capture
    // AND blocks the engage path while it's set, so clicks landing in
    // the window during a cutscene don't engage the camera path even
    // though SDL events now go through the regular on_mouse_button
    // dispatch.
    mouse_capture_set_suppressed(true);

    // Open file
    AVFormatContext* fmt_ctx = nullptr;
    if (avformat_open_input(&fmt_ctx, filename, nullptr, nullptr) < 0) {
        fprintf(stderr, "[video] cannot open: %s\n", filename);
        return false;
    }
    if (avformat_find_stream_info(fmt_ctx, nullptr) < 0) {
        fprintf(stderr, "[video] cannot find stream info: %s\n", filename);
        avformat_close_input(&fmt_ctx);
        return false;
    }

    // Find video and audio streams
    int video_idx = -1, audio_idx = -1;
    for (unsigned i = 0; i < fmt_ctx->nb_streams; i++) {
        if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO && video_idx < 0)
            video_idx = (int)i;
        if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO && audio_idx < 0)
            audio_idx = (int)i;
    }
    if (video_idx < 0) {
        fprintf(stderr, "[video] no video stream in: %s\n", filename);
        avformat_close_input(&fmt_ctx);
        return false;
    }

    // Open video decoder
    const AVCodec* vcodec = avcodec_find_decoder(fmt_ctx->streams[video_idx]->codecpar->codec_id);
    AVCodecContext* vctx = avcodec_alloc_context3(vcodec);
    avcodec_parameters_to_context(vctx, fmt_ctx->streams[video_idx]->codecpar);
    avcodec_open2(vctx, vcodec, nullptr);

    // Open audio decoder (if present)
    const AVCodec* acodec = nullptr;
    AVCodecContext* actx = nullptr;
    SwrContext* swr = nullptr;
    AudioStream audio = {};
    bool has_audio = false;

    if (audio_idx >= 0) {
        acodec = avcodec_find_decoder(fmt_ctx->streams[audio_idx]->codecpar->codec_id);
        if (acodec) {
            actx = avcodec_alloc_context3(acodec);
            avcodec_parameters_to_context(actx, fmt_ctx->streams[audio_idx]->codecpar);
            if (avcodec_open2(actx, acodec, nullptr) == 0) {
                // Set up resampler: convert to S16 interleaved
                AVChannelLayout out_layout;
                if (actx->ch_layout.nb_channels >= 2)
                    av_channel_layout_default(&out_layout, 2);
                else
                    av_channel_layout_default(&out_layout, 1);

                swr_alloc_set_opts2(&swr,
                    &out_layout, AV_SAMPLE_FMT_S16, actx->sample_rate,
                    &actx->ch_layout, actx->sample_fmt, actx->sample_rate,
                    0, nullptr);
                swr_init(swr);

                has_audio = audio_init(&audio, actx->sample_rate, out_layout.nb_channels);
                av_channel_layout_uninit(&out_layout);
            }
        }
    }

    // Pixel format converter: whatever Bink outputs → RGB24
    SwsContext* sws = sws_getContext(
        vctx->width, vctx->height, vctx->pix_fmt,
        vctx->width, vctx->height, AV_PIX_FMT_RGB24,
        SWS_BILINEAR, nullptr, nullptr, nullptr);

    // Allocate RGB frame buffer
    int rgb_linesize = vctx->width * 3;
    // Align to 32 bytes for sws_scale
    rgb_linesize = (rgb_linesize + 31) & ~31;
    uint8_t* rgb_data = (uint8_t*)av_malloc(rgb_linesize * vctx->height);

    // Create video texture via graphics engine abstraction
    GEVideoTexture tex = ge_video_texture_create(vctx->width, vctx->height);

    // Playback loop
    AVPacket* pkt = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();
    bool done = false;
    bool ok = true;
    double video_timebase = av_q2d(fmt_ctx->streams[video_idx]->time_base);
    uint64_t start_ticks = sdl3_get_ticks(); // wall clock = master for everything

    // Audio sample accumulation buffer
    int16_t* audio_buf = nullptr;
    int audio_buf_cap = 0;

    // Calibrate input_frame snapshot with one update so any buttons / keys
    // held coming into the video don't fire as "rising edges" on the first
    // input_frame_update inside the loop. After this call, prev=curr — the
    // first real edge happens only on a fresh press during playback.
    ds_poll_registry(0.0f);
    ds_update_input();
    input_frame_update();

    while (!done && ShellActive) {
        // Drain events via the shared sdl3_poll_events path so every
        // callback (focus, mouse, keyboard, window close, resize)
        // fires the same way as during the regular main loop. The
        // capture module is suppressed for the playback duration
        // (see mouse_capture_set_suppressed above), so on_mouse_button
        // callbacks don't try to engage during a cutscene. on_close
        // sets ShellActive=FALSE which the loop condition catches.
        if (!sdl3_poll_events())
            ShellActive = UC_FALSE;

        host_process_pending_resize();
        // Refresh DualSense + gamepad + keyboard snapshot AFTER events
        // are dispatched so the snapshot reflects this iteration.
        ds_poll_registry(0.033f);
        ds_update_input();
        input_frame_update();

        // --- Skip on any user input ---
        if (allow_skip) {
            if (input_key_just_pressed(ACT_CINE_VIDEO_SKIP_1_KKEY)
                || input_key_just_pressed(ACT_CINE_VIDEO_SKIP_2_KKEY)
                || input_key_just_pressed(ACT_CINE_VIDEO_SKIP_3_KKEY)) {
                done = true;
            }
            // Any common gamepad button rising edge = skip — a "any button"
            // wildcard, not a single binding (so no ACT_*_GBTN constant; see
            // act_cinematic.h). The scan bound is the named device-vocabulary
            // count GBTN_COMMON_COUNT rather than a magic 17.
            if (!done) {
                for (int i = 0; i < GBTN_COMMON_COUNT; i++) {
                    if (input_btn_just_pressed(i)) {
                        done = true;
                        break;
                    }
                }
            }
        }
        if (done)
            break;

        // --- Flush accumulated audio into OpenAL buffers ---
        if (has_audio)
            audio_flush(&audio);

        // --- Read and decode packets ---
        int ret = av_read_frame(fmt_ctx, pkt);
        if (ret < 0) {
            done = true;
            break;
        }

        if (pkt->stream_index == video_idx) {
            avcodec_send_packet(vctx, pkt);
            while (avcodec_receive_frame(vctx, frame) == 0) {
                // Wall-clock-driven presentation. Three cases:
                //  1. pts > elapsed → frame is in the future, wait for it.
                //  2. pts < elapsed - DROP_THRESHOLD → frame is too late
                //     (renderer fell behind, e.g. slow GPU or debug
                //     `lock_frame_rate(g_render_fps_cap)` artificially
                //     throttling the outer loop). Skip drawing — keeps the
                //     video timeline tied to real time instead of to render
                //     cadence. Audio plays independently via the OpenAL
                //     accumulator, so no resync needed.
                //  3. otherwise → on time, draw.
                double pts = frame->pts * video_timebase;
                double elapsed = (double)(sdl3_get_ticks() - start_ticks) / 1000.0;
                // Half a 30-fps frame period — frames within this window of
                // current wall clock are considered "on time".
                static constexpr double DROP_THRESHOLD = 0.016;
                if (pts < elapsed - DROP_THRESHOLD) {
                    // Drop this decoded frame, move on to the next one.
                    continue;
                }
                if (pts > elapsed + 0.005) {
                    uint32_t wait_ms = (uint32_t)((pts - elapsed) * 1000.0);
                    if (wait_ms > 100)
                        wait_ms = 100; // safety cap
                    if (wait_ms > 0)
                        SDL_Delay(wait_ms);
                }

                // Convert to RGB
                uint8_t* dst_data[1] = { rgb_data };
                int dst_linesize[1] = { rgb_linesize };
                sws_scale(sws, frame->data, frame->linesize, 0, vctx->height,
                    dst_data, dst_linesize);

                // Upload and render via graphics engine
                ge_video_texture_upload(tex, vctx->width, vctx->height, rgb_data, rgb_linesize);
                ge_video_draw_and_swap(tex, vctx->width, vctx->height);
                extern void lock_frame_rate(SLONG fps);
                extern SLONG g_render_fps_cap;
                lock_frame_rate(g_render_fps_cap);
            }
        } else if (pkt->stream_index == audio_idx && has_audio) {
            avcodec_send_packet(actx, pkt);
            while (avcodec_receive_frame(actx, frame) == 0) {
                int out_samples = swr_get_out_samples(swr, frame->nb_samples);
                int needed = out_samples * audio.channels;
                if (needed > audio_buf_cap) {
                    av_free(audio_buf);
                    audio_buf_cap = needed;
                    audio_buf = (int16_t*)av_malloc(audio_buf_cap * sizeof(int16_t));
                }
                uint8_t* out_ptr = (uint8_t*)audio_buf;
                int converted = swr_convert(swr, &out_ptr, out_samples,
                    (const uint8_t**)frame->extended_data, frame->nb_samples);
                if (converted > 0) {
                    audio_accumulate(&audio, audio_buf, converted);
                }
            }
        }

        av_packet_unref(pkt);
    }

    // Cleanup
    av_free(audio_buf);
    av_frame_free(&frame);
    av_packet_free(&pkt);
    av_free(rgb_data);

    if (sws)
        sws_freeContext(sws);
    if (swr)
        swr_free(&swr);
    if (actx)
        avcodec_free_context(&actx);
    avcodec_free_context(&vctx);
    avformat_close_input(&fmt_ctx);

    if (has_audio)
        audio_destroy(&audio);

    ge_video_texture_destroy(tex);

    // Release the engage-suppression we set at the top — the regular
    // capture state machine takes over again on the next LibShellActive
    // tick (auto-engages if conditions are met, since suppression also
    // reset s_was_in_gameplay).
    mouse_capture_set_suppressed(false);

    return ok;
}

// ---------------------------------------------------------------------------
// High-level wrappers
// ---------------------------------------------------------------------------

// uc_orig: PlayQuickMovie / Display::RunFMV (fallen/DDLibrary/Source/GDisplay.cpp)
void video_play_intro(void)
{
    // Respect "play_movie" env setting (config.ini [Movie] section), same as original.
    if (!ENV_get_value_number((CBYTE*)"play_movie", 1, (CBYTE*)"Movie"))
        return;

    video_play("bink/Eidos.bik", true);
    video_play("bink/logo24.bik", true);
    video_play("bink/PCINTRO_withsound.bik", true);
}

// uc_orig: Display::RunCutscene (fallen/DDLibrary/Source/GDisplay.cpp)
void video_play_cutscene(int id)
{
    char path[256];
    snprintf(path, sizeof(path), "bink/New_PCcutscene%d_300.bik", id);
    video_play(path, true);
}
