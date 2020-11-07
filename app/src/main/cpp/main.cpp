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
#include "logging.h"

#define BITMAP_TYPE_DPAD 0

/**
 * Shared state for our app.
 */
struct engine {
    struct android_app* app;
    JNIEnv *env;

    jobject bitmapDpad;

    int32_t width;
    int32_t height;

    int32_t bufferWidth;
    int32_t bufferHeight;
};

std::unique_ptr<FunkyBoy::Emulator> emulator;
std::shared_ptr<FunkyBoy::Controller::DisplayController> emuDisplayController;
bool pickedRom = false; // TODO: Find a better way

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
    LOGI("# loadBitmap A %p", nativeActivity->clazz);
    jclass nativeActivityClass = env->GetObjectClass(nativeActivity->clazz);
    LOGI("# loadBitmap B");
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
    int32_t bufferWidth = std::ceil(engine->width * scale);
    int32_t bufferHeight = std::ceil(engine->height * scale);
    int32_t offsetX = (FB_GB_DISPLAY_WIDTH - bufferWidth) / 2;
    int32_t offsetY = (FB_GB_DISPLAY_HEIGHT - bufferHeight) / 2;

    engine->bufferWidth = bufferWidth;
    engine->bufferHeight = bufferHeight;

    auto result = ANativeWindow_setBuffersGeometry(window, bufferWidth, bufferHeight, WINDOW_FORMAT_RGBA_8888);
    if (result != 0) {
        LOGW("Unable to set buffers geometry");
    }

    jobject bitmapDpad = loadBitmap(engine, BITMAP_TYPE_DPAD);
    if (bitmapDpad != nullptr) {
        bitmapDpad = engine->env->NewGlobalRef(bitmapDpad);
        engine->bitmapDpad = bitmapDpad;
    } else {
        LOGW("Unable to load DPad bitmap");
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

    if (pickedRom) {
        controller->setWindow(window);
        retCode = emulator->doTick();
        controller->setWindow(nullptr);
    }

    if (retCode & FB_RET_NEW_FRAME) {
        jobject bitmap = engine->bitmapDpad;
        if (bitmap != nullptr && drawBitmap(engine->env, controller->getBuffer(), bitmap, 10, engine->bufferHeight - 60) != 0) {
            LOGW("Render of DPad failed");
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
}

/**
 * Process the next input event.
 */
static int32_t engine_handle_input(struct android_app* app, AInputEvent* event) {
    auto* engine = (struct engine*)app->userData;
    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
        // TODO: Handle input
        if (!pickedRom) {
            requestPickRom(engine);
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
    controllers->setDisplay(emuDisplayController);
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
        if (result == FunkyBoy::CartridgeStatus::Loaded) {
            pickedRom = true;
        }
    }
}

//END_INCLUDE(all)
