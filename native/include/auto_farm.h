#pragma once
#include "farm_game_api.h"

namespace ayaz {
namespace farm {

// Farm durumu (JNI köprüsü için)
enum class FarmStatus {
    IDLE,
    SEARCHING,   // metin aranıyor
    MOVING,      // metine gidiliyor
    ATTACKING,   // metine saldırılıyor
    STUCK,       // takıldı, kurtulmaya çalışıyor
    LOOTING,     // item toplanıyor
};

// Farm başlat/durdur
void StartFarm();
void StopFarm();
bool IsFarmRunning();
FarmStatus GetFarmStatus();

// Her frame/tick çağrılır (reader thread'den)
void FarmTick(const LibInfo& lib);

// Sabitler
static constexpr float kAttackRange      = 500.0f;   // bu mesafede saldır
static constexpr float kMoveRange        = 5000.0f;  // bu kadara kadar metin ara
static constexpr float kPickupRange      = 400.0f;   // item toplama mesafesi
static constexpr float kStuckThreshold   = 20.0f;    // bu kadar hareket yoksa takıldı say
static constexpr int   kStuckTickLimit   = 10;        // kaç tick hareketsiz kalınca stuck
static constexpr float kStuckRadius      = 200.0f;   // kurtulma dairesi yarıçapı
static constexpr int   kStuckTryCount    = 8;         // daire üzerinde kaç nokta dene

} // namespace farm
} // namespace ayaz
