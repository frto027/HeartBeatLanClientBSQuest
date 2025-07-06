# include some defines automatically made by qpm
include(qpm_defines.cmake)
include(target.cmake)

# override mod id
set(MOD_ID "HeartBeatLanReceiver")

# Enable link time optimization
# In my experience, this can be highly unstable but it nets a huge size optimization and likely performance
# However, the instability was seen using Android.mk/ndk-build builds. With Ninja + CMake, this problem seems to have been solved.
# As always, test thoroughly
# - Fern
# set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)

message("VCPKG $ENV{VCPKG_ROOT}")
set(VCPKG_TARGET_TRIPLET arm64-android)
set(VCPKG_CHAINLOAD_TOOLCHAIN_FILE ${CMAKE_ANDROID_NDK}/build/cmake/android.toolchain.cmake)
set(VCPKG_MANIFEST_DIR ${CMAKE_SOURCE_DIR})
set(VCPKG_INSTALLED_DIR ${CMAKE_SOURCE_DIR}/vcpkg_installed)
string(REPLACE "\\" "/" VCPKG_ROOT_WINDOWS_FIX $ENV{VCPKG_ROOT})
set(CMAKE_TOOLCHAIN_FILE ${VCPKG_ROOT_WINDOWS_FIX}/scripts/buildsystems/vcpkg.cmake)

set(websocketpp_DIR ${CMAKE_SOURCE_DIR}/vcpkg_installed/arm64-android/share/websocketpp)

cmake_minimum_required(VERSION 3.21)
project(${COMPILE_ID})

# export compile commands for significantly better intellisense
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# c++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED 20)

# define that stores the actual source directory
set(SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

# compile options used
add_compile_options(-frtti -fexceptions)
add_compile_options(-O3)
# compile definitions used
add_compile_definitions(VERSION=\"${MOD_VERSION}\")
add_compile_definitions(MOD_ID=\"${MOD_ID}\")

add_compile_definitions("GAME_VER_${TARGET_GAME_VERSION}")
add_compile_definitions("GAME_VERSION=\"${TARGET_GAME_VERSION}\"")

# handle some custom commands
option(WITH_REPLAY "Enable Replay mod support" OFF)
if(WITH_REPLAY)
  add_compile_definitions(WITH_REPLAY)
endif()

# recursively get all src files
RECURSE_FILES(cpp_file_list ${SOURCE_DIR}/*.cpp)
RECURSE_FILES(c_file_list ${SOURCE_DIR}/*.c)

if(
        (TARGET_GAME_VERSION STREQUAL "1_40_4") OR
        (TARGET_GAME_VERSION STREQUAL "1_40_6")
        )

        RECURSE_FILES(src_inline_hook_beatsaber_hook_local_extra_c ${EXTERN_DIR}/includes/beatsaber-hook/shared/inline-hook/*.c)
        RECURSE_FILES(src_inline_hook_beatsaber_hook_local_extra_cpp ${EXTERN_DIR}/includes/beatsaber-hook/shared/inline-hook/*.cpp)
        # add all src files to compile
        add_library(
                ${COMPILE_ID}
                SHARED
                ${cpp_file_list}
                ${c_file_list}
                ${src_inline_hook_beatsaber_hook_local_extra_c}
                ${src_inline_hook_beatsaber_hook_local_extra_cpp}
        )

else()

        # add all src files to compile
        add_library(
                ${COMPILE_ID}
                SHARED
                ${cpp_file_list}
                ${c_file_list}
        )

endif()

target_include_directories(${COMPILE_ID} PRIVATE .)

# add src dir as include dir
target_include_directories(${COMPILE_ID} PRIVATE ${SOURCE_DIR})
# add include dir as include dir
target_include_directories(${COMPILE_ID} PRIVATE ${INCLUDE_DIR})
# add shared dir as include dir
target_include_directories(${COMPILE_ID} PUBLIC ${SHARED_DIR})
# codegen includes 
target_include_directories(${COMPILE_ID} PRIVATE ${EXTERN_DIR}/includes/${CODEGEN_ID}/include)

find_package(websocketpp CONFIG REQUIRED)
target_link_libraries(${COMPILE_ID} PRIVATE websocketpp::websocketpp)

target_link_libraries(${COMPILE_ID} PRIVATE -llog)
# add extern stuff like libs and other includes
include(extern.cmake)

add_custom_command(TARGET ${COMPILE_ID} POST_BUILD
        COMMAND ${CMAKE_STRIP} -d --strip-all
        "lib${COMPILE_ID}.so" -o "stripped_lib${COMPILE_ID}.so"
        COMMENT "Strip debug symbols done on final binary.")

add_custom_command(TARGET ${COMPILE_ID} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory debug
        COMMENT "Make directory for debug symbols"
        )

add_custom_command(TARGET ${COMPILE_ID} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E rename lib${COMPILE_ID}.so debug/lib${COMPILE_ID}.so
        COMMENT "Rename the lib to debug_ since it has debug symbols"
        )

# strip debug symbols from the .so and all dependencies
add_custom_command(TARGET ${COMPILE_ID} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E rename stripped_lib${COMPILE_ID}.so lib${COMPILE_ID}.so
        COMMENT "Rename the stripped lib to regular"
        )
        foreach(so_file ${so_list})
        cmake_path(GET so_file FILENAME file)
        
        add_custom_command(TARGET ${COMPILE_ID} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy ${so_file} debug/${file}
                COMMENT "Copy so files for ndk stack"
        )
        
        add_custom_command(TARGET ${COMPILE_ID} POST_BUILD
                COMMAND ${CMAKE_STRIP} -g -S -d --strip-all ${so_file} -o ${file}
                COMMENT "Strip debug symbols from the dependencies")
        endforeach()
        
        foreach(a_file ${a_list})
        cmake_path(GET a_file FILENAME file)
        
        add_custom_command(TARGET ${COMPILE_ID} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy ${a_file} debug/${file}
        COMMENT "Copy a files for ndk stack")
        endforeach()
