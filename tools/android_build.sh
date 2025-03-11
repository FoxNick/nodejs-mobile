#!/bin/bash

set -e

ROOT=${PWD}

if [ $# -lt 2 ]; then
  echo "Requires a path to the Android NDK and an SDK version number (optionally: target arch)"
  echo "Usage: android_build.sh <ndk_path> <sdk_version> [target_arch]"
  exit 1
fi

ANDROID_SDK_VERSION="$2"

SCRIPT_DIR="$(dirname "$BASH_SOURCE")"
cd "$SCRIPT_DIR"
SCRIPT_DIR=${PWD}

cd "$ROOT"
cd "$1"
ANDROID_NDK_PATH=${PWD}
cd "$SCRIPT_DIR"
cd ../

BUILD_ARCH() {
  # Clean previous compilation
  make clean
  rm -rf android-toolchain/

  # Compile
  # 新增环境变量
  export AR=$ANDROID_NDK_PATH/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-ar
  export RANLIB=$ANDROID_NDK_PATH/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-ranlib
  export STRIP=$ANDROID_NDK_PATH/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-strip
  export CFLAGS="-fPIC -static -Oz -DANDROID -D__ANDROID_API__=$ANDROID_SDK_VERSION"
  export LDFLAGS="-static -Wl,--gc-sections -llog -landroid"
  
  eval '"./android-configure" "$ANDROID_NDK_PATH" $ANDROID_SDK_VERSION $TARGET_ARCH \
    -DANDROID_STL="c++_static" \
    -DCMAKE_CXX_FLAGS="-fPIC -static-libstdc++" \
    -DCMAKE_EXE_LINKER_FLAGS="-static -llog -landroid"'
  make -j $(getconf _NPROCESSORS_ONLN)
  $STRIP --strip-unneeded out/Release/node

  # Move binaries
  TARGET_ARCH_FOLDER="$TARGET_ARCH"
  if [ "$TARGET_ARCH_FOLDER" == "arm" ]; then
    # Use the Android NDK ABI name.
    TARGET_ARCH_FOLDER="armeabi-v7a"
  elif [ "$TARGET_ARCH_FOLDER" == "arm64" ]; then
    # Use the Android NDK ABI name.
    TARGET_ARCH_FOLDER="arm64-v8a"
  fi
  mkdir -p "out_android/$TARGET_ARCH_FOLDER/"
  OUTPUT1="out/Release"
  OUTPUT2="out/Release/obj.target/libnode.so"
  if [ -d "$OUTPUT1" ]; then
    cp -rp "$OUTPUT1" "out_android/$TARGET_ARCH_FOLDER"
  elif [ -f "$OUTPUT2" ]; then
    cp "$OUTPUT2" "out_android/$TARGET_ARCH_FOLDER/libnode.so"
  else
    echo "Could not find libnode.so file after compilation"
    exit 1
  fi
}

if [ $# -eq 2 ]; then
  TARGET_ARCH="arm"
  BUILD_ARCH
  # TARGET_ARCH="x86"
  # BUILD_ARCH
  TARGET_ARCH="arm64"
  BUILD_ARCH
  TARGET_ARCH="x86_64"
  BUILD_ARCH
else
  TARGET_ARCH=$3
  BUILD_ARCH
fi

source $SCRIPT_DIR/copy_libnode_headers.sh android

cd "$ROOT"
