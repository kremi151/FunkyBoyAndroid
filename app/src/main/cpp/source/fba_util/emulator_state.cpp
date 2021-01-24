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

#include "emulator_state.h"

#include <fba_util/shared.h>
#include <fba_util/logging.h>
#include <fb_jni.h>

#include <fstream>

FunkyBoy::CartridgeStatus FunkyBoyAndroid::loadROM(const char *inRomPath) {
    auto result = State::emulator->loadGame(inRomPath);
    LOGD("ROM load status: %d, loaded from %s", result, inRomPath);
    if (result == FunkyBoy::CartridgeStatus::Loaded) {
        State::romPath = inRomPath;
    }
    return result;
}

void FunkyBoyAndroid::loadSaveGame(struct engine* engine) {
    FunkyBoy::fs::path saveGamePath = getSavePath(engine, State::emulator->getROMHeader());
    State::emulator->savePath = saveGamePath;
    State::initialSaveLoaded = true;
    LOGD("Save path: %s", saveGamePath.c_str());
    if (!saveGamePath.empty() && State::emulator->supportsSaving() /*&& FunkyBoy::fs::exists(saveGamePath)*/) {
        std::ifstream file(saveGamePath, std::ios::binary | std::ios::in);
        State::emulator->loadCartridgeRam(file);
    }
}

void FunkyBoyAndroid::saveGame() {
    auto &saveGamePath = State::emulator->savePath;
    if (!saveGamePath.empty() && State::emulator->supportsSaving() /*&& FunkyBoy::fs::exists(saveGamePath)*/) {
        std::ofstream file(saveGamePath, std::ios::binary | std::ios::out);
        State::emulator->writeCartridgeRam(file);
        LOGD("Cartridge RAM written to file");
    } else {
        LOGD("Game has no cartridge RAM");
    }
}