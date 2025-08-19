# build libcurl manually
cmake -B build_third_party/curl -S third_party/curl -G Ninja "-DCMAKE_TOOLCHAIN_FILE=$ENV:ANDROID_NDK_HOME\build\cmake\android.toolchain.cmake" -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=24 -DCURL_ENABLE_SSL=OFF -DCURL_USE_LIBPSL=OFF -DBUILD_STATIC_LIBS=ON -DCURL_ZLIB=OFF
pushd build_third_party/curl
ninja
popd