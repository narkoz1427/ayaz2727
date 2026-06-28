#pragma once
#include "farm_game_api.h"

namespace ayaz {
namespace farm {

// Yakındaki yerdeki itemleri toplar
// actor: ana karakter instance pointer
// item_mgr: CPythonItem singleton pointer
// pickup_range: kaç piksel içindeki itemleri topla
void PickupNearbyItems(uintptr_t actor, uintptr_t item_mgr, float pickup_range);

} // namespace farm
} // namespace ayaz
