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
#include <fba_util/logging.h>
#include <state/engine.h>
#include <ui/draw_controls.h>
#include <ui/draw_text.h>
#include <util/frame_executor.h>

#define BITMAP_TYPE_BUTTONS 0
#define BITMAP_FONT_UPPERCASE 1

#define FBA_KEY_A 0b00000001
#define FBA_KEY_B 0b00000010
#define FBA_KEY_START 0b00000100
#define FBA_KEY_SELECT 0b00001000
#define FBA_KEY_LEFT 0b00010000
#define FBA_KEY_UP 0b00100000
#define FBA_KEY_RIGHT 0b01000000
#define FBA_KEY_DOWN 0b10000000

using namespace FunkyBoyAndroid;

static int msgPipe[2];
struct timeval tp;

std::unique_ptr<FunkyBoy::Emulator> emulator;
std::shared_ptr<FunkyBoy::Controller::DisplayController> emuDisplayController;

bool initialSaveLoaded = false;

static void requestPickRom(struct engine* engine) {
    ANativeActivity *nativeActivity = engine->app->activity;
    JNIEnv *env = engine->env;

    jobject nativeActivityObj = nativeActivity->clazz; // "clazz" is misnamed, this is the actual activity instance
    jclass nativeActivityClass = env->GetObjectClass(nativeActivity->clazz);
    jmethodID method = env->GetMethodID(nativeActivityClass, "requestPickRom", "()V");

    env->CallVoidMethod(nativeActivityObj, method);
}

static jobject loadBitmap(struct engine* engine, jint type) {
    ANativeActivity *nativeActivity = engine->app->activity;
    JNIEnv *env = engine->env;

    jobject nativeActivityObj = nativeActivity->clazz; // "clazz" is misnamed, this is the actual activity instance
    jclass nativeActivityClass = env->GetObjectClass(nativeActivity->clazz);
    jmethodID method = env->GetMethodID(nativeActivityClass, "loadBitmap", "(I)Landroid/graphics/Bitmap;");

    jobject bitmap = env->CallObjectMethod(nativeActivityObj, method, type);

    return bitmap;
}

static std::string getSavePath(struct engine* engine, const FunkyBoy::ROMHeader *romHeader) {
    ANativeActivity *nativeActivity = engine->app->activity;
    JNIEnv *env = engine->env;

    jobject nativeActivityObj = nativeActivity->clazz; // "clazz" is misnamed, this is the actual activity instance
    jclass nativeActivityClass = env->GetObjectClass(nativeActivity->clazz);
    jmethodID method = env->GetMethodID(nativeActivityClass, "getSavePath", "(Ljava/lang/String;II)Ljava/lang/String;");

    jstring romTitle = env->NewStringUTF(reinterpret_cast<const char*>(romHeader->title));
    auto path = static_cast<jstring>(env->CallObjectMethod(
            nativeActivityObj, method, romTitle,
            romHeader->destinationCode,
            (romHeader->globalChecksum[0] << 8) | romHeader->globalChecksum[1]
            )
    );

    std::string savePath;
    jboolean isCopy;
    const char *jstr = env->GetStringUTFChars(path, &isCopy);
    savePath = jstr;
    env->ReleaseStringUTFChars(path, jstr);

    env->DeleteLocalRef(romTitle);
    env->DeleteLocalRef(path);

    return savePath;
}

static int engine_init_display(struct engine* engine) {
    LOGD("engine_init_display");

    ANativeWindow *window = engine->app->window;

    engine->width = ANativeWindow_getWidth(window);
    engine->height = ANativeWindow_getHeight(window);

    float scale = std::max(((float)FB_GB_DISPLAY_HEIGHT) / ((float)engine->height), ((float)FB_GB_DISPLAY_WIDTH) / ((float)engine->width));
    engine->uiScale = scale;
    int32_t bufferWidth = std::ceil(engine->width * scale);
    int32_t bufferHeight = std::ceil(engine->height * scale);
    int32_t offsetX = (FB_GB_DISPLAY_WIDTH - bufferWidth) / 2;
    int32_t offsetY = (FB_GB_DISPLAY_HEIGHT - bufferHeight) / 2;

    engine->bufferWidth = bufferWidth;
    engine->bufferHeight = bufferHeight;

    uint dpadX = 10;
    uint dpadY = bufferHeight - 90;

    ui_obj uiObjTemplate;
    uiObjTemplate.x = dpadX + 17;
    uiObjTemplate.y = dpadY;
    uiObjTemplate.width = 16;
    uiObjTemplate.height = 16;
    engine->keyUp = uiObjTemplate;

    uiObjTemplate.x = dpadX + 17;
    uiObjTemplate.y = dpadY + 34;
    engine->keyDown = uiObjTemplate;

    uiObjTemplate.x = dpadX;
    uiObjTemplate.y = dpadY + 17;
    engine->keyLeft = uiObjTemplate;

    uiObjTemplate.x = dpadX + 34;
    uiObjTemplate.y = dpadY + 17;
    engine->keyRight = uiObjTemplate;

    uiObjTemplate.x = bufferWidth - 30;
    uiObjTemplate.y = bufferHeight - 90;
    uiObjTemplate.width = 25;
    uiObjTemplate.height = 25;
    engine->keyA = uiObjTemplate;

    uiObjTemplate.x = bufferWidth - 60;
    uiObjTemplate.y = bufferHeight - 60;
    engine->keyB = uiObjTemplate;

    uiObjTemplate.width = 25;
    uiObjTemplate.height = 10;
    uiObjTemplate.x = (bufferWidth / 2) - 35;
    uiObjTemplate.y = bufferHeight - 20;
    engine->keySelect = uiObjTemplate;

    uiObjTemplate.x = (bufferWidth / 2) + 10;
    uiObjTemplate.y = bufferHeight - 20;
    engine->keyStart = uiObjTemplate;

    uiObjTemplate.x = (FB_GB_DISPLAY_WIDTH - 25) / 2;
    uiObjTemplate.y = FB_GB_DISPLAY_HEIGHT + 10;

    auto result = ANativeWindow_setBuffersGeometry(window, bufferWidth, bufferHeight, WINDOW_FORMAT_RGBA_8888);
    if (result != 0) {
        LOGW("Unable to set buffers geometry");
    }

    jobject bitmap = loadBitmap(engine, BITMAP_TYPE_BUTTONS);
    if (bitmap != nullptr) {
        bitmap = engine->env->NewGlobalRef(bitmap);
        engine->bitmapButtons = bitmap;
    } else {
        LOGW("Unable to load buttons texture bitmap");
    }
    bitmap = loadBitmap(engine, BITMAP_FONT_UPPERCASE);
    if (bitmap != nullptr) {
        bitmap = engine->env->NewGlobalRef(bitmap);
        engine->bitmapFontsUppercase = bitmap;
    } else {
        LOGW("Unable to load uppercase font bitmap");
    }

    engine->keyLatch = 0;

    return result;
}

static void loadSaveGame(struct engine* engine) {
    FunkyBoy::fs::path saveGamePath = getSavePath(engine, emulator->getROMHeader());
    emulator->savePath = saveGamePath;
    initialSaveLoaded = true;
    LOGD("Save path: %s", saveGamePath.c_str());
    if (!saveGamePath.empty() && emulator->supportsSaving() /*&& FunkyBoy::fs::exists(saveGamePath)*/) {
        std::ifstream file(saveGamePath, std::ios::binary | std::ios::in);
        emulator->loadCartridgeRam(file);
    }
}

static void saveGame() {
    auto &saveGamePath = emulator->savePath;
    if (!saveGamePath.empty() && emulator->supportsSaving() /*&& FunkyBoy::fs::exists(saveGamePath)*/) {
        std::ofstream file(saveGamePath, std::ios::binary | std::ios::out);
        emulator->writeCartridgeRam(file);
        LOGD("Cartridge RAM written to file");
    } else {
        LOGD("Game has no cartridge RAM");
    }
}

/**
 * Just the current frame in the display.
 */
static void engine_draw_frame(struct engine* engine) {
    ANativeWindow *window = engine->app->window;
    auto controller = dynamic_cast<FunkyBoyAndroid::Controller::DisplayControllerAndroid *>(emuDisplayController.get());

    if (emulator->getCartridgeStatus() == FunkyBoy::CartridgeStatus::Loaded) {
        if (!initialSaveLoaded) {
            loadSaveGame(engine);
        }
        controller->setWindow(window);
        FunkyBoy::ret_code retCode;
        do {
            retCode = emulator->doTick();
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
        switch (emulator->getCartridgeStatus()) {
            case FunkyBoy::NoROMLoaded:
                text = "No ROM loaded";
                break;
            case FunkyBoy::ROMFileNotReadable:
                text = "ROM not readable";
                break;
            case FunkyBoy::ROMParseError:
                text = "ROM not parsable";
                break;
            case FunkyBoy::ROMTooBig:
                text = "ROM is too big";
                break;
            case FunkyBoy::ROMSizeMismatch:
                text = "ROM size mismatch";
                break;
            case FunkyBoy::ROMUnsupportedMBC:
                text = "Unsupported MBC";
                break;
            case FunkyBoy::RAMSizeUnsupported:
                text = "Unsupported RAM size";
                break;
            case FunkyBoy::Loaded:
                text = "";
                break;
            default:
                text = "Unknown status";
                break;
        }
        size_t text_width = measureTextWidth(text, 0);
        drawTextAt(engine->env, buffer, engine->bitmapFontsUppercase, text, 0, (FB_GB_DISPLAY_WIDTH - text_width) / 2, 32);

        // Draw headline
        gettimeofday(&tp, nullptr);
        if (tp.tv_sec % 2 == 1) {
            text = "PRESS START!";
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

    if (emulator->getCartridgeStatus() != FunkyBoy::CartridgeStatus::Loaded) {
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
            activePointerIds.push_back(AMotionEvent_getPointerId(event, 0));
            break;
        case AMOTION_EVENT_ACTION_POINTER_DOWN: {
            uint32_t pointerIndex = (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
            activePointerIds.push_back(AMotionEvent_getPointerId(event, pointerIndex));
            break;
        }
        case AMOTION_EVENT_ACTION_UP:
            // FILO: First in, last out (primary pointer)
            activePointerIds.pop_back();
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
        emulator->setInputState(FunkyBoy::Controller::JoypadKey::JOYPAD_A, keyLatch & FBA_KEY_A);
        emulator->setInputState(FunkyBoy::Controller::JoypadKey::JOYPAD_B, keyLatch & FBA_KEY_B);
        emulator->setInputState(FunkyBoy::Controller::JoypadKey::JOYPAD_START, keyLatch & FBA_KEY_START);
        emulator->setInputState(FunkyBoy::Controller::JoypadKey::JOYPAD_SELECT, keyLatch & FBA_KEY_SELECT);
        emulator->setInputState(FunkyBoy::Controller::JoypadKey::JOYPAD_LEFT, keyLatch & FBA_KEY_LEFT);
        emulator->setInputState(FunkyBoy::Controller::JoypadKey::JOYPAD_UP, keyLatch & FBA_KEY_UP);
        emulator->setInputState(FunkyBoy::Controller::JoypadKey::JOYPAD_RIGHT, keyLatch & FBA_KEY_RIGHT);
        emulator->setInputState(FunkyBoy::Controller::JoypadKey::JOYPAD_DOWN, keyLatch & FBA_KEY_DOWN);
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
        case APP_CMD_SAVE_STATE:
            LOGD("CMD: APP_CMD_SAVE_STATE");
            // The system has asked us to save our current state.  Do so.
            // TODO: Save state
            /* engine->app->savedState = malloc(sizeof(struct saved_state));
            *((struct saved_state*)engine->app->savedState) = engine->state;
            engine->app->savedStateSize = sizeof(struct saved_state); */
            break;
        case APP_CMD_INIT_WINDOW:
            // The window is being shown, get it ready.
            LOGD("CMD: APP_CMD_INIT_WINDOW");
            engine->activePointerIds.clear();
            if (engine->app->window != nullptr) {
                engine_init_display(engine);
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
            engine->animating = true;
            break;
        case APP_CMD_LOST_FOCUS:
            LOGD("CMD: APP_CMD_LOST_FOCUS");
            engine->activePointerIds.clear();
            engine->animating = false;
            engine_draw_frame(engine);
            break;
        default:
            break;
    }
}

static int handleCustomMessage(int fd, int events, void* data) {
    size_t strln;
    read(fd, reinterpret_cast<char *>(&strln), sizeof(size_t));

    char *romPath = (char*) calloc(strln + 1, sizeof(char));
    read(fd, romPath, strln);

    LOGD("RECV rom path: %s", romPath);

    auto result = emulator->loadGame(romPath);
    LOGD("ROM load status: %d", result);

    initialSaveLoaded = false;

    free(romPath);
    return 1;
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

    auto controllers = std::make_shared<FunkyBoy::Controller::Controllers>();
    emuDisplayController = std::make_shared<FunkyBoyAndroid::Controller::DisplayControllerAndroid>(&engine);
    controllers->setDisplay(emuDisplayController);
    emulator = std::make_unique<FunkyBoy::Emulator>(FunkyBoy::GameBoyType::GameBoyDMG, controllers);

    if (state->savedState != nullptr) {
        // We are starting with a previous saved state; restore from it.
        // TODO: Restore state
        // engine.state = *(struct saved_state*)state->savedState;
    }

    pipe(msgPipe);
    ALooper_addFd(state->looper, msgPipe[0], ALOOPER_POLL_CALLBACK , ALOOPER_EVENT_INPUT, handleCustomMessage, state);

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

extern "C" {
    JNIEXPORT void JNICALL Java_lu_kremi151_funkyboy_FunkyBoyActivity_romPicked(JNIEnv *env, jobject, jstring path) {
        jboolean isCopy;
        auto path_cstr = env->GetStringUTFChars(path, &isCopy);

        size_t strln = strlen(path_cstr);

        write(msgPipe[1], reinterpret_cast<char *>(&strln), sizeof(size_t));
        write(msgPipe[1], path_cstr, strln);

        env->ReleaseStringUTFChars(path, path_cstr);
    }
}

//END_INCLUDE(all)
