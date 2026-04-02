#pragma once

// Video playback for .bik (Bink Video) files via FFmpeg.
// Renders fullscreen via OpenGL, audio via OpenAL streaming.

// Play a video file fullscreen. Blocks until video ends or user skips.
// Returns true if played successfully, false if file not found or decode error.
bool video_play(const char* filename, bool allow_skip);

// Play the intro FMV sequence (Eidos logo, Mucky Foot logo, intro).
// Respects "play_movie" env setting. Each video is skippable.
void video_play_intro(void);

// Play a numbered cutscene (1, 2, 3).
void video_play_cutscene(int id);
