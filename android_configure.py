import platform
import sys
import os

# TODO: In next version, it will be a JSON file listing all the patches, and then it will iterate through to apply them.
def patch_android():
    print("- Patches List -")
    print("[1] [deps/v8/src/trap-handler/trap-handler.h] related to https://github.com/nodejs/node/issues/36287")
    if platform.system() == "Linux":
        os.system('patch -f ./deps/v8/src/trap-handler/trap-handler.h < ./android-patches/trap-handler.h.patch')
        os.system('patch -f ./deps/v8/src/codegen/arm/constants-arm.h < ./android-patches/constants-arm.h.patch')
        os.system('patch -f ./deps/v8/src/heap/base/asm/x64/push_registers_asm.cc < ./android-patches/push_registers_asm.cc.patch')

    print("\033[92mInfo: \033[0m" + "Tried to patch.")

if platform.system() == "Windows":
    print("android-configure is not supported on Windows yet.")
    sys.exit(1)

if len(sys.argv) == 2 and sys.argv[1] == "patch":
    patch_android()
    sys.exit(0)

if len(sys.argv) != 4:
    print("Usage: ./android-configure [patch] <path to the Android NDK> <Android SDK version> <target architecture>")
    sys.exit(1)

if not os.path.exists(sys.argv[1]) or not os.listdir(sys.argv[1]):
    print("\033[91mError: \033[0m" + "Invalid path to the Android NDK")
    sys.exit(1)

if int(sys.argv[2]) < 24:
    print("\033[91mError: \033[0m" + "Android SDK version must be at least 24 (Android 7.0)")
    sys.exit(1)

android_ndk_path = sys.argv[1]
android_sdk_version = sys.argv[2]
arch = sys.argv[3]

if arch == "arm":
    DEST_CPU = "arm"
    TOOLCHAIN_PREFIX = "armv7a-linux-androideabi"
elif arch in ("aarch64", "arm64"):
    DEST_CPU = "arm64"
    TOOLCHAIN_PREFIX = "aarch64-linux-android"
    arch = "arm64"
elif arch == "x86":
    DEST_CPU = "ia32"
    TOOLCHAIN_PREFIX = "i686-linux-android"
elif arch == "x86_64":
    DEST_CPU = "x64"
    TOOLCHAIN_PREFIX = "x86_64-linux-android"
    arch = "x64"
else:
    print("\033[91mError: \033[0m" + "Invalid target architecture, must be one of: arm, arm64, aarch64, x86, x86_64")
    sys.exit(1)

print("\033[92mInfo: \033[0m" + "Configuring for " + DEST_CPU + "...")

if platform.system() == "Darwin":
    host_os = "darwin"
    toolchain_path = android_ndk_path + "/toolchains/llvm/prebuilt/darwin-x86_64"

elif platform.system() == "Linux":
    host_os = "linux"
    toolchain_path = android_ndk_path + "/toolchains/llvm/prebuilt/linux-x86_64"

os.environ['PATH'] += os.pathsep + toolchain_path + "/bin"
os.environ['CC'] = toolchain_path + "/bin/" + TOOLCHAIN_PREFIX + android_sdk_version + "-" +  "clang"
os.environ['CXX'] = toolchain_path + "/bin/" + TOOLCHAIN_PREFIX + android_sdk_version + "-" + "clang++"
# nodejs-mobile patch: add host CC and CXX
os.environ['CC_host'] = os.popen('command -v gcc').read().strip()
os.environ['CXX_host'] = os.popen('command -v g++').read().strip()

GYP_DEFINES = "target_arch=" + arch
GYP_DEFINES += " v8_target_arch=" + DEST_CPU
GYP_DEFINES += " android_target_arch=" + arch
GYP_DEFINES += " host_os=" + host_os + " OS=android"
GYP_DEFINES += " ANDROID_NDK_ROOT=" + android_ndk_path
GYP_DEFINES += " ANDROID_NDK_SYSROOT=" + toolchain_path + "/sysroot"
os.environ['GYP_DEFINES'] = GYP_DEFINES

if os.path.exists("./configure"):
    # nodejs-mobile patch: added --with-intl=none 和 --shared 
    os.system("./configure --dest-cpu=" + DEST_CPU + " --dest-os=android --openssl-no-asm --cross-compiling --enable-static --disable-shared --with-intl=full-icu")
