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
    builder.setFramesPerDataCallback(FB_ANDROID_AUDIO_BUFFER_SIZE / 2);
    builder.setFramesPerDataCallback(100);

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
    // TODO: See https://github.com/google/oboe/blob/master/docs/GettingStarted.md#creating-an-audio-stream

    // We requested AudioFormat::Float so we assume we got it.
    // For production code always check what format
    // the stream has and cast to the appropriate type.
    /*auto *outputData = static_cast<float *>(audioData);

    // Generate random numbers (white noise) centered around zero.
    const float amplitude = 0.2f;
    for (int i = 0; i < numFrames; ++i){
        outputData[i] = ((float)drand48() - 0.5f) * 2 * amplitude;
    }

    return oboe::DataCallbackResult::Continue;*/

    /*float *floatData = (float *) audioData;
    for (int i = 0; i < numFrames; ++i) {
        float sampleValue = kAmplitude * sinf(mPhase);
        for (int j = 0; j < kChannelCount; j++) {
            floatData[i * kChannelCount + j] = sampleValue;
        }
        mPhase += mPhaseIncrement;
        if (mPhase >= kTwoPi) mPhase -= kTwoPi;
    }*/

    const size_t bufferPos = bufferPosition;
    bufferPosition = FB_ANDROID_AUDIO_BUFFER_SIZE; // Soft lock

    // Copy samples
    float *floatData = (float *) audioData;
    const int32_t samples = std::min(bufferPos, static_cast<size_t>(numFrames * 2));
    for (int32_t i = 0 ; i < samples ; i++) {
        floatData[i] = buffer[i];
    }

    audioStream->setBufferSizeInFrames(samples / 2);

    // Move remaining samples
    if (samples < bufferPos) {
        const size_t samplesToMove = bufferPos - samples;
        size_t j = 0;
        for (size_t i = 0 ; i < samplesToMove ; i++) {
            buffer[j++] = buffer[samples + i];
        }

        // Unlock buffer
        bufferPosition = samplesToMove;
    } else {
        // Unlock buffer
        bufferPosition = 0;
    }

    return oboe::DataCallbackResult::Continue;
}

void AudioControllerAndroid::pushSample(float left, float right) {
    while (bufferPosition >= FB_ANDROID_AUDIO_BUFFER_SIZE - 2) {
        // Wait
    }
    buffer[bufferPosition++] = left;
    buffer[bufferPosition++] = right;
}