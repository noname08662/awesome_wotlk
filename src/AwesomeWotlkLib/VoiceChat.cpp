#include "VoiceChat.h"

#include <ankerl/unordered_dense.h>
#include <windows.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>
#include <iterator>
#include <limits>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "Extensions.h"
#include "Utils.h"

#include "include/Lib/Lua.h"

#include <sapi.h>
#include <sphelper.h>

namespace {
enum PlaybackDest : int {
    eDestLocalPlayback = 1,
    eDestQueuedLocalPlayback = 4,
};

enum class SpeechEventKind : uint8_t { eStart, eEnd };

struct SpeechEvent {
    SpeechEventKind kind;
    ULONG stream_num;
};

struct Voice {
    int id;
    std::wstring name;
    ISpObjectToken* token;
};

struct Utterance {
    int id;
    int dest;
    bool started;
};

class SpeechEventQueue {
public:
    SpeechEventQueue() : first_(std::make_unique<Chunk>()), hint_(first_.get()), read_chunk_(first_.get()) {}

    ~SpeechEventQueue() { releaseOverflow(); }

    SpeechEventQueue(const SpeechEventQueue&) = delete;
    SpeechEventQueue& operator=(const SpeechEventQueue&) = delete;
    SpeechEventQueue(SpeechEventQueue&&) = delete;
    SpeechEventQueue& operator=(SpeechEventQueue&&) = delete;

    void push(SpeechEvent speech_event) {
        const uint64_t ticket = next_ticket_.fetch_add(1, std::memory_order_relaxed);
        Chunk* chunk = hint_.load(std::memory_order_acquire);
        if (ticket < chunk->base) { chunk = first_.get(); }
        while (ticket >= chunk->base + kChunkSize) {
            Chunk* next = chunk->next.load(std::memory_order_acquire);
            if (next == nullptr) {
                auto fresh = std::make_unique<Chunk>();
                fresh->base = chunk->base + kChunkSize;
                Chunk* expected = nullptr;
                if (chunk->next.compare_exchange_strong(
                        expected, fresh.get(), std::memory_order_acq_rel, std::memory_order_acquire)) {
                    next = fresh.release();
                } else {
                    next = expected;
                }
            }
            chunk = next;
        }
        hint_.store(chunk, std::memory_order_release);
        auto& [ready, event] = chunk->slots[ticket % kChunkSize];
        event = speech_event;
        ready.store(true, std::memory_order_release);
    }

    template <typename F>
    void drain(const F& handler) {
        for (;;) {
            if (read_ >= read_chunk_->base + kChunkSize) {
                Chunk* next = read_chunk_->next.load(std::memory_order_acquire);
                if (next == nullptr) { return; }
                read_chunk_ = next;
            }
            const auto& [ready, event] = read_chunk_->slots[read_ % kChunkSize];
            if (!ready.load(std::memory_order_acquire)) { return; }
            ++read_;
            handler(event);
        }
    }

    void reset() {
        releaseOverflow();
        for (auto& [ready, event] : first_->slots) {
            ready.store(false, std::memory_order_relaxed);
        }
        next_ticket_.store(0, std::memory_order_relaxed);
        hint_.store(first_.get(), std::memory_order_relaxed);
        read_chunk_ = first_.get();
        read_ = 0;
    }

private:
    static constexpr uint64_t kChunkSize = 64;

    struct Slot {
        std::atomic<bool> ready{false};
        SpeechEvent event{};
    };

    struct Chunk {
        std::array<Slot, kChunkSize> slots;
        std::atomic<Chunk*> next{nullptr};
        uint64_t base = 0;
    };

    void releaseOverflow() const {
        std::unique_ptr<Chunk> chunk{first_->next.exchange(nullptr, std::memory_order_acq_rel)};
        while (chunk != nullptr) {
            chunk.reset(chunk->next.load(std::memory_order_acquire));
        }
    }

    std::unique_ptr<Chunk> first_;
    std::atomic<Chunk*> hint_;
    std::atomic<uint64_t> next_ticket_{0};
    Chunk* read_chunk_;
    uint64_t read_ = 0;
};
}  // namespace

namespace {
std::vector<Voice> enumerateVoices() {
    std::vector<Voice> voices;
    IEnumSpObjectTokens* enumerator = nullptr;
    ULONG count = 0;

    const HRESULT hr = SpEnumTokens(SPCAT_VOICES, nullptr, nullptr, &enumerator);
    if (SUCCEEDED(hr) && enumerator != nullptr) {
        ISpObjectToken* token = nullptr;
        while (enumerator->Next(1, &token, &count) == S_OK && count != 0) {
            WCHAR* description = nullptr;
            if (SUCCEEDED(SpGetDescription(token, &description)) && description != nullptr) {
                token->AddRef();
                voices.push_back({.id = static_cast<int>(voices.size()), .name = description, .token = token});
                CoTaskMemFree(description);
            }
            token->Release();
        }
        enumerator->Release();
    }
    return voices;
}

void releaseVoices(std::vector<Voice>& voices) {
    for (const Voice& v : voices) {
        if (v.token != nullptr) { v.token->Release(); }
    }
    voices.clear();
}

int liveVoiceCount() {
    std::vector<Voice> voices = enumerateVoices();
    const auto count = static_cast<int>(voices.size());
    releaseVoices(voices);
    return count;
}

using extensions::console::kEventRegistry;

constexpr auto kEventFailed = "VOICE_CHAT_TTS_PLAYBACK_FAILED";
constexpr auto kEventFinished = "VOICE_CHAT_TTS_PLAYBACK_FINISHED";
constexpr auto kEventStarted = "VOICE_CHAT_TTS_PLAYBACK_STARTED";
constexpr auto kEventVoicesUpdate = "VOICE_CHAT_TTS_VOICES_UPDATE";
constexpr auto kEventSpeakTextUpdate = "VOICE_CHAT_TTS_SPEAK_TEXT_UPDATE";

void __stdcall onSpeechNotify(WPARAM, LPARAM);

struct TTS {
private:
    std::atomic<ISpVoice*> voice{nullptr};
    std::atomic<int> callbacks_in_flight{0};
    bool com_owned = false;
    std::vector<Voice> voices;
    ankerl::unordered_dense::map<ULONG, Utterance> streams;
    SpeechEventQueue events;
    int next_utterance_id = 1;

public:
    std::vector<Voice> getVoices() { return voices; }

    int nextUtteranceId() {
        const int id = next_utterance_id;
        next_utterance_id = (id == (std::numeric_limits<int>::max)()) ? 1 : id + 1;
        return id;
    }

    [[nodiscard]]
    std::string voiceName(int voice_id) const {
        if (voice_id < 0 || voice_id >= std::ssize(voices)) { return {}; }
        return utils::wideToUtf8(voices[voice_id].name);
    }

    ULONG startSpeaking(int voice_id, const std::wstring& text, int rate, USHORT volume) const {
        ISpVoice* current = voice.load(std::memory_order_acquire);
        if (current == nullptr || voice_id < 0 || voice_id >= std::ssize(voices)) { return 0; }

        ISpObjectToken* token = voices[voice_id].token;
        if (token == nullptr || FAILED(current->SetVoice(token))) { return 0; }

        current->SetRate(std::clamp(rate, -10, 10));
        current->SetVolume(static_cast<USHORT>(std::clamp(static_cast<int>(volume), 0, 100)));

        ULONG stream_num = 0;
        if (FAILED(current->Speak(text.c_str(), SPF_ASYNC, &stream_num))) { return 0; }
        return stream_num;
    }

    void speakText(int voice_id, const std::wstring& text, int destination, int rate, int volume) {
        const int utterance_id = nextUtteranceId();
        const int dest = (destination == eDestQueuedLocalPlayback) ? eDestQueuedLocalPlayback : eDestLocalPlayback;

        const ULONG stream_num = startSpeaking(voice_id, text, rate, static_cast<USHORT>(volume));
        if (stream_num == 0) {
            kEventRegistry->fire(kEventFailed, "%s%d%d", "InternalError", utterance_id, dest);
            return;
        }
        streams[stream_num] = Utterance{.id = utterance_id, .dest = dest, .started = false};
    }

    void stopAllAsync() const {
        ISpVoice* current = voice.load(std::memory_order_acquire);
        if (current == nullptr) { return; }
        current->AddRef();
        std::thread([current] {
            const HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
            current->Speak(nullptr, SPF_PURGEBEFORESPEAK, nullptr);
            current->Release();
            if (SUCCEEDED(hr)) { CoUninitialize(); }
        }).detach();
    }

    void refreshVoices() {
        std::vector<Voice> fresh = enumerateVoices();
        const bool changed =
            !std::ranges::equal(fresh, voices, [](const Voice& a, const Voice& b) { return a.name == b.name; });

        if (!changed) {
            releaseVoices(fresh);
            return;
        }
        releaseVoices(voices);
        voices = std::move(fresh);
        kEventRegistry->fire(kEventVoicesUpdate, "");
    }

    void handleSpeechNotify() {
        callbacks_in_flight.fetch_add(1);
        ISpVoice* current = voice.load();
        if (current == nullptr) {
            callbacks_in_flight.fetch_sub(1);
            return;
        }

        SPEVENT ev = {};
        ULONG fetched = 0;
        while (SUCCEEDED(current->GetEvents(1, &ev, &fetched)) && fetched == 1) {
            switch (ev.eEventId) {
                case SPEI_START_INPUT_STREAM:
                    events.push({.kind = SpeechEventKind::eStart, .stream_num = ev.ulStreamNum});
                    break;
                case SPEI_END_INPUT_STREAM:
                    events.push({.kind = SpeechEventKind::eEnd, .stream_num = ev.ulStreamNum});
                    break;
                default:
                    break;
            }
            SpClearEvent(&ev);
        }
        callbacks_in_flight.fetch_sub(1);
    }

    void enter() {
        if (voice.load(std::memory_order_acquire) != nullptr) { return; }

        if (SUCCEEDED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED))) { com_owned = true; }

        ISpVoice* created = nullptr;
        if (FAILED(CoCreateInstance(CLSID_SpVoice, nullptr, CLSCTX_ALL, IID_PPV_ARGS(&created)))) { return; }

        created->SetNotifyCallbackFunction(onSpeechNotify, 0, 0);
        constexpr ULONGLONG kInterest = SPFEI(SPEI_START_INPUT_STREAM) | SPFEI(SPEI_END_INPUT_STREAM);
        created->SetInterest(kInterest, kInterest);
        voice.store(created, std::memory_order_release);
        refreshVoices();
    }

    void leave() {
        if (ISpVoice* current = voice.exchange(nullptr); current != nullptr) {
            current->Speak(nullptr, SPF_PURGEBEFORESPEAK, nullptr);
            while (callbacks_in_flight.load() != 0) {
                std::this_thread::yield();
            }
            current->Release();
        }
        events.reset();
        streams.clear();
        releaseVoices(voices);

        if (com_owned) {
            CoUninitialize();
            com_owned = false;
        }
    }

    void update() {
        events.drain([this](const SpeechEvent& ev) {
            const auto it = streams.find(ev.stream_num);
            if (it == streams.end()) { return; }

            auto& [id, dest, started] = it->second;
            if (ev.kind == SpeechEventKind::eStart) {
                if (started) { return; }
                started = true;
                kEventRegistry->fire(kEventStarted, "%d%d%d%d", 1, id, 0, dest);
            } else {
                kEventRegistry->fire(kEventFinished, "%d%d%d", 1, id, dest);
                streams.erase(it);
            }
        });
    }
};

inline constexpr utils::Accessor<TTS, struct TTSTag> kTTS;

void __stdcall onSpeechNotify(WPARAM, LPARAM) { kTTS->handleSpeechNotify(); }
}  // namespace

namespace {
void pushVoiceList(LuaState* l, const std::vector<Voice>& voices) {
    lua::createTable(l, 0, 0);

    int i = 1;
    for (const auto& [id, name, token] : voices) {
        lua::newTable(l);

        lua::pushNumber(l, id);
        lua::setField(l, -2, "voiceID");

        const std::string name_utf8 = utils::wideToUtf8(name);
        lua::pushString(l, name_utf8.c_str());
        lua::setField(l, -2, "name");

        lua::rawSetI(l, -2, i++);
    }
}

int luaGetTTSVoices(LuaState* l) {
    kTTS->refreshVoices();
    pushVoiceList(l, kTTS->getVoices());
    return 1;
}

int luaGetRemoteTTSVoices(LuaState* l) {
    std::vector<Voice> voices = enumerateVoices();
    pushVoiceList(l, voices);
    releaseVoices(voices);
    return 1;
}

int luaSpeakText(LuaState* l) {
    const auto voice_id = static_cast<int>(lua::checkNumber(l, 1));
    const char* text = lua::checkLString(l, 2, nullptr);

    int dest = eDestLocalPlayback;
    if (lua::getTop(l) >= 3 && !lua::isNil(l, 3)) {
        dest = static_cast<int>(lua::checkNumber(l, 3));
        dest = (dest == eDestQueuedLocalPlayback) ? eDestQueuedLocalPlayback : eDestLocalPlayback;
    }

    int rate = 0;
    if (lua::getTop(l) >= 4 && !lua::isNil(l, 4)) { rate = static_cast<int>(lua::checkNumber(l, 4)); }

    int volume = 100;
    if (lua::getTop(l) >= 5 && !lua::isNil(l, 5)) { volume = static_cast<int>(lua::checkNumber(l, 5)); }

    kTTS->speakText(voice_id, utils::utf8ToWide(text), dest, rate, volume);
    return 0;
}

int luaStopSpeakingText(LuaState*) {
    kTTS->stopAllAsync();
    return 0;
}

int luaOpenVoiceChat(LuaState* l) {
    static constexpr std::array<lua::LuaLReg, 4> kFuncs = {{
        {.name = "GetTtsVoices", .func = luaGetTTSVoices},
        {.name = "GetRemoteTtsVoices", .func = luaGetRemoteTTSVoices},
        {.name = "SpeakText", .func = luaSpeakText},
        {.name = "StopSpeakingText", .func = luaStopSpeakingText},
    }};

    lua::createTable(l, 0, kFuncs.size());
    for (const auto& [name, func] : kFuncs) {
        lua::pushCFunction(l, func);
        lua::setField(l, -2, name);
    }
    lua::setGlobal(l, "C_VoiceChat");
    return 0;
}
}  // namespace

namespace {
int luaRefreshVoices(LuaState*) {
    kTTS->refreshVoices();
    return 0;
}

int luaSetDefaultSettings(LuaState*) {
    auto& cvars = *extensions::console::kCvarRegistry;
    const int default_voice = (liveVoiceCount() > 1) ? 1 : 0;
    cvars.set("ttsVoice", std::to_string(default_voice).c_str());
    cvars.set("ttsSpeed", "0");
    cvars.set("ttsVolume", "100");
    kEventRegistry->fire(kEventVoicesUpdate, "");
    return 0;
}

int luaSetSpeechRate(LuaState* l) {
    extensions::console::kCvarRegistry->set("ttsSpeed", lua::checkString(l, 1));
    return 0;
}

int luaSetSpeechVolume(LuaState* l) {
    extensions::console::kCvarRegistry->set("ttsVolume", lua::checkString(l, 1));
    return 0;
}

int luaSetVoiceOptionById(LuaState* l) {
    if (liveVoiceCount() > 0) {
        extensions::console::kCvarRegistry->set("ttsVoice", lua::checkString(l, 1));
        kEventRegistry->fire(kEventVoicesUpdate, "");
    }
    return 0;
}

int luaSetVoiceOptionByName(LuaState* l) {
    const std::wstring wname = utils::utf8ToWide(lua::checkString(l, 1));

    std::vector<Voice> voices = enumerateVoices();
    const auto it = std::ranges::find_if(voices, [&](const Voice& v) { return utils::iequals(v.name, wname); });
    const int found = (it != voices.end()) ? std::distance(voices.begin(), it) : -1;
    releaseVoices(voices);

    if (found >= 0) {
        extensions::console::kCvarRegistry->set("ttsVoice", std::to_string(found).c_str());
        kEventRegistry->fire(kEventVoicesUpdate, "");
    }
    return 0;
}

int luaGetSpeechRate(LuaState* l) {
    lua::pushNumber(l, extensions::console::kCvarRegistry->ref<"ttsSpeed", int>());
    return 1;
}

int luaGetSpeechVolume(LuaState* l) {
    lua::pushNumber(l, extensions::console::kCvarRegistry->ref<"ttsVolume", int>());
    return 1;
}

int luaGetSpeechVoiceId(LuaState* l) {
    lua::pushNumber(l, extensions::console::kCvarRegistry->ref<"ttsVoice", int>());
    return 1;
}

int luaGetVoiceOptionName(LuaState* l) {
    lua::pushString(l, kTTS->voiceName(extensions::console::kCvarRegistry->ref<"ttsVoice", int>()).c_str());
    return 1;
}

int luaOpenTTSSettings(LuaState* l) {
    static constexpr std::array<lua::LuaLReg, 10> kFuncs = {{
        {.name = "GetSpeechRate", .func = luaGetSpeechRate},
        {.name = "GetSpeechVolume", .func = luaGetSpeechVolume},
        {.name = "GetSpeechVoiceID", .func = luaGetSpeechVoiceId},
        {.name = "GetVoiceOptionName", .func = luaGetVoiceOptionName},
        {.name = "SetDefaultSettings", .func = luaSetDefaultSettings},
        {.name = "SetSpeechRate", .func = luaSetSpeechRate},
        {.name = "SetSpeechVolume", .func = luaSetSpeechVolume},
        {.name = "SetVoiceOption", .func = luaSetVoiceOptionById},
        {.name = "SetVoiceOptionByName", .func = luaSetVoiceOptionByName},
        {.name = "RefreshVoices", .func = luaRefreshVoices},
    }};

    lua::createTable(l, 0, 0);
    for (const auto& [name, func] : kFuncs) {
        lua::pushCFunction(l, func);
        lua::setField(l, -2, name);
    }
    lua::setGlobal(l, "C_TTSSettings");
    return 0;
}
}  // namespace

void voice_chat::initialize(hookkit::HookTransaction&) {
    auto& cvars = *extensions::console::kCvarRegistry;

    cvars.add<int>({.name = "ttsVoice", .init = 1, .min{0}, .max{[] { return std::max(liveVoiceCount() - 1, 0); }}});
    cvars.add<int>({.name = "ttsSpeed", .init = 0, .min{-10}, .max{10}});
    cvars.add<int>({.name = "ttsVolume", .init = 100, .min{0}, .max{100}});

    for (const char* name : {kEventFailed, kEventFinished, kEventStarted, kEventVoicesUpdate, kEventSpeakTextUpdate}) {
        kEventRegistry->add(name);
    }

    static_cast<void>(*kTTS);

    extensions::console::kLuaLibRegistry->add(luaOpenVoiceChat);
    extensions::console::kLuaLibRegistry->add(luaOpenTTSSettings);

    extensions::framescript::kOnEnter->add([] { kTTS->enter(); });
    extensions::framescript::kOnLeave->add([] { kTTS->leave(); });
    extensions::framescript::kOnUpdate->add([] { kTTS->update(); });
}
