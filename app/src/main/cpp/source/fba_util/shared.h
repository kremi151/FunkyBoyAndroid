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

#ifndef FB_ANDROID_UTIL_SHARED_H
#define FB_ANDROID_UTIL_SHARED_H

#include <memory>
#include <emulator/emulator.h>

namespace FunkyBoyAndroid::State {
    extern std::unique_ptr<FunkyBoy::Emulator> emulator;
    extern std::shared_ptr<FunkyBoy::Controller::DisplayController> emuDisplayController;
    extern std::shared_ptr<FunkyBoy::Controller::AudioController> emuAudioController;

    extern bool initialSaveLoaded;
    extern std::string romPath;
}

#endif //FB_ANDROID_UTIL_SHARED_H
