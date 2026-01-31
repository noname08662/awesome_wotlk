#pragma once
#include "MSDF.h"
#include "MSDFCache.h"

class MSDFPregen {
public:
    static void RegisterForPreGen(FT_Face aface, const FT_Byte* data, FT_Long size, FT_Long faceIndex);
    static bool TryStartPreGen();
    static void Shutdown() noexcept;

private:
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