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

#include "audio_android.h"

#include <algorithm>
#include <fba_util/logging.h>

using namespace FunkyBoyAndroid::Controller;

AudioControllerAndroid::AudioControllerAndroid() {
    oboe::AudioStreamBuilder builder;
    builder.setDirection(oboe::Direction::Output);
    builder.setPerformanceMode(oboe::PerformanceMode::LowLatency);
    builder.setSharingMode(oboe::SharingMode::Exclusive);
    builder.setFormat(oboe::AudioFormat::Float);
    builder.setChannelCount(oboe::ChannelCount::Stereo);
    builder.setDataCallback(this);
    builder.setFramesPerDataCallback(400);

    streamResult = builder.openManagedStream(managedStream);
    if (streamResult == oboe::Result::OK) {
        managedStream->requestStart();
    } else {
        LOGE("Failed to create stream. Error: %s", oboe::convertToText(streamResult));
    }
}

AudioControllerAndroid::~AudioControllerAndroid() {
    if (streamResult == oboe::Result::OK) {
        managedStream->requestStop();
    }
}

oboe::DataCallbackResult AudioControllerAndroid::onAudioReady(oboe::AudioStream *audioStream, void *audioData, int32_t numFrames) {
    auto floatData = (float *) audioData;

    float sample;
    const size_t samples = std::min(queue.size(), static_cast<size_t>(numFrames * 2));
    for (size_t i = 0 ; i < samples ; i++) {
        if (!queue.pop(sample)) {
            break;
        }
        floatData[i] = sample;
    }

    return oboe::DataCallbackResult::Continue;
}

void AudioControllerAndroid::pushSample(float left, float right) {
    while (!queue.push(left)) {
        // Wait
    }
    while (!queue.push(right)) {
        // Wait
    }
}