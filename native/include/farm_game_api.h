#pragma once
#include "game_reader.h"
#include <cstdint>

namespace ayaz {
namespace farm {

// Fonksiyon pointer tipleri
using FnGetPixelPos     = void(*)(uintptr_t instance, Vector3* out);
using FnSetPixelPos     = void(*)(uintptr_t instance, const Vector3* pos);
using FnMoveToDest      = void(*)(uintptr_t instance, const Vector3* dest);
using FnAttack          = void(*)(uintptr_t instance, float angle);
using FnIsStone         = bool(*)(uintptr_t instance);
using FnGetMainActor    = uintptr_t(*)(uintptr_t player_ptr);
using FnGetCloseStone   = uintptr_t(*)(uintptr_t charmgr_ptr, uintptr_t main_actor);
using FnGetCloseItem    = bool(*)(uintptr_t item_mgr, const Vector3* pos, unsigned int* vid_out, unsigned int range);
using FnGetGroundItemPos= bool(*)(uintptr_t item_mgr, unsigned int vid, Vector3* pos_out);
using FnCanMoveToDest   = bool(*)(uintptr_t instance, const Vector3* dest);
using FnIsWalking       = bool(*)(uintptr_t instance);
using FnStartWalking    = void(*)(uintptr_t instance);

struct GameApi {
    FnGetPixelPos     getPixelPos     = nullptr;
    FnSetPixelPos     setPixelPos     = nullptr;
    FnMoveToDest      moveToDest      = nullptr;
    FnAttack          attack          = nullptr;
    FnIsStone         isStone         = nullptr;
    FnGetMainActor    getMainActor    = nullptr;
    FnGetCloseStone   getCloseStone   = nullptr;
    FnGetCloseItem    getCloseItem    = nullptr;
    FnGetGroundItemPos getGroundItemPos= nullptr;
    FnCanMoveToDest   canMoveToDest   = nullptr;
    FnIsWalking       isWalking       = nullptr;
    FnStartWalking    startWalking    = nullptr;
    bool              ready           = false;
};

// API'yi başlat (ilk çağrıda bind eder)
bool EnsureApi(const LibInfo& lib);

// Global API erişimi
GameApi& GetApi();

// Yardımcı: instance'ın pixel pozisyonunu oku
bool GetInstancePos(uintptr_t instance, Vector3& out);

// Yardımcı: hedefe hareket et
void MoveToPos(uintptr_t instance, const Vector3& dest);

// Yardımcı: saldır
void AttackInstance(uintptr_t actor, uintptr_t target);

// 2D mesafe
float Distance2D(const Vector3& a, const Vector3& b);

} // namespace farm
} // namespace ayaz
