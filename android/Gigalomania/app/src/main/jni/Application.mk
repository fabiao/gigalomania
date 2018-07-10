
# Uncomment this if you're using STL in your project
# See CPLUSPLUS-SUPPORT.html in the NDK documentation for more information

# we need support for STL and C++ exceptions:
APP_STL := stlport_static 
#APP_STL := gnustl_static

APP_ABI := armeabi-v7a armeabi x86
#APP_ABI := armeabi-v7a armeabi
#APP_ABI := armeabi
# N.B., APP_ABI seems to be ignored in Android Studio, instead this is set via abiFilters in build.gradle
# see https://code.google.com/p/android/issues/detail?id=225123

# The NDK takes the APP_PLATFORM from project.properties by default, but I had to change
# project.properties from API 12 to 15 (to work in Eclipse, as SDK 12 doesn't seem available anymore).
# So we set APP_PLATFORM to 12 here.
APP_PLATFORM := android-12

# Needed to avoid compilation errors for SDL_image - oddly only appear when using SDL 2.0.4; 2.0.3 was fine
APP_CFLAGS += -Wno-error=format-security
