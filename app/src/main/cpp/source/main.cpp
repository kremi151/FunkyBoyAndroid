/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Modifications copyright (C) 2020 Michel Kremer (kremi151)
 *
 */

//BEGIN_INCLUDE(all)
#include <initializer_list>
#include <memory>
#include <cstdlib>
#include <cstring>
#include <jni.h>
#include <cerrno>
#include <cassert>
#include <cmath>
#include <algorithm>
#include <ctime>
#include <fstream>

#include <android_native_app_glue.h>

#include <emulator/emulator.h>
#include <unistd.h>
#include <controllers/display_android.h>
#include <controllers/audio_android.h>
#include <fba_util/logging.h>
#include <fba_util/app_state.h>
#include <fba_util/emulator_state.h>
#include <fba_util/shared.h>
#include <engine/engine.h>
#include <engine/init_display.h>
#include <ui/draw_controls.h>
#include <ui/draw_text.h>
#include <util/frame_executor.h>
#include <util/membuf.h>
#include <fb_app_strings.h>

#include "fb_jni.h"

#define FBA_KEY_A 0b00000001
#define FBA_KEY_B 0b00000010
#define FBA_KEY_START 0b00000100
#define FBA_KEY_SELECT 0b00001000
#define FBA_KEY_LEFT 0b00010000
#define FBA_KEY_UP 0b00100000
#define FBA_KEY_RIGHT 0b01000000
#define FBA_KEY_DOWN 0b10000000

using namespace FunkyBoyAndroid;

int fbMsgPipe[2];
struct timeval tp;

namespace FunkyBoyAndroid::State {
    std::unique_ptr<FunkyBoy::Emulator> emulator;
    std::shared_ptr<FunkyBoy::Controller::DisplayController> emuDisplayController;
    std::shared_ptr<FunkyBoy::Controller::AudioController> emuAudioController;

    bool initialSaveLoaded = false;
    std::string romPath;
}

struct {
    std::string noRomLoaded;
    std::string romNotReadable;
    std::string romNotParsable;
    std::string romTooBig;
    std::string romSizeMismatch;
    std::string unsupportedMBC;
    std::string unsupportedRAMSize;
    std::string unknownStatus;
    std::string pressStart;
} fb_strings;

namespace FunkyBoyAndroid {

    void reloadStrings(JNIEnv *env, ANativeActivity *nativeActivity) {
        fb_strings.noRomLoaded = FunkyBoyAndroid::R::getString(env, nativeActivity, FunkyBoyAndroid::R::String::no_rom_loaded);
        fb_strings.romNotReadable = FunkyBoyAndroid::R::getString(env, nativeActivity, FunkyBoyAndroid::R::String::rom_not_readable);
        fb_strings.romNotParsable = FunkyBoyAndroid::R::getString(env, nativeActivity, FunkyBoyAndroid::R::String::rom_not_parsable);
        fb_strings.romTooBig = FunkyBoyAndroid::R::getString(env, nativeActivity, FunkyBoyAndroid::R::String::rom_too_big);
        fb_strings.romSizeMismatch = FunkyBoyAndroid::R::getString(env, nativeActivity, FunkyBoyAndroid::R::String::rom_size_mismatch);
        fb_strings.unsupportedMBC = FunkyBoyAndroid::R::getString(env, nativeActivity, FunkyBoyAndroid::R::String::unsupported_mbc);
        fb_strings.unsupportedRAMSize = FunkyBoyAndroid::R::getString(env, nativeActivity, FunkyBoyAndroid::R::String::unsupported_ram_size);
        fb_strings.unknownStatus = FunkyBoyAndroid::R::getString(env, nativeActivity, FunkyBoyAndroid::R::String::unknown_status);
        fb_strings.pressStart = FunkyBoyAndroid::R::getString(env, nativeActivity, FunkyBoyAndroid::R::String::press_start);
    }

}

/**
 * Just the current frame in the display.
 */
static void engine_draw_frame(struct engine* engine) {
    ANativeWindow *window = engine->app->window;
    auto controller = dynamic_cast<FunkyBoyAndroid::Controller::DisplayControllerAndroid *>(FunkyBoyAndroid::State::emuDisplayController.get());

    if (FunkyBoyAndroid::State::emulator->getCartridgeStatus() == FunkyBoy::CartridgeStatus::Loaded) {
        if (!FunkyBoyAndroid::State::initialSaveLoaded) {
            loadSaveGame(engine);
        }
        controller->setWindow(window);
        FunkyBoy::ret_code retCode;
        do {
            retCode = FunkyBoyAndroid::State::emulator->doTick();
        } while ((retCode & FB_RET_NEW_FRAME) == 0);
        controller->setWindow(nullptr);
    } else {
        ANativeWindow_acquire(window);
        ANativeWindow_Buffer buffer;
        if (ANativeWindow_lock(window, &buffer, nullptr) < 0) {
            LOGW("Unable to lock native window");
            ANativeWindow_release(window);
            return;
        }

        // White background
        std::memset(buffer.bits, 255, buffer.stride * FB_GB_DISPLAY_HEIGHT * 4);

        const char *text;

        // Draw ROM status
        switch (FunkyBoyAndroid::State::emulator->getCartridgeStatus()) {
            case FunkyBoy::NoROMLoaded:
                text = fb_strings.noRomLoaded.c_str();
                break;
            case FunkyBoy::ROMFileNotReadable:
                text = fb_strings.romNotReadable.c_str();
                break;
            case FunkyBoy::ROMParseError:
                text = fb_strings.romNotParsable.c_str();
                break;
            case FunkyBoy::ROMTooBig:
                text = fb_strings.romTooBig.c_str();
                break;
            case FunkyBoy::ROMSizeMismatch:
                text = fb_strings.romSizeMismatch.c_str();
                break;
            case FunkyBoy::ROMUnsupportedMBC:
                text = fb_strings.unsupportedMBC.c_str();
                break;
            case FunkyBoy::RAMSizeUnsupported:
                text = fb_strings.unsupportedRAMSize.c_str();
                break;
            case FunkyBoy::Loaded:
                text = "";
                break;
            default:
                text = fb_strings.unknownStatus.c_str();
                break;
        }
        size_t text_width = measureTextWidth(text, 0);
        drawTextAt(engine->env, buffer, engine->bitmapFontsUppercase, text, 0, (FB_GB_DISPLAY_WIDTH - text_width) / 2, 32);

        // Draw headline
        gettimeofday(&tp, nullptr);
        if (tp.tv_sec % 2 == 1) {
            text = fb_strings.pressStart.c_str();
            text_width = measureTextWidth(text, 0);
            drawTextAt(engine->env, buffer, engine->bitmapFontsUppercase, text, 0, (FB_GB_DISPLAY_WIDTH - text_width) / 2, 110);
        }

        // Draw controls
        drawControls(engine, buffer);

        if (ANativeWindow_unlockAndPost(window) < 0) {
            LOGW("Unable to unlock and post to native window");
        }
        ANativeWindow_release(window);
    }
}

/**
 * Tear down the EGL context currently associated with the display.
 */
static void engine_term_display(struct engine* engine) {
    LOGD("engine_term_display");
    if (engine->bitmapButtons != nullptr) {
        engine->env->DeleteGlobalRef(engine->bitmapButtons);
        engine->bitmapButtons = nullptr;
    }
    if (engine->bitmapFontsUppercase != nullptr) {
        engine->env->DeleteGlobalRef(engine->bitmapFontsUppercase);
        engine->bitmapFontsUppercase = nullptr;
    }
    engine->animating = false;
}

static bool isTouched(const ui_obj &obj, float scaledX, float scaledY) {
    return scaledX >= obj.x && scaledX < obj.x + obj.width
        && scaledY >= obj.y && scaledY < obj.y + obj.height;
}

static void handleInputPointer(int index, AInputEvent* event, struct engine *engine, int &latch) {
    float scaledX = AMotionEvent_getX(event, index) * engine->uiScale;
    float scaledY = AMotionEvent_getY(event, index) * engine->uiScale;
    bool touched = isTouched(engine->keyA, scaledX, scaledY);
    if (touched) {
        latch |= FBA_KEY_A;
    }
    touched = isTouched(engine->keyB, scaledX, scaledY);
    if (touched) {
        latch |= FBA_KEY_B;
    }
    touched = isTouched(engine->keyLeft, scaledX, scaledY);
    if (touched) {
        latch |= FBA_KEY_LEFT;
    }
    touched = isTouched(engine->keyUp, scaledX, scaledY);
    if (touched) {
        latch |= FBA_KEY_UP;
    }
    touched = isTouched(engine->keyRight, scaledX, scaledY);
    if (touched) {
        latch |= FBA_KEY_RIGHT;
    }
    touched = isTouched(engine->keyDown, scaledX, scaledY);
    if (touched) {
        latch |= FBA_KEY_DOWN;
    }
    touched = isTouched(engine->keyStart, scaledX, scaledY);
    if (touched) {
        latch |= FBA_KEY_START;
    }
    touched = isTouched(engine->keySelect, scaledX, scaledY);
    if (touched) {
        latch |= FBA_KEY_SELECT;
    }
}

static int findPointerIndex(const AInputEvent* event, size_t id) {
    int count = AMotionEvent_getPointerCount(event);
    for (int i = 0; i < count; i++) {
        if (id == AMotionEvent_getPointerId(event, i)) {
            return i;
        }
    }
    return -1;
}

/**
 * Process the next input event.
 */
static int32_t engine_handle_input(struct android_app* app, AInputEvent* event) {
    if (AInputEvent_getType(event) != AINPUT_EVENT_TYPE_MOTION) {
        return 0;
    }
    auto* engine = (struct engine*)app->userData;
    int action = AMotionEvent_getAction(event);
    uint flags = action & AMOTION_EVENT_ACTION_MASK;

    if (FunkyBoyAndroid::State::emulator->getCartridgeStatus() != FunkyBoy::CartridgeStatus::Loaded) {
        if (flags == AMOTION_EVENT_ACTION_DOWN) {
            float scaledX = AMotionEvent_getX(event, 0) * engine->uiScale;
            float scaledY = AMotionEvent_getY(event, 0) * engine->uiScale;
            bool touched = isTouched(engine->keyStart, scaledX, scaledY);
            if (touched) {
                requestPickRom(engine);
            }
        }
        return 1;
    }

    auto &activePointerIds = engine->activePointerIds;
    switch (flags) {
        case AMOTION_EVENT_ACTION_DOWN:
            // For AMOTION_EVENT_ACTION_DOWN, only the primary pointer is involved
            if (activePointerIds.empty()) {
                // For some reason, it may happen that AMOTION_EVENT_ACTION_DOWN is called
                // sequentially without a AMOTION_EVENT_ACTION_UP.
                // With this we do some basic deduplication to avoid sticky inputs
                activePointerIds.push_back(AMotionEvent_getPointerId(event, 0));
            }
            break;
        case AMOTION_EVENT_ACTION_POINTER_DOWN: {
            uint32_t pointerIndex = (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
            activePointerIds.push_back(AMotionEvent_getPointerId(event, pointerIndex));
            break;
        }
        case AMOTION_EVENT_ACTION_UP:
            // FILO: First in, last out (primary pointer)
            if (!activePointerIds.empty()) {
                activePointerIds.pop_back();
            }
            break;
        case AMOTION_EVENT_ACTION_POINTER_UP: {
            uint pointerId = (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
            pointerId = AMotionEvent_getPointerId(event, pointerId);
            auto it = activePointerIds.begin();
            auto it_end = activePointerIds.end();
            for (; it != it_end; ++it) {
                if (*it == pointerId) {
                    activePointerIds.erase(it);
                    break;
                }
            }
            break;
        }
        default:
            break;
    }

    int keyLatch = 0;

    auto it = activePointerIds.begin();
    auto it_end = activePointerIds.end();
    for (; it != it_end; ++it) {
        auto pointerIndex = findPointerIndex(event, *it);
        if (pointerIndex != -1) {
            handleInputPointer(pointerIndex, event, engine, keyLatch);
        }
    }

    if (keyLatch != engine->keyLatch) {
        FunkyBoyAndroid::State::emulator->setInputState(FunkyBoy::Controller::JoypadKey::JOYPAD_A, keyLatch & FBA_KEY_A);
        FunkyBoyAndroid::State::emulator->setInputState(FunkyBoy::Controller::JoypadKey::JOYPAD_B, keyLatch & FBA_KEY_B);
        FunkyBoyAndroid::State::emulator->setInputState(FunkyBoy::Controller::JoypadKey::JOYPAD_START, keyLatch & FBA_KEY_START);
        FunkyBoyAndroid::State::emulator->setInputState(FunkyBoy::Controller::JoypadKey::JOYPAD_SELECT, keyLatch & FBA_KEY_SELECT);
        FunkyBoyAndroid::State::emulator->setInputState(FunkyBoy::Controller::JoypadKey::JOYPAD_LEFT, keyLatch & FBA_KEY_LEFT);
        FunkyBoyAndroid::State::emulator->setInputState(FunkyBoy::Controller::JoypadKey::JOYPAD_UP, keyLatch & FBA_KEY_UP);
        FunkyBoyAndroid::State::emulator->setInputState(FunkyBoy::Controller::JoypadKey::JOYPAD_RIGHT, keyLatch & FBA_KEY_RIGHT);
        FunkyBoyAndroid::State::emulator->setInputState(FunkyBoy::Controller::JoypadKey::JOYPAD_DOWN, keyLatch & FBA_KEY_DOWN);
        engine->keyLatch = keyLatch;
    }

    return 1;
}

/**
 * Process the next main command.
 */
static void engine_handle_cmd(struct android_app* app, int32_t cmd) {
    auto* engine = (struct engine*)app->userData;
    switch (cmd) {
        case APP_CMD_SAVE_STATE: {
            LOGD("CMD: APP_CMD_SAVE_STATE");
            // The system has asked us to save our current state.  Do so.
            auto *state = static_cast<app_save_state *>(calloc(
                    sizeof(FunkyBoyAndroid::app_save_state), sizeof(char)));

            FunkyBoyAndroid::serializeState(state);

            engine->app->savedState = state;
            engine->app->savedStateSize = sizeof(FunkyBoyAndroid::app_save_state);

            LOGD("Emulation state has been serialized");
            break;
        }
        case APP_CMD_INIT_WINDOW:
            // The window is being shown, get it ready.
            LOGD("CMD: APP_CMD_INIT_WINDOW");
            engine->activePointerIds.clear();
            if (engine->app->window != nullptr) {
                Engine::initDisplay(engine);
                engine_draw_frame(engine);
            }
            break;
        case APP_CMD_TERM_WINDOW:
            LOGD("CMD: APP_CMD_TERM_WINDOW");
            engine->activePointerIds.clear();
            // The window is being hidden or closed, clean it up.
            engine_term_display(engine);
            break;
        case APP_CMD_GAINED_FOCUS:
            LOGD("CMD: APP_CMD_GAINED_FOCUS");
            engine->activePointerIds.clear();
            // When our app gains focus, we start animating again.
            dynamic_cast<FunkyBoyAndroid::Controller::AudioControllerAndroid*>(FunkyBoyAndroid::State::emuAudioController.get())->setPlaying(true);
            engine->animating = true;
            break;
        case APP_CMD_LOST_FOCUS:
            LOGD("CMD: APP_CMD_LOST_FOCUS");
            engine->activePointerIds.clear();
            dynamic_cast<FunkyBoyAndroid::Controller::AudioControllerAndroid*>(FunkyBoyAndroid::State::emuAudioController.get())->setPlaying(false);
            engine->animating = false;
            engine_draw_frame(engine);
            break;
        default:
            break;
    }
}

namespace FunkyBoyAndroid {

    static int handleCustomMessage(int fd, int events, void *data) {
        size_t strln;
        read(fd, reinterpret_cast<char *>(&strln), sizeof(size_t));

        char *inRomPath = (char *) calloc(strln + 1, sizeof(char));
        read(fd, inRomPath, strln);

        LOGD("RECV rom path: %s", inRomPath);
        loadROM(inRomPath);

        State::initialSaveLoaded = false;

        free(inRomPath);
        return 1;
    }

}

/**
 * This is the main entry point of a native application that is using
 * android_native_app_glue.  It runs in its own thread, with its own
 * event loop for receiving input events and doing other things.
 */
void android_main(struct android_app* state) {
    struct engine engine{};

    memset(&engine, 0, sizeof(engine));
    state->userData = &engine;
    state->onAppCmd = engine_handle_cmd;
    state->onInputEvent = engine_handle_input;
    engine.app = state;

    JNIEnv *env = state->activity->env;
    JavaVM *jvm = state->activity->vm;

    JavaVMAttachArgs vmAttachArgs;
    vmAttachArgs.version = JNI_VERSION_1_6;
    vmAttachArgs.name = "FBNativeThread";
    vmAttachArgs.group = nullptr;
    auto result = jvm->AttachCurrentThread(&env, &vmAttachArgs);
    if (result == JNI_ERR) {
        LOGW("Could not attach native thread to JVM");
        return;
    }
    engine.env = env;

    FunkyBoyAndroid::reloadStrings(env, state->activity);

    FunkyBoyAndroid::State::emuDisplayController = std::make_shared<FunkyBoyAndroid::Controller::DisplayControllerAndroid>(&engine);
    FunkyBoyAndroid::State::emuAudioController = std::make_shared<FunkyBoyAndroid::Controller::AudioControllerAndroid>();

    FunkyBoyAndroid::State::emulator = std::make_unique<FunkyBoy::Emulator>(FunkyBoy::GameBoyType::GameBoyDMG);
    FunkyBoyAndroid::State::emulator->setControllers(FunkyBoy::Controller::Controllers()
            .withDisplay(FunkyBoyAndroid::State::emuDisplayController)
            .withAudio(FunkyBoyAndroid::State::emuAudioController));

    if (state->savedState != nullptr) {
        // We are starting with a previous saved state; restore from it.
        auto savedState = static_cast<FunkyBoyAndroid::app_save_state*>(state->savedState);
        FunkyBoyAndroid::resumeFromState(savedState);
    }

    pipe(fbMsgPipe);
    ALooper_addFd(state->looper, fbMsgPipe[0], ALOOPER_POLL_CALLBACK , ALOOPER_EVENT_INPUT, handleCustomMessage, state);

    FunkyBoy::Util::FrameExecutor executeFrame([&engine](){
        engine_draw_frame(&engine);
    }, FB_TARGET_FPS);

    while (true) {
        // Read all pending events.
        int ident;
        int events;
        struct android_poll_source* source;

        // If not animating, we will block forever waiting for events.
        // If animating, we loop until all events are read, then continue
        // to draw the next frame of animation.
        while ((ident=ALooper_pollAll(engine.animating ? 0 : -1, nullptr, &events,
                                      (void**)&source)) >= 0) {

            // Process this event.
            if (source != nullptr) {
                source->process(state, source);
            }

            // Check if we are exiting.
            if (state->destroyRequested != 0) {
                engine_term_display(&engine);
                return;
            }
        }

        if (engine.animating) {
            executeFrame();
        }
    }

    jvm->DetachCurrentThread();
}

//END_INCLUDE(all)
