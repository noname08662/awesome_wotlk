#include "MSDFPregen.h"
#include "MSDFFont.h"
#include "MSDFValidator.h"

void MSDFPregen::RegisterForPreGen(FT_Face face, const FT_Byte* data, FT_Long size, FT_Long faceIndex) {
    if (!face || !data || size <= 0) return;

    std::string name = face->family_name ? face->family_name : "";
    std::string style = face->style_name ? face->style_name : "";

    for (PreGenRequest& r : s_pendingRequests) {
        if (r.faceIndex == faceIndex && r.familyName == name && r.styleName == style) {
            r.data = data;
            r.size = size;
            return;
        }
    }
    s_pendingRequests.push_back({ face, data, size, faceIndex, name, style });
}

bool MSDFPregen::AcquirePreGenLock() {
    std::filesystem::create_directories(MSDFCache::CACHE_DIR);
    HANDLE h = CreateFileA(
        (std::filesystem::path(MSDFCache::CACHE_DIR) / "pregen.lock").string().c_str(),
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_NEW,
        FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE,
        NULL
    );
    if (h == INVALID_HANDLE_VALUE) return false;
    s_pregenLockFile = h;
    return true;
}

void MSDFPregen::ReleasePreGenLock() {
    if (s_pregenLockFile != INVALID_HANDLE_VALUE) {
        CloseHandle(s_pregenLockFile);
        s_pregenLockFile = INVALID_HANDLE_VALUE;
    }
}

bool MSDFPregen::TryStartPreGen() {
    if (s_pendingRequests.empty()) return false;

    if (!AcquirePreGenLock()) {
        printf("Pre-generation already running in another instance.\n");
        return false;
    }

    ExecutePreGeneration();
    ReleasePreGenLock();
    return true;
}

void MSDFPregen::Shutdown() noexcept {
    s_pendingRequests.clear();
    ReleasePreGenLock();
}

void MSDFPregen::ExecutePreGeneration() {
    if (s_pendingRequests.empty()) return;

    ConsoleGuard consoleGuard;
    if (!consoleGuard.allocated) return;

    std::string locale = MSDF::GetGameLocale();

    std::vector<FT_Face> invalid;
    invalid.reserve(s_pendingRequests.size());

    for (size_t i = 0; i < s_pendingRequests.size(); ++i) {
        auto it = MSDFFont::s_fontHandles.find(s_pendingRequests[i].face);
        if (it == MSDFFont::s_fontHandles.end() ||
                !MSDFValidator::IsFontMSDFCompatible(it->second->m_msdfFont)) {
            if (it->second) invalid.push_back(it->second->m_ftFace);
        }
    }
    for (FT_Face face : invalid) MSDFFont::Unregister(face);

    while (!s_pendingRequests.empty()) {
        printf("\n=== MSDF Font Pre-Generation ===\n");
        printf("Detected Locale: %s\n\n", locale.c_str());
        printf("Available Fonts (Current Progress):\n");

        for (size_t i = 0; i < s_pendingRequests.size(); ++i) {
            const auto& req = s_pendingRequests[i];

            printf("%zu. %s %s", i + 1, req.familyName.c_str(), req.styleName.c_str());

            MSDFCache probe(nullptr, 0, req.familyName.c_str(), req.styleName.c_str(),
                MSDF::SDF_RENDER_SIZE, MSDF::SDF_SPREAD);
            size_t count = probe.GetManifestSize();
            if (count > 0) {
                printf(" (Cache found: %zu entries%s)", count,
                    count >= MSDF::CJK_CACHE_THRESHOLD ? " [CJK-READY]" : "");
            }
            printf("\n");
        }

        printf("\nOptions:\n");
        printf("0. Exit\n");
        printf("Select font number: ");

        int choice = 0;
        if (scanf_s("%d", &choice) != 1) {
            FlushStdin();
            printf("ERROR: %s\n", "Invalid input");
            continue;
        }
        FlushStdin();

        if (choice == 0) break;
        if (choice < 1 || choice > static_cast<int>(s_pendingRequests.size())) {
            printf("ERROR: %s\n", "Invalid selection");
            continue;
        }
        GenerateFont(s_pendingRequests[choice - 1]);
    }
}

bool MSDFPregen::GenerateFont(const PreGenRequest& req) {
    std::string locale = MSDF::GetGameLocale();
    const char* locale_str = locale.c_str();

    uint32_t start = 0, end = 0;
    const char* rangeName = "Unknown";

    if (strcmp(locale_str, "zhCN") == 0 || strcmp(locale_str, "zhTW") == 0) {
        start = 0x0020; end = 0x9FFF;
        rangeName = "CJK Unified Ideographs (Chinese)";
    }
    else if (strcmp(locale_str, "koKR") == 0) {
        start = 0x0020; end = 0xD7AF;
        rangeName = "Hangul Syllables (Korean)";
    }
    else if (strcmp(locale_str, "ruRU") == 0) {
        start = 0x0020; end = 0x04FF;
        rangeName = "Cyrillic (Russian)";
    }
    else {
        start = 0x0020; end = 0x00FF;
        rangeName = "Basic Latin / Extended ASCII";
    }

    printf("\n=== Generating: %s %s ===\n", req.familyName.c_str(), req.styleName.c_str());
    printf("Select Generation Depth:\n");
    printf("1. Standard %s (U+%04X - U+%04X)\n", rangeName, start, end);
    printf("2. Custom Range\n");
    printf("0. Exit\n");
    printf("Choice: ");

    int choice = 1;
    if (scanf_s("%d", &choice) != 1) {
        FlushStdin();
        printf("ERROR: %s\n", "Invalid input");
        return false;
    }
    FlushStdin();

    if (choice == 0) return false;

    if (choice == 2) {
        printf("Enter Start Hex (e.g. 4E00): ");
        if (scanf_s("%x", &start) != 1) {
            FlushStdin();
            printf("ERROR: %s\n", "Invalid input");
            return false;
        }
        FlushStdin();
        printf("Enter End Hex (e.g. 9FFF): ");
        if (scanf_s("%x", &end) != 1) {
            FlushStdin();
            printf("ERROR: %s\n", "Invalid input");
            return false;
        }
        FlushStdin();
    }

    if (end < start) {
        printf("ERROR: End must be >= start\n");
        return false;
    }
    const uint64_t total64 = static_cast<uint64_t>(end) - start + 1;
    if (total64 > 0xFFFFFFFF) {
        printf("ERROR: Range too large (max 4 billion glyphs)\n");
        return false;
    }
    const uint32_t total = static_cast<uint32_t>(total64);

    double cpuLimit = 100.0;
    printf("\nEnter CPU usage limit (1-100%%, 100 for unlimited):\n");
    printf("Choice (%%): ");

    if (scanf_s("%lf", &cpuLimit) != 1) {
        FlushStdin();
        printf("ERROR: Invalid input. Using 100%% (unlimited).\n");
        cpuLimit = 100.0;
    }
    FlushStdin();

    cpuLimit = std::clamp(cpuLimit, 1.0, 100.0);
    printf("CPU limit set to: %.0f%%\n", cpuLimit);

    printf("\nGenerating %u glyphs...\n", total);

    if (total == 0) {
        printf("ERROR: %s\n", "Nothing to generate");
        return false;
    }

    MSDFCache cache(req.data, req.size,
        req.familyName.c_str(), req.styleName.c_str(),
        MSDF::SDF_RENDER_SIZE, MSDF::SDF_SPREAD);

    unsigned int hw = std::thread::hardware_concurrency();
    if (hw == 0) hw = 4;
    const unsigned int numThreads = hw;

    std::vector<FT_Face> threadFaces(numThreads, nullptr);
    FT_Library ftLib = nullptr;

    if (FT_Init_FreeType(&ftLib) != 0 || !ftLib) {
        printf("ERROR: %s\n", "FT_Init_FreeType failed");
        return false;
    }

    bool allHandlesValid = true;
    for (unsigned int i = 0; i < numThreads; ++i) {
        if (FT_New_Memory_Face(ftLib, req.data, req.size, req.faceIndex, &threadFaces[i]) != 0) {
            const char* name = threadFaces[i] ? threadFaces[i]->family_name : nullptr;
            printf("ERROR: FT_New_Memory_Face failed for %s\n", name ? name : "unknown font");
            allHandlesValid = false;
            break;
        }
    }

    std::vector<std::unique_ptr<MSDFFont>> threadMSDFFonts(numThreads);
    if (allHandlesValid) {
        for (unsigned int i = 0; i < numThreads; ++i) {
            threadMSDFFonts[i] = std::make_unique<MSDFFont>(threadFaces[i], req.data, req.size, req.faceIndex);
        }
    }

    if (!allHandlesValid) {
        for (unsigned int i = 0; i < numThreads; ++i) {
            threadMSDFFonts[i].reset();
            if (threadFaces[i]) FT_Done_Face(threadFaces[i]);
        }
        if (ftLib) FT_Done_FreeType(ftLib);
        printf("ERROR: %s\n", "Failed to create MSDFFont instances");
        return false;
    }

    std::atomic<uint32_t> nextCp(start);
    std::atomic<uint32_t> doneCount(0);
    std::atomic<bool> workerError(false);
    std::mutex cacheMutex;

    std::thread progressThread([&]() {
        while (!workerError.load(std::memory_order_acquire)) {
            uint32_t now = doneCount.load(std::memory_order_relaxed);
            if (now >= total) break;
            if (total > 0) {
                printf("\rProgress: %u/%u (%.1f%%)   ", now, total,
                    static_cast<double>(now) / total * 100.0);
                fflush(stdout);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
        });

    auto worker = [&](uint32_t workerId, MSDFFont* font, FT_Face localFace) {
        if (!font || !localFace) {
            workerError.store(true, std::memory_order_release);
            printf("ERROR: Invalid handles in worker %u\n", workerId);
            return;
        }
        if (FT_Set_Pixel_Sizes(localFace, MSDF::SDF_RENDER_SIZE, MSDF::SDF_RENDER_SIZE) != 0) {
            workerError.store(true, std::memory_order_release);
            printf("ERROR: FT_Set_Pixel_Sizes failed in worker %u\n", workerId);
            return;
        }

        Throttle throttle(cpuLimit);
        VectorPool<uint8_t> pool;
        auto msdfData = pool.Acquire(512 * 512 * 4);

        while (true) {
            if (workerError.load(std::memory_order_acquire)) break;

            uint32_t cp = nextCp.fetch_add(1, std::memory_order_acq_rel);
            if (cp > end) break;

            throttle.StartWork();

            if (FT_Load_Glyph(localFace, FT_Get_Char_Index(localFace, cp),
                FT_LOAD_NO_BITMAP | FT_LOAD_NO_HINTING) != 0) {
                doneCount.fetch_add(1, std::memory_order_relaxed);
                throttle.EndWork();
                continue;
            }

            const bool hasOutline = localFace->glyph->format == FT_GLYPH_FORMAT_OUTLINE &&
                localFace->glyph->outline.n_contours > 0;

            uint16_t width = 0;
            uint16_t height = 0;
            int16_t bitmapLeft = static_cast<int16_t>(localFace->glyph->bitmap_left);
            int16_t bitmapTop = static_cast<int16_t>(localFace->glyph->bitmap_top);

            if (hasOutline) {
                FT_BBox bbox;
                FT_Outline_Get_BBox(&localFace->glyph->outline, &bbox);

                int xMin = bbox.xMin >> 6;
                int yMin = bbox.yMin >> 6;
                int xMax = (bbox.xMax + 63) >> 6;
                int yMax = (bbox.yMax + 63) >> 6;
                int w = std::max(0, xMax - xMin);
                int h = std::max(0, yMax - yMin);

                if (w > 0 && h > 0) {
                    int sdfW = w + 2 * MSDF::SDF_SPREAD;
                    int sdfH = h + 2 * MSDF::SDF_SPREAD;

                    if (sdfW > 0 && sdfH > 0 && sdfW <= 512 && sdfH <= 512) {
                        msdfData.clear();
                        if (font->GenerateMSDF(msdfData, cp, sdfW, sdfH)) {
                            size_t expectedSize = static_cast<size_t>(sdfW) * sdfH * 4;
                            if (msdfData.size() == expectedSize) {
                                width = static_cast<uint16_t>(sdfW);
                                height = static_cast<uint16_t>(sdfH);
                            }
                            else {
                                printf("WARNING: Glyph U+%04X size mismatch: got %zu, expected %zu\n",
                                    cp, msdfData.size(), expectedSize);
                            }
                        }
                    }
                }
            }
            throttle.EndWork();

            GlyphMetricsToStore gm = {};
            gm.codepoint = cp;
            gm.width = width;
            gm.height = height;
            gm.bitmapLeft = bitmapLeft;
            gm.bitmapTop = bitmapTop;
            gm.ownedPixelData.assign(msdfData.begin(), msdfData.end());
            gm.dataSize = static_cast<uint32_t>(gm.ownedPixelData.size());

            {
                std::lock_guard<std::mutex> lock(cacheMutex);
                if (!cache.StoreGlyph(std::move(gm))) printf("WARNING: Failed to store glyph U+%04X\n", cp);
            }
            doneCount.fetch_add(1, std::memory_order_relaxed);
        }
        pool.Release(std::move(msdfData));
        };

    std::vector<std::thread> threads;
    threads.reserve(numThreads);
    for (unsigned int i = 0; i < numThreads; ++i) {
        threads.emplace_back(worker, i, threadMSDFFonts[i].get(), threadFaces[i]);
    }

    for (auto& t : threads) {
        if (t.joinable()) t.join();
    }
    if (progressThread.joinable()) progressThread.join();

    printf("\rProgress: %u/%u (100.0%%)                      \n", doneCount.load(), total);

    printf("Writing to disk...");
    fflush(stdout);
    cache.FlushPendingWrites();
    printf(" Done.\n");

    threadMSDFFonts.clear();
    for (auto face : threadFaces) {
        if (face) FT_Done_Face(face);
    }
    if (ftLib) FT_Done_FreeType(ftLib);

    bool success = !workerError.load(std::memory_order_acquire);
    if (!success) {
        printf("\nGeneration encountered errors. Press Enter to continue...");
        getchar();
    }
    else {
        printf("Generation complete. Press Enter to continue...");
        getchar();
    }
    return success;
}