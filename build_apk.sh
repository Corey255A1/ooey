#!/bin/bash
set -e

# Configuration
ANDROID_HOME="/home/corey/android/sdk"
ANDROID_NDK_HOME="/home/corey/android/ndk/android-ndk-r27d"
ABI="arm64-v8a"
API_LEVEL="30"
BUILD_TOOLS_VERSION="34.0.0"
BUILD_DIR="build_android_${ABI}"
APK_DIR="android_build"

# Set up PATH to include build-tools and platform-tools
export PATH="${ANDROID_HOME}/build-tools/${BUILD_TOOLS_VERSION}:${ANDROID_HOME}/platform-tools:$PATH"

echo "=== 1. Compiling Shared Library via NDK toolchain ==="
cmake -B ${BUILD_DIR} \
      -G Ninja \
      -DCMAKE_TOOLCHAIN_FILE=${ANDROID_NDK_HOME}/build/cmake/android.toolchain.cmake \
      -DANDROID_ABI=${ABI} \
      -DANDROID_PLATFORM=android-${API_LEVEL} \
      -DCMAKE_BUILD_TYPE=Release

cmake --build ${BUILD_DIR} --target hello_ooey

echo "=== 2. Structuring APK directory ==="
mkdir -p ${BUILD_DIR}/apk/lib/${ABI}
if [ -f "${BUILD_DIR}/examples/libhello_ooey.so" ]; then
    cp ${BUILD_DIR}/examples/libhello_ooey.so ${BUILD_DIR}/apk/lib/${ABI}/
else
    cp ${BUILD_DIR}/lib/libhello_ooey.so ${BUILD_DIR}/apk/lib/${ABI}/
fi

# Copy assets if any
if [ -d "assets" ]; then
    cp -r assets ${BUILD_DIR}/apk/
fi

echo "=== 3. Running aapt packaging ==="
aapt package -f -M ${APK_DIR}/AndroidManifest.xml \
             -S ${APK_DIR}/res \
             -I ${ANDROID_HOME}/platforms/android-${API_LEVEL}/android.jar \
             -F ${BUILD_DIR}/app-unsigned.apk \
             ${BUILD_DIR}/apk

echo "=== 4. Running zipalign (4-byte optimization) ==="
zipalign -f 4 ${BUILD_DIR}/app-unsigned.apk ${BUILD_DIR}/app-aligned.apk

echo "=== 5. Generating signing certificate (if missing) ==="
if [ ! -f my-key.keystore ]; then
    keytool -genkeypair -v \
            -keystore my-key.keystore \
            -alias ooey_key \
            -keyalg RSA \
            -keysize 2048 \
            -validity 10000 \
            -storepass password \
            -keypass password \
            -dname "CN=ooey, OU=dev, O=ooey, L=Earth, S=Universe, C=US"
fi

echo "=== 6. Signing APK package ==="
apksigner sign --ks my-key.keystore \
               --ks-key-alias ooey_key \
               --ks-pass pass:password \
               --key-pass pass:password \
               --out ${BUILD_DIR}/app.apk \
               ${BUILD_DIR}/app-aligned.apk

echo "=== 7. Build complete! ==="
echo "APK location: ${BUILD_DIR}/app.apk"

if [ "$1" == "--install" ]; then
    echo "=== 8. Deploying to active device ==="
    adb install -r ${BUILD_DIR}/app.apk
fi
