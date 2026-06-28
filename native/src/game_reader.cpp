#include "game_reader.h"
#include <link.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <android/log.h>

#define TAG "AyazReader"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

namespace ayaz {

// dl_iterate_phdr callback - libayaz2client.so'yu bul
struct FindLibCtx {
    LibInfo result;
};

static int find_lib_cb(dl_phdr_info* info, size_t, void* data) {
    if (!info->dlpi_name) return 0;
    if (strstr(info->dlpi_name, "libayaz2client.so") == nullptr) return 0;

    FindLibCtx* ctx = reinterpret_cast<FindLibCtx*>(data);
    ctx->result.base = info->dlpi_addr;

    // toplam boyutu hesapla
    for (int i = 0; i < info->dlpi_phnum; i++) {
        const auto& ph = info->dlpi_phdr[i];
        if (ph.p_type == PT_LOAD) {
            uintptr_t seg_end = info->dlpi_addr + ph.p_vaddr + ph.p_memsz;
            if (seg_end > ctx->result.base + ctx->result.size)
                ctx->result.size = seg_end - ctx->result.base;
        }
    }
    LOGI("libayaz2client.so bulundu: base=0x%lx size=0x%lx",
         ctx->result.base, ctx->result.size);
    return 1; // dur
}

LibInfo FindGameLib() {
    FindLibCtx ctx{};
    dl_iterate_phdr(find_lib_cb, &ctx);
    return ctx.result;
}

// Singleton pointer oku
uintptr_t GetSingletonPtr(const LibInfo& lib, uintptr_t singleton_offset) {
    uintptr_t addr = lib.base + singleton_offset;
    uintptr_t ptr = 0;
    if (!SafeRead(addr, ptr)) return 0;
    return ptr;
}

// mincore ile sayfa erişilebilirlik kontrolü (cache'li)
bool IsReadable(uintptr_t addr, size_t size) {
    if (addr == 0 || addr == UINTPTR_MAX) return false;
    static long page_size = sysconf(_SC_PAGESIZE);
    uintptr_t page_start = addr & ~(page_size - 1);
    uintptr_t page_end   = (addr + size + page_size - 1) & ~(page_size - 1);
    size_t    page_count = (page_end - page_start) / page_size;

    unsigned char vec[64] = {};
    if (page_count > 64) return false;
    if (mincore(reinterpret_cast<void*>(page_start), page_count * page_size, vec) != 0)
        return false;
    for (size_t i = 0; i < page_count; i++)
        if (!(vec[i] & 1)) return false;
    return true;
}

// Karakter ismini oku
// CPythonPlayer singleton'ında isim std::string olarak tutuluyor
// offset 0x5C8 civarı - runtime'da doğrulanmalı, şimdilik Python wrapper kullan
std::string GetPlayerName(const LibInfo& lib) {
    uintptr_t player_ptr = GetSingletonPtr(lib, OFF_PLAYER_SINGLETON);
    if (!player_ptr) return "";

    // CPythonPlayer::ms_singleton -> CPythonPlayer* -> m_stName offset
    // Metin2 kaynak kodundan bilinen offset: 0x5C8
    static constexpr uintptr_t NAME_OFFSET = 0x5C8;
    uintptr_t name_addr = player_ptr + NAME_OFFSET;

    if (!IsReadable(name_addr, 32)) return "";

    // std::string layout: [ptr(8)][size(8)][cap(8)] veya SSO
    struct StdString {
        union {
            char sso[16];
            char* ptr;
        };
        size_t size;
        size_t cap;
    };

    StdString s{};
    if (!SafeRead(name_addr, s)) return "";

    if (s.size == 0 || s.size > 64) return "";

    // SSO: cap'in LSB 0 ise stack'te
    if (s.cap < 15) {
        // SSO buffer
        return std::string(s.sso, s.size);
    } else {
        if (!IsReadable(reinterpret_cast<uintptr_t>(s.ptr), s.size + 1)) return "";
        return std::string(s.ptr, s.size);
    }
}

} // namespace ayaz
