#include "MSDFPregen.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <charconv>
#include <cmath>
#include <exception>
#include <expected>
#include <filesystem>
#include <format>
#include <iterator>
#include <memory>
#include <mutex>
#include <optional>
#include <print>
#include <ranges>
#include <span>
#include <string_view>
#include <system_error>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include "MSDF.h"
#include "MSDFCache.h"
#include "MSDFFaceSweep.h"
#include "MSDFFont.h"
#include "MSDFManager.h"

#include FT_SFNT_NAMES_H
#include FT_TRUETYPE_IDS_H
#include "include/Client/Client.h"

namespace {
constexpr DWORD kCancelWaitMs = 4000;
constexpr std::chrono::milliseconds kProgressInterval{500};
constexpr uint32_t kMaxCodepoint = 0x10FFFF;
constexpr double kPregenCompleteRatio = 0.95;

constexpr std::array<FT_UShort, 4> kLabelNameIds{
    TT_NAME_ID_FULL_NAME, TT_NAME_ID_PS_NAME, TT_NAME_ID_VERSION_STRING, TT_NAME_ID_UNIQUE_ID};
constexpr size_t kMaxLabelLength = 40;

FILE* console_out = nullptr;
FILE* console_in = nullptr;

struct CodepointRange {
    uint32_t start;
    uint32_t end;
    const char* name;
};

CodepointRange rangeForLocale(std::string_view locale) {
    if (locale == "zhCN" || locale == "zhTW") {
        return {.start = 0x0020, .end = 0x9FFF, .name = "CJK Unified Ideographs (Chinese)"};
    }
    if (locale == "koKR") { return {.start = 0x0020, .end = 0xD7AF, .name = "Hangul Syllables (Korean)"}; }
    if (locale == "ruRU") { return {.start = 0x0020, .end = 0x04FF, .name = "Cyrillic (Russian)"}; }
    return {.start = 0x0020, .end = 0x00FF, .name = "Basic Latin / Extended ASCII"};
}

template <bool Newline, typename... Args>
void consoleWrite(std::format_string<Args...> fmt, Args&&... args) noexcept {
    if (console_out == nullptr) { return; }
    try {
        if constexpr (Newline) {
            std::println(console_out, fmt, std::forward<Args>(args)...);
        } else {
            std::print(console_out, fmt, std::forward<Args>(args)...);
        }
    } catch (const std::exception& e) {
        OutputDebugStringA("MSDFPregen: console print failed: ");
        OutputDebugStringA(e.what());
        OutputDebugStringA("\n");
    } catch (...) { OutputDebugStringA("MSDFPregen: console print failed\n"); }
}

template <typename... Args>
void consolePrint(std::format_string<Args...> fmt, Args&&... args) noexcept {
    consoleWrite<false>(fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void consolePrintln(std::format_string<Args...> fmt, Args&&... args) noexcept {
    consoleWrite<true>(fmt, std::forward<Args>(args)...);
}

std::optional<std::string> readLine(const std::atomic<bool>* cancel) {
    std::string line;
    int c;
    while (console_in != nullptr && (c = std::fgetc(console_in)) != '\n' && c != EOF) {
        line.push_back(static_cast<char>(c));
    }
    if (cancel != nullptr && cancel->load(std::memory_order_acquire)) { return std::nullopt; }
    return line;
}

std::string_view trimmed(std::string_view sv) {
    const size_t first = sv.find_first_not_of(" \t\r");
    if (first == std::string_view::npos) { return {}; }
    return sv.substr(first, sv.find_last_not_of(" \t\r") - first + 1);
}

template <typename T>
std::optional<T> readValue(int base = 10, const std::atomic<bool>* cancel = nullptr) {
    const std::optional<std::string> line = readLine(cancel);
    if (!line) { return std::nullopt; }

    std::string_view sv = trimmed(*line);
    if (sv.empty()) { return std::nullopt; }
    if (base == 16 && (sv.starts_with("0x") || sv.starts_with("0X"))) { sv.remove_prefix(2); }
    if (std::is_floating_point_v<T> && sv.ends_with('%')) {
        sv.remove_suffix(1);
        sv = sv.substr(0, sv.find_last_not_of(" \t\r") + 1);
    }

    T value{};
    const char* const first_ch = std::to_address(sv.begin());
    const char* const last = std::to_address(sv.end());
    std::from_chars_result res;
    if constexpr (std::is_floating_point_v<T>) {
        res = std::from_chars(first_ch, last, value);
    } else {
        res = std::from_chars(first_ch, last, value, base);
    }
    if (res.ec != std::errc{} || res.ptr != last) { return std::nullopt; }
    if constexpr (std::is_floating_point_v<T>) {
        if (!std::isfinite(value)) { return std::nullopt; }
    }
    return value;
}

bool inputClosed(const std::atomic<bool>& cancel) {
    return cancel.load(std::memory_order_acquire) || console_in == nullptr || std::feof(console_in) != 0;
}

std::expected<std::vector<size_t>, std::string> parseSelection(std::string_view text, size_t count) {
    constexpr std::string_view kSeparators = ", \t\r";
    std::vector<std::string_view> tokens;
    for (size_t pos = text.find_first_not_of(kSeparators); pos != std::string_view::npos;) {
        const size_t end = std::min(text.find_first_of(kSeparators, pos), text.size());
        tokens.push_back(text.substr(pos, end - pos));
        pos = text.find_first_not_of(kSeparators, end);
    }
    if (tokens.empty()) { return std::unexpected("Invalid input"); }

    const auto is_all = [](std::string_view token) {
        return token == "*" || std::ranges::equal(token, std::string_view("all"), {}, [](char c) {
            return std::tolower(static_cast<unsigned char>(c));
        });
    };
    if (tokens.size() == 1 && tokens[0] == "0") { return std::vector<size_t>{}; }
    if (tokens.size() == 1 && is_all(tokens[0])) {
        return std::views::iota(size_t{0}, count) | std::ranges::to<std::vector>();
    }

    const auto entry = [count](std::string_view token) -> std::optional<size_t> {
        size_t value = 0;
        const auto [ptr, ec] = std::from_chars(token.data(), token.data() + token.size(), value);
        if (ec != std::errc{} || ptr != token.data() + token.size() || value < 1 || value > count) {
            return std::nullopt;
        }
        return value - 1;
    };

    std::vector<size_t> indices;
    std::vector picked(count, false);
    for (const std::string_view token : tokens) {
        const size_t dash = token.find('-');
        const std::optional<size_t> first = entry(token.substr(0, dash));
        const std::optional<size_t> last = dash == std::string_view::npos ? first : entry(token.substr(dash + 1));
        if (!first || !last || *last < *first) {
            return std::unexpected(std::format("Invalid selection '{}', fonts are 1-{}", token, count));
        }
        for (size_t i = *first; i <= *last; ++i) {
            if (!picked[i]) {
                picked[i] = true;
                indices.push_back(i);
            }
        }
    }
    return indices;
}

std::string asciiName(FT_Face face, FT_UShort name_id) {
    std::string mac;
    const FT_UInt count = FT_Get_Sfnt_Name_Count(face);
    for (FT_UInt i = 0; i < count; ++i) {
        FT_SfntName name{};
        if (FT_Get_Sfnt_Name(face, i, &name) != 0 || name.name_id != name_id || name.string == nullptr) { continue; }
        const bool windows =
            name.platform_id == TT_PLATFORM_MICROSOFT && name.language_id == TT_MS_LANGID_ENGLISH_UNITED_STATES;
        const bool apple = name.platform_id == TT_PLATFORM_MACINTOSH && name.language_id == TT_MAC_LANGID_ENGLISH;
        if (!windows && !(apple && mac.empty())) { continue; }

        const FT_UInt step = windows ? 2 : 1;
        const std::span bytes(name.string, name.string_len);
        std::string text;
        for (FT_UInt k = 0; k + step <= bytes.size(); k += step) {
            const unsigned c = windows ? (bytes[k] << 8u) | bytes[k + 1] : bytes[k];
            if (c < 0x20 || c > 0x7E) {
                text.clear();
                break;
            }
            text.push_back(static_cast<char>(c));
        }
        if (windows && !text.empty()) { return std::move(text); }
        if (apple) { mac = std::move(text); }
    }
    return std::move(mac);
}

std::vector<std::string> labelNames(FT_Face face) {
    std::vector<std::string> names;
    names.reserve(kLabelNameIds.size());
    for (const FT_UShort id : kLabelNameIds) {
        std::string text = face != nullptr ? asciiName(face, id) : std::string();
        if (text.size() > kMaxLabelLength) { text = text.substr(0, kMaxLabelLength - 3) + "..."; }
        names.push_back(std::move(text));
    }
    return names;
}

template <typename T>
bool allDistinct(std::vector<T> values) {
    std::ranges::sort(values);
    return std::ranges::adjacent_find(values) == values.end();
}

std::string displayName(const std::filesystem::path& path) {
    try {
        return path.string();
    } catch (...) { return "?"; }
}

template <typename Request>
std::string describe(const Request& req) {
    std::string text = std::format("{} {}", req.family_name, req.style_name);
    if (!req.addon_name.empty()) { text += std::format(" [{}]", req.addon_name); }
    if (!req.variant.empty()) { text += std::format(" <{}>", req.variant); }
    return text;
}
}  // namespace

bool MSDFPregen::tryStartPreGen() noexcept {
    try {
        if (!acquirePreGenLock()) {
            consolePrintln("Pre-generation already running in another instance.");
            return false;
        }
        const FinalAction release_lock([] { releasePreGenLock(); });
        executePreGeneration();
        return true;
    } catch (const std::exception& e) {
        OutputDebugStringA("MSDFPregen: pre-generation aborted: ");
        OutputDebugStringA(e.what());
        OutputDebugStringA("\n");
    } catch (...) { OutputDebugStringA("MSDFPregen: pre-generation aborted\n"); }
    return false;
}

void MSDFPregen::shutdown() noexcept { releasePreGenLock(); }

std::vector<MSDFPregen::PreGenRequest> MSDFPregen::collectRequests() {
    std::vector<PreGenRequest> requests;
    const auto find_listed = [&requests](FontHash hash, FT_Long face_index) {
        return std::ranges::find_if(
            requests, [&](const PreGenRequest& r) { return r.hash == hash && r.face_index == face_index; });
    };

    for (const auto& [face, font] : *MSDFFont::kFontHandles) {
        const FontHash hash = hashFont(font->font_data_, font->data_size_);
        if (find_listed(hash, face->face_index) != requests.end()) { continue; }
        requests.push_back({
            .face = face,
            .data = font->font_data_,
            .size = font->data_size_,
            .face_index = face->face_index,
            .family_name = face->family_name != nullptr ? face->family_name : "Unknown",
            .style_name = face->style_name != nullptr ? face->style_name : "",
            .hash = hash,
            .names = labelNames(face)
        });
    }

    FT_Library library = nullptr;
    if (FT_Init_FreeType(&library) == 0) {
        std::vector<uint8_t> buffer;
        const std::filesystem::path root = std::filesystem::current_path() / "Interface" / "AddOns";
        MSDFFaceSweep::forEachAddonFace(library, buffer, {}, [&](const MSDFFaceSweep::AddonFace& font) {
            const std::filesystem::path relative = font.kPath.lexically_relative(root);
            std::string addon_name = relative.empty() ? std::string() : displayName(*relative.begin());
            if (const auto it = find_listed(font.hash, 0); it != requests.end()) {
                if (it->addon_name.empty()) { it->addon_name = std::move(addon_name); }
                return;
            }
            requests.push_back({
                .family_name = font.family,
                .style_name = font.style,
                .hash = font.hash,
                .path = font.kPath,
                .addon_name = std::move(addon_name),
                .names = labelNames(font.face)
            });
        });
        FT_Done_FreeType(library);
    }

    const auto listed_as = [](const PreGenRequest& r) { return std::tie(r.family_name, r.style_name, r.addon_name); };
    std::ranges::sort(requests, {}, listed_as);

    for (auto first = requests.begin(); first != requests.end();) {
        const auto last =
            std::find_if(first, requests.end(), [&](const auto& r) { return listed_as(r) != listed_as(*first); });
        const std::span group(first, last);
        first = last;
        if (group.size() < 2) { continue; }

        const auto label = [&](const auto& make) {
            std::vector<std::string> labels;
            for (const PreGenRequest& r : group) {
                labels.push_back(make(r));
            }
            const bool unreadable = std::ranges::any_of(labels, [](const std::string& s) { return s.empty(); });
            if (unreadable || !allDistinct(labels)) { return false; }
            for (size_t i = 0; i < group.size(); ++i) {
                group[i].variant = std::move(labels[i]);
            }
            return true;
        };
        bool labeled = false;
        for (size_t id = 0; id < kLabelNameIds.size() && !labeled; ++id) {
            labeled = label([id](const PreGenRequest& r) { return id < r.names.size() ? r.names[id] : std::string(); });
        }
        labeled = labeled || label([](const PreGenRequest& r) { return std::format("#{:08X}", r.hash >> 32); }) ||
            label([](const PreGenRequest& r) { return std::format("#{:016X}", r.hash); });
        if (!labeled) {
            label([](const PreGenRequest& r) { return std::format("#{:016X} face {}", r.hash, r.face_index); });
        }
    }
    return requests;
}

void MSDFPregen::executePreGeneration() {
    ConsoleGuard console_guard;
    if (!console_guard.allocated) { return; }

    console_out = console_guard.fp_out;
    console_in = console_guard.fp_in;

    struct ActiveStreamGuard {
        ~ActiveStreamGuard() {
            console_out = nullptr;
            console_in = nullptr;
        }

        ActiveStreamGuard() = default;
        ActiveStreamGuard(const ActiveStreamGuard&) = delete;
        ActiveStreamGuard& operator=(const ActiveStreamGuard&) = delete;
        ActiveStreamGuard(ActiveStreamGuard&&) = delete;
        ActiveStreamGuard& operator=(ActiveStreamGuard&&) = delete;
    } active_stream_guard;

    struct CtrlHandlerGuard {
        CtrlHandlerGuard() {
            cancel_pregen_.store(false, std::memory_order_relaxed);
            pregen_finished_event_ = CreateEventW(nullptr, TRUE, TRUE, nullptr);
            SetConsoleCtrlHandler(consoleCtrlHandler, TRUE);
        }

        ~CtrlHandlerGuard() {
            SetConsoleCtrlHandler(consoleCtrlHandler, FALSE);
            if (pregen_finished_event_ != nullptr) {
                CloseHandle(pregen_finished_event_);
                pregen_finished_event_ = nullptr;
            }
        }

        CtrlHandlerGuard(const CtrlHandlerGuard&) = delete;
        CtrlHandlerGuard& operator=(const CtrlHandlerGuard&) = delete;
        CtrlHandlerGuard(CtrlHandlerGuard&&) = delete;
        CtrlHandlerGuard& operator=(CtrlHandlerGuard&&) = delete;
    } ctrl_handler_guard;

    consolePrintln("Ctrl+C at any time, or Ctrl+Z then Enter at a prompt, closes pre-generation.\n");
    consolePrintln("Scanning AddOns for fonts...");
    std::vector<PreGenRequest> requests = collectRequests();
    if (requests.empty()) {
        consolePrint("No fonts found. Press Enter to continue...");
        flushStdin();
        return;
    }

    const std::string locale = client::getGameLocale();

    while (!requests.empty()) {
        if (cancel_pregen_.load(std::memory_order_acquire)) { break; }

        consolePrintln("\n=== MSDF Font Pre-Generation ===");
        consolePrintln("Detected Locale: {}\n", locale);
        consolePrintln("Available Fonts (Current Progress):");

        for (size_t i = 0; i < requests.size(); ++i) {
            const auto& req = requests[i];

            consolePrint("{}. {}", i + 1, describe(req));

            const auto [count, pregen_complete] =
                MSDFCache::summarizeCache(req.family_name.c_str(), req.style_name.c_str(), req.hash);
            if (count > 0) {
                const auto* tag = "";
                if (pregen_complete) { tag = msdf::is_cjk ? " [CJK-READY]" : " [COMPLETE]"; }
                consolePrint(" (Cache found: {} entries{})", count, tag);
            }
            consolePrintln("");
        }

        consolePrintln("\nOptions:");
        consolePrintln("0. Exit");
        consolePrintln("Several at once: numbers and ranges (1 3 5-7, or 1, 3, 5-7), or all");
        consolePrint("Select font number: ");

        const std::optional<std::string> line = readLine(&cancel_pregen_);
        const auto selection = parseSelection(line.value_or(std::string()), requests.size());
        if (!selection) {
            if (inputClosed(cancel_pregen_)) {
                consolePrintln("ERROR: input closed, exiting pre-generation");
                break;
            }
            consolePrintln("ERROR: {}", selection.error());
            continue;
        }
        if (selection->empty()) { break; }

        const size_t batch_size = selection->size();
        const bool batch = batch_size > 1;
        const std::optional<GenerateSettings> settings =
            promptSettings(batch ? std::format("{} fonts", batch_size) : describe(requests[selection->front()]));
        if (!settings) {
            if (inputClosed(cancel_pregen_)) { break; }
            continue;
        }

        size_t complete = 0;
        size_t skipped = 0;
        size_t failed = 0;
        size_t attempted = 0;
        for (const size_t index : *selection) {
            if (inputClosed(cancel_pregen_)) { break; }
            ++attempted;
            PreGenRequest req = requests[index];
            if (batch) { consolePrintln("\n--- [{}/{}] {} ---", attempted, batch_size, describe(req)); }

            std::vector<uint8_t> file_data;
            if (req.face == nullptr) {
                if (!readFontFile(req.path, file_data) ||
                    hashFont(file_data.data(), static_cast<FT_Long>(file_data.size())) != req.hash) {
                    consolePrintln("ERROR: Font file changed or unreadable since the scan, run /msdfpregen again");
                    ++skipped;
                    continue;
                }
                req.data = file_data.data();
                req.size = static_cast<FT_Long>(file_data.size());
            }
            const GenerateOutcome outcome = generateFont(req, *settings);
            refreshLoadedFonts(req);
            if (outcome == GenerateOutcome::eCancelled) { break; }
            size_t& tally = outcome == GenerateOutcome::eComplete ? complete
                : outcome == GenerateOutcome::eSkipped            ? skipped
                                                                  : failed;
            ++tally;
        }

        if (batch) {
            consolePrintln("\n=== Batch: {} complete, {} skipped, {} failed, {} not finished (of {}) ===", complete,
                skipped, failed, batch_size - complete - skipped - failed, batch_size);
        }
        if (inputClosed(cancel_pregen_)) { break; }
        consolePrint("Press Enter to continue...");
        flushStdin();
    }
}

bool MSDFPregen::acquirePreGenLock() {
    std::error_code ec;
    std::filesystem::create_directories(MSDFCache::kCacheDir, ec);
    const std::string lock_path = (std::filesystem::path(MSDFCache::kCacheDir) / "pregen.lock").string();
    HANDLE h = CreateFileA(lock_path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW,
        FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, nullptr);
    if (h == INVALID_HANDLE_VALUE) { return false; }
    pregen_lock_file_ = h;
    return true;
}

void MSDFPregen::releasePreGenLock() {
    if (pregen_lock_file_ != INVALID_HANDLE_VALUE) {
        CloseHandle(pregen_lock_file_);
        pregen_lock_file_ = INVALID_HANDLE_VALUE;
    }
}

std::optional<MSDFPregen::GenerateSettings> MSDFPregen::promptSettings(const std::string& title) {
    const auto [default_start, default_end, range_name] = rangeForLocale(client::getGameLocale());
    uint32_t start = default_start;
    uint32_t end = default_end;

    consolePrintln("\n=== Generating: {} ===", title);
    consolePrintln("Select Generation Depth:");
    consolePrintln("1. Standard {} (U+{:04X} - U+{:04X})", range_name, start, end);
    consolePrintln("2. Custom Range");
    consolePrintln("0. Exit");
    consolePrint("Choice: ");

    const std::optional<int> choice = readValue<int>(10, &cancel_pregen_);
    if (!choice) {
        if (!inputClosed(cancel_pregen_)) { consolePrintln("ERROR: Invalid input"); }
        return std::nullopt;
    }

    if (*choice == 0) { return std::nullopt; }
    if (*choice != 1 && *choice != 2) {
        consolePrintln("ERROR: Invalid selection");
        return std::nullopt;
    }

    if (*choice == 2) {
        consolePrint("Enter Start Hex (e.g. 4E00): ");
        const std::optional<uint32_t> custom_start = readValue<uint32_t>(16, &cancel_pregen_);
        if (!custom_start) {
            if (!inputClosed(cancel_pregen_)) { consolePrintln("ERROR: Invalid input"); }
            return std::nullopt;
        }
        consolePrint("Enter End Hex (e.g. 9FFF): ");
        const std::optional<uint32_t> custom_end = readValue<uint32_t>(16, &cancel_pregen_);
        if (!custom_end) {
            if (!inputClosed(cancel_pregen_)) { consolePrintln("ERROR: Invalid input"); }
            return std::nullopt;
        }
        start = *custom_start;
        end = *custom_end;
    }

    if (end < start) {
        consolePrintln("ERROR: End must be >= start");
        return std::nullopt;
    }
    if (end > kMaxCodepoint) {
        consolePrintln("ERROR: End must be <= U+{:X}", kMaxCodepoint);
        return std::nullopt;
    }
    consolePrintln("\nEnter CPU usage limit (1-100%, 100 for unlimited):");
    consolePrint("Choice (%): ");

    const std::optional<double> cpu_input = readValue<double>(10, &cancel_pregen_);
    if (!cpu_input) {
        if (inputClosed(cancel_pregen_)) { return std::nullopt; }
        consolePrintln("ERROR: Invalid input. Using 100% (unlimited).");
    }
    const double cpu_limit = std::clamp(cpu_input.value_or(100.0), 1.0, 100.0);
    consolePrintln("CPU limit set to: {:.0f}%", cpu_limit);
    return GenerateSettings{.start = start, .end = end, .cpu_limit = cpu_limit};
}

MSDFPregen::GenerateOutcome MSDFPregen::generateFont(const PreGenRequest& req, const GenerateSettings& settings) {
    const CodepointRange standard = rangeForLocale(client::getGameLocale());
    const uint32_t start = settings.start;
    const uint32_t end = settings.end;
    const double cpu_limit = settings.cpu_limit;

    struct GenerateEventGuard {
        GenerateEventGuard() {
            if (pregen_finished_event_ != nullptr) { ResetEvent(pregen_finished_event_); }
        }

        ~GenerateEventGuard() {
            if (pregen_finished_event_ != nullptr) { SetEvent(pregen_finished_event_); }
        }

        GenerateEventGuard(const GenerateEventGuard&) = delete;
        GenerateEventGuard& operator=(const GenerateEventGuard&) = delete;
        GenerateEventGuard(GenerateEventGuard&&) = delete;
        GenerateEventGuard& operator=(GenerateEventGuard&&) = delete;
    } event_guard;

    if (cancel_pregen_.load(std::memory_order_acquire)) {
        consolePrintln("\nGeneration cancelled.");
        return GenerateOutcome::eCancelled;
    }

    MSDFCache cache(
        req.data, req.size, req.family_name.c_str(), req.style_name.c_str(), msdf::kSdfRenderSize, msdf::kSdfSpread);

    const unsigned int hw = std::thread::hardware_concurrency();
    const unsigned int num_threads = hw != 0 ? hw : 4;

    std::vector<FT_Face> thread_faces(num_threads, nullptr);
    FT_Library ft_lib = nullptr;

    if (FT_Init_FreeType(&ft_lib) != 0 || ft_lib == nullptr) {
        consolePrintln("ERROR: FT_Init_FreeType failed");
        return GenerateOutcome::eFailed;
    }

    std::vector<std::unique_ptr<MSDFFont>> thread_msdf_fonts(num_threads);
    const auto release_fonts = [&] {
        thread_msdf_fonts.clear();
        for (auto* face : thread_faces) {
            if (face != nullptr) { FT_Done_Face(face); }
        }
        FT_Done_FreeType(ft_lib);
    };

    for (unsigned int i = 0; i < num_threads; ++i) {
        if (FT_New_Memory_Face(ft_lib, req.data, req.size, req.face_index, &thread_faces[i]) != 0) {
            consolePrintln("ERROR: FT_New_Memory_Face failed for {} {}", req.family_name, req.style_name);
            release_fonts();
            return GenerateOutcome::eFailed;
        }
    }

    std::vector<uint32_t> codepoints;
    FT_UInt gindex = 0;
    for (FT_ULong cp = FT_Get_First_Char(thread_faces[0], &gindex); gindex != 0 && cp <= end;) {
        if (cp >= start) { codepoints.push_back(static_cast<uint32_t>(cp)); }
        const FT_ULong next = FT_Get_Next_Char(thread_faces[0], cp, &gindex);
        if (next <= cp) { break; }
        cp = next;
    }

    const auto total = codepoints.size();
    if (total == 0) {
        consolePrintln("\nThe font has no glyphs in U+{:04X} - U+{:04X}.", start, end);
        release_fonts();
        return GenerateOutcome::eSkipped;
    }

    for (unsigned int i = 0; i < num_threads; ++i) {
        thread_msdf_fonts[i] = std::make_unique<MSDFFont>(thread_faces[i], req.data, req.size);
        if (thread_msdf_fonts[i]->msdf_font_ == nullptr) {
            consolePrintln("ERROR: msdfgen handle unavailable for {} {}", req.family_name, req.style_name);
            release_fonts();
            return GenerateOutcome::eFailed;
        }
    }
    if (cache.getFaceRecord().verdict == MSDFCache::FaceVerdict::eUnknown) {
        if (msdfgen::FontHandle* handle = MSDFFont::createMsdfHandle(req.data, req.size); handle != nullptr) {
            cache.setFaceRecord(MSDFFont::evaluateFace(handle));
            msdfgen::destroyFont(handle);
        }
    }

    consolePrintln("\nGenerating {} glyphs...", total);

    std::atomic<size_t> next_index(0);
    std::atomic<uint32_t> done_count(0);
    std::atomic<uint32_t> written_count(0);
    std::atomic worker_error(false);
    std::mutex cache_mutex;

    const auto progress = [&] {
        while (!worker_error.load(std::memory_order_acquire) && !cancel_pregen_.load(std::memory_order_acquire)) {
            const uint32_t now = done_count.load(std::memory_order_relaxed);
            if (now >= total) { break; }
            consolePrint("\rProgress: {}/{} ({:.1f}%)   ", now, total, static_cast<double>(now) / total * 100.0);
            if (console_out != nullptr) { static_cast<void>(std::fflush(console_out)); }
            std::this_thread::sleep_for(kProgressInterval);
        }
    };

    const auto generateOne = [&](MSDFFont* font, uint32_t cp, Throttle& throttle) {
        try {
            GlyphMetricsToStore gm;
            throttle.startWork();
            const bool loaded = font->generateGlyphData(cp, gm);
            throttle.endWork(&cancel_pregen_);
            if (!loaded) {
                consolePrintln("WARNING: Failed to load glyph U+{:04X}", cp);
                return false;
            }
            std::scoped_lock lock(cache_mutex);
            if (cache.storeGlyph(std::move(gm))) { return true; }
            consolePrintln("WARNING: Failed to store glyph U+{:04X}", cp);
        } catch (const std::exception& e) {
            consolePrintln("ERROR: exception generating U+{:04X}: {}", cp, e.what());
        } catch (...) { consolePrintln("ERROR: unknown exception generating U+{:04X}", cp); }
        return false;
    };

    const auto worker = [&](uint32_t worker_id, MSDFFont* font) {
        if (font == nullptr) {
            worker_error.store(true, std::memory_order_release);
            consolePrintln("ERROR: Invalid handles in worker {}", worker_id);
            return;
        }

        Throttle throttle(cpu_limit);

        while (!worker_error.load(std::memory_order_acquire) && !cancel_pregen_.load(std::memory_order_acquire)) {
            const size_t i = next_index.fetch_add(1, std::memory_order_relaxed);
            if (i >= codepoints.size()) { break; }
            if (generateOne(font, codepoints[i], throttle)) { written_count.fetch_add(1, std::memory_order_relaxed); }
            done_count.fetch_add(1, std::memory_order_relaxed);
        }
    };

    std::thread progress_thread;
    std::vector<std::thread> threads;
    try {
        progress_thread = std::thread(progress);
        threads.reserve(num_threads);
        for (unsigned int i = 0; i < num_threads; ++i) {
            threads.emplace_back(worker, i, thread_msdf_fonts[i].get());
        }
    } catch (const std::exception& e) {
        worker_error.store(true, std::memory_order_release);
        consolePrintln("ERROR: failed to start generation threads: {}", e.what());
    }

    for (auto& t : threads) {
        if (t.joinable()) { t.join(); }
    }
    if (progress_thread.joinable()) { progress_thread.join(); }

    const uint32_t finished = done_count.load(std::memory_order_relaxed);
    consolePrintln("\rProgress: {}/{} ({:.1f}%)                      ", finished, total,
        static_cast<double>(finished) / total * 100.0);

    consolePrint("Writing to disk...");
    if (console_out != nullptr) { static_cast<void>(std::fflush(console_out)); }
    cache.flushPendingWrites();
    consolePrintln(" Done.");
    release_fonts();

    const uint32_t written = written_count.load(std::memory_order_relaxed);
    if (written < finished) { consolePrintln("{} of {} glyphs failed.", finished - written, finished); }

    const bool cancelled = cancel_pregen_.load(std::memory_order_acquire);
    const bool success = !worker_error.load(std::memory_order_acquire) && !cancelled;
    if (success && start <= standard.start && end >= standard.end) {
        const double written_share = static_cast<double>(written) / total;
        if (written_share < kPregenCompleteRatio) {
            consolePrintln("Standard range not marked complete: {:.1f}% of glyphs written, {:.0f}% needed.",
                written_share * 100.0, kPregenCompleteRatio * 100.0);
        } else if (!cache.markPregenComplete()) {
            consolePrintln("WARNING: Failed to record the completed standard range");
        }
    }
    if (cancelled) {
        consolePrintln("\nGeneration cancelled.");
        return GenerateOutcome::eCancelled;
    }
    if (!success) {
        consolePrintln("\nGeneration encountered errors.");
        return GenerateOutcome::eFailed;
    }
    consolePrintln("Generation complete.");
    return GenerateOutcome::eComplete;
}

void MSDFPregen::refreshLoadedFonts(const PreGenRequest& req) {
    const std::filesystem::path written = MSDFCache::getCacheBasePath(
        req.family_name.c_str(), req.style_name.c_str(), msdf::kSdfRenderSize, msdf::kSdfSpread, req.hash);
    for (const auto& font : *MSDFFont::kFontHandles | std::views::values) {
        if (!font || !font->cache_ || font->cache_->cache_base_path_ != written) { continue; }
        font->cache_->refresh();
        font->glyph_pool_.clear();
        font->atlas_pages_.clear();
        font->oldest_page_ = 0;
        font->eviction_count_++;
    }
    MSDFManager::flushAll();
}

int WINAPI MSDFPregen::consoleCtrlHandler(DWORD ctrl_type) {
    if (ctrl_type == CTRL_CLOSE_EVENT || ctrl_type == CTRL_C_EVENT || ctrl_type == CTRL_BREAK_EVENT) {
        cancel_pregen_.store(true, std::memory_order_release);

        if (HANDLE in = CreateFileW(L"CONIN$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                nullptr, OPEN_EXISTING, 0, nullptr);
            in != INVALID_HANDLE_VALUE) {
            std::array<INPUT_RECORD, 2> rec{};
            rec[0].EventType = KEY_EVENT;
            rec[0].Event.KeyEvent.bKeyDown = TRUE;
            rec[0].Event.KeyEvent.wRepeatCount = 1;
            rec[0].Event.KeyEvent.wVirtualKeyCode = VK_RETURN;
            rec[0].Event.KeyEvent.uChar.UnicodeChar = L'\r';
            rec[1] = rec[0];
            rec[1].Event.KeyEvent.bKeyDown = FALSE;
            DWORD written = 0;
            WriteConsoleInputW(in, rec.data(), rec.size(), &written);
            CloseHandle(in);
        }
        if (pregen_finished_event_ != nullptr) { WaitForSingleObject(pregen_finished_event_, kCancelWaitMs); }
        return TRUE;
    }
    return FALSE;
}

void MSDFPregen::flushStdin() {
    int c;
    while (console_in != nullptr && (c = std::fgetc(console_in)) != '\n' && c != EOF) {}
}
