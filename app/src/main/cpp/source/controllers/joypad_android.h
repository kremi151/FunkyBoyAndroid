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

#ifndef FB_ANDROID_CONTROLLER_JOYPAD_ANDROID_H
#define FB_ANDROID_CONTROLLER_JOYPAD_ANDROID_H

#include <controllers/joypad.h>

namespace FunkyBoyAndroid {

    namespace Controller {

        class JoypadControllerAndroid: public FunkyBoy::Controller::JoypadController {
        public:
            bool a;
            bool b;
            bool left;
            bool up;
            bool right;
            bool down;
            bool start;
            bool select;

            JoypadControllerAndroid();

            bool isKeyPressed(FunkyBoy::Controller::JoypadKey key) override;
        };

    }

}

#endif // FB_ANDROID_CONTROLLER_JOYPAD_ANDROID_H