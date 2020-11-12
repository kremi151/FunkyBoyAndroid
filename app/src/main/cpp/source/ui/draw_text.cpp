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

#include "draw_text.h"

#include <fba_util/logging.h>
#include <android/bitmap.h>
#include <cstring>

#define CHAR_WIDTH 7
#define CHAR_HEIGHT 7

#define FONT_WIDTH (26 * CHAR_WIDTH)
#define FONT_HEIGHT CHAR_HEIGHT

int FunkyBoyAndroid::drawTextAt(JNIEnv *env, ANativeWindow_Buffer &buffer, jobject font, const char *text, size_t len, uint x, uint y) {
    char *fontData = nullptr;
    if (AndroidBitmap_lockPixels(env, font, (void **) &fontData) < 0) {
        LOGW("Unable to lock pixels");
        return -3;
    }
    if (AndroidBitmap_unlockPixels(env, font) < 0) {
        LOGW("Unable to unlock pixels");
        return -4;
    }
    if (len == 0) {
        len = std::strlen(text);
    }
    const char *c = text;
    const char *end = text + len;
    char chr;
    auto *bitmapPixes = (uint32_t *) fontData;
    while (c != end) {
        chr = *(c++);
        if (chr >= 97 && chr <= 122) {
            chr -= 97;
        } else if (chr >= 65 && chr <= 90) {
            chr -= 65;
        } else {
            x += 7;
            continue;
        }
        auto *line = (uint32_t *) buffer.bits + (y * buffer.stride);
        for (int _y = 0; _y < FONT_HEIGHT; _y++) {
            for (int _x = 0; _x < CHAR_WIDTH; _x++) {
                line[x + _x] = bitmapPixes[(FONT_WIDTH * _y) + _x + (chr * CHAR_WIDTH)];
            }
            line = line + buffer.stride;
        }
        x += 7;
    }
    return 0;
}

size_t FunkyBoyAndroid::measureTextWidth(const char *text, size_t len) {
    if (len == 0) {
        len = std::strlen(text);
    }
    return len * CHAR_WIDTH;
}