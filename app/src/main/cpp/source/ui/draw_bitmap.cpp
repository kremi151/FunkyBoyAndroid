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

#include "draw_bitmap.h"

#include <fba_util/logging.h>

int FunkyBoyAndroid::drawBitmap(JNIEnv *env, ANativeWindow_Buffer &buffer, jobject bitmap, uint u, uint v, uint w, uint h, uint x, uint y) {
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
    for (int _y = 0; _y < h; _y++) {
        for (int _x = 0; _x < w; _x++) {
            line[x + _x] = bitmapPixes[info.width * (_y + v) + _x + u];
        }
        line = line + buffer.stride;
    }
    return 0;
}

int FunkyBoyAndroid::drawBitmap(JNIEnv *env, ANativeWindow_Buffer &buffer, jobject bitmap, uint x, uint y) {
    if (bitmap == nullptr) {
        return -1;
    }
    AndroidBitmapInfo info;
    if (AndroidBitmap_getInfo(env, bitmap, &info) < 0) {
        LOGW("Unable to get bitmap info");
        return -2;
    }
    return drawBitmap(env, buffer, bitmap, 0, 0, info.width, info.height, x, y);
}
