// Android app: one APK that runs the Lumen2D player (editor mode is added in M9).
// SDL3's Java side (org.libsdl.app.*) is taken from android/third_party/SDL, which CI clones
// at the same tag as the native SDL3 build (see .github/workflows/build-android.yml).
plugins {
    id("com.android.application")
}

android {
    namespace = "com.blackglass.lumen"
    compileSdk = 34
    ndkVersion = "26.1.10909125"

    defaultConfig {
        applicationId = "com.blackglass.lumen"
        minSdk = 24
        targetSdk = 34
        versionCode = 1
        versionName = "0.1.0"

        ndk {
            abiFilters += listOf("arm64-v8a", "x86_64")
        }

        externalNativeBuild {
            cmake {
                arguments(
                    "-DLUMEN_ROOT=${rootProject.projectDir.parentFile.absolutePath}",
                    "-DLUMEN_SDL_DIR=${rootProject.file("third_party/SDL").absolutePath}",
                )
            }
        }
    }

    externalNativeBuild {
        cmake {
            path = file("src/main/cpp/CMakeLists.txt")
            version = "3.22.1"
        }
    }

    sourceSets {
        getByName("main") {
            java.srcDirs(
                "src/main/java",
                rootProject.file("third_party/SDL/android-project/app/src/main/java"),
            )
        }
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }

    buildTypes {
        release {
            isMinifyEnabled = false
        }
    }
}
