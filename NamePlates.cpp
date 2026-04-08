#include "NamePlates.h"
#include <format>
#include <bit>
#include <algorithm>
#include <unordered_dense/include/ankerl/unordered_dense.h>

#undef min
#undef max

namespace {
    constexpr auto NAME_PLATE_CREATED = "NAME_PLATE_CREATED";
    constexpr auto NAME_PLATE_UNIT_ADDED = "NAME_PLATE_UNIT_ADDED";
    constexpr auto NAME_PLATE_UNIT_REMOVED = "NAME_PLATE_UNIT_REMOVED";
    constexpr float EPS = 1e-4f;

    constexpr uint16_t MAX_PLATES = 768;
    static_assert(MAX_PLATES % 64 == 0, "MAX_PLATES must be a multiple of 64 for pending bitset words");
    static_assert(MAX_PLATES <= 1024, "activeCollisions array sized for up to 1024 plates (32 uint32_t words)");

    CVar* s_cvar_nameplateDistance;
    CVar* s_cvar_nameplateStacking;
    CVar* s_cvar_nameplateBandX;
    CVar* s_cvar_nameplateBandY;
    CVar* s_cvar_nameplateHitboxWidthE;
    CVar* s_cvar_nameplateHitboxHeightE;
    CVar* s_cvar_nameplateHitboxWidthF;
    CVar* s_cvar_nameplateHitboxHeightF;
    CVar* s_cvar_nameplatePlacement;
    CVar* s_cvar_nameplateMouseMode;
    CVar* s_cvar_nameplateRaiseSpeed;
    CVar* s_cvar_nameplateLowerSpeed;
    CVar* s_cvar_nameplatePullSpeed;
    CVar* s_cvar_nameplateRaiseDistance;
    CVar* s_cvar_nameplatePullDistance;
    CVar* s_cvar_nameplateOcclusionAlpha;
    CVar* s_cvar_nameplateNonTargetAlpha;
    CVar* s_cvar_nameplateAlphaSpeed;
    CVar* s_cvar_nameplateClampTop;
    CVar* s_cvar_nameplateClampTopOffset;

    enum EStackingMode : uint8_t {
        S_DISABLED = 0,
        S_ALL = 1,
        S_ENEMY = 2,
        S_FRIENDLY = 3,
    };

    enum EClampMode : uint8_t {
        C_DISABLED = 0,
        C_ALL = 1,
        C_BOSS = 2,
    };

    enum EMouseMode : uint8_t {
        M_DISABLED = 0,
        M_CLICK_THRU_ENEMY = 1 << 0,
        M_CLICK_THRU_FRIEND = 1 << 1,
        M_OVER_ALWAYS = 1 << 2,
        M_OVER_COMBAT = 1 << 3,
    };

    constexpr uint32_t g_mouseModeMap[] = {
        M_DISABLED,
        M_CLICK_THRU_ENEMY,
        M_CLICK_THRU_ENEMY | M_OVER_ALWAYS,
        M_CLICK_THRU_ENEMY | M_OVER_COMBAT,
        M_CLICK_THRU_FRIEND,
        M_CLICK_THRU_FRIEND | M_OVER_ALWAYS,
        M_CLICK_THRU_FRIEND | M_OVER_COMBAT,
        M_OVER_ALWAYS,
        M_OVER_COMBAT
    };

    enum ESortType : uint8_t {
        ST_IN = 1 << 0,
        ST_OUT = 1 << 1
    };

    EStackingMode g_stackingMode = EStackingMode::S_DISABLED;
    EMouseMode g_mouseOverMode = EMouseMode::M_DISABLED;
    EClampMode g_clampTop = EClampMode::C_DISABLED;
    float g_bandX = 0.7f;
    float g_bandY = 1.0f;
    float g_hitWidthE = 1.0f;
    float g_hitHeightE = 1.0f;
    float g_hitWidthF = 1.0f;
    float g_hitHeightF = 1.0f;
    float g_speedRaise = 100.0f;
    float g_speedLower = 100.0f;
    float g_speedPull = 50.0f;
    float g_maxPull = 0.25f;
    float g_maxRaise = 8.0f;
    float g_clampTopOffset = 0.1f;
    float g_nonTargetAlpha = 0.5f;
    float g_occlusionAlpha = 1.0f;
    float g_alphaSpd = 0.25f;

    auto* const g_alloc = reinterpret_cast<CDataAllocator*>(0x00DCEC44);
    auto* const g_lockedTarget = reinterpret_cast<guid_t*>(0x00BD07B0);
    auto* const g_nameplateFocus = reinterpret_cast<CGNamePlate**>(0x00CA1204);

    bool g_inCombat = false;

    struct alignas(64) Entry {
        enum class IEState : uint8_t {
            NONE = 0,
            SHOULD_STACK = 0x1,
            SHOULD_CLAMP = 0x2,
            IS_FRIENDLY = 0x4,
            IS_TRANSIENT = 0x8,
            IS_FRESH = 0x10,
            IS_ACTIVE = 0x20,
        };

        bool hasState(IEState flag) const { return (static_cast<uint32_t>(state) & static_cast<uint32_t>(flag)) != 0; }
        void setState(IEState flag, bool on) {
            if (on) state = static_cast<IEState>(static_cast<uint32_t>(state) | static_cast<uint32_t>(flag));
            else state = static_cast<IEState>(static_cast<uint32_t>(state) & ~static_cast<uint32_t>(flag));
        }

        CGNamePlate* ptr = nullptr;
        CDataChunk* chunk = nullptr;
        guid_t guid = 0;
        uint16_t block = 0;
        ECreatureRank classification = RANK_NORMAL;

        float momentumY = 0.0f;
        float momentumX = 0.0f;

        int pushCount = 0;

        float commitTargetX = 0.0f;
        float commitTargetY = 0.0f;
        float targetOffsetX = 0.0f;
        float targetOffsetY = 0.0f;
        float smoothTargetX = 0.0f;
        float smoothTargetY = 0.0f;
        float stackOffsetX = 0.0f;
        float stackOffsetY = 0.0f;

        float stableNDCY = 0.0f;
        float stableNDCX = 0.0f;
        float velocityX = 0.0f;
        float velocityY = 0.0f;
        float lastStableNDCX = 0.0f;
        float lastStableNDCY = 0.0f;

        IEState state = IEState::NONE;

        uint32_t activeCollisions[MAX_PLATES / 32];

        void freshState() {
            targetOffsetY = 0.0f;
            targetOffsetX = 0.0f;
            pushCount = 0;
        }

        void clearState() {
            commitTargetY = 0.0f;
            commitTargetX = 0.0f;
            targetOffsetY = 0.0f;
            targetOffsetX = 0.0f;
            stackOffsetY = 0.0f;
            stackOffsetX = 0.0f;
            smoothTargetX = 0.0f;
            smoothTargetY = 0.0f;
            momentumX = 0.0f;
            momentumY = 0.0f;
            stableNDCY = ptr->m_NDCproj.y;
            stableNDCX = ptr->m_NDCproj.x;
            velocityX = 0.0f;
            velocityY = 0.0f;
            lastStableNDCX = 0.0f;
            lastStableNDCY = 0.0f;
            state = IEState::IS_FRESH;
        }

        void setActiveCollision(int id) { activeCollisions[id >> 5] |= (1u << (id & 31)); }
        void setInactiveCollision(int id) { activeCollisions[id >> 5] &= ~(1u << (id & 31)); }

        float getTopNDC(float perc = 1.0f) const { return stableNDCY + ptr->m_height * 0.5f * perc; }
        float getBotNDC(float perc = 1.0f) const { return stableNDCY - ptr->m_height * 0.5f * perc; }
        float getAvgWFor(const Entry* e, float perc = 1.0f) const { return (ptr->m_width + e->ptr->m_width) * 0.5f * perc; }
        float getAvgHFor(const Entry* e, float perc = 1.0f) const { return (ptr->m_height + e->ptr->m_height) * 0.5f * perc; }
        float getReqYFor(const Entry* e, float perc = 1.0f) const { return stableNDCY + targetOffsetY + getAvgHFor(e, perc) - e->stableNDCY; }
        float getReqXFor(const Entry* e) const { return (stableNDCX + commitTargetX) - (e->stableNDCX + e->commitTargetX); }
        float getReqDXFor(const Entry* e) const { return std::abs((stableNDCX + commitTargetX) - (e->stableNDCX + e->commitTargetX)); }
        float getReqDYFor(const Entry* e) const { return std::abs((stableNDCY + commitTargetY) - (e->stableNDCY + e->commitTargetY)); }
        float getProximity(const Entry* e, float bx = 1.0f, float by = 1.0f) const {
            return std::clamp(std::min(getReqDXFor(e) / getAvgWFor(e, bx), getReqDYFor(e) / getAvgHFor(e, by)), 0.0f, 1.0f);
        }
        int getRankWeight() const {
            switch (classification) {
            case RANK_TRIVIAL: return 0;
            case RANK_NORMAL: return 1;
            case RANK_RARE: return 2;
            case RANK_ELITE: return 3;
            case RANK_RAREELITE: return 4;
            case RANK_WORLDBOSS: return 5;
            default: return 0;
            }
        }

        bool resolvePush(const Entry* e, float sep, float hyst) const {
            return ((e->getTarY() > getBotNDC(sep * (hyst + 1.0f)) + targetOffsetY) && (e->getTarY() < getTopNDC(sep * (hyst + 1.0f)) + targetOffsetY)
                || (e->getBotNDC() + e->stackOffsetY > getTopNDC() + targetOffsetY && e->getBotNDC() + e->targetOffsetY < getTopNDC() + targetOffsetY));
        }

        float getVisY() const { return ptr->m_NDCproj.y + stackOffsetY; }
        float getVisX() const { return ptr->m_NDCproj.x + stackOffsetX; }

        float getTarY() const { return ptr->m_NDCproj.y + targetOffsetY; }
        float getTarX() const { return ptr->m_NDCproj.x + targetOffsetX; }

        void updStableAnchor(float delta) {
            float t = 1.0f - std::exp(-5.0f * delta);
            stableNDCX += (ptr->m_NDCproj.x - stableNDCX) * t;
            stableNDCY += (ptr->m_NDCproj.y - stableNDCY) * t;
            velocityX = (stableNDCX - lastStableNDCX) / delta;
            velocityY = (stableNDCY - lastStableNDCY) / delta;
            lastStableNDCX = stableNDCX;
            lastStableNDCY = stableNDCY;
        }

        bool isAtX(float x) const { return std::abs(stackOffsetX - x) < EPS; }
        bool isAtY(float y) const { return std::abs(stackOffsetY - y) < EPS; }
        bool isAt(float x, float y) const { return isAtX(x) && isAtY(y); }

        void updVis(const float spdY, const float spdX, const float delta, const float maxY, const float ceilY) {
            float maxPull = ptr->m_width * g_maxPull;
            float maxRaise = (hasState(IEState::SHOULD_CLAMP)
                ? std::min(ptr->m_height * maxY, NDC_Y - ceilY - getTopNDC())
                : ptr->m_height * maxY);

            float gapY = targetOffsetY - commitTargetY;
            float gapX = targetOffsetX - commitTargetX;

            float wantMomY = std::abs(gapY) > EPS ? (gapY > 0.0f ? 1.0f : -1.0f) : 0.0f;
            float wantMomX = std::abs(gapX) > EPS ? (gapX > 0.0f ? 1.0f : -1.0f) : 0.0f;

            float rateY = (wantMomY * momentumY < 0.0f) ? 1.0f : spdY * 0.025f;
            float rateX = (wantMomX * momentumX < 0.0f) ? 1.0f : spdX * 0.025f;

            momentumY += (wantMomY - momentumY) * std::clamp(rateY * delta, 0.0f, 1.0f);
            momentumX += (wantMomX - momentumX) * std::clamp(rateX * delta, 0.0f, 1.0f);

            if (!isAt(targetOffsetX, targetOffsetY)) {
                float commitAlpha = std::clamp(4.0f * std::abs(momentumY) * delta, 0.0f, 1.0f);
                commitTargetY += (targetOffsetY - commitTargetY) * commitAlpha;
                commitAlpha = std::clamp(4.0f * std::abs(momentumX) * delta, 0.0f, 1.0f);
                commitTargetX += (targetOffsetX - commitTargetX) * commitAlpha;
            }
            else {
                commitTargetY = targetOffsetY;
                commitTargetX = targetOffsetX;
                momentumY *= 0.1f;
                momentumX *= 0.1f;
            }
            float sAlphaY = 1.0f - std::exp(-spdY * std::abs(momentumY) * delta);
            float sAlphaX = 1.0f - std::exp(-spdX * std::abs(momentumX) * delta);
            smoothTargetY += (commitTargetY - smoothTargetY) * sAlphaY;
            smoothTargetX += (commitTargetX - smoothTargetX) * sAlphaX;

            float a2y = std::pow(std::clamp(spdY * delta, 0.0f, 1.0f), 1.5f) * std::abs(momentumY * momentumY * momentumY);
            float a2x = std::pow(std::clamp(spdX * delta, 0.0f, 1.0f), 1.5f) * std::abs(momentumX * momentumX * momentumX);
            float dy = smoothTargetY - stackOffsetY;
            float dx = smoothTargetX - stackOffsetX;
            if (std::abs(dy) > EPS) stackOffsetY += dy * std::clamp(a2y, 0.0f, 1.0f);
            else stackOffsetY = smoothTargetY;
            if (std::abs(dx) > EPS) stackOffsetX += dx * std::clamp(a2x, 0.0f, 1.0f);
            else stackOffsetX = smoothTargetX;

            stackOffsetY = maxRaise < 0.0f ? maxRaise : std::min(stackOffsetY, maxRaise);
            stackOffsetX = std::clamp(stackOffsetX, -maxPull, maxPull);
        }
    };

    class EntryManager {
        class ChunkManager {
            friend class EntryManager;
            struct ChunkMeta {
                uintptr_t start;
                uintptr_t end;
                CDataChunk* chunk;
            };
            uintptr_t chunksHeadCached = 0;

            std::vector<ChunkMeta> chunks;

            void updateHead(uintptr_t h) {
                if (h != chunksHeadCached || chunks.empty()) {
                    chunks.clear();
                    for (auto* cur = g_alloc->m_chunkList; cur; cur = cur->m_nextChunk) {
                        const auto start = reinterpret_cast<uintptr_t>(&cur->m_data);
                        const auto end = start + g_alloc->m_blocksPerChunk * g_alloc->m_blockSize;
                        chunks.push_back({ .start = start, .end = end, .chunk = cur });
                    }
                    std::ranges::sort(chunks, {}, &ChunkMeta::start);
                    chunksHeadCached = h;
                }
            }
            const ChunkMeta* findChunk(uintptr_t addr) {
                if (g_alloc->m_chunkList) {
                    updateHead(reinterpret_cast<uintptr_t>(g_alloc->m_chunkList));
                    if (chunks.empty()) return nullptr;
                    if (auto it = std::ranges::upper_bound(chunks, addr, {}, &ChunkMeta::start); it != chunks.begin()) {
                        --it; if (addr >= it->start && addr < it->end) return &(*it);
                    }
                }
                return nullptr;
            }
            void reserve(size_t r) { chunks.reserve(r); }
            void clear() { chunks.clear(); chunksHeadCached = 0; }
        } chunkMgr;

        class PairsManager {
            friend class EntryManager;
            struct alignas(32) PairState {
                uint64_t timestamp = 0;

                int hystSteps = 0;
                float hystDecay = 0.0f;
                float hysteresis = 1.0f;

                uint64_t proximate = 0;

                const Entry* e1 = nullptr;
                const Entry* e2 = nullptr;

                bool isStale(uint64_t ms) const { return timestamp < ms; }
                bool isApart(uint64_t ms) const { return proximate < ms; }
                void commit(uint64_t ms, float hyst) {
                    if (hystDecay > 0.0f && timestamp == 0) {
                        hystSteps++;
                        hystDecay = 1.0f;
                    }
                    else if (timestamp == 0) {
                        hystDecay = 1.0f;
                    }
                    timestamp = ms;
                    hysteresis = hyst;
                }
                void cooldown(uint64_t ms, float delta) {
                    if (!e1 || !e2) {
                        hystDecay = 0.0f;
                        hystSteps = 0;
                        return;
                    }
                    if (hystDecay > 0.0f) {
                        hystDecay -= (e1->getProximity(e2) * 0.15f) * delta;
                        if (hystDecay <= 0.0f) {
                            hystDecay = 0.0f;
                            hystSteps = 0;
                        }
                    }
                }
                void seed(uint64_t ms, const Entry* e1_, const Entry* e2_) {
                    proximate = ms;
                	if (!e1 || !e2) { e1 = e1_; e2 = e2_; }
                }
                void reset(bool full = false) {
                    hysteresis = 1.0f;
                    timestamp = 0;
                    if (full) {
                        proximate = 0;
                        hystDecay = 0.0f;
                        hystSteps = 0;
                        e1 = nullptr;
                        e2 = nullptr;
                    }
                }
            };
            // inline pool for the speed
            size_t size = 0;
            std::unique_ptr<PairState[]> pairs;
            void init(size_t r) { pairs = std::make_unique<PairState[]>(r * r); size = r; }
            void wipe() const { if (pairs) std::fill_n(pairs.get(), size * size, PairState{}); }
            PairState* get(int id1, int id2) { return &pairs[std::min(id1, id2) * size + std::max(id1, id2)]; }
        } pairsMgr;

        std::vector<Entry>  byId;
        std::vector<Entry*> entries;
        std::vector<const char*> tokenCache;

        uint64_t pending[MAX_PLATES / 64] = {};

        enum class IESortMode : uint8_t { DEFAULT, TARGET, FOCUS, TARGET_FOCUS };
        template <IESortMode mode, bool out>
        void sort(guid_t targetGuid = 0, CGNamePlate* focus = nullptr) {
            std::sort(entries.begin(), entries.end(), [this, targetGuid, focus](const Entry* a, const Entry* b) {
                if constexpr (mode == IESortMode::TARGET || mode == IESortMode::TARGET_FOCUS) {
                    bool isT_a = (a->guid == targetGuid);
                    bool isT_b = (b->guid == targetGuid);
                    if (isT_a != isT_b) return isT_a;
                }
                if constexpr (mode == IESortMode::FOCUS || mode == IESortMode::TARGET_FOCUS) {
                    bool isF_a = (a->ptr == focus);
                    bool isF_b = (b->ptr == focus);
                    if (isF_a != isF_b) return isF_a;
                }
                if constexpr (out) return a->ptr->m_depthZ < b->ptr->m_depthZ;
                if (std::abs((a->stableNDCY + a->targetOffsetY) - (b->stableNDCY + b->targetOffsetY)) > EPS) {
                    return (a->stableNDCY + a->targetOffsetY) < (b->stableNDCY + b->targetOffsetY);
                }
                return a->getRankWeight() > b->getRankWeight();
                });
        }

        void onPlateCreated(CGNamePlate* plate) {
            lua_State* L = Lua::GetLuaState();
            Lua::lua_pushstring(L, NAME_PLATE_CREATED);
            Lua::lua_pushframe(L, plate);
            Lua::lua_pushlightuserdata(L, this);
            Lua::lua_pushcclosure(L, [](lua_State* L) -> int {
                auto* self = static_cast<EntryManager*>(Lua::lua_touserdata(L, Lua::upvalueindex(1)));
                if (!self) return 0;
                Lua::lua_rawgeti(L, 1, 0);
                // toframe doesn't seem to work/I'm doing it wrong
                void* frame = Lua::lua_touserdata(L, -1);
                Lua::lua_settop(L, -2);
                if (frame) {
                    auto* plate = static_cast<CGNamePlate*>(frame);
                    if (const int index = plate->GetPlateId(); index >= 0 && index < std::ssize(self->byId) && self->byId[index].ptr == plate) {
                        auto& e = self->byId[index];
                        e.setState(Entry::IEState::IS_TRANSIENT, !Lua::lua_toboolean(L, 2));
                        applyStackingState(&e);
                    }
                }
                return 0;
                }, 1);
            Lua::lua_setfield(L, -2, "SetStackingEnabled");
            Lua::lua_pushlightuserdata(L, this);
            Lua::lua_pushcclosure(L, [](lua_State* L) -> int {
                auto* self = static_cast<EntryManager*>(Lua::lua_touserdata(L, Lua::upvalueindex(1)));
                if (!self) return 0;
                Lua::lua_rawgeti(L, 1, 0);
                void* frame = Lua::lua_touserdata(L, -1);
                Lua::lua_settop(L, -2);
                if (frame) {
                    CGNamePlate* plate = static_cast<CGNamePlate*>(frame);
                    if (const int index = plate->GetPlateId(); index >= 0 && index < std::ssize(self->byId) && self->byId[index].ptr == plate) {
                        auto& e = self->byId[index];
                        Lua::lua_pushboolean(L, e.hasState(Entry::IEState::SHOULD_STACK));
                        return 1;
                    }
                }
                return 0;
                }, 1);
            Lua::lua_setfield(L, -2, "GetStackingEnabled");
            FrameScript::FireEvent_inner(FrameScript::GetEventIdByName(NAME_PLATE_CREATED), L, 2);
            Lua::lua_pop(L, 2);
        }

    public:
        std::vector<Entry*>& get() { return entries; }
        int getTotalSize() const { return std::ssize(byId); }

        PairsManager::PairState* getPair(const Entry* e1, const Entry* e2) {
            return pairsMgr.get(e1->ptr->GetPlateId(), e2->ptr->GetPlateId());
        }

        void commitPair(const Entry* e1, const Entry* e2, uint64_t ms, float delta, float by, float bx) {
            // set bits, update hysteresis
            auto* ps = pairsMgr.get(e1->ptr->GetPlateId(), e2->ptr->GetPlateId());
            float decayFloor = 1.25f + std::min(ps->hystSteps * 0.15f, 0.75f);
            if ((e1->getTopNDC() + e1->targetOffsetY + e1->getAvgHFor(e2, by)) > (e2->getBotNDC() + e2->targetOffsetY)
                && e1->getReqDXFor(e2) < e1->getAvgWFor(e2, bx)) {
                ps->commit(ms, decayFloor); // still overlapping naturally
            }
            else {
                // bboxes no longer overlap — compute separation-scaled decay rate
                ps->commit(ms, std::max(1.0f, ps->hysteresis - (e1->getProximity(e2) * 0.05f) * delta));
            }
        }

        void seedPair(Entry* e1, Entry* e2, uint64_t ms) {
            const int id1 = e1->ptr->GetPlateId(); const int id2 = e2->ptr->GetPlateId();
            pairsMgr.get(id1, id2)->seed(ms, e1, e2);
            e1->setActiveCollision(id2); e2->setActiveCollision(id1);
        }

        void resolvePairs(Entry* e, uint64_t ms, float delta) {
            // cleanup
            const int id1 = e->ptr->GetPlateId();
            const int n = (getTotalSize() + 31) >> 5;
            for (int w = 0; w < n; ++w) {
                uint32_t mask = e->activeCollisions[w];
                while (mask) {
                    int id2 = (w << 5) | (std::countr_zero(mask));
                    auto* ps = pairsMgr.get(id1, id2);
                    bool apart = ps->isApart(ms);
                    if (ps->isStale(ms)) {
                        ps->reset(apart);
                        ps->cooldown(ms, delta);
                    }
                    if (apart) e->setInactiveCollision(id2);
                    mask &= mask - 1;
                }
            }
        }

        void sort(ESortType type) {
            if (type & ESortType::ST_OUT) {
                if (guid_t targetGuid = ObjectMgr::GetTargetGuid()) {
                    CGNamePlate* focus = *g_nameplateFocus;
                    if (focus && (g_mouseOverMode & EMouseMode::M_OVER_ALWAYS || (g_mouseOverMode & EMouseMode::M_OVER_COMBAT && g_inCombat))) {
                        sort<IESortMode::TARGET_FOCUS, true>(targetGuid, focus);
                    }
                    else {
                        sort<IESortMode::TARGET, true>(targetGuid);
                    }
                    return;
                }
                else if (CGNamePlate* focus = *g_nameplateFocus) {
                    if (g_mouseOverMode & EMouseMode::M_OVER_ALWAYS || (g_mouseOverMode & EMouseMode::M_OVER_COMBAT && g_inCombat)) {
                        sort<IESortMode::FOCUS, true>(targetGuid, focus);
                        return;
                    }
                }
                sort<IESortMode::DEFAULT, true>();
                return;
            }
            sort<IESortMode::DEFAULT, false>();
        }

        static void applyReaction(Entry* e) {
            if (CGUnit_C* unit = ObjectMgr::Get<CGUnit_C>(e->ptr->m_ownerGuid, TYPEMASK_UNIT)) {
                e->setState(Entry::IEState::IS_FRIENDLY, unit->IsFriendly());
                applyStackingState(e);
            }
        }

        static void applyStackingState(Entry* e) {
            if (e->hasState(Entry::IEState::IS_TRANSIENT)) {
                e->setState(Entry::IEState::SHOULD_STACK, false);
                return;
            }
            bool shouldStack = (g_stackingMode != EStackingMode::S_DISABLED);
            if (g_stackingMode == EStackingMode::S_FRIENDLY) {
                shouldStack = e->hasState(Entry::IEState::IS_FRIENDLY);
            }
            else if (g_stackingMode == EStackingMode::S_ENEMY) {
                shouldStack = !e->hasState(Entry::IEState::IS_FRIENDLY);
            }
            e->setState(Entry::IEState::SHOULD_STACK, shouldStack);
        }

        static void applyClampingState(Entry* e) {
            if (CGUnit_C* unit = ObjectMgr::Get<CGUnit_C>(e->ptr->m_ownerGuid, TYPEMASK_UNIT)) {
                e->setState(Entry::IEState::SHOULD_CLAMP, g_clampTop == EClampMode::C_ALL
                    || (g_clampTop == EClampMode::C_BOSS && unit->GetCreatureRank() == ECreatureRank::RANK_WORLDBOSS));
            }
        }

        void flushAdded() {
            lua_State* L = Lua::GetLuaState();
            const int n = (std::ssize(byId) + 63) / 64;
            for (int w = 0; w < n; ++w) {
                uint64_t word = pending[w];
                while (word) {
                    const int bit = std::countr_zero(word);
                    const int index = w * 64 + bit;
                    Entry* e = &byId[index];
                    const CGUnit_C* unit = ObjectMgr::Get<CGUnit_C>(e->ptr->m_ownerGuid, TYPEMASK_UNIT);
                    if (unit && unit->m_nameplate) {
                        e->guid = unit->m_nameplate->m_ownerGuid;
                        Lua::lua_pushframe(L, e->ptr);
                        Lua::lua_pushstring(L, tokenCache[index]);
                        Lua::lua_setfield(L, -2, "unit");
                        Lua::lua_pop(L, 1);
                        FrameScript::FireEvent(NAME_PLATE_UNIT_ADDED, "%s", tokenCache[index]);
                    }
                    word &= word - 1;
                }
            }
        }

        void flushRemoved() {
            std::erase_if(entries, [](Entry* e) {
                if (!e->hasState(Entry::IEState::IS_ACTIVE)) {
                    e->guid = 0; return true;
                }
                return false;
                });
        }

        void appendAdded(Entry* e) {
            const int index = e->ptr->GetPlateId();
            if (index < 0 || index >= std::ssize(byId)) return;
            pending[index / 64] |= (1ULL << (index % 64));
            if (!e->hasState(Entry::IEState::IS_ACTIVE)) {
                e->setState(Entry::IEState::IS_ACTIVE, true);
                entries.push_back(e);
            }
        }

        void appendAdded(int index) {
            if (index < 0 || index >= std::ssize(byId)) return;
            pending[index / 64] |= (1ULL << (index % 64));
            if (auto* e = &byId[index]; !e->hasState(Entry::IEState::IS_ACTIVE)) {
                e->setState(Entry::IEState::IS_ACTIVE, true);
                entries.push_back(e);
            }
        }

        void clearPending() {
            std::memset(pending, 0, ((std::ssize(byId) + 63) / 64) * sizeof(uint64_t));
        }

        guid_t getTokenGuid(int index) const {
            if (index < 0 || index >= std::ssize(byId)) return 0;
            return byId[index].guid;
        }

        int getTokenId(guid_t guid) const {
            if (!guid) return -1;
            for (int i = 0; i < std::ssize(byId); ++i) {
                if (byId[i].guid == guid) return i;
            }
            return -1;
        }

        Entry* getEntry(guid_t guid) {
            const int index = getTokenId(guid);
            if (index < 0 || index >= std::ssize(byId)) return nullptr;
            auto& e = byId[index];
            return (e.guid != 0) ? &e : nullptr;
        }

        Entry* getEntry(int index) {
            if (index < 0 || index >= std::ssize(byId)) return nullptr;
            auto& e = byId[index];
            return (e.guid != 0) ? &e : nullptr;
        }

        Entry* initEntry(CGNamePlate* plate) {
            const int index = plate->GetPlateId();
            if (index == -1) {
                const auto addr = reinterpret_cast<uintptr_t>(plate);
                const auto* meta = chunkMgr.findChunk(addr);
                if (!meta) return nullptr;

                // pointers stay valid, cleared on reload
                Entry e{};
                e.ptr = reinterpret_cast<CGNamePlate*>(addr);
                e.chunk = meta->chunk;
                e.block = static_cast<uint16_t>((addr - meta->start) / g_alloc->m_blockSize);
                e.guid = 0;

                byId.push_back(e);
                plate->SetPlateId(std::ssize(byId) - 1);

                onPlateCreated(plate);
                return &byId.back();
            }
            return index < 0 || index >= std::ssize(byId) ? nullptr : &byId[index];
        }

        const char* getToken(guid_t guid) const {
            for (const Entry& e : byId) {
                if (e.guid == guid) return tokenCache[e.ptr->GetPlateId()];
            }
            return "none"; // this one is a valid unitId
        }

        const char* getToken(int index) const {
            if (index >= 0 && index < std::ssize(byId)) {
                return tokenCache[index];
            }
            return "none"; // this one is a valid unitId
        }

        void initializeTokens(int r) {
            for (int i = 1; i <= r; ++i) {
                std::string s = std::format("nameplate{}", i);
                tokenCache.push_back(_strdup(s.c_str()));
            }
        }

        void reserveAll(size_t r) {
            byId.reserve(r);
            chunkMgr.reserve(r);
            pairsMgr.init(r);
            entries.reserve(r);
        }

        void clearAll() {
            chunkMgr.clear();
            pairsMgr.wipe();
            byId.clear();
            entries.clear();
            std::memset(pending, 0, sizeof(pending));
        }
    } g_entries;

    auto(*CGNamePlate__OnUpdate_site)() = reinterpret_cast<DummyCallback_t>(0x0098E9F9);
    constexpr uintptr_t CGNamePlate__OnUpdate_site_jmpback = 0x0098EA27;

    auto(*CGWorldFrame__UpdateNamePlatePositions_site)() = reinterpret_cast<DummyCallback_t>(0x004F90E2);
    constexpr uintptr_t CGWorldFrame__UpdateNamePlatePositions_site_jmpback = 0x004F90EC;

    auto(*CSimpleFrame__SetFrameAlpha_site)() = reinterpret_cast<DummyCallback_t>(0x0098EAA7);
    constexpr uintptr_t CSimpleFrame__SetFrameAlpha_site_jmpback = 0x0098EAD2;

    auto(*CGUnit_C__ShouldShowNamePlate_site)() = reinterpret_cast<DummyCallback_t>(0x0072B2BD);
    constexpr uintptr_t CGUnit_C__ShouldShowNamePlate_site_jmpback = 0x0072B2C7;
    constexpr uintptr_t CGUnit_C__ShouldShowNamePlate_site_pass = 0x0072B247;

    auto(*CGWorldFrame__GetScreenCoordinates_site)() = reinterpret_cast<DummyCallback_t>(0x0071574A);

    const auto isValidObjFn = reinterpret_cast<int(*)(void*)>(0x0077F0B0);

    enum class IEClickLogic : uint8_t { NONE, THRU_ENEMY, THRU_FRIEND };
    template <IEClickLogic mode>
    void findBestPlate(C3Vector* pos, CGNamePlate*& prio) {
        for (const auto& buf = g_entries.get(); const auto& e : buf) {
            if ((e->ptr->m_flags & 0x100) == 0 || *g_lockedTarget == e->ptr->m_ownerGuid) continue;

            // original logic, clamped search boundaries
            const bool isFriendly = e->hasState(Entry::IEState::IS_FRIENDLY);
            const Vec2D hitBox = isFriendly ? Vec2D{ .x = g_hitWidthF, .y = g_hitHeightF } : Vec2D{ .x = g_hitWidthE, .y = g_hitHeightE };

            if (e->ptr->IsAtTargetPos(pos, hitBox)) {
                if (!prio) prio = e->ptr;

                if constexpr (mode == IEClickLogic::THRU_ENEMY) {
                    if (isFriendly) { prio = e->ptr; break; }
                }
                else if constexpr (mode == IEClickLogic::THRU_FRIEND) {
                    if (!isFriendly) { prio = e->ptr; break; }
                }
                else {
                    break;
                }
            }
        }
    }

    int __cdecl CGWorldFrame__UpdateNamePlatePositionsHk(CGWorldFrame* pThis) {
        pThis->EnumerateChildren([&](CSimpleFrame* child) {
            auto* plate = reinterpret_cast<CGNamePlate*>(child);
            const int id = plate->GetPlateId();
            auto* e = g_entries.getEntry(id);
            if (child->m_isShown == 0) {
                if (e && e->hasState(Entry::IEState::IS_ACTIVE)) {
                    FrameScript::FireEvent(NAME_PLATE_UNIT_REMOVED, "%s", g_entries.getToken(id));
                    e->setState(Entry::IEState::IS_ACTIVE, false);
                }
            }
            else {
                CGUnit_C* unit = ObjectMgr::Get<CGUnit_C>(plate->m_ownerGuid, TYPEMASK_UNIT);
                if (e && e->hasState(Entry::IEState::IS_ACTIVE)) {
                    if (!unit || !unit->m_nameplate) {
                        FrameScript::FireEvent(NAME_PLATE_UNIT_REMOVED, "%s", g_entries.getToken(id));
                        e->setState(Entry::IEState::IS_ACTIVE, false);
                    }
                    else if (unit->m_nameplate->m_ownerGuid != e->guid) {
                        FrameScript::FireEvent(NAME_PLATE_UNIT_REMOVED, "%s", g_entries.getToken(id));
                        g_entries.appendAdded(id);
                    }
                }
                else if (unit) {
                	if (const char* name = unit->GetName(nullptr, 1)) {
						const char* unknownText = FrameScript::GetText("UNKNOWNOBJECT", -1, 0);
                		if ((!unknownText || !*unknownText || std::strcmp(name, unknownText) != 0) && std::strcmp(name, "Unknown Being") != 0) {
							if (e = g_entries.initEntry(plate); e) {
								e->clearState();
								e->setState(Entry::IEState::IS_FRIENDLY, unit->IsFriendly());
								e->classification = unit->GetCreatureRank();

								EntryManager::applyStackingState(e);
								EntryManager::applyClampingState(e);
								g_entries.resolvePairs(e, -1, 0);
								g_entries.appendAdded(e);

								// for occlusion
								plate->SetPlateState(EFrameState::NP_IS_FRESH, true);
							}
						}
					}
                }
            }
            });

        g_entries.flushRemoved(); // bulk flush now to ensure callbacks recieve a complete gapless snapshot of the previous frame
        const auto& buf = g_entries.get();
        if (buf.empty()) {
            g_entries.clearPending();
            return CLayoutFrame::ResizePending();
        }

        const int n = std::ssize(buf);
        uint32_t level = static_cast<uint32_t>(n) * 10;

        g_entries.sort(ESortType::ST_IN); // no target/focus
        for (auto& e : buf) e->freshState();

        const float sceneTime = std::min(0.02f, pThis->m_sceneTime);
        const uint64_t ms = CGGameUI::OsGetAsyncTimeMsFn();

        for (int i = 0; i < n; ++i) {
            Entry* e1 = buf[i];
            e1->updStableAnchor(sceneTime);

            if (!e1->hasState(Entry::IEState::SHOULD_STACK) || e1->hasState(Entry::IEState::IS_FRESH)) {
                e1->updVis(g_speedLower, g_speedPull, sceneTime, g_maxRaise, g_clampTopOffset);
                e1->ptr->SetPoint(1, pThis, 6, e1->getVisX(), e1->getVisY(), 1);
                e1->ptr->SetFrameDepth(e1->ptr->m_depthZ - pThis->m_depth, 1);
                continue;
            }
            if (e1->pushCount > 1) e1->targetOffsetX /= static_cast<float>(e1->pushCount);
            bool freed = false;

            for (int j = i + 1; j < n; ++j) {
                Entry* e2 = buf[j];
                // skip fresh plates, UNIT_ADDED callbacks later might disable collisions
                if (e2->hasState(Entry::IEState::SHOULD_STACK) && !e2->hasState(Entry::IEState::IS_FRESH)) {
                    // extra x buffer in case e2 gets pulled later
                    g_entries.seedPair(e1, e2, ms);
                    auto* ps = g_entries.getPair(e1, e2);
                    float dx = e1->getReqDXFor(e2);
                    float minSepX = e1->getAvgWFor(e2, g_bandX * ps->hysteresis);
                    if (dx < minSepX) {
                        const float dvx = e1->velocityX - e2->velocityX;
                        const float dvy = e1->velocityY - e2->velocityY;
                        const float damp = std::exp(-2.0f * std::sqrt(dvx * dvx + dvy * dvy));
                        if (float reqY = e1->getReqYFor(e2, g_bandY * damp); reqY > e2->targetOffsetY) {
                            if (e2->getRankWeight() > e1->getRankWeight()) {
                                reqY = e1->getReqYFor(e1, g_bandY * damp);
                                if (reqY > e1->targetOffsetY) {
                                    e1->targetOffsetY = reqY;
                                    g_entries.commitPair(e1, e2, ms, sceneTime, g_bandY, g_bandX);
                                }
                            }
                            else {
                                if (!freed && !e1->resolvePush(e2, g_bandY * damp, ps->hysteresis)) {
                                    freed = true;
                                    continue;
                                }
                                e2->targetOffsetY = reqY;
                                e2->targetOffsetX += e1->getReqXFor(e2) * std::pow(std::clamp(1.0f - (dx / minSepX), 0.0f, 1.0f), 1.5f) * damp;
                                e2->pushCount++;
                                g_entries.commitPair(e1, e2, ms, sceneTime, g_bandY, g_bandX);
                            }
                        }
                    }
                }
            }
        }

        g_entries.sort(ESortType::ST_OUT);
        for (auto* e : buf) {
            g_entries.resolvePairs(e, ms, sceneTime);
            e->updVis(((e->targetOffsetY - e->stackOffsetY) > 0.0f) ? g_speedRaise : g_speedLower,
                g_speedPull, sceneTime, g_maxRaise, g_clampTopOffset);
            e->setState(Entry::IEState::IS_FRESH, false);
            e->ptr->SetPoint(1, pThis, 6, e->getVisX(), e->getVisY(), 1);
            e->ptr->SetFrameDepth(e->ptr->m_depthZ - pThis->m_depth, 1);
            e->ptr->SetFrameLevel(level, 1);
            level -= 10; // addons buffer
        }
        pThis->m_renderDirtyFlags |= 1; // clearing this prevents further CGWorldFrame__UpdateNamePlatePositions calls until any plate's raw ndc changes

        const int result = CLayoutFrame::ResizePending();
        g_entries.flushAdded(); // all set, fire callbacks
        g_entries.clearPending();
        return result;
    }

    int __fastcall CGUnit_C__UpdateReactionHk(CGUnit_C* unit, void* edx, int updateAll) {
        // original logic
        const int result = unit->UpdateReaction(updateAll);
        if (unit->GetGUID() == ObjectMgr::GetPlayerGuid()) {
            for (const auto& buf = g_entries.get(); const auto& e : buf) EntryManager::applyReaction(e);
        }
        else if (CGNamePlate* plate = unit->m_nameplate) {
            if (Entry* e = g_entries.getEntry(plate->GetPlateId())) EntryManager::applyReaction(e);
        }
        return result;
    }

    void __cdecl CGUnit_C__SetNamePlateFocusHk(C3Vector* pos) {
        // original logic
        uint32_t* activeInput = CGInputControl::GetActive();
        if (activeInput && (activeInput[1] & 0x6000003) == 0) {
            CGNamePlate* prio = nullptr;
            if (g_mouseOverMode & EMouseMode::M_CLICK_THRU_ENEMY) findBestPlate<IEClickLogic::THRU_ENEMY>(pos, prio);
            else if (g_mouseOverMode & EMouseMode::M_CLICK_THRU_FRIEND) findBestPlate<IEClickLogic::THRU_FRIEND>(pos, prio);
            else findBestPlate<IEClickLogic::NONE>(pos, prio);

            if (CGNamePlate* focus = *g_nameplateFocus; prio != focus) {
                if (focus) focus->OnLoseFocus();
                *g_nameplateFocus = prio;
                if (prio) prio->OnGainFocus();
                CGWorldFrame::GetWorldFrame()->m_renderDirtyFlags |= 1;
            }
        }
    }

    void __fastcall CGPlayer_C__NotifyCombatChangeHk(CGPlayer_C* pThis, void* edx, int offs, int val) {
        // just in case everything is perfectly stationary (render flags won't get set)
        g_inCombat = val; CGWorldFrame::GetWorldFrame()->m_renderDirtyFlags |= 1;
        return pThis->NotifyCombatChange(offs, val);
    }

    void __cdecl CGGameUI__TargetHk(guid_t guid) {
        CGGameUI::TargetFn(guid); CGWorldFrame::GetWorldFrame()->m_renderDirtyFlags |= 1;
    }

    void __cdecl CGGameUI__ClearTargetHk(guid_t guid, int flag) {
        CGGameUI::ClearTargetFn(guid, flag); CGWorldFrame::GetWorldFrame()->m_renderDirtyFlags |= 1;
    }

    bool __fastcall CGUnit_C__ShouldShowNamePlate_siteWrapper(const CGUnit_C* unit) {
        return g_clampTop != EClampMode::C_BOSS || unit->GetCreatureRank() == ECreatureRank::RANK_WORLDBOSS || isValidObjFn(unit->m_worldObject);
    }

    bool __fastcall CGWorldFrame__GetScreenCoordinates_siteWrapper(CGWorldFrame* pThis, void* edx, C3Vector* pos, Vec2D<float>* out, int* res) {
        int result; bool r = pThis->GetScreenCoordinates(pos, out, &result);
        return (result == 7 || r); // 7 = unit is outside the top edge of the viewport
    }

	uint8_t __fastcall CSimpleFrame__SetFrameAlpha_siteWrapper(CGUnit_C* unit) {
    	// original logic + traceline + alpha blending
    	if (!unit || !unit->m_nameplate) return 255;
    	uint8_t targetAlpha = (*g_lockedTarget)
			? ((unit->GetEntry<UnitEntry>()->m_guid == *g_lockedTarget) ? 255 : 255 * g_nonTargetAlpha)
			: 255;
    	float current = unit->m_nameplate->m_alpha;
    	if (g_occlusionAlpha < 1.0f) {
    		if (CGCamera* cam = CGCamera::GetActiveCamera()) {
    			C3Vector hitPoint; float dist = 1.0f;
    			C3Vector start = cam->m_pos; C3Vector end;
    			unit->GetPosition(end); end.Z += unit->m_unitHeight * 0.666f;
    			if (CGGameUI::TraceLine(start, end, 0x100111, hitPoint, dist)) {
    				if (unit->m_nameplate->HasPlateState(EFrameState::NP_IS_FRESH)) {
    					unit->m_nameplate->SetPlateState(EFrameState::NP_IS_FRESH, false);
    					return static_cast<uint8_t>(targetAlpha * g_occlusionAlpha);
    				}
    				return static_cast<uint8_t>(current + (targetAlpha * g_occlusionAlpha - current) * g_alphaSpd);
    			}
    		}
    	}
    	return static_cast<uint8_t>(current + (targetAlpha - current) * g_alphaSpd);
    }

    guid_t* __cdecl CGGameUI__WipeActivePlatesHk() {
        g_entries.clearAll();
        return CGGameUI::WipeActivePlatesFn();
    }

    int __cdecl CGGameUI__DestroyPlatePoolHk() {
        g_entries.clearAll();
        return CGGameUI::DestroyPlatePoolFn();
    }

    CGNamePlate* GetNameplateByGuid(guid_t guid) {
        const Entry* e = g_entries.getEntry(guid);
        return (e && e->guid != 0) ? e->ptr : nullptr;
    }

    void __declspec(naked) CGNamePlate__OnUpdate_siteHk() {
        __asm {
            push edi;
            jmp CGNamePlate__OnUpdate_site_jmpback;
        }
    }

    void __declspec(naked) CGUnit_C__ShouldShowNamePlate_siteHk() {
        __asm {
            mov ecx, esi;
            call CGUnit_C__ShouldShowNamePlate_siteWrapper;
            test eax, eax;
            jz pass;
            mov ecx, [ebp + 0Ch];
            mov edx, [ebp + 08h];
            jmp CGUnit_C__ShouldShowNamePlate_site_jmpback;
        pass:
            jmp CGUnit_C__ShouldShowNamePlate_site_pass;
        }
    }

    void __declspec(naked) CGWorldFrame__GetScreenCoordinates_siteHk() {
        __asm {
            call CGWorldFrame__GetScreenCoordinates_siteWrapper;
            mov esp, ebp;
            pop ebp;
            retn;
        }
    }

    void __declspec(naked) CGWorldFrame__UpdateNamePlatePositions_siteHk() {
        __asm {
            jmp CGWorldFrame__UpdateNamePlatePositions_site_jmpback; // skip flag cleanup
        }
    }

    void __declspec(naked) CSimpleFrame__SetFrameAlpha_siteHk() {
        __asm {
            mov ecx, edi;
            call CSimpleFrame__SetFrameAlpha_siteWrapper;
            push eax;
            jmp CSimpleFrame__SetFrameAlpha_site_jmpback;
        }
    }

    int C_NamePlate_GetNamePlates(lua_State* L) {
        Lua::lua_createtable(L, 0, 0);
        int id = 1;
        const auto& buf = g_entries.get();
        for (const auto& e : buf) {
            Lua::lua_pushframe(L, e->ptr);
            Lua::lua_rawseti(L, -2, id++);
        }
        return 1;
    }

    int C_NamePlate_GetNamePlateForUnit(lua_State* L) {
        const char* token = Lua::luaL_checkstring(L, 1);
        if (!token) return 0;
        guid_t guid = ObjectMgr::String2Guid(token);
        if (!guid) return 0;
        CGNamePlate* nameplate = GetNameplateByGuid(guid);
        if (!nameplate) return 0;
        Lua::lua_pushframe(L, nameplate);
        return 1;
    }

    int C_NamePlate_GetNamePlateByGUID(lua_State* L) {
        const char* guidStr = Lua::luaL_checkstring(L, 1);
        if (!guidStr) return 0;
        guid_t guid = strtoull(guidStr, nullptr, 0);
        if (!guid) return 0;
        CGNamePlate* nameplate = GetNameplateByGuid(guid);
        if (!nameplate) return 0;
        Lua::lua_pushframe(L, nameplate);
        return 1;
    }

    int C_NamePlate_GetNamePlateTokenByGUID(lua_State* L) {
        const char* guidStr = Lua::luaL_checkstring(L, 1);
        if (!guidStr) return 0;
        guid_t guid = strtoull(guidStr, nullptr, 0);
        if (!guid) return 0;
        if (const char* token = g_entries.getToken(guid)) {
            Lua::lua_pushstring(L, token);
            return 1;
        }
        return 0;
    }

    int lua_openlibnameplates(lua_State* L) {
        constexpr Lua::luaL_Reg methods[] = {
            {"GetNamePlates", C_NamePlate_GetNamePlates},
            {"GetNamePlateForUnit", C_NamePlate_GetNamePlateForUnit},
            {"GetNamePlateByGUID", C_NamePlate_GetNamePlateByGUID},
            {"GetNamePlateTokenByGUID", C_NamePlate_GetNamePlateTokenByGUID},
        };

        Lua::lua_createtable(L, 0, std::size(methods));
        for (const auto& method : methods) {
            Lua::lua_pushcfunction(L, method.func);
            Lua::lua_setfield(L, -2, method.name);
        }
        Lua::lua_setglobal(L, "C_NamePlate");
        return 0;
    }

    int CVarHandler_NameplateDistance(CVar* cvar, const char*, const char* value, void*) {
        float f;
        const int result = cvar->Sync(value, &f, 41.0f, 100.0f, "%.2f");
        *reinterpret_cast<float*>(0x00ADAA7C) = f * f;
        if (CGWorldFrame* wf = CGWorldFrame::GetWorldFrame()) wf->m_renderDirtyFlags |= 1; return result;
    }
	int CVarHandler_NameplatePlacement(CVar* cvar, const char*, const char* value, void*) {
        float f;
    	const int result = cvar->Sync(value, &f, -1.0f, 2.0f, "%.4f");
    	DWORD oldProtect;
    	void* targetAddr = CGNamePlate::VerticalPlacementOffset;
    	if (VirtualProtect(targetAddr, sizeof(float), PAGE_EXECUTE_READWRITE, &oldProtect)) {
    		*CGNamePlate::VerticalPlacementOffset = f;
    		VirtualProtect(targetAddr, sizeof(float), oldProtect, &oldProtect);
    	}
        if (CGWorldFrame* wf = CGWorldFrame::GetWorldFrame()) wf->m_renderDirtyFlags |= 1; return result;
    }
    int CVarHandler_NameplateBandX(CVar* cvar, const char*, const char* value, void*) {
        const int result = cvar->Sync(value, &g_bandX, 0.1f, 1.0f, "%.2f");
        if (CGWorldFrame* wf = CGWorldFrame::GetWorldFrame()) wf->m_renderDirtyFlags |= 1; return result;
    }
    int CVarHandler_NameplateBandY(CVar* cvar, const char*, const char* value, void*) {
        const int result = cvar->Sync(value, &g_bandY, 0.1f, 1.5f, "%.2f");
        if (CGWorldFrame* wf = CGWorldFrame::GetWorldFrame()) wf->m_renderDirtyFlags |= 1; return result;
    }
    int CVarHandler_NameplateHitboxWidthE(CVar* cvar, const char*, const char* value, void*) {
        const int result = cvar->Sync(value, &g_hitWidthE, 0.0f, 1.0f, "%.2f");
        if (CGWorldFrame* wf = CGWorldFrame::GetWorldFrame()) wf->m_renderDirtyFlags |= 1; return result;
    }
    int CVarHandler_NameplateHitboxHeightE(CVar* cvar, const char*, const char* value, void*) {
        const int result = cvar->Sync(value, &g_hitHeightE, 0.0f, 1.0f, "%.2f");
        if (CGWorldFrame* wf = CGWorldFrame::GetWorldFrame()) wf->m_renderDirtyFlags |= 1; return result;
    }
    int CVarHandler_NameplateHitboxWidthF(CVar* cvar, const char*, const char* value, void*) {
        const int result = cvar->Sync(value, &g_hitWidthF, 0.0f, 1.0f, "%.2f");
        if (CGWorldFrame* wf = CGWorldFrame::GetWorldFrame()) wf->m_renderDirtyFlags |= 1; return result;
    }
    int CVarHandler_NameplateHitboxHeightF(CVar* cvar, const char*, const char* value, void*) {
        const int result = cvar->Sync(value, &g_hitHeightF, 0.0f, 1.0f, "%.2f");
        if (CGWorldFrame* wf = CGWorldFrame::GetWorldFrame()) wf->m_renderDirtyFlags |= 1; return result;
    }
    int CVarHandler_NameplateRaiseSpeed(CVar* cvar, const char*, const char* value, void*) {
        const int result = cvar->Sync(value, &g_speedRaise, 1.0f, 250.0f, "%.2f");
        if (CGWorldFrame* wf = CGWorldFrame::GetWorldFrame()) wf->m_renderDirtyFlags |= 1; return result;
    }
    int CVarHandler_NameplateLowerSpeed(CVar* cvar, const char*, const char* value, void*) {
        const int result = cvar->Sync(value, &g_speedLower, 1.0f, 250.0f, "%.2f");
        if (CGWorldFrame* wf = CGWorldFrame::GetWorldFrame()) wf->m_renderDirtyFlags |= 1; return result;
    }
    int CVarHandler_NameplatePullSpeed(CVar* cvar, const char*, const char* value, void*) {
        const int result = cvar->Sync(value, &g_speedPull, 1.0f, 250.0f, "%.2f");
        if (CGWorldFrame* wf = CGWorldFrame::GetWorldFrame()) wf->m_renderDirtyFlags |= 1; return result;
    }
    int CVarHandler_NameplateRaiseDistance(CVar* cvar, const char*, const char* value, void*) {
        const int result = cvar->Sync(value, &g_maxRaise, 1.0f, 20.0f, "%.2f");
        if (CGWorldFrame* wf = CGWorldFrame::GetWorldFrame()) wf->m_renderDirtyFlags |= 1; return result;
    }
    int CVarHandler_NameplatePullDistance(CVar* cvar, const char*, const char* value, void*) {
        const int result = cvar->Sync(value, &g_maxPull, 0.0f, 0.75f, "%.2f");
        if (CGWorldFrame* wf = CGWorldFrame::GetWorldFrame()) wf->m_renderDirtyFlags |= 1; return result;
    }
    int CVarHandler_NameplateClampTopOffset(CVar* cvar, const char*, const char* value, void*) {
        const int result = cvar->Sync(value, &g_clampTopOffset, 0.0f, NDC_Y * 0.25f, "%.2f");
        if (CGWorldFrame* wf = CGWorldFrame::GetWorldFrame()) wf->m_renderDirtyFlags |= 1; return result;
    }
    int CVarHandler_NameplateOcclusionAlpha(CVar* cvar, const char*, const char* value, void*) {
        const int result = cvar->Sync(value, &g_occlusionAlpha, 0.0f, 1.0f, "%.2f");
        if (CGWorldFrame* wf = CGWorldFrame::GetWorldFrame()) wf->m_renderDirtyFlags |= 1; return result;
    }
    int CVarHandler_NameplateNonTargetAlpha(CVar* cvar, const char*, const char* value, void*) {
        const int result = cvar->Sync(value, &g_nonTargetAlpha, 0.0f, 1.0f, "%.2f");
        if (CGWorldFrame* wf = CGWorldFrame::GetWorldFrame()) wf->m_renderDirtyFlags |= 1; return result;
    }
    int CVarHandler_NameplateAlphaSpeed(CVar* cvar, const char*, const char* value, void*) {
        const int result = cvar->Sync(value, &g_alphaSpd, 0.01f, 1.00f, "%.3f");
        if (CGWorldFrame* wf = CGWorldFrame::GetWorldFrame()) wf->m_renderDirtyFlags |= 1; return result;
    }
    int CVarHandler_NameplateMouseMode(CVar* cvar, const char*, const char* value, void*) {
        int f;
        const int result = cvar->Sync(value, &f, static_cast<int>(EMouseMode::M_DISABLED), static_cast<int>(std::size(g_mouseModeMap)) - 1, "%d");
        g_mouseOverMode = static_cast<EMouseMode>(g_mouseModeMap[f]);
        if (CGWorldFrame* wf = CGWorldFrame::GetWorldFrame()) wf->m_renderDirtyFlags |= 1; return result;
    }
    int CVarHandler_NameplateClampTop(CVar* cvar, const char*, const char* value, void*) {
        const int result = cvar->Sync(value, reinterpret_cast<int*>(&g_clampTop), static_cast<int>(EClampMode::C_DISABLED), static_cast<int>(EClampMode::C_BOSS), "%d");
        DetourTransactionBegin(); DetourUpdateThread(GetCurrentThread());
        if (g_clampTop == EClampMode::C_DISABLED) {
            Hooks::Detach(&CGUnit_C__ShouldShowNamePlate_site, CGUnit_C__ShouldShowNamePlate_siteHk);
            Hooks::Detach(&CGWorldFrame__GetScreenCoordinates_site, CGWorldFrame__GetScreenCoordinates_siteHk);
        }
        else {
            Hooks::Detour(&CGUnit_C__ShouldShowNamePlate_site, CGUnit_C__ShouldShowNamePlate_siteHk);
            Hooks::Detour(&CGWorldFrame__GetScreenCoordinates_site, CGWorldFrame__GetScreenCoordinates_siteHk);
        }
        DetourTransactionCommit();
        for (auto& buf = g_entries.get(); const auto& e : buf) EntryManager::applyClampingState(e);
        if (CGWorldFrame* wf = CGWorldFrame::GetWorldFrame()) wf->m_renderDirtyFlags |= 1; return result;
    }
    int CVarHandler_NameplateStacking(CVar* cvar, const char*, const char* value, void*) {
        const int result = cvar->Sync(value, reinterpret_cast<int*>(&g_stackingMode), static_cast<int>(EStackingMode::S_DISABLED), static_cast<int>(EStackingMode::S_FRIENDLY), "%d");
        auto& buf = g_entries.get();
        for (auto& e : buf) EntryManager::applyStackingState(e);
        if (CGWorldFrame* wf = CGWorldFrame::GetWorldFrame()) wf->m_renderDirtyFlags |= 1;
        return result;
    }
}

guid_t NamePlates::GetTokenGuid(int index) { return g_entries.getTokenGuid(index); }
int NamePlates::GetTokenId(guid_t guid) { return g_entries.getTokenId(guid); }

void NamePlates::initialize() {
    g_entries.reserveAll(MAX_PLATES);
    g_entries.initializeTokens(MAX_PLATES);

    Hooks::FrameXML::registerLuaLib(lua_openlibnameplates);
    Hooks::FrameXML::registerEvent(NAME_PLATE_CREATED);
    Hooks::FrameXML::registerEvent(NAME_PLATE_UNIT_ADDED);
    Hooks::FrameXML::registerEvent(NAME_PLATE_UNIT_REMOVED);
    Hooks::FrameXML::registerCVar(&s_cvar_nameplateDistance, "nameplateDistance", nullptr, "41.0", CVarHandler_NameplateDistance);
    Hooks::FrameXML::registerCVar(&s_cvar_nameplatePlacement, "nameplatePlacement", nullptr, "0.0", CVarHandler_NameplatePlacement);
    Hooks::FrameXML::registerCVar(&s_cvar_nameplateMouseMode, "nameplateMouseMode", nullptr, "0", CVarHandler_NameplateMouseMode);
    Hooks::FrameXML::registerCVar(&s_cvar_nameplateBandX, "nameplateBandX", nullptr, "0.7", CVarHandler_NameplateBandX);
    Hooks::FrameXML::registerCVar(&s_cvar_nameplateBandY, "nameplateBandY", nullptr, "1.0", CVarHandler_NameplateBandY);
    Hooks::FrameXML::registerCVar(&s_cvar_nameplateHitboxWidthE, "nameplateHitboxWidthE", nullptr, "1.0", CVarHandler_NameplateHitboxWidthE);
    Hooks::FrameXML::registerCVar(&s_cvar_nameplateHitboxHeightE, "nameplateHitboxHeightE", nullptr, "1.0", CVarHandler_NameplateHitboxHeightE);
    Hooks::FrameXML::registerCVar(&s_cvar_nameplateHitboxWidthF, "nameplateHitboxWidthF", nullptr, "1.0", CVarHandler_NameplateHitboxWidthF);
    Hooks::FrameXML::registerCVar(&s_cvar_nameplateHitboxHeightF, "nameplateHitboxHeightF", nullptr, "1.0", CVarHandler_NameplateHitboxHeightF);
    Hooks::FrameXML::registerCVar(&s_cvar_nameplateRaiseSpeed, "nameplateRaiseSpeed", nullptr, "100.0", CVarHandler_NameplateRaiseSpeed);
    Hooks::FrameXML::registerCVar(&s_cvar_nameplateLowerSpeed, "nameplateLowerSpeed", nullptr, "100.0", CVarHandler_NameplateLowerSpeed);
    Hooks::FrameXML::registerCVar(&s_cvar_nameplatePullSpeed, "nameplatePullSpeed", nullptr, "50.0", CVarHandler_NameplatePullSpeed);
    Hooks::FrameXML::registerCVar(&s_cvar_nameplateRaiseDistance, "nameplateRaiseDistance", nullptr, "8.0", CVarHandler_NameplateRaiseDistance);
    Hooks::FrameXML::registerCVar(&s_cvar_nameplatePullDistance, "nameplatePullDistance", nullptr, "0.25", CVarHandler_NameplatePullDistance);
    Hooks::FrameXML::registerCVar(&s_cvar_nameplateOcclusionAlpha, "nameplateOcclusionAlpha", nullptr, "1.0", CVarHandler_NameplateOcclusionAlpha);
    Hooks::FrameXML::registerCVar(&s_cvar_nameplateNonTargetAlpha, "nameplateNonTargetAlpha", nullptr, "0.5", CVarHandler_NameplateNonTargetAlpha);
    Hooks::FrameXML::registerCVar(&s_cvar_nameplateAlphaSpeed, "nameplateAlphaSpeed", nullptr, "0.25", CVarHandler_NameplateAlphaSpeed);
    Hooks::FrameXML::registerCVar(&s_cvar_nameplateClampTop, "nameplateClampTop", nullptr, "0", CVarHandler_NameplateClampTop);
    Hooks::FrameXML::registerCVar(&s_cvar_nameplateClampTopOffset, "nameplateClampTopOffset", nullptr, "0.1", CVarHandler_NameplateClampTopOffset);
    Hooks::FrameXML::registerCVar(&s_cvar_nameplateStacking, "nameplateStacking", nullptr, "0", CVarHandler_NameplateStacking);

    Hooks::Detour(&CSimpleFrame__SetFrameAlpha_site, CSimpleFrame__SetFrameAlpha_siteHk);

    Hooks::Detour(&CGNamePlate__OnUpdate_site, CGNamePlate__OnUpdate_siteHk);

    Hooks::Detour(&CGUnit_C::UpdateReactionFn, CGUnit_C__UpdateReactionHk);
    Hooks::Detour(&CGUnit_C::SetNamePlateFocusFn, CGUnit_C__SetNamePlateFocusHk);
    Hooks::Detour(&CGUnit_C__ShouldShowNamePlate_site, CGUnit_C__ShouldShowNamePlate_siteHk);

    Hooks::Detour(&CGPlayer_C::NotifyCombatChangeFn, CGPlayer_C__NotifyCombatChangeHk);

    Hooks::Detour(&CGWorldFrame::UpdateNamePlatePositionsFn, CGWorldFrame__UpdateNamePlatePositionsHk);
    Hooks::Detour(&CGWorldFrame__UpdateNamePlatePositions_site, CGWorldFrame__UpdateNamePlatePositions_siteHk);
    Hooks::Detour(&CGWorldFrame__GetScreenCoordinates_site, CGWorldFrame__GetScreenCoordinates_siteHk);

    Hooks::Detour(&CGGameUI::DestroyPlatePoolFn, CGGameUI__DestroyPlatePoolHk);
    Hooks::Detour(&CGGameUI::WipeActivePlatesFn, CGGameUI__WipeActivePlatesHk);
    Hooks::Detour(&CGGameUI::ClearTargetFn, CGGameUI__ClearTargetHk);
    Hooks::Detour(&CGGameUI::TargetFn, CGGameUI__TargetHk);

    Hooks::FrameScript::registerToken("nameplate", GetTokenGuid, GetTokenId);
}