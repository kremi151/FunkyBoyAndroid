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

#include "app_state.h"

#include <fba_util/shared.h>
#include <fba_util/logging.h>
#include <fba_util/emulator_state.h>
#include <util/membuf.h>

void FunkyBoyAndroid::serializeState(app_save_state *state) {
    if (State::emulator->getCartridgeStatus() != FunkyBoy::CartridgeStatus::Loaded) {
        LOGD("No ROM loaded, skipping serialization");
        return;
    }

    if (State::romPath.size() < FB_ANDROID_APP_STATE_ROM_PATH_BUFFER_SIZE) {
        std::strcpy(state->romPath, State::romPath.c_str());
    } else {
        LOGW("ROM path size is too large, cannot be serialized\n");
    }

    FunkyBoy::Util::membuf membuf(state->state, FB_SAVE_STATE_MAX_BUFFER_SIZE, false);
    std::ostream ostream(&membuf);
    State::emulator->saveState(ostream);
}

void FunkyBoyAndroid::resumeFromState(app_save_state *state) {
    if (State::emulator->getCartridgeStatus() != FunkyBoy::CartridgeStatus::Loaded) {
        if (std::strlen(state->romPath) == 0) {
            LOGW("ROM path was not serialized, not resuming from previous state");
            return;
        }
        if (FunkyBoyAndroid::loadROM(state->romPath) != FunkyBoy::CartridgeStatus::Loaded) {
            LOGE("ROM could not be loaded, resuming from previous state failed");
            return;
        }
    }
    FunkyBoy::Util::membuf membuf(state->state, FB_SAVE_STATE_MAX_BUFFER_SIZE, true);
    std::istream istream(&membuf);
    State::emulator->loadState(istream);
    LOGD("Resumed emulation from previous state");
}