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

#include <android_native_app_glue.h>
#include <android/bitmap.h>

#include <emulator/emulator.h>
#include "display_android.h"
#include "joypad_android.h"
#include "logging.h"

#define BITMAP_TYPE_DPAD 0
#define BITMAP_TYPE_KEY_A 1
#define BITMAP_TYPE_KEY_B 2
#define BITMAP_TYPE_KEY_START 3
#define BITMAP_TYPE_KEY_SELECT 4

typedef struct {
    uint x;
    uint y;
    uint width;
    uint height;
} ui_obj;

/**
 * Shared state for our app.
 */
struct engine {
    struct android_app* app;
    JNIEnv *env;

    jobject bitmapDpad;
    jobject bitmapKeyA;
    jobject bitmapKeyB;
    jobject bitmapKeyStart;
    jobject bitmapKeySelect;

    int32_t width;
    int32_t height;

    int32_t bufferWidth;
    int32_t bufferHeight;

    ui_obj keyUp;
    ui_obj keyDown;
    ui_obj keyLeft;
    ui_obj keyRight;
    ui_obj keyA;
    ui_obj keyB;
    ui_obj keyStart;
    ui_obj keySelect;

    float uiScale;
};

std::unique_ptr<FunkyBoy::Emulator> emulator;
std::shared_ptr<FunkyBoy::Controller::DisplayController> emuDisplayController;
std::shared_ptr<FunkyBoy::Controller::JoypadControllerAndroid> emuJoypadController;

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

static int drawBitmap(JNIEnv *env, ANativeWindow_Buffer &buffer, jobject bitmap, int x, int y) {
    if (bitmap == nullptr) {
        return -1;
    }
    AndroidBitmapInfo info;
    if (AndroidBitmap_getInfo(env, bitmap, &info) < 0) {
        LOGW("Unable to get bitmap info");
        return -2;
    }
    char *data = nullptr;
    if (AndroidBitmap_lockPixels(env, bitmap, (void **) &data) < 0) {
        LOGW("Unable to lock pixels");
        return -3;
    }
    if (AndroidBitmap_unlockPixels(env, bitmap) < 0) {
        LOGW("Unable to unlock pixels");
        return -4;
    }
    auto *bitmapPixes = (int32_t *) data;
    auto *line = (uint32_t *) buffer.bits + (y * buffer.stride);
    for (int _y = 0; _y < info.height; _y++) {
        for (int _x = 0; _x < info.width; _x++) {
            line[x + _x] = bitmapPixes[info.height * _y + _x];
        }
        line = line + buffer.stride;
    }
    return 0;
}

static int engine_init_display(struct engine* engine) {
    LOGI("engine_init_display");

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

    auto result = ANativeWindow_setBuffersGeometry(window, bufferWidth, bufferHeight, WINDOW_FORMAT_RGBA_8888);
    if (result != 0) {
        LOGW("Unable to set buffers geometry");
    }

    jobject bitmap = loadBitmap(engine, BITMAP_TYPE_DPAD);
    if (bitmap != nullptr) {
        bitmap = engine->env->NewGlobalRef(bitmap);
        engine->bitmapDpad = bitmap;
    } else {
        LOGW("Unable to load DPad bitmap");
    }
    bitmap = loadBitmap(engine, BITMAP_TYPE_KEY_A);
    if (bitmap != nullptr) {
        bitmap = engine->env->NewGlobalRef(bitmap);
        engine->bitmapKeyA = bitmap;
    } else {
        LOGW("Unable to load A key bitmap");
    }
    bitmap = loadBitmap(engine, BITMAP_TYPE_KEY_B);
    if (bitmap != nullptr) {
        bitmap = engine->env->NewGlobalRef(bitmap);
        engine->bitmapKeyB = bitmap;
    } else {
        LOGW("Unable to load B key bitmap");
    }
    bitmap = loadBitmap(engine, BITMAP_TYPE_KEY_START);
    if (bitmap != nullptr) {
        bitmap = engine->env->NewGlobalRef(bitmap);
        engine->bitmapKeyStart = bitmap;
    } else {
        LOGW("Unable to load start key bitmap");
    }
    bitmap = loadBitmap(engine, BITMAP_TYPE_KEY_SELECT);
    if (bitmap != nullptr) {
        bitmap = engine->env->NewGlobalRef(bitmap);
        engine->bitmapKeySelect = bitmap;
    } else {
        LOGW("Unable to load select key bitmap");
    }

    return result;
}

/**
 * Just the current frame in the display.
 */
static void engine_draw_frame(struct engine* engine) {
    // Just fill the screen with a color.

    // TODO: Draw emulator here
    /*glClearColor(((float)engine->state.x)/engine->width, engine->state.angle,
                 ((float)engine->state.y)/engine->height, 1);
    glClear(GL_COLOR_BUFFER_BIT);*/

    FunkyBoy::ret_code retCode = 0;
    ANativeWindow *window = engine->app->window;
    auto controller = dynamic_cast<FunkyBoy::Controller::DisplayControllerAndroid *>(emuDisplayController.get());

    if (emulator->getCartridge().getStatus() == FunkyBoy::CartridgeStatus::Loaded) {
        controller->setWindow(window);
        retCode = emulator->doTick();
        controller->setWindow(nullptr);
    }

    if (retCode & FB_RET_NEW_FRAME) {
        jobject bitmap = engine->bitmapDpad;
        if (bitmap != nullptr && drawBitmap(engine->env, controller->getBuffer(), bitmap, engine->keyLeft.x, engine->keyUp.y) != 0) {
            LOGW("Render of DPad failed");
        }
        bitmap = engine->bitmapKeyA;
        if (bitmap != nullptr && drawBitmap(engine->env, controller->getBuffer(), bitmap, engine->keyA.x, engine->keyA.y) != 0) {
            LOGW("Render of A key failed");
        }
        bitmap = engine->bitmapKeyB;
        if (bitmap != nullptr && drawBitmap(engine->env, controller->getBuffer(), bitmap, engine->keyB.x, engine->keyB.y) != 0) {
            LOGW("Render of B key failed");
        }
        bitmap = engine->bitmapKeyStart;
        if (bitmap != nullptr && drawBitmap(engine->env, controller->getBuffer(), bitmap, engine->keyStart.x, engine->keyStart.y) != 0) {
            LOGW("Render of start key failed");
        }
        bitmap = engine->bitmapKeySelect;
        if (bitmap != nullptr && drawBitmap(engine->env, controller->getBuffer(), bitmap, engine->keySelect.x, engine->keySelect.y) != 0) {
            LOGW("Render of select key failed");
        }
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
    LOGI("engine_term_display");
    if (engine->bitmapDpad != nullptr) {
        engine->env->DeleteGlobalRef(engine->bitmapDpad);
        engine->bitmapDpad = nullptr;
    }
    if (engine->bitmapKeyA != nullptr) {
        engine->env->DeleteGlobalRef(engine->bitmapKeyA);
        engine->bitmapKeyA = nullptr;
    }
    if (engine->bitmapKeyB != nullptr) {
        engine->env->DeleteGlobalRef(engine->bitmapKeyB);
        engine->bitmapKeyB = nullptr;
    }
    if (engine->bitmapKeyStart != nullptr) {
        engine->env->DeleteGlobalRef(engine->bitmapKeyStart);
        engine->bitmapKeyStart = nullptr;
    }
    if (engine->bitmapKeySelect != nullptr) {
        engine->env->DeleteGlobalRef(engine->bitmapKeySelect);
        engine->bitmapKeySelect = nullptr;
    }
}

static bool isTouched(const ui_obj &obj, float scaledX, float scaledY) {
    return scaledX >= obj.x && scaledX < obj.x + obj.width
        && scaledY >= obj.y && scaledY < obj.y + obj.height;
}

/**
 * Process the next input event.
 */
static int32_t engine_handle_input(struct android_app* app, AInputEvent* event) {
    auto* engine = (struct engine*)app->userData;
    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
        if (emulator->getCartridge().getStatus() != FunkyBoy::CartridgeStatus::Loaded) {
            requestPickRom(engine);
            return 1;
        }
        float scaledX = AMotionEvent_getX(event, 0) * engine->uiScale;
        float scaledY = AMotionEvent_getY(event, 0) * engine->uiScale;
        auto joypad = dynamic_cast<FunkyBoy::Controller::JoypadControllerAndroid *>(emuJoypadController.get());
        const bool isUp = (AMOTION_EVENT_ACTION_MASK & AMotionEvent_getAction(event)) == AMOTION_EVENT_ACTION_UP;
        const bool isDown = (AMOTION_EVENT_ACTION_MASK & AMotionEvent_getAction(event)) == AMOTION_EVENT_ACTION_DOWN;
        if (!isUp && !isDown) {
            return 1;
        }
        bool touched = isTouched(engine->keyA, scaledX, scaledY);
        if (touched) {
            joypad->a = isDown;
        }
        touched = isTouched(engine->keyB, scaledX, scaledY);
        if (touched) {
            joypad->b = isDown;
        }
        touched = isTouched(engine->keyLeft, scaledX, scaledY);
        if (touched) {
            joypad->left = isDown;
        }
        touched = isTouched(engine->keyUp, scaledX, scaledY);
        if (touched) {
            joypad->up = isDown;
        }
        touched = isTouched(engine->keyRight, scaledX, scaledY);
        if (touched) {
            joypad->right = isDown;
        }
        touched = isTouched(engine->keyDown, scaledX, scaledY);
        if (touched) {
            joypad->down = isDown;
        }
        touched = isTouched(engine->keyStart, scaledX, scaledY);
        if (touched) {
            joypad->start = isDown;
        }
        touched = isTouched(engine->keySelect, scaledX, scaledY);
        if (touched) {
            joypad->select = isDown;
        }
        return 1;
    }
    return 0;
}

/**
 * Process the next main command.
 */
static void engine_handle_cmd(struct android_app* app, int32_t cmd) {
    auto* engine = (struct engine*)app->userData;
    switch (cmd) {
        case APP_CMD_SAVE_STATE:
            // The system has asked us to save our current state.  Do so.
            // TODO: Save state
            /* engine->app->savedState = malloc(sizeof(struct saved_state));
            *((struct saved_state*)engine->app->savedState) = engine->state;
            engine->app->savedStateSize = sizeof(struct saved_state); */
            break;
        case APP_CMD_INIT_WINDOW:
            // The window is being shown, get it ready.
            if (engine->app->window != nullptr) {
                engine_init_display(engine);
                engine_draw_frame(engine);
            }
            break;
        case APP_CMD_TERM_WINDOW:
            // The window is being hidden or closed, clean it up.
            engine_term_display(engine);
            break;
        case APP_CMD_GAINED_FOCUS:
            // When our app gains focus, we start monitoring the accelerometer.
            break;
        case APP_CMD_LOST_FOCUS:
            // When our app loses focus, we stop monitoring the accelerometer.
            // This is to avoid consuming battery while not being used.
            engine_draw_frame(engine);
            break;
        default:
            break;
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

    auto controllers = std::make_shared<FunkyBoy::Controller::Controllers>();
    emuDisplayController = std::make_shared<FunkyBoy::Controller::DisplayControllerAndroid>();
    emuJoypadController = std::make_shared<FunkyBoy::Controller::JoypadControllerAndroid>();
    controllers->setDisplay(emuDisplayController);
    controllers->setJoypad(emuJoypadController);
    emulator = std::make_unique<FunkyBoy::Emulator>(FunkyBoy::GameBoyType::GameBoyDMG, controllers);

    if (state->savedState != nullptr) {
        // We are starting with a previous saved state; restore from it.
        // TODO: Restore state
        // engine.state = *(struct saved_state*)state->savedState;
    }

    // loop waiting for stuff to do.

    while (true) {
        // Read all pending events.
        int ident;
        int events;
        struct android_poll_source* source;

        // If not animating, we will block forever waiting for events.
        // If animating, we loop until all events are read, then continue
        // to draw the next frame of animation.
        while ((ident=ALooper_pollAll(0, nullptr, &events,
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

        if (true /* TODO */) {
            // Done with events; draw next animation frame.
            // TODO: Emulator tick
            /* engine.state.angle += .01f;
            if (engine.state.angle > 1) {
                engine.state.angle = 0;
            } */

            // Drawing is throttled to the screen update rate, so there
            // is no need to do timing here.
            engine_draw_frame(&engine);
        }
    }

    jvm->DetachCurrentThread();
}

extern "C" {
    JNIEXPORT void JNICALL Java_lu_kremi151_funkyboy_FunkyBoyActivity_romPicked(JNIEnv *env, jobject, jstring path) {
        jboolean isCopy;
        auto path_cstr = env->GetStringUTFChars(path, &isCopy);
        auto result = emulator->loadGame(path_cstr);
        env->ReleaseStringUTFChars(path, path_cstr);
        LOGI("ROM load status: %d", result);
    }
}

//END_INCLUDE(all)
