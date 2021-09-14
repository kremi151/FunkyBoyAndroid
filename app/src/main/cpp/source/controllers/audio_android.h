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

#ifndef FB_ANDROID_CONTROLLERS_AUDIO_ANDROID_H
#define FB_ANDROID_CONTROLLERS_AUDIO_ANDROID_H

#include <oboe/Oboe.h>
#include <controllers/audio.h>
#include <util/LockFreeQueue.h>

#define FB_ANDROID_AUDIO_QUEUE_SIZE 4096

namespace FunkyBoyAndroid::Controller {

    class AudioControllerAndroid: public oboe::AudioStreamDataCallback, public FunkyBoy::Controller::AudioController {
    private:
        oboe::ManagedStream managedStream;
        oboe::Result streamResult;
        bool playing;

        LockFreeQueue<float, FB_ANDROID_AUDIO_QUEUE_SIZE, size_t> queue;

    public:
        AudioControllerAndroid();
        ~AudioControllerAndroid() override;

        void pushSample(float left, float right) override;

        void setPlaying(bool playing);

        oboe::DataCallbackResult onAudioReady(oboe::AudioStream *audioStream, void *audioData, int32_t numFrames) override;
    };

}

#endif //FB_ANDROID_CONTROLLERS_AUDIO_ANDROID_H
