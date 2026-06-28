#include "farm_loot.h"
#include <android/log.h>

#define TAG "AyazLoot"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  TAG, __VA_ARGS__)

namespace ayaz {
namespace farm {

// Oyun içinde pickup komutu vermek için
// chrPickupItem Python wrapper'ı yerine doğrudan item VID'yi kullanıyoruz.
// CPythonPlayer singleton üzerinden pickup çağrısı yapılır.
// Offset: playerPickupItem → 0x6e98a0 (playerGetTarget ile aynı bölge)
using FnPickupItem = void(*)(uintptr_t player_ptr, unsigned int vid);
static constexpr uintptr_t OFF_PICKUP_ITEM = 0x006e98a0;

static FnPickupItem g_pickupFn = nullptr;
static bool         g_pickupBound = false;

static void EnsurePickup(uintptr_t base) {
    if (g_pickupBound) return;
    g_pickupFn = reinterpret_cast<FnPickupItem>(base + OFF_PICKUP_ITEM);
    g_pickupBound = true;
}

void PickupNearbyItems(uintptr_t actor, uintptr_t item_mgr, float pickup_range) {
    if (!actor || !item_mgr) return;

    GameApi& api = GetApi();
    if (!api.ready) return;

    // Karakterin mevcut pozisyonu
    Vector3 actor_pos{};
    if (!GetInstancePos(actor, actor_pos)) return;

    // Base adres game_reader'dan zaten biliniyor, static cache kullan
    // EnsurePickup için base'i farm_game_api'den al
    // Pickup offset'ini getCloseStone'un base'inden türet
    uintptr_t base = reinterpret_cast<uintptr_t>(api.getCloseStone)
                     - OFF_GET_CLOSE_STONE;
    EnsurePickup(base);

    // En fazla 10 item dene (her tick'te çok fazla işlem yapma)
    static constexpr int MAX_ITEMS_PER_TICK = 10;
    unsigned int player_singleton = *reinterpret_cast<unsigned int*>(
        base + OFF_PLAYER_SINGLETON);

    for (int i = 0; i < MAX_ITEMS_PER_TICK; i++) {
        unsigned int vid = 0;
        unsigned int range = static_cast<unsigned int>(pickup_range);

        // Yakındaki item var mı?
        bool found = api.getCloseItem(item_mgr, &actor_pos, &vid, range);
        if (!found || vid == 0) break;

        // Item pozisyonunu kontrol et
        Vector3 item_pos{};
        if (!api.getGroundItemPos(item_mgr, vid, &item_pos)) break;

        float dist = Distance2D(actor_pos, item_pos);
        if (dist > pickup_range) break;

        LOGI("Item topla: vid=%u dist=%.1f", vid, dist);

        // Pickup komutu ver
        if (g_pickupFn && player_singleton) {
            g_pickupFn(player_singleton, vid);
        }
    }
}

} // namespace farm
} // namespace ayaz
