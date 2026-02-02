#pragma once
#include "MSDF.h"
#include "MSDFCache.h"

class MSDFPregen {
public:
    static void RegisterForPreGen(FT_Face aface, const FT_Byte* data, FT_Long size, FT_Long faceIndex);
    static bool TryStartPreGen();
    static void Shutdown() noexcept;

private:
    struct ThreadLocalBatch {
        std::vector<std::pair<uint32_t, GlyphMetrics>> glyphs;
        std::vector<std::vector<uint8_t>> ownedBuffers;
        std::mutex mutex;
        size_t memoryUsed = 0;
    };

    struct PreGenRequest {
        FT_Face face = nullptr;
        const FT_Byte* data = nullptr;
        FT_Long size = 0;
        FT_Long faceIndex = 0;
        std::string familyName;
        std::string styleName;
    };

    static void ExecutePreGeneration();
    static bool AcquirePreGenLock();
    static void ReleasePreGenLock();
    static bool GenerateFont(const PreGenRequest& req);
    static void FlushStdin() {
        int c;
        while ((c = getchar()) != '\n' && c != EOF);
    }

    inline static std::vector<PreGenRequest> s_pendingRequests;
    inline static HANDLE s_pregenLockFile = INVALID_HANDLE_VALUE;
};