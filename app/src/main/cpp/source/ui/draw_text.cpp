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

#define CHAR_SPACING 1
#define CHAR_ACTUAL_WIDTH (CHAR_WIDTH + CHAR_SPACING)

#define FONT_WIDTH (26 * CHAR_WIDTH)
#define FONT_HEIGHT FBA_CHAR_HEIGHT

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
    unsigned short fy_start;
    unsigned short fy_end;
    auto *bitmapPixes = (uint32_t *) fontData;
    while (c != end) {
        chr = *(c++);
        // Looks ugly, but provides fast branch selection
        switch (chr) {
            case 65: case 66: case 67: case 68: case 69: case 70: case 71: case 72: case 73: case 74:
            case 75: case 76: case 77: case 78: case 79: case 80: case 81: case 82: case 83: case 84:
            case 85: case 86: case 87: case 88: case 89: case 90: {
                // Uppercase letters
                chr -= 65;
                fy_start = 0;
                fy_end = FONT_HEIGHT;
                break;
            }
            case 97: case 98: case 99: case 100: case 101: case 102: case 103: case 104: case 105:
            case 106: case 107: case 108: case 109: case 110: case 111: case 112: case 113: case 114:
            case 115: case 116: case 117: case 118: case 119: case 120: case 121: case 122: {
                // Lowercase letters
                chr -= 97;
                fy_start = FONT_HEIGHT;
                fy_end = 2 * FONT_HEIGHT;
                break;
            }
            case 48: case 49: case 50: case 51: case 52: case 53: case 54: case 55: case 56: case 57: {
                // Numbers
                chr -= 48;
                fy_start = 2 * FONT_HEIGHT;
                fy_end = 3 * FONT_HEIGHT;
                break;
            }
            case 32: {
                // Space
                x += CHAR_ACTUAL_WIDTH;
                continue;
            }
            case 33: {
                // !
                chr = 10;
                fy_start = 2 * FONT_HEIGHT;
                fy_end = 3 * FONT_HEIGHT;
                break;
            }
            case 46: {
                // .
                chr = 12;
                fy_start = 2 * FONT_HEIGHT;
                fy_end = 3 * FONT_HEIGHT;
                break;
            }
            case 44: {
                // ,
                chr = 13;
                fy_start = 2 * FONT_HEIGHT;
                fy_end = 3 * FONT_HEIGHT;
                break;
            }
            default:
            case 63: {
                // ?
                chr = 11;
                fy_start = 2 * FONT_HEIGHT;
                fy_end = 3 * FONT_HEIGHT;
                break;
            }
        }
        auto *line = (uint32_t *) buffer.bits + (y * buffer.stride);
        for (; fy_start < fy_end; fy_start++) {
            for (int _x = 0; _x < CHAR_WIDTH; _x++) {
                line[x + _x] = bitmapPixes[(FONT_WIDTH * fy_start) + _x + (chr * CHAR_WIDTH)];
            }
            line = line + buffer.stride;
        }
        x += CHAR_ACTUAL_WIDTH;
    }
    return 0;
}

size_t FunkyBoyAndroid::measureTextWidth(const char *text, size_t len) {
    if (len == 0) {
        len = std::strlen(text);
    }
    if (len != 0) {
        return (len * CHAR_ACTUAL_WIDTH) - CHAR_SPACING;
    } else {
        return 0;
    }
}