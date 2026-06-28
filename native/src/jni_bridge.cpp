#include "auto_farm.h"
#include "game_reader.h"
#include <jni.h>
#include <android/log.h>
#include <pthread.h>
#include <unistd.h>
#include <atomic>
#include <string>

#define TAG "AyazJNI"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  TAG, __VA_ARGS__)

// ─── Reader Thread ────────────────────────────────────────────────────────────

static std::atomic<bool> g_reader_running{false};
static pthread_t         g_reader_thread{};
static ayaz::LibInfo     g_lib{};

static void* ReaderThreadFn(void*) {
    LOGI("Reader thread başladı");

    // libayaz2client.so'yu bul
    while (g_reader_running.load()) {
        g_lib = ayaz::FindGameLib();
        if (g_lib.valid()) break;
        LOGI("libayaz2client.so bekleniyor...");
        usleep(500000); // 0.5 sn
    }

    LOGI("lib bulundu, farm döngüsü başlıyor");

    // Farm tick döngüsü: 200ms aralık
    while (g_reader_running.load()) {
        ayaz::farm::FarmTick(g_lib);
        usleep(200000); // 200ms
    }

    LOGI("Reader thread bitti");
    return nullptr;
}

// ─── JNI Fonksiyonları ───────────────────────────────────────────────────────
// Paket: com.ayaz.nativeui

extern "C" {

// Reader thread'i başlat (uygulama açılırken çağrılır)
JNIEXPORT void JNICALL
Java_com_ayaz_nativeui_AyazNative_nativeStartReader(JNIEnv*, jclass) {
    if (g_reader_running.load()) return;
    g_reader_running.store(true);
    pthread_create(&g_reader_thread, nullptr, ReaderThreadFn, nullptr);
}

// Reader thread'i durdur
JNIEXPORT void JNICALL
Java_com_ayaz_nativeui_AyazNative_nativeStopReader(JNIEnv*, jclass) {
    g_reader_running.store(false);
    ayaz::farm::StopFarm();
    pthread_join(g_reader_thread, nullptr);
}

// Farm başlat
JNIEXPORT void JNICALL
Java_com_ayaz_nativeui_AyazNative_nativeStartAutoFarm(JNIEnv*, jclass) {
    ayaz::farm::StartFarm();
}

// Farm durdur
JNIEXPORT void JNICALL
Java_com_ayaz_nativeui_AyazNative_nativeStopAutoFarm(JNIEnv*, jclass) {
    ayaz::farm::StopFarm();
}

// Farm çalışıyor mu?
JNIEXPORT jboolean JNICALL
Java_com_ayaz_nativeui_AyazNative_nativeIsFarmRunning(JNIEnv*, jclass) {
    return ayaz::farm::IsFarmRunning() ? JNI_TRUE : JNI_FALSE;
}

// Farm durumu string olarak
JNIEXPORT jstring JNICALL
Java_com_ayaz_nativeui_AyazNative_nativeGetFarmStatus(JNIEnv* env, jclass) {
    const char* s = "IDLE";
    switch (ayaz::farm::GetFarmStatus()) {
        case ayaz::farm::FarmStatus::SEARCHING: s = "ARAMA"; break;
        case ayaz::farm::FarmStatus::MOVING:    s = "GİDİYOR"; break;
        case ayaz::farm::FarmStatus::ATTACKING: s = "SALDIRIYOR"; break;
        case ayaz::farm::FarmStatus::STUCK:     s = "TAKILDI"; break;
        case ayaz::farm::FarmStatus::LOOTING:   s = "TOPLUYOR"; break;
        default: break;
    }
    return env->NewStringUTF(s);
}

// Karakter ismini döner
JNIEXPORT jstring JNICALL
Java_com_ayaz_nativeui_AyazNative_nativeGetPlayerName(JNIEnv* env, jclass) {
    if (!g_lib.valid()) return env->NewStringUTF("");
    std::string name = ayaz::GetPlayerName(g_lib);
    return env->NewStringUTF(name.c_str());
}

// Login kontrolü (lib yüklendi mi?)
JNIEXPORT jboolean JNICALL
Java_com_ayaz_nativeui_AyazNative_nativeIsLoggedIn(JNIEnv*, jclass) {
    return g_lib.valid() ? JNI_TRUE : JNI_FALSE;
}

} // extern "C"
