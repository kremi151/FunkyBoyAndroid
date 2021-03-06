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
#include <ui/draw_controls.h>

#include <fba_util/logging.h>

using namespace FunkyBoyAndroid::Controller;

DisplayControllerAndroid::DisplayControllerAndroid(struct engine *engine)
    : engine(engine)
    , window(nullptr)
    , pixels(new uint32_t[FB_GB_DISPLAY_WIDTH * FB_GB_DISPLAY_HEIGHT])
{
}

DisplayControllerAndroid::~DisplayControllerAndroid() {
    delete[] pixels;
}

void DisplayControllerAndroid::setWindow(ANativeWindow *w) {
    window = w;
}

void DisplayControllerAndroid::drawScanLine(FunkyBoy::u8 y, FunkyBoy::u8 *buffer) {
    uint32_t pixel;
    for (FunkyBoy::u8 x = 0 ; x < FB_GB_DISPLAY_WIDTH ; x++) {
        auto &color = FunkyBoy::Palette::ARGB8888::DMG[*(buffer + x)];
        pixel = (255u << 24u) | (color[0] << 16) | (color[1] << 8) | color[2];
        pixels[(y * FB_GB_DISPLAY_WIDTH) + x] = pixel;
    }
}

void DisplayControllerAndroid::drawScreen() {
    if (window == nullptr) {
        // Window is not yet initialized
        return;
    }

    ANativeWindow_acquire(window);
    if (ANativeWindow_lock(window, &buffer, nullptr) < 0) {
        LOGW("Unable to lock native window");
        ANativeWindow_release(window);
        return;
    }
    auto *line = (uint32_t *) buffer.bits;
    for (int y = 0 ; y < FB_GB_DISPLAY_HEIGHT ; y++) {
        for (int x = 0 ; x < FB_GB_DISPLAY_WIDTH ; x++) {
            line[x] = pixels[(y * FB_GB_DISPLAY_WIDTH) + x];
        }
        line = line + buffer.stride;
    }

    drawControls(engine, buffer);

    if (ANativeWindow_unlockAndPost(window) < 0) {
        LOGW("Unable to unlock and post to native window");
    }
    ANativeWindow_release(window);
}
