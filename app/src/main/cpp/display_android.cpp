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
#include <palette/dmg_palette.h>

#include "logging.h"

using namespace FunkyBoy::Controller;

DisplayControllerAndroid::DisplayControllerAndroid()
    : nativeWindow(nullptr)
    , pixels(new uint32_t[FB_GB_DISPLAY_WIDTH * FB_GB_DISPLAY_HEIGHT])
{
}

DisplayControllerAndroid::~DisplayControllerAndroid() {
    delete[] pixels;
}

void DisplayControllerAndroid::setNativeWindow(ANativeWindow *aNativeWindow) {
    nativeWindow = aNativeWindow;
}

void DisplayControllerAndroid::drawScanLine(u8 y, u8 *buffer) {
    uint32_t pixel;
    for (u8 x = 0 ; x < FB_GB_DISPLAY_WIDTH ; x++) {
        auto &color = Palette::ARGB8888::DMG[*(buffer + x)];
        pixel = (255u << 24u) | (color[0] << 16) | (color[1] << 8) | color[2];
        pixels[(y * FB_GB_DISPLAY_WIDTH) + x] = pixel;
    }
}

void DisplayControllerAndroid::drawScreen() {
    if (nativeWindow == nullptr) {
        return;
    }
    ANativeWindow_acquire(nativeWindow);
    ANativeWindow_Buffer buffer;
    if (ANativeWindow_lock(nativeWindow, &buffer, nullptr) < 0) {
        LOGW("Unable to lock native window");
        ANativeWindow_release(nativeWindow);
        return;
    }
    auto *line = (uint32_t *) buffer.bits;
    for (int y = 0 ; y < FB_GB_DISPLAY_HEIGHT ; y++) {
        for (int x = 0 ; x < FB_GB_DISPLAY_WIDTH ; x++) {
            line[x] = pixels[(y * FB_GB_DISPLAY_WIDTH) + x];
        }
        line = line + buffer.stride;
    }
    if (ANativeWindow_unlockAndPost(nativeWindow) < 0) {
        LOGW("Unable to unlock and post to native window");
    }
    ANativeWindow_release(nativeWindow);
}
