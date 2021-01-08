/**
 * Copyright 2021 Michel Kremer (kremi151)
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

#include "init_display.h"

#include <fba_util/logging.h>
#include <util/typedefs.h>
#include <cmath>
#include <fb_jni.h>

#define BITMAP_TYPE_BUTTONS 0
#define BITMAP_FONT_UPPERCASE 1

int FunkyBoyAndroid::Engine::initDisplay(struct engine *engine) {
    LOGD("engine_init_display");

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

    uiObjTemplate.x = (FB_GB_DISPLAY_WIDTH - 25) / 2;
    uiObjTemplate.y = FB_GB_DISPLAY_HEIGHT + 10;

    auto result = ANativeWindow_setBuffersGeometry(window, bufferWidth, bufferHeight, WINDOW_FORMAT_RGBA_8888);
    if (result != 0) {
        LOGW("Unable to set buffers geometry");
    }

    jobject bitmap = loadBitmap(engine, BITMAP_TYPE_BUTTONS);
    if (bitmap != nullptr) {
        bitmap = engine->env->NewGlobalRef(bitmap);
        engine->bitmapButtons = bitmap;
    } else {
        LOGW("Unable to load buttons texture bitmap");
    }
    bitmap = loadBitmap(engine, BITMAP_FONT_UPPERCASE);
    if (bitmap != nullptr) {
        bitmap = engine->env->NewGlobalRef(bitmap);
        engine->bitmapFontsUppercase = bitmap;
    } else {
        LOGW("Unable to load uppercase font bitmap");
    }

    engine->keyLatch = 0;

    return result;
}