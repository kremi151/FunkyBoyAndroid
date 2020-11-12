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

#include "joypad_android.h"

using namespace FunkyBoyAndroid::Controller;

JoypadControllerAndroid::JoypadControllerAndroid()
    : a(false)
    , b(false)
    , left(false)
    , up(false)
    , right(false)
    , down(false)
    , start(false)
    , select(false)
{
}

bool JoypadControllerAndroid::isKeyPressed(FunkyBoy::Controller::JoypadKey key) {
    switch (key) {
        case FunkyBoy::Controller::JoypadKey::JOYPAD_A:
            return a;
        case FunkyBoy::Controller::JoypadKey::JOYPAD_B:
            return b;
        case FunkyBoy::Controller::JoypadKey::JOYPAD_LEFT:
            return left;
        case FunkyBoy::Controller::JoypadKey::JOYPAD_UP:
            return up;
        case FunkyBoy::Controller::JoypadKey::JOYPAD_RIGHT:
            return right;
        case FunkyBoy::Controller::JoypadKey::JOYPAD_DOWN:
            return down;
        case FunkyBoy::Controller::JoypadKey::JOYPAD_START:
            return start;
        case FunkyBoy::Controller::JoypadKey::JOYPAD_SELECT:
            return select;
        default:
            return false;
    }
}
