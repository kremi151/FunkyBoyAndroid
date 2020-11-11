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

#include "draw_controls.h"

#include <jni.h>
#include <fba_util/logging.h>
#include <ui/draw_bitmap.h>

void FunkyBoyAndroid::drawControls(struct engine* engine, ANativeWindow_Buffer &buffer) {
    jobject bitmap = engine->bitmapDpad;
    if (bitmap != nullptr && drawBitmap(engine->env, buffer, bitmap, engine->keyLeft.x, engine->keyUp.y) != 0) {
        LOGW("Render of DPad failed");
    }
    bitmap = engine->bitmapKeyA;
    if (bitmap != nullptr && drawBitmap(engine->env, buffer, bitmap, engine->keyA.x, engine->keyA.y) != 0) {
        LOGW("Render of A key failed");
    }
    bitmap = engine->bitmapKeyB;
    if (bitmap != nullptr && drawBitmap(engine->env, buffer, bitmap, engine->keyB.x, engine->keyB.y) != 0) {
        LOGW("Render of B key failed");
    }
    bitmap = engine->bitmapKeyStart;
    if (bitmap != nullptr && drawBitmap(engine->env, buffer, bitmap, engine->keyStart.x, engine->keyStart.y) != 0) {
        LOGW("Render of start key failed");
    }
    bitmap = engine->bitmapKeySelect;
    if (bitmap != nullptr && drawBitmap(engine->env, buffer, bitmap, engine->keySelect.x, engine->keySelect.y) != 0) {
        LOGW("Render of select key failed");
    }
}