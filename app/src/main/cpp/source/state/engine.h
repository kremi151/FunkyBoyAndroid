/**
 * Copyright 2020 Michel Kremer (kremi151)
 *
 * Licensed under the Apache License, Version 2.0 (the License);
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef FB_ANDROID_STATE_ENGINE_H
#define FB_ANDROID_STATE_ENGINE_H

#include <vector>
#include <state/ui_obj.h>
#include <jni.h>

namespace FunkyBoyAndroid {

    /**
     * Shared app state
     */
    struct engine {
        struct android_app* app;
        JNIEnv *env;

        jobject bitmapDpad;
        jobject bitmapKeyA;
        jobject bitmapKeyB;
        jobject bitmapKeyStart;
        jobject bitmapKeySelect;
        jobject bitmapFontsUppercase;

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

        bool animating;

        std::vector<size_t> activePointerIds;
    };

}

#endif