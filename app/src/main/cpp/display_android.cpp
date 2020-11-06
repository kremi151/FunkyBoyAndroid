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

#include "display_android.h"

#include <android/surface_control.h>
#include <android/window.h>
#include <android/native_window_jni.h>

using namespace FunkyBoy::Controller;

DisplayControllerAndroid::DisplayControllerAndroid(): nativeWindow(nullptr) {
}

DisplayControllerAndroid::~DisplayControllerAndroid() {
    if (nativeWindow != nullptr) {
        ANativeWindow_release(nativeWindow);
        nativeWindow = nullptr;
    }
}

void DisplayControllerAndroid::setNativeWindow(ANativeWindow *aNativeWindow) {
    if (nativeWindow != nullptr) {
        ANativeWindow_release(nativeWindow);
    }
    nativeWindow = aNativeWindow;
}

void DisplayControllerAndroid::drawScanLine(u8 y, u8 *buffer) {

}

void DisplayControllerAndroid::drawScreen() {

}
