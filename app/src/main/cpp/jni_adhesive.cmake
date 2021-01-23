set(FB_ANDROID_DYNAMIC_INCLUDE_DIR "${CMAKE_BINARY_DIR}/generated/include")

macro(fb_generate_strings_cpp)
    file(STRINGS ../res/values/strings.xml FB_STRINGS_XML)

    message(STATUS "# READ ${FB_STRINGS_XML}")

    set(FB_STRINGS_IDX 0)
    foreach(LINE IN LISTS FB_STRINGS_XML)
        string(REGEX MATCH "<string name=\"([^\"]+)\"" _ "${LINE}")
        if(NOT "${CMAKE_MATCH_1}" STREQUAL "")
            message(STATUS "String: ${CMAKE_MATCH_1}")

            set(FB_STRINGS_CPP_ENUM_LINES "${FB_STRINGS_CPP_ENUM_LINES}\t\t${CMAKE_MATCH_1} = ${FB_STRINGS_IDX},\n")
            set(FB_STRINGS_CPP_SWITCH_LINES "${FB_STRINGS_CPP_SWITCH_LINES}\tcase ${CMAKE_MATCH_1}:\n\t\tstrName = \"${CMAKE_MATCH_1}\";\n\t\tbreak;\n")

            MATH(EXPR FB_STRINGS_IDX "${FB_STRINGS_IDX}+1")
        endif()
    endforeach()

    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/source/dynamic/fb_app_strings.h.in" "${CMAKE_BINARY_DIR}/generated/include/fb_app_strings.h" @ONLY)
    set(FB_ANDROID_DYNAMIC_SOURCES ${FB_ANDROID_DYNAMIC_SOURCES} "${CMAKE_BINARY_DIR}/generated/include/fb_app_strings.h")

    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/source/dynamic/fb_app_strings.cpp.in" "${CMAKE_BINARY_DIR}/generated/fb_app_strings.cpp" @ONLY)
    set(FB_ANDROID_DYNAMIC_SOURCES ${FB_ANDROID_DYNAMIC_SOURCES} "${CMAKE_BINARY_DIR}/generated/fb_app_strings.cpp")
endmacro()

