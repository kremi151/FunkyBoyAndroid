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

#ifndef FB_ANDROID_UTIL_SAVED_STATE_H
#define FB_ANDROID_UTIL_SAVED_STATE_H

#include <util/typedefs.h>

#define FB_ANDROID_APP_STATE_ROM_PATH_BUFFER_SIZE 256

namespace FunkyBoyAndroid {

    typedef struct {
        char state[FB_SAVE_STATE_MAX_BUFFER_SIZE];
        char romPath[FB_ANDROID_APP_STATE_ROM_PATH_BUFFER_SIZE];
    } app_save_state;

    void serializeState(app_save_state *state);
    void resumeFromState(app_save_state *state);

}

#endif //FB_ANDROID_UTIL_SAVED_STATE_H
