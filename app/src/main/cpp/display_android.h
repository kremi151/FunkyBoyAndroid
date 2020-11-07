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

#ifndef FB_ANDROID_CONTROLLER_DISPLAY_ANDROID_H
#define FB_ANDROID_CONTROLLER_DISPLAY_ANDROID_H

#include <controllers/display.h>

#include <android/native_window.h>

namespace FunkyBoy {

    namespace Controller {

        class DisplayControllerAndroid: public DisplayController {
        private:
            ANativeWindow *nativeWindow;
            int32_t offsetX;
            int32_t offsetY;
            uint32_t *pixels;
        public:
            DisplayControllerAndroid();
            ~DisplayControllerAndroid() override;

            void setNativeWindow(ANativeWindow *aNativeWindow, int32_t offsetX, int32_t offsetY);

            void drawScanLine(u8 y, u8 *buffer) override;
            void drawScreen() override;
        };

    }

}

#endif // FB_ANDROID_CONTROLLER_DISPLAY_ANDROID_H
