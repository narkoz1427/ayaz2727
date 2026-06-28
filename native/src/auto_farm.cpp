#include "auto_farm.h"
#include "farm_loot.h"
#include <android/log.h>
#include <cmath>
#include <atomic>

#define TAG "AyazFarm"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

namespace ayaz {
namespace farm {

// ─── State ──────────────────────────────────────────────────────────────────

static std::atomic<bool>       g_running{false};
static std::atomic<FarmStatus> g_status{FarmStatus::IDLE};

// Stuck detection
static Vector3 g_last_pos{};
static int     g_stuck_ticks   = 0;
static int     g_stuck_try_idx = 0;  // hangi daire noktasındayız

// Mevcut hedef
static uintptr_t g_current_target = 0;

// ─── Yardımcılar ─────────────────────────────────────────────────────────────

// Daire üzerindeki N. noktayı hesapla (0'dan kStuckTryCount-1'e)
static Vector3 CirclePoint(const Vector3& center, float radius, int idx, int total) {
    float angle = (2.0f * 3.14159265f / total) * idx;
    return Vector3{
        center.x + radius * cosf(angle),
        center.y + radius * sinf(angle),
        center.z
    };
}

// Pozisyon değişti mi?
static bool HasMoved(const Vector3& current, const Vector3& last) {
    return Distance2D(current, last) > kStuckThreshold;
}

// Singleton pointer'larını al
static bool GetSingletons(const LibInfo& lib,
                          uintptr_t& out_player,
                          uintptr_t& out_charmgr,
                          uintptr_t& out_item_mgr)
{
    out_player   = GetSingletonPtr(lib, OFF_PLAYER_SINGLETON);
    out_charmgr  = GetSingletonPtr(lib, OFF_CHARMGR_SINGLETON);
    out_item_mgr = GetSingletonPtr(lib, OFF_ITEM_SINGLETON);

    if (!out_player || !out_charmgr || !out_item_mgr) {
        LOGE("Singleton alınamadı");
        return false;
    }
    return true;
}

// ─── Ana tick ────────────────────────────────────────────────────────────────

void FarmTick(const LibInfo& lib) {
    if (!g_running.load()) return;
    if (!EnsureApi(lib))   return;

    GameApi& api = GetApi();

    // Singleton'ları al
    uintptr_t player_ptr = 0, charmgr_ptr = 0, item_mgr = 0;
    if (!GetSingletons(lib, player_ptr, charmgr_ptr, item_mgr)) return;

    // Ana karakter instance pointer
    uintptr_t actor = api.getMainActor(player_ptr);
    if (!actor || !IsReadable(actor, 8)) {
        g_status.store(FarmStatus::SEARCHING);
        return;
    }

    // Karakterin mevcut pozisyonu
    Vector3 cur_pos{};
    if (!GetInstancePos(actor, cur_pos)) return;

    // ── LOOT: önce yakındaki itemleri topla ──────────────────────────────────
    g_status.store(FarmStatus::LOOTING);
    PickupNearbyItems(actor, item_mgr, kPickupRange);

    // ── STUCK DETECTION ──────────────────────────────────────────────────────
    if (g_current_target != 0) {
        // Hedefe gidiyorken takıldık mı?
        if (!HasMoved(cur_pos, g_last_pos)) {
            g_stuck_ticks++;
        } else {
            g_stuck_ticks   = 0;
            g_stuck_try_idx = 0;
        }

        if (g_stuck_ticks >= kStuckTickLimit) {
            g_status.store(FarmStatus::STUCK);

            if (g_stuck_try_idx >= kStuckTryCount) {
                // Tüm noktaları denedik, hedefi sıfırla ve yeniden ara
                LOGI("Stuck: kurtulamadı, hedef sıfırlanıyor");
                g_current_target = 0;
                g_stuck_ticks    = 0;
                g_stuck_try_idx  = 0;
            } else {
                // Daire üzerindeki bir sonraki noktaya git
                Vector3 escape = CirclePoint(cur_pos, kStuckRadius,
                                             g_stuck_try_idx, kStuckTryCount);
                LOGI("Stuck: daire noktası %d (%.0f, %.0f)",
                     g_stuck_try_idx, escape.x, escape.y);
                MoveToPos(actor, escape);
                g_stuck_try_idx++;
            }

            g_last_pos = cur_pos;
            return;
        }
    }

    g_last_pos = cur_pos;

    // ── MEVCUT HEDEFİ DOĞRULA ────────────────────────────────────────────────
    if (g_current_target != 0) {
        // Hâlâ geçerli mi?
        if (!IsReadable(g_current_target, 8) || !api.isStone(g_current_target)) {
            LOGI("Hedef geçersiz, yeniden aranıyor");
            g_current_target = 0;
        }
    }

    // ── YENİ HEDEFİ BUL ──────────────────────────────────────────────────────
    if (g_current_target == 0) {
        g_status.store(FarmStatus::SEARCHING);
        uintptr_t stone = api.getCloseStone(charmgr_ptr, actor);

        if (!stone || !IsReadable(stone, 8)) {
            // Bölgede metin yok, dur
            LOGI("Bölgede metin taşı yok");
            return;
        }

        // Sadece metin taşı (IsStone kontrolü)
        if (!api.isStone(stone)) {
            LOGI("Bulunan instance metin taşı değil, atlanıyor");
            return;
        }

        g_current_target = stone;
        g_stuck_ticks    = 0;
        g_stuck_try_idx  = 0;
        LOGI("Yeni hedef metin taşı: 0x%lx", stone);
    }

    // ── HEDEFE SALDIRI VEYA HAREKET ──────────────────────────────────────────
    Vector3 target_pos{};
    if (!GetInstancePos(g_current_target, target_pos)) {
        g_current_target = 0;
        return;
    }

    float dist = Distance2D(cur_pos, target_pos);

    if (dist <= kAttackRange) {
        // Saldır
        g_status.store(FarmStatus::ATTACKING);
        AttackInstance(actor, g_current_target);
    } else if (dist <= kMoveRange) {
        // Yaklaş
        g_status.store(FarmStatus::MOVING);
        MoveToPos(actor, target_pos);
    } else {
        // Çok uzak, yeniden ara
        LOGI("Hedef çok uzak (%.0f px), yeniden aranıyor", dist);
        g_current_target = 0;
    }
}

// ─── Başlat / Durdur ─────────────────────────────────────────────────────────

void StartFarm() {
    g_current_target = 0;
    g_stuck_ticks    = 0;
    g_stuck_try_idx  = 0;
    g_last_pos       = {};
    g_running.store(true);
    g_status.store(FarmStatus::SEARCHING);
    LOGI("Farm başladı");
}

void StopFarm() {
    g_running.store(false);
    g_status.store(FarmStatus::IDLE);
    g_current_target = 0;
    LOGI("Farm durduruldu");
}

bool IsFarmRunning() {
    return g_running.load();
}

FarmStatus GetFarmStatus() {
    return g_status.load();
}

} // namespace farm
} // namespace ayaz
