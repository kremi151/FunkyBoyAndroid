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

#include "fb_jni.h"
#include <cstring>
#include <unistd.h>
#include <android/native_activity.h>

void FunkyBoyAndroid::requestPickRom(struct engine* engine) {
    ANativeActivity *nativeActivity = engine->app->activity;
    JNIEnv *env = engine->env;

    jobject nativeActivityObj = nativeActivity->clazz; // "clazz" is misnamed, this is the actual activity instance
    jclass nativeActivityClass = env->GetObjectClass(nativeActivity->clazz);
    jmethodID method = env->GetMethodID(nativeActivityClass, "requestPickRom", "()V");

    env->CallVoidMethod(nativeActivityObj, method);
}

jobject FunkyBoyAndroid::loadBitmap(struct engine* engine, jint type) {
    ANativeActivity *nativeActivity = engine->app->activity;
    JNIEnv *env = engine->env;

    jobject nativeActivityObj = nativeActivity->clazz; // "clazz" is misnamed, this is the actual activity instance
    jclass nativeActivityClass = env->GetObjectClass(nativeActivity->clazz);
    jmethodID method = env->GetMethodID(nativeActivityClass, "loadBitmap", "(I)Landroid/graphics/Bitmap;");

    jobject bitmap = env->CallObjectMethod(nativeActivityObj, method, type);

    return bitmap;
}

std::string FunkyBoyAndroid::getSavePath(struct engine* engine, const FunkyBoy::ROMHeader *romHeader) {
    ANativeActivity *nativeActivity = engine->app->activity;
    JNIEnv *env = engine->env;

    jobject nativeActivityObj = nativeActivity->clazz; // "clazz" is misnamed, this is the actual activity instance
    jclass nativeActivityClass = env->GetObjectClass(nativeActivity->clazz);
    jmethodID method = env->GetMethodID(nativeActivityClass, "getSavePath", "(Ljava/lang/String;II)Ljava/lang/String;");

    char romTitleSafe[FB_ROM_HEADER_TITLE_BYTES + 1]{};
    std::memcpy(romTitleSafe, romHeader->title, FB_ROM_HEADER_TITLE_BYTES);

    jstring romTitle = env->NewStringUTF(romTitleSafe);
    auto path = static_cast<jstring>(env->CallObjectMethod(
            nativeActivityObj, method, romTitle,
            romHeader->destinationCode,
            (romHeader->globalChecksum[0] << 8) | romHeader->globalChecksum[1]
    ));

    std::string savePath;
    jboolean isCopy;
    const char *jstr = env->GetStringUTFChars(path, &isCopy);
    savePath = jstr;
    env->ReleaseStringUTFChars(path, jstr);

    env->DeleteLocalRef(romTitle);
    env->DeleteLocalRef(path);

    return savePath;
}

extern "C" {

    JNIEXPORT void JNICALL Java_lu_kremi151_funkyboy_FunkyBoyActivity_romPicked(JNIEnv *env, jobject, jstring path) {
        jboolean isCopy;
        auto path_cstr = env->GetStringUTFChars(path, &isCopy);

        size_t strln = std::strlen(path_cstr);

        write(fbMsgPipe[1], reinterpret_cast<char *>(&strln), sizeof(size_t));
        write(fbMsgPipe[1], path_cstr, strln);

        env->ReleaseStringUTFChars(path, path_cstr);
    }

}