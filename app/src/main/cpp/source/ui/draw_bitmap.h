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

#ifndef FB_ANDROID_UI_DRAW_BITMAP_H
#define FB_ANDROID_UI_DRAW_BITMAP_H

#include <cstdlib>
#include <jni.h>
#include <android/bitmap.h>
#include <android/native_window.h>

namespace FunkyBoyAndroid {

    int drawBitmap(JNIEnv *env, ANativeWindow_Buffer &buffer, jobject bitmap, uint x, uint y);
    int drawBitmap(JNIEnv *env, ANativeWindow_Buffer &buffer, jobject bitmap, uint u, uint v, uint w, uint h, uint x, uint y);

}

#endif