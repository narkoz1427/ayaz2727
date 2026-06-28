#include "farm_game_api.h"
#include <android/log.h>
#include <cmath>

#define TAG "AyazApi"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

namespace ayaz {
namespace farm {

static GameApi g_api;

// offset'ten fonksiyon pointer üret
template<typename FnPtr>
static FnPtr Bind(uintptr_t base, uintptr_t offset) {
    return reinterpret_cast<FnPtr>(base + offset);
}

bool EnsureApi(const LibInfo& lib) {
    if (g_api.ready) return true;
    if (!lib.valid()) {
        LOGE("EnsureApi: lib geçersiz");
        return false;
    }

    uintptr_t b = lib.base;

    g_api.getPixelPos    = Bind<FnGetPixelPos>   (b, OFF_GET_PIXEL_POS);
    g_api.setPixelPos    = Bind<FnSetPixelPos>   (b, OFF_SET_PIXEL_POS);
    g_api.moveToDest     = Bind<FnMoveToDest>    (b, OFF_MOVE_TO_DEST);
    g_api.attack         = Bind<FnAttack>        (b, OFF_ATTACK);
    g_api.isStone        = Bind<FnIsStone>       (b, OFF_IS_STONE);
    g_api.getMainActor   = Bind<FnGetMainActor>  (b, OFF_GET_MAIN_ACTOR);
    g_api.getCloseStone  = Bind<FnGetCloseStone> (b, OFF_GET_CLOSE_STONE);
    g_api.getCloseItem   = Bind<FnGetCloseItem>  (b, OFF_GET_CLOSE_ITEM);
    g_api.getGroundItemPos = Bind<FnGetGroundItemPos>(b, OFF_GET_GROUND_ITEM_POS);
    g_api.canMoveToDest  = Bind<FnCanMoveToDest> (b, OFF_CAN_MOVE_TO_DEST);
    g_api.isWalking      = Bind<FnIsWalking>     (b, OFF_IS_WALKING);
    g_api.startWalking   = Bind<FnStartWalking>  (b, OFF_START_WALKING);

    g_api.ready = true;
    LOGI("GameApi bind tamamlandı, base=0x%lx", b);
    return true;
}

GameApi& GetApi() {
    return g_api;
}

bool GetInstancePos(uintptr_t instance, Vector3& out) {
    if (!instance || !g_api.ready) return false;
    if (!IsReadable(instance, 8)) return false;
    g_api.getPixelPos(instance, &out);
    return true;
}

void MoveToPos(uintptr_t instance, const Vector3& dest) {
    if (!instance || !g_api.ready) return;
    g_api.moveToDest(instance, &dest);
}

void AttackInstance(uintptr_t actor, uintptr_t target) {
    if (!actor || !target || !g_api.ready) return;

    // Saldırı açısını hesapla: actor -> target yönü
    Vector3 apos{}, tpos{};
    if (!GetInstancePos(actor, apos)) return;
    if (!GetInstancePos(target, tpos)) return;

    float dx = tpos.x - apos.x;
    float dy = tpos.y - apos.y;
    float angle = atan2f(dy, dx) * (180.0f / 3.14159265f);

    g_api.attack(actor, angle);
}

float Distance2D(const Vector3& a, const Vector3& b) {
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    return sqrtf(dx * dx + dy * dy);
}

} // namespace farm
} // namespace ayaz
