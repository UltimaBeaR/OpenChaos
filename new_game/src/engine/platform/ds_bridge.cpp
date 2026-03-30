// DualSense bridge implementation — see ds_bridge.h for API.
//
// Platform HID layer uses SDL3's cross-platform hidapi (SDL_hid_*),
// adapted from Gamepad-Core-Tests linux_device_info (works on all platforms).

#include "engine/platform/ds_bridge.h"

#include <SDL3/SDL_hidapi.h>
#include <SDL3/SDL_hints.h>

#include <GCore/Interfaces/IPlatformHardwareInfo.h>
#include <GCore/Templates/TBasicDeviceRegistry.h>
#include <GCore/Templates/TGenericHardwareInfo.h>
#include <GCore/Types/ECoreGamepad.h>
#include <GCore/Types/Structs/Context/DeviceContext.h>
#include <GImplementations/Utils/GamepadSensors.h>
#include <GImplementations/Libraries/DualSense/DualSenseLibrary.h>
#include <GCore/Interfaces/Segregations/IGamepadTrigger.h>

#include <cstring>
#include <unordered_set>

// ---------------------------------------------------------------------------
// Sony vendor/product IDs
// ---------------------------------------------------------------------------

static constexpr uint16_t SONY_VENDOR_ID       = 0x054C;
static constexpr uint16_t DUALSHOCK4_PID_V1    = 0x05C4;
static constexpr uint16_t DUALSHOCK4_PID_V2    = 0x09CC;
static constexpr uint16_t DUALSENSE_PID        = 0x0CE6;
static constexpr uint16_t DUALSENSE_EDGE_PID   = 0x0DF2;

// ===========================================================================
// HID platform layer via SDL3 hidapi (cross-platform)
// ===========================================================================

namespace uc_hid {

static void read(FDeviceContext* ctx)
{
    if (!ctx || !ctx->Handle) return;
    auto* dev = static_cast<SDL_hid_device*>(ctx->Handle);

    if (ctx->ConnectionType == EDSDeviceConnection::Bluetooth &&
        ctx->DeviceType == EDSDeviceType::DualShock4)
    {
        SDL_hid_read(dev, ctx->BufferDS4, 547);
        return;
    }

    const size_t len = (ctx->ConnectionType == EDSDeviceConnection::Bluetooth) ? 78 : 64;
    int result = SDL_hid_read(dev, ctx->Buffer, len);
    if (result < 0) {
        // Disconnected
        SDL_hid_close(dev);
        ctx->Handle = INVALID_PLATFORM_HANDLE;
        ctx->IsConnected = false;
    }
}

static void write(FDeviceContext* ctx)
{
    if (!ctx || !ctx->Handle) return;
    auto* dev = static_cast<SDL_hid_device*>(ctx->Handle);

    const size_t base_len = (ctx->DeviceType == EDSDeviceType::DualShock4) ? 32 : 74;
    const size_t len = (ctx->ConnectionType == EDSDeviceConnection::Bluetooth) ? 78 : base_len;

    int result = SDL_hid_write(dev, ctx->GetRawOutputBuffer(), len);
    if (result < 0) {
        SDL_hid_close(dev);
        ctx->Handle = INVALID_PLATFORM_HANDLE;
        ctx->IsConnected = false;
    }
}

static void detect(std::vector<FDeviceContext>& devices)
{
    devices.clear();

    static const std::unordered_set<uint16_t> supported = {
        DUALSHOCK4_PID_V1, DUALSHOCK4_PID_V2,
        DUALSENSE_PID, DUALSENSE_EDGE_PID
    };

    SDL_hid_device_info* devs = SDL_hid_enumerate(SONY_VENDOR_ID, 0);
    if (!devs) return;

    for (auto* cur = devs; cur; cur = cur->next) {
        if (!supported.contains(cur->product_id)) continue;

        FDeviceContext dc;
        dc.Path = std::string(cur->path);
        dc.IsConnected = true;
        dc.Handle = nullptr;

        switch (cur->product_id) {
            case DUALSHOCK4_PID_V1:
            case DUALSHOCK4_PID_V2:
                dc.DeviceType = EDSDeviceType::DualShock4;
                break;
            case DUALSENSE_EDGE_PID:
                dc.DeviceType = EDSDeviceType::DualSenseEdge;
                break;
            default:
                dc.DeviceType = EDSDeviceType::DualSense;
                break;
        }

        // interface_number == -1 typically means Bluetooth
        dc.ConnectionType = (cur->interface_number == -1)
            ? EDSDeviceConnection::Bluetooth
            : EDSDeviceConnection::Usb;

        devices.push_back(dc);
    }
    SDL_hid_free_enumeration(devs);
}

static bool create_handle(FDeviceContext* ctx)
{
    if (!ctx) return false;

    auto* dev = SDL_hid_open_path(ctx->Path.c_str());
    if (!dev) return false;

    SDL_hid_set_nonblocking(dev, 1);
    ctx->Handle = static_cast<FPlatformDeviceHandle>(dev);

    // Read calibration data (feature report 0x05)
    unsigned char feat[41] = {};
    feat[0] = 0x05;
    if (SDL_hid_get_feature_report(dev, feat, 41)) {
        FGamepadCalibration cal;
        FGamepadSensors::DualSenseCalibrationSensors(feat, cal);
        ctx->Calibration = cal;
    }

    return true;
}

static void invalidate_handle(FDeviceContext* ctx)
{
    if (!ctx) return;
    if (ctx->Handle) {
        SDL_hid_close(static_cast<SDL_hid_device*>(ctx->Handle));
    }
    ctx->Handle = INVALID_PLATFORM_HANDLE;
    ctx->IsConnected = false;
    ctx->Path.clear();
    std::memset(ctx->Buffer, 0, sizeof(ctx->Buffer));
    std::memset(ctx->BufferDS4, 0, sizeof(ctx->BufferDS4));
    std::memset(ctx->BufferAudio, 0, sizeof(ctx->BufferAudio));
    std::memset(ctx->GetRawOutputBuffer(), 0, 78);
}

static void process_audio_haptic(FDeviceContext* ctx)
{
    if (!ctx || !ctx->Handle) return;
    auto* dev = static_cast<SDL_hid_device*>(ctx->Handle);

    if (ctx->ConnectionType == EDSDeviceConnection::Bluetooth) {
        int written = SDL_hid_write(dev, ctx->BufferAudio, 147);
        if (written < 0) {
            SDL_hid_write(dev, ctx->BufferAudio, 78);
        }
    }
}

static void initialize_audio_device(FDeviceContext* /*ctx*/)
{
    // No-op for now (audio-to-haptic is step B7, deferred)
}

// Policy struct for TGenericHardwareInfo
struct sdl_hid_policy {
    void Read(FDeviceContext* ctx)               { uc_hid::read(ctx); }
    void Write(FDeviceContext* ctx)              { uc_hid::write(ctx); }
    void Detect(std::vector<FDeviceContext>& d)  { uc_hid::detect(d); }
    bool CreateHandle(FDeviceContext* ctx)        { return uc_hid::create_handle(ctx); }
    void InvalidateHandle(FDeviceContext* ctx)    { uc_hid::invalidate_handle(ctx); }
    void ProcessAudioHaptic(FDeviceContext* ctx)  { uc_hid::process_audio_haptic(ctx); }
    void InitializeAudioDevice(FDeviceContext* ctx) { uc_hid::initialize_audio_device(ctx); }
};

using sdl_hardware = GamepadCore::TGenericHardwareInfo<sdl_hid_policy>;

} // namespace uc_hid

// ===========================================================================
// Device registry policy for our engine
// ===========================================================================

struct UCRegistryPolicy {
    using EngineIdType = uint32_t;
    struct Hasher {
        size_t operator()(uint32_t id) const { return std::hash<uint32_t>{}(id); }
    };

    static uint32_t AllocEngineDevice() {
        static uint32_t next_id = 0;
        return next_id++;
    }
    static void DisconnectDevice(uint32_t /*id*/) {}
    static void DispatchNewGamepad(uint32_t /*id*/) {}
};

// ===========================================================================
// Module state
// ===========================================================================

static GamepadCore::TBasicDeviceRegistry<UCRegistryPolicy>* s_registry = nullptr;
static uint32_t s_active_device_id = 0;
static bool s_has_device = false;
static bool s_hid_initialized = false;

// ===========================================================================
// Public API
// ===========================================================================

void ds_init()
{
    // Tell SDL3 to NOT grab DualSense via its gamepad subsystem —
    // we handle it ourselves via HID.
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_PS5, "0");

    // Initialize SDL HID subsystem
    if (SDL_hid_init() != 0) return;
    s_hid_initialized = true;

    // Register our SDL-based HID platform layer with GamepadCore
    IPlatformHardwareInfo::SetInstance(std::make_unique<uc_hid::sdl_hardware>());

    // Create device registry
    s_registry = new GamepadCore::TBasicDeviceRegistry<UCRegistryPolicy>();
}

void ds_shutdown()
{
    if (s_has_device && s_registry) {
        auto* gamepad = s_registry->GetLibrary(s_active_device_id);
        if (gamepad) gamepad->ShutdownLibrary();
    }

    delete s_registry;
    s_registry = nullptr;
    s_has_device = false;

    if (s_hid_initialized) {
        SDL_hid_exit();
        s_hid_initialized = false;
    }
}

void ds_poll_registry(float delta_time)
{
    if (!s_registry) return;
    s_registry->PlugAndPlay(delta_time);

    // Check if we have a device now
    if (!s_has_device) {
        // Try device ID 0 (first allocated)
        auto* gp = s_registry->GetLibrary(s_active_device_id);
        if (gp && gp->IsConnected()) {
            s_has_device = true;
        }
    } else {
        auto* gp = s_registry->GetLibrary(s_active_device_id);
        if (!gp || !gp->IsConnected()) {
            s_has_device = false;
        }
    }
}

bool ds_update_input(float delta_time)
{
    if (!s_has_device || !s_registry) return false;
    auto* gp = s_registry->GetLibrary(s_active_device_id);
    if (!gp) { s_has_device = false; return false; }

    gp->UpdateInput(delta_time);
    return true;
}

void ds_update_output()
{
    if (!s_has_device || !s_registry) return;
    auto* gp = s_registry->GetLibrary(s_active_device_id);
    if (gp) gp->UpdateOutput();
}

bool ds_is_connected()
{
    return s_has_device;
}

bool ds_get_input(DS_InputState* out)
{
    if (!s_has_device || !s_registry || !out) return false;

    auto* gp = s_registry->GetLibrary(s_active_device_id);
    if (!gp) return false;

    auto* ctx = gp->GetMutableDeviceContext();
    if (!ctx) return false;

    FInputContext* input = ctx->GetInputState();

    out->left_stick_x  = input->LeftAnalog.X;
    out->left_stick_y  = input->LeftAnalog.Y;
    out->right_stick_x = input->RightAnalog.X;
    out->right_stick_y = input->RightAnalog.Y;
    out->trigger_left  = input->LeftTriggerAnalog;
    out->trigger_right = input->RightTriggerAnalog;

    out->cross    = input->bCross;
    out->circle   = input->bCircle;
    out->square   = input->bSquare;
    out->triangle = input->bTriangle;

    out->l1 = input->bLeftShoulder;
    out->r1 = input->bRightShoulder;
    out->l2_digital = input->bLeftTriggerThreshold;
    out->r2_digital = input->bRightTriggerThreshold;

    out->dpad_up    = input->bDpadUp;
    out->dpad_down  = input->bDpadDown;
    out->dpad_left  = input->bDpadLeft;
    out->dpad_right = input->bDpadRight;

    out->start     = input->bStart;
    out->share     = input->bShare;
    out->ps_button = input->bPSButton;

    out->l3 = input->bLeftStick;
    out->r3 = input->bRightStick;

    out->mute          = input->bMute;
    out->touchpad_click = input->bTouch;

    return true;
}

// ---------------------------------------------------------------------------
// Output functions
// ---------------------------------------------------------------------------

static ISonyGamepad* ds_get_gamepad()
{
    if (!s_has_device || !s_registry) return nullptr;
    return s_registry->GetLibrary(s_active_device_id);
}

void ds_set_lightbar(uint8_t r, uint8_t g, uint8_t b)
{
    auto* gp = ds_get_gamepad();
    if (gp) gp->SetLightbar({r, g, b, 1});
}

void ds_set_player_led(uint8_t led_mask)
{
    auto* gp = ds_get_gamepad();
    if (gp) gp->SetPlayerLed(static_cast<EDSPlayer>(led_mask), 255);
}

void ds_set_vibration(uint8_t left, uint8_t right)
{
    auto* gp = ds_get_gamepad();
    if (gp) gp->SetVibration(left, right);
}

static EDSGamepadHand to_hand(uint8_t hand)
{
    switch (hand) {
        case 0: return EDSGamepadHand::Left;
        case 1: return EDSGamepadHand::Right;
        default: return EDSGamepadHand::AnyHand;
    }
}

static IGamepadTrigger* ds_get_triggers()
{
    auto* gp = ds_get_gamepad();
    return gp ? gp->GetIGamepadTrigger() : nullptr;
}

void ds_trigger_off(uint8_t hand)
{
    auto* t = ds_get_triggers();
    if (t) t->StopTrigger(to_hand(hand));
}

void ds_trigger_resistance(uint8_t start_zone, uint8_t strength, uint8_t hand)
{
    auto* t = ds_get_triggers();
    if (t) t->SetResistance(start_zone, strength, to_hand(hand));
}

void ds_trigger_weapon(uint8_t start_zone, uint8_t amplitude, uint8_t behavior, uint8_t trigger, uint8_t hand)
{
    auto* t = ds_get_triggers();
    if (t) t->SetWeapon25(start_zone, amplitude, behavior, trigger, to_hand(hand));
}

void ds_trigger_bow(uint8_t start_zone, uint8_t snap_back, uint8_t hand)
{
    auto* t = ds_get_triggers();
    if (t) t->SetBow22(start_zone, snap_back, to_hand(hand));
}

void ds_trigger_machine(uint8_t start_zone, uint8_t behavior_flag, uint8_t force,
                        uint8_t amplitude, uint8_t period, uint8_t frequency, uint8_t hand)
{
    auto* t = ds_get_triggers();
    if (t) t->SetMachine27(start_zone, behavior_flag, force, amplitude, period, frequency, to_hand(hand));
}
