#pragma once
#include <cstdint>
#include <string>

// libayaz2client.so içindeki singleton adresleri (offset - base adrese eklenecek)
static constexpr uintptr_t OFF_CHARMGR_SINGLETON  = 0x00db44c8;
static constexpr uintptr_t OFF_PLAYER_SINGLETON    = 0x00db4528;
static constexpr uintptr_t OFF_ITEM_SINGLETON      = 0x00db44d8;

// Fonksiyon offsetleri
static constexpr uintptr_t OFF_GET_PIXEL_POS       = 0x006418e8; // CInstanceBase::NEW_GetPixelPosition
static constexpr uintptr_t OFF_SET_PIXEL_POS       = 0x006418e0; // CInstanceBase::NEW_SetPixelPosition
static constexpr uintptr_t OFF_MOVE_TO_DEST        = 0x00641334; // CInstanceBase::NEW_MoveToDestPixelPositionDirection
static constexpr uintptr_t OFF_ATTACK              = 0x0063b04c; // CInstanceBase::NEW_Attack(float)
static constexpr uintptr_t OFF_IS_STONE            = 0x00637b3c; // CInstanceBase::IsStone
static constexpr uintptr_t OFF_GET_MAIN_ACTOR      = 0x006d9ee4; // CPythonPlayer::NEW_GetMainActorPtr
static constexpr uintptr_t OFF_GET_CLOSE_STONE     = 0x0065c6c8; // CPythonCharacterManager::GetCloseAttackableStoneInstance
static constexpr uintptr_t OFF_GET_CLOSE_ITEM      = 0x00681364; // CPythonItem::GetCloseItem
static constexpr uintptr_t OFF_GET_GROUND_ITEM_POS = 0x00681a94; // CPythonItem::GetGroundItemPosition
static constexpr uintptr_t OFF_GET_PLAYER_NAME     = 0x006dfeb8; // CPythonPlayer isim alma (wrapper)
static constexpr uintptr_t OFF_CAN_MOVE_TO_DEST    = 0x006408e4; // CInstanceBase::NEW_CanMoveToDestPixelPosition
static constexpr uintptr_t OFF_IS_WALKING          = 0x006410f0; // CInstanceBase::IsWalking
static constexpr uintptr_t OFF_START_WALKING       = 0x00641080; // CInstanceBase::StartWalking

struct Vector3 {
    float x, y, z;
};

namespace ayaz {

struct LibInfo {
    uintptr_t base = 0;
    uintptr_t size = 0;
    bool valid() const { return base != 0; }
};

// libayaz2client.so'yu bulur, base adresini döner
LibInfo FindGameLib();

// Singleton pointer'ı okur
uintptr_t GetSingletonPtr(const LibInfo& lib, uintptr_t singleton_offset);

// Karakter ismi
std::string GetPlayerName(const LibInfo& lib);

// IsReadable - adres okunabilir mi (mincore ile)
bool IsReadable(uintptr_t addr, size_t size);

// Generic bellek okuma
template<typename T>
bool SafeRead(uintptr_t addr, T& out) {
    if (!IsReadable(addr, sizeof(T))) return false;
    out = *reinterpret_cast<T*>(addr);
    return true;
}

} // namespace ayaz
