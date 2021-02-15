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

#ifndef FB_ANDROID_JNI_H
#define FB_ANDROID_JNI_H

#include <jni.h>
#include <string>
#include <cartridge/header.h>
#include <engine/engine.h>

extern int fbMsgPipe[2];

namespace FunkyBoyAndroid {

    void showOptionsActivity(struct engine* engine);
    std::string getInitialRomPath(struct engine* engine);
    jobject loadBitmap(struct engine* engine, jint type);
    std::string getSavePath(struct engine* engine, const FunkyBoy::ROMHeader *romHeader);

}

#endif //FB_ANDROID_JNI_H
