#
# Copyright (C) The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# Modifications copyright (C) 2020 Michel Kremer (kremi151)
#

cmake_minimum_required(VERSION 3.13)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(FB_ROOT_DIR ${CMAKE_SOURCE_DIR}/funkyboy)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${FB_ROOT_DIR}/cmake-common)

find_package (oboe REQUIRED CONFIG)

include(jni_adhesive.cmake)

# build native_app_glue as a static lib
set(${CMAKE_C_FLAGS}, "${CMAKE_C_FLAGS}")
add_library(native_app_glue STATIC
    ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)

# now build app's shared lib
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11 -Wall")

# Export ANativeActivity_onCreate(),
# Refer to: https://github.com/android-ndk/ndk/issues/381.
set(CMAKE_SHARED_LINKER_FLAGS
    "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")

set(SOURCES
        source/main.cpp
        source/fb_jni.cpp
        source/fba_util/app_state.cpp
        source/fba_util/emulator_state.cpp
        source/engine/init_display.cpp
        source/ui/draw_bitmap.cpp
        source/ui/draw_controls.cpp
        source/ui/draw_text.cpp
        source/controllers/display_android.cpp
        source/controllers/audio_android.cpp
        )

set(HEADERS
        source/fb_jni.h
        source/fba_util/logging.h
        source/fba_util/app_state.h
        source/fba_util/emulator_state.h
        source/fba_util/shared.h
        source/engine/engine.h
        source/engine/ui_obj.h
        source/engine/init_display.h
        source/ui/draw_bitmap.h
        source/ui/draw_controls.h
        source/ui/draw_text.h
        source/controllers/display_android.h
        source/controllers/audio_android.h
        )

fb_generate_strings_cpp()

add_library(fb_android SHARED ${SOURCES} ${HEADERS} ${FB_ANDROID_DYNAMIC_SOURCES})

target_include_directories(fb_android PRIVATE
    ${ANDROID_NDK}/sources/android/native_app_glue
    "${FB_ROOT_DIR}/core/source"
    "${CMAKE_CURRENT_SOURCE_DIR}/source"
    ${FB_ANDROID_DYNAMIC_INCLUDE_DIR}
    )

add_subdirectory("${FB_ROOT_DIR}/core" fb_core_build)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DFB_DEBUG")

fb_use_autosave(fb_core)
fb_use_sound(fb_core)

# add lib dependencies
target_link_libraries(fb_android
    android
    native_app_glue
    jnigraphics
    fb_core
    log
    oboe::oboe
    )
