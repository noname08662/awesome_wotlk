#include "NamePlates.h"
#include "Camera.h"
#include <bit>
#include <bitset>
#include <algorithm>
#include <unordered_dense/include/ankerl/unordered_dense.h>

#undef min
#undef max

namespace {
    constexpr const char* NAME_PLATE_CREATED = "NAME_PLATE_CREATED";
    constexpr const char* NAME_PLATE_UNIT_ADDED = "NAME_PLATE_UNIT_ADDED";
    constexpr const char* NAME_PLATE_UNIT_REMOVED = "NAME_PLATE_UNIT_REMOVED";
    constexpr const float EPS = 1e-4f;

    constexpr const uint16_t MAX_PLATES = 768;
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
    CVar* s_cvar_nameplateMomentum;
    CVar* s_cvar_nameplateRaiseDistance;
    CVar* s_cvar_nameplatePullDistance;
    CVar* s_cvar_nameplateOcclusionAlpha;
    CVar* s_cvar_nameplateNonTargetAlpha;
    CVar* s_cvar_nameplateAlphaSpeed;
    CVar* s_cvar_nameplateClampTop;
    CVar* s_cvar_nameplateClampTopOffset;

    enum EStackingMode : uint32_t {
        S_DISABLED = 0,
        S_ALL = 1,
        S_ENEMY = 2,
        S_FRIENDLY = 3,
    };

    enum EClampMode : uint32_t {
        C_DISABLED = 0,
        C_ALL = 1,
        C_BOSS = 2,
    };

    enum EMouseMode : uint32_t {
        M_DISABLED = 0,
        M_CLICK_THRU_ENEMY = 1 << 0,
        M_CLICK_THRU_FRIEND = 1 << 1,
        M_OVER_ALWAYS = 1 << 2,
        M_OVER_COMBAT = 1 << 3,
    };

    static const uint32_t g_mouseModeMap[] = {
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

    enum ESortMode : uint32_t {
        ST_DISABLED = 0,
        ST_FRIENDLY = 1,
        ST_ENEMY = 2,
    };

    enum ESortType : uint32_t {
        ST_NONE = 0,
        ST_IN = 1 << 0,
        ST_OUT = 1 << 1
    };

    EStackingMode g_stackingMode = EStackingMode::S_DISABLED;
    EMouseMode g_mouseMode = EMouseMode::M_DISABLED;
    EClampMode g_clampTop = EClampMode::C_DISABLED;
    float g_bandX = 0.7f; 
    float g_bandY = 1.0f;
    float g_hitWidthE = 1.0f;
    float g_hitHeightE = 1.0f;
    float g_hitWidthF = 1.0f;
    float g_hitHeightF = 1.0f;
    float g_placement = 0.0f;
    float g_speedRaise = 8.0f;
    float g_speedLower = 8.0f;
    float g_speedPull = 8.0f;
    float g_maxPull = 0.25f;
    float g_maxRaise = 6.0f;
    float g_momMult = 1.25f;
    float g_clampTopOffset = 0.15f;
    float g_nonTargetAlpha = 0.5f;
    float g_occlusionAlpha = 1.0f;
    float g_alphaSpd = 0.25f;

    auto* const g_alloc = reinterpret_cast<CDataAllocator*>(0x00DCEC44);
    auto* const g_namePlateAllowOverlap = reinterpret_cast<int*>(0x00BD09D8);
    auto* const g_lockedTarget = reinterpret_cast<guid_t*>(0x00BD07B0);
    auto* const g_nameplateFocus = reinterpret_cast<CGNamePlate**>(0x00CA1204);

    bool g_inCombat = false;

    struct alignas(128) Entry {
        enum class IEState {
            NONE = 0,
            SHOULD_STACK = 0x1,
            SHOULD_CLAMP = 0x2,
            IS_FRIENDLY = 0x4,
            IS_TRANSIENT = 0x8,
            IS_FRESH = 0x10,
        };

        bool hasState(IEState flag) const { return (static_cast<uint32_t>(state) & static_cast<uint32_t>(flag)) != 0; }
        void setState(IEState flag, bool on) {
            if (on) state = static_cast<IEState>(static_cast<uint32_t>(state) | static_cast<uint32_t>(flag));
            else state = static_cast<IEState>(static_cast<uint32_t>(state) & ~static_cast<uint32_t>(flag));
        }

        CGUnit_C* unit = nullptr;
        CGNamePlate* ptr = nullptr;
        CDataChunk* chunk = nullptr;
        uint16_t block = 0;
        guid_t guid = 0;

        int sortKey = -1;

        float momentumY = 0.0f;
        float momentumX = 0.0f;
        float lastNDCy = 0.0f;
        float lastNDCx = 0.0f;

        float commitTargetX = 0.0f;
        float commitTargetY = 0.0f;
        float targetOffsetX = 0.0f;
        float targetOffsetY = 0.0f;
        float smoothTargetX = 0.0f;
        float smoothTargetY = 0.0f;
        float stackOffsetX = 0.0f;
        float stackOffsetY = 0.0f;

        IEState state = IEState::NONE;

        struct YieldNode {
            Entry* entry = nullptr;
            YieldNode* next = nullptr;
            YieldNode* prev = nullptr;
            bool heap = false;
            bool committed = false;
        };

        int yieldCount = 0;
        YieldNode yieldNodes[64];
        YieldNode* yieldHead = nullptr;
        YieldNode* yieldTail = nullptr;

        uint32_t activeCollisions[MAX_PLATES / 32];

        void freshState() {
            targetOffsetY = 0.0f;
            targetOffsetX = 0.0f;
            yieldHead = nullptr;
            yieldTail = nullptr;
            yieldCount = 0;
        }

        void clearState() {
            freshState();
            unit = nullptr;
            commitTargetX = 0.0f;
            commitTargetY = 0.0f;
            stackOffsetY = 0.0f;
            stackOffsetX = 0.0f;
            smoothTargetX = 0.0f;
            smoothTargetY = 0.0f;
            state = IEState::IS_FRESH;
            sortKey = -1;
            momentumX = 0.0f;
            momentumY = 0.0f;
            lastNDCx = 0.0f;
            lastNDCy = 0.0f;
        }

        void setActiveCollision(int id) { activeCollisions[id >> 5] |= (1u << (id & 31)); }
        void setInactiveCollision(int id) { activeCollisions[id >> 5] &= ~(1u << (id & 31)); }

        YieldNode* getYieldHead() const { return yieldHead; }
        YieldNode* getYieldTail() const { return yieldTail; }

        void appendYield(Entry* e) {
            bool heap = (yieldCount >= 64);
            YieldNode* node = heap ? new YieldNode() : &yieldNodes[yieldCount++];
            node->entry = e; node->next = nullptr; node->prev = yieldTail;
            node->heap = heap; node->committed = false;
            if (!yieldHead) { yieldHead = node; yieldTail = node; }
            else { yieldTail->next = node; yieldTail = node; }
        }

        bool isAtX(float x, bool clamp = false) {
            if (std::abs(stackOffsetX - x) < EPS) { if (clamp) stackOffsetX = x; return true; }
            return false;
        }
        bool isAtY(float y, bool clamp = false) {
            if (std::abs(stackOffsetY - y) < EPS) { if (clamp) stackOffsetY = y; return true; }
            return false;
        }
        bool isAt(float x, float y, bool clamp = false) { return isAtX(x, clamp) && isAtY(y, clamp); }

        float getTopNDC(float perc = 1.0f) { return ptr->m_NDCproj.y + ptr->m_height * 0.5f * perc; }
        float getBotNDC(float perc = 1.0f) { return ptr->m_NDCproj.y - ptr->m_height * 0.5f * perc; }
        float getAvgWFor(Entry* e, float perc = 1.0f) { return (ptr->m_width + e->ptr->m_width) * 0.5f * perc; }
        float getAvgHFor(Entry* e, float perc = 1.0f) { return (ptr->m_height + e->ptr->m_height) * 0.5f * perc; }
        float getReqYFor(Entry* e, float perc = 1.0f) { return ptr->m_NDCproj.y + targetOffsetY + getAvgHFor(e, perc) - e->ptr->m_NDCproj.y; }
        float getReqXFor(Entry* e) const { return (ptr->m_NDCproj.x + targetOffsetX) - (e->ptr->m_NDCproj.x + e->targetOffsetX); }
        float getReqDXFor(Entry* e) const { return std::abs((ptr->m_NDCproj.x + targetOffsetX) - (e->ptr->m_NDCproj.x + e->targetOffsetX)); }

        float resolvePush(Entry* e, float sep, float hyst) {
            float reqY = getReqYFor(e, sep); // min vertical space required
            if (reqY <= 0.0f || reqY < e->targetOffsetY) return 0.0f; // e2 is too high/-er already
            return (e->getTarY() > getBotNDC(hyst) + targetOffsetY) && (e->getTarY() < getTopNDC(hyst) + targetOffsetY) ? reqY : 0.0f;
        }

        float getVisY() const { return ptr->m_NDCproj.y + stackOffsetY; }
        float getVisX() const { return ptr->m_NDCproj.x + stackOffsetX; }

        float getTarY() const { return ptr->m_NDCproj.y + targetOffsetY; }
        float getTarX() const { return ptr->m_NDCproj.x + targetOffsetX; }

        void updVis(float spdY, float spdX, float momMult, float delta, float maxY, float ceilY) {
            float maxPull = ptr->m_width * g_maxPull;
            float maxRaise = (hasState(IEState::SHOULD_CLAMP)
                ? std::min(ptr->m_height * maxY, NDC_Y - ceilY - getTopNDC())
                : ptr->m_height * maxY);

            // momentum = inertia
            // targetOffset = final position, volatile
            // smoothTarget = effective target, smoothed final position
            // stackOffset = current visual target this frame

            float ndcDeltaY = std::abs(ptr->m_NDCproj.y - lastNDCy);
            lastNDCy = ptr->m_NDCproj.y;
            float ndcScaleY = std::clamp(ndcDeltaY / NDC_Y, 0.0f, 1.0f);
            float shiftRateY = momMult * (1.0f + ndcScaleY * 2.5f);

            float gapY = targetOffsetY - commitTargetY;
            float dirY = (gapY > EPS) ? 1.0f : (gapY < -EPS) ? -1.0f : 0.0f;
            momentumY += (dirY - momentumY) * std::clamp(shiftRateY * delta, 0.0f, 1.0f) * momMult;

            float ndcDeltaX = std::abs(ptr->m_NDCproj.x - lastNDCx);
            lastNDCx = ptr->m_NDCproj.x;
            float ndcScaleX = std::clamp(ndcDeltaX / NDC_X, 0.0f, 1.0f);
            float shiftRateX = momMult * (1.0f + ndcScaleX * 2.5f);

            float gapX = targetOffsetX - commitTargetX;
            float dirX = (gapX > EPS) ? 1.0f : (gapX < -EPS) ? -1.0f : 0.0f;
            momentumX += (dirX - momentumX) * std::clamp(shiftRateX * delta, 0.0f, 1.0f) * momMult;

            float absMomY = std::abs(momentumY);
            float absMomX = std::abs(momentumX);
            smoothTargetY += (commitTargetY - smoothTargetY) * (1.0f - std::exp(-spdY * absMomY * delta));
            smoothTargetX += (commitTargetX - smoothTargetX) * (1.0f - std::exp(-spdX * absMomX * delta));

            float a2y = std::pow(std::clamp(spdY * delta, 0.0f, 1.0f), 1.5f) * (absMomY * absMomY * absMomY);
            float a2x = std::pow(std::clamp(spdX * delta, 0.0f, 1.0f), 1.5f) * (absMomX * absMomX * absMomX);
            float dy = smoothTargetY - stackOffsetY;
            float dx = smoothTargetX - stackOffsetX;
            float stOY = stackOffsetY + std::copysign(std::min(std::abs(dy) * a2y, std::abs(dy)), dy);
            float stOX = stackOffsetX + std::copysign(std::min(std::abs(dx) * a2x, std::abs(dx)), dx);
            stackOffsetY = maxRaise < 0.0f ? maxRaise : std::min(stOY, maxRaise);
            stackOffsetX = std::clamp(stOX, -maxPull, maxPull);
        }
    };

    class EntryManager {
    private:
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
                        const auto end = start + static_cast<uintptr_t>(g_alloc->m_blocksPerChunk) * g_alloc->m_blockSize;
                        chunks.push_back({ start, end, cur });
                    }
                    std::sort(chunks.begin(), chunks.end(),
                        [](const ChunkMeta& a, const ChunkMeta& b) { return a.start < b.start; });
                    chunksHeadCached = h;
                }
            }
            const ChunkMeta* findChunk(uintptr_t addr) {
                if (g_alloc->m_chunkList) {
                    updateHead(reinterpret_cast<uintptr_t>(g_alloc->m_chunkList));
                    if (chunks.empty()) return nullptr;
                    auto it = std::upper_bound(chunks.begin(), chunks.end(), addr,
                        [](uintptr_t val, const ChunkMeta& meta) { return val < meta.start; });
                    if (it != chunks.begin()) {
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
                float hysteresis = 1.0f;

                bool isStale(uint64_t ms) const { return timestamp < ms; }
                void commit(uint64_t ms, float hyst) {
                    timestamp = ms;
                    hysteresis = hyst;
                }
                void reset() {
                    hysteresis = 1.0f;
                    timestamp = 0;
                }
            };
            // inline pool for the speed
            size_t size = 0;
            std::unique_ptr<PairState[]> pairs;
            void init(size_t r) { pairs = std::make_unique<PairState[]>(r * r); size = r; }
            void wipe() { if (pairs) std::fill(pairs.get(), pairs.get() + (size * size), PairState{}); }
            PairState* get(int id1, int id2) { return &pairs[std::min(id1, id2) * size + std::max(id1, id2)]; }
        } pairsMgr;

        std::vector<Entry>  byId;
        std::vector<Entry*> entries;
        std::vector<const char*> tokenCache;

        uint64_t pending[MAX_PLATES / 64] = {};
        ankerl::unordered_dense::map<guid_t, int> guidToId;

        enum class IESortMode { DEFAULT, TARGET, FOCUS, TARGET_FOCUS };
        template <IESortMode mode, bool out>
        void sort(guid_t targetGuid = 0, CGNamePlate* focus = nullptr) {
            std::stable_sort(entries.begin(), entries.end(), [targetGuid, focus](Entry* a, Entry* b) {
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
                // sortkeys frozen while both are lerping, new entries get to resolve their pos unconditionally
                int keyA = a->sortKey; int keyB = b->sortKey;
                return (keyA > 0 && keyB > 0) ? (keyA < keyB) : (a->ptr->m_depthZ < b->ptr->m_depthZ);
                });
        }

        void addPlateMethods(CGNamePlate* plate) {
            lua_State* L = Lua::GetLuaState();
            Lua::lua_pushstring(L, NAME_PLATE_CREATED);
            Lua::lua_pushframe(L, reinterpret_cast<CSimpleFrame*>(plate));
            Lua::lua_pushlightuserdata(L, this);
            Lua::lua_pushcclosure(L, [](lua_State* L) -> int {
                auto* self = reinterpret_cast<EntryManager*>(Lua::lua_touserdata(L, -10003));
                if (!self) return 0;
                Lua::lua_rawgeti(L, 1, 0);
                // toframe doesn't seem to work/I'm doing it wrong
                void* frame = Lua::lua_touserdata(L, -1);
                Lua::lua_settop(L, -2);
                if (frame) {
                    CGNamePlate* plate = reinterpret_cast<CGNamePlate*>(frame);
                    const int index = plate->GetPlateId();
                    if (index >= 0 && index < static_cast<int>(self->byId.size()) && self->byId[index].ptr == plate) {
                        auto& e = self->byId[index];
                        e.setState(Entry::IEState::IS_TRANSIENT, !Lua::lua_toboolean(L, 2));
                        self->applyStackingState(&e);
                    }
                }
                return 0;
                }, 1);
            Lua::lua_setfield(L, -2, "SetStackingEnabled");
            Lua::lua_pushlightuserdata(L, this);
            Lua::lua_pushcclosure(L, [](lua_State* L) -> int {
                auto* self = reinterpret_cast<EntryManager*>(Lua::lua_touserdata(L, -10003));
                if (!self) return 0;
                Lua::lua_rawgeti(L, 1, 0);
                void* frame = Lua::lua_touserdata(L, -1);
                Lua::lua_settop(L, -2);
                if (frame) {
                    CGNamePlate* plate = reinterpret_cast<CGNamePlate*>(frame);
                    const int index = plate->GetPlateId();
                    if (index >= 0 && index < static_cast<int>(self->byId.size()) && self->byId[index].ptr == plate) {
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
        size_t getTotalSize() { return byId.size(); }

        int allPlaced = 0;

        float getHystPair(Entry* e1, Entry* e2) {
            const int id1 = e1->ptr->GetPlateId(); const int id2 = e2->ptr->GetPlateId();
            return pairsMgr.get(id1, id2)->hysteresis;
        }

        void commitPair(Entry* e1, Entry* e2, uint64_t ms, float delta, float by, float bx) {
            // set bits, update hysteresis
            const int id1 = e1->ptr->GetPlateId(); const int id2 = e2->ptr->GetPlateId();
            auto* ps = pairsMgr.get(id1, id2);
            float weight = 1.5f + std::clamp((std::abs(e1->ptr->m_depthZ - e2->ptr->m_depthZ) - 1.0f) / 99.0f, 0.0f, 1.0f);
            if ((e1->getTopNDC() + e1->targetOffsetY + e1->getAvgHFor(e2, by)) > (e2->getBotNDC() + e2->targetOffsetY)
                && e1->getReqDXFor(e2) < e1->getAvgWFor(e2, bx)) {
                ps->commit(ms, weight);
            }
            else {
                float decayed = ps->hysteresis + (1.0f - ps->hysteresis) * (1.0f - std::exp(-2.0f * delta));
                if (std::abs(decayed - weight) < EPS) ps->commit(ms, weight);
                else ps->commit(ms, decayed);
            }
            e1->setActiveCollision(id2); e2->setActiveCollision(id1);
        }

        void resolvePairs(Entry* e, uint64_t ms, float delta) {
            // cleanup
            const int id1 = e->ptr->GetPlateId();
            const int n = ((static_cast<int>(getTotalSize()) + 31) >> 5);
            for (int w = 0; w < n; ++w) {
                uint32_t mask = e->activeCollisions[w];
                while (mask) {
                    int id2 = (w << 5) | (std::countr_zero(mask));
                    auto* ps = pairsMgr.get(id1, id2);
                    if (ps->isStale(ms)) {
                        e->setInactiveCollision(id2);
                        ps->reset();
                    }
                    mask &= mask - 1;
                }
            }
        }

        void sort(ESortType type) {
            if (type & ESortType::ST_OUT) {
                guid_t targetGuid = ObjectMgr::GetTargetGuid();
                if (targetGuid) {
                    CGNamePlate* focus = *g_nameplateFocus;
                    if (focus && (g_mouseMode & EMouseMode::M_OVER_ALWAYS || (g_mouseMode & EMouseMode::M_OVER_COMBAT && g_inCombat))) {
                        sort<IESortMode::TARGET_FOCUS, true>(targetGuid, focus);
                    }
                    else {
                        sort<IESortMode::TARGET, true>(targetGuid);
                    }
                    return;
                }
                else if (CGNamePlate* focus = *g_nameplateFocus) {
                    if (g_mouseMode & EMouseMode::M_OVER_ALWAYS || (g_mouseMode & EMouseMode::M_OVER_COMBAT && g_inCombat)) {
                        sort<IESortMode::FOCUS, true>(targetGuid, focus);
                        return;
                    }
                }
                sort<IESortMode::DEFAULT, true>();
                return;
            }
            sort<IESortMode::DEFAULT, false>();
        }

        void applyReaction(Entry* e) {
            e->setState(Entry::IEState::IS_FRIENDLY, e->unit->IsFriendly());
            applyStackingState(e);
        }

        void applyStackingState(Entry* e) {
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

        void applyClampingState(Entry* e) {
            e->setState(Entry::IEState::SHOULD_CLAMP, g_clampTop == EClampMode::C_ALL
                || (g_clampTop == EClampMode::C_BOSS && e->unit->GetCreatureRank() == ECreatureRank::RANK_WORLDBOSS));
        }

        void flushAdded() {
            lua_State* L = Lua::GetLuaState();
            const int n = std::size(pending);
            for (int w = 0; w < n; ++w) {
                uint64_t word = pending[w];
                while (word) {
                    const int bit = std::countr_zero(word);
                    const int index = w * 64 + bit;
                    Entry* e = &byId[index];
                    if (e->unit->m_nameplate) {
                        // swapped
                        auto it = guidToId.find(e->unit->m_nameplate->m_ownerGuid);
                        if (it != guidToId.end() && it->second != index) {
                            int index_o = it->second;
                            if ((pending[index_o / 64] & (1ULL << (index_o % 64))) == 0) {
                                byId[index_o].guid = 0;
                                Entry* ptr_o = &byId[index_o];
                                entries.erase(std::remove(entries.begin(), entries.end(), ptr_o), entries.end());
                                guidToId.erase(it);
                            }
                        }
                        e->guid = e->unit->m_nameplate->m_ownerGuid;
                        guidToId[e->guid] = index;

                        Lua::lua_pushframe(L, reinterpret_cast<CSimpleFrame*>(e->ptr));
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
            const int n = std::size(pending);
            for (int w = 0; w < n; ++w) {
                uint64_t word = pending[w];
                while (word) {
                    const int index = w * 64 + std::countr_zero(word);
                    Entry* e = &byId[index];
                    if (e->guid != 0 && (!e->unit->m_nameplate || e->unit->m_nameplate->m_ownerGuid != e->guid)) {
                        // no cleanup just yet, ensures addons are able to read complete state
                        FrameScript::FireEvent(NAME_PLATE_UNIT_REMOVED, "%s", tokenCache[index]);
                    }
                    word &= word - 1;
                }
            }
            for (int w = 0; w < n; ++w) {
                uint64_t word = pending[w];
                while (word) {
                    const int bit = std::countr_zero(word);
                    const int index = w * 64 + bit;
                    Entry* e = &byId[index];
                    if (e->guid != 0 && (!e->unit->m_nameplate || e->unit->m_nameplate->m_ownerGuid != e->guid)) {
                        auto it = guidToId.find(e->guid);
                        if (it != guidToId.end() && it->second == index) {
                            guidToId.erase(it);
                        }
                        if (!e->unit->m_nameplate) {
                            // gone
                            e->guid = 0;
                            pending[w] &= ~(1ULL << bit);
                        }
                        else {
                            // swapped
                            e->guid = e->unit->m_nameplate->m_ownerGuid;
                            guidToId[e->guid] = index;
                        }
                    }
                    word &= word - 1;
                }
            }
            entries.erase(std::remove_if(entries.begin(), entries.end(),
                [this](Entry* e) {
                    const int index = e->ptr->GetPlateId();
                    return e->guid == 0 && ((pending[index / 64] & (1ULL << (index % 64))) == 0);
                }), entries.end());
        }

        void appendAdded(Entry* e) {
            const int index = e->ptr->GetPlateId();
            if (index < 0 || index >= static_cast<int>(byId.size())) return;
            pending[index / 64] |= (1ULL << (index % 64));
            auto it = std::find_if(entries.begin(), entries.end(), [e](const auto& entry) {
                return entry == e;
                });
            if (it == entries.end()) entries.push_back(e);
        }

        void appendAdded(int index) {
            if (index < 0 || index >= static_cast<int>(byId.size())) return;
            pending[index / 64] |= (1ULL << (index % 64));
            auto* e = &byId[index];
            auto it = std::find_if(entries.begin(), entries.end(), [e](const auto& entry) {
                return entry == e;
                });
            if (it == entries.end()) entries.push_back(e);
        }

        void appendRemoved(Entry* e) {
            const int index = e->ptr->GetPlateId();
            if (index < 0 || index >= static_cast<int>(byId.size())) return;
            pending[index / 64] |= (1ULL << (index % 64));
        }

        void appendRemoved(int index) {
            if (index < 0 || index >= static_cast<int>(byId.size())) return;
            pending[index / 64] |= (1ULL << (index % 64));
        }

        void clearPending() {
            std::memset(pending, 0, sizeof(pending));
        }

        guid_t getTokenGuid(int index) {
            if (index < 0 || index >= static_cast<int>(byId.size())) return 0;
            return byId[index].guid;
        }

        int getTokenId(guid_t guid) {
            if (!guid) return -1;
            const auto it = guidToId.find(guid);
            return (it != guidToId.end()) ? it->second : -1;
        }

        Entry* getEntry(guid_t guid) {
            const int index = getTokenId(guid);
            if (index < 0 || index >= static_cast<int>(byId.size())) return nullptr;
            auto& e = byId[index];
            return (e.guid != 0) ? &e : nullptr;
        }

        Entry* getEntry(int index) {
            if (index < 0 || index >= static_cast<int>(byId.size())) return nullptr;
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
                e.block = static_cast<uint32_t>((addr - meta->start) / g_alloc->m_blockSize);
                e.guid = 0;

                byId.push_back(e);
                plate->SetPlateId(static_cast<int>(byId.size()) - 1);

                addPlateMethods(plate);
                return &byId.back();
            }
            return (index < 0 || index >= static_cast<int>(byId.size())) ? nullptr : &byId[index];
        }

        const char* getToken(guid_t guid) {
            const auto it = guidToId.find(guid);
            if (it == guidToId.end()) return "none";
            const int index = it->second;
            if (index >= 0 && index < static_cast<int>(byId.size())) {
                return tokenCache[index];
            }
            return "none"; // this one is a valid unitId
        }

        const char* getToken(int index) {
            if (index >= 0 && index < static_cast<int>(byId.size())) {
                return tokenCache[index];
            }
            return "none"; // this one is a valid unitId
        }

        void initializeTokens(size_t r) {
            for (int i = 1; i <= r; ++i) {
                char buf[16];
                snprintf(buf, sizeof(buf), "nameplate%d", i);
                tokenCache.push_back(strdup(buf));
            }
        }

        void reserveAll(size_t r) {
            byId.reserve(r);
            chunkMgr.reserve(r);
            pairsMgr.init(r);
            guidToId.reserve(r);
            entries.reserve(r);
        }

        void clearAll() {
            chunkMgr.clear();
            pairsMgr.wipe();
            byId.clear();
            entries.clear();
            guidToId.clear();
            std::memset(pending, 0, sizeof(pending));
        }
    };
    EntryManager g_entries;

    void(*CGNamePlate__OnUpdate_site)() = reinterpret_cast<DummyCallback_t>(0x0098E9F9);
    constexpr uintptr_t CGNamePlate__OnUpdate_site_jmpback = 0x0098EA27;

    void(*CGWorldFrame__UpdateNamePlatePositions_site)() = reinterpret_cast<DummyCallback_t>(0x004F90E2);
    constexpr uintptr_t CGWorldFrame__UpdateNamePlatePositions_site_jmpback = 0x004F90EC;

    void(*CSimpleFrame__SetFrameAlpha_site)() = reinterpret_cast<DummyCallback_t>(0x0098EAA7);
    constexpr uintptr_t CSimpleFrame__SetFrameAlpha_site_jmpback = 0x0098EAD2;

    void(*CGNamePlate__Initialize_site)() = reinterpret_cast<DummyCallback_t>(0x00725766);
    void(*CSimpleFrame__Hide_site)() = reinterpret_cast<DummyCallback_t>(0x0072587D);

    void(*CGUnit_C__SetNamePlateFocus_site)() = reinterpret_cast<DummyCallback_t>(0x007271E8);
    constexpr uintptr_t CGUnit_C__SetNamePlateFocus_site_jmpback = 0x0072728A;

    void(*CGUnit_C__ShouldShowNamePlate_site)() = reinterpret_cast<DummyCallback_t>(0x0072B2BD);
    constexpr uintptr_t CGUnit_C__ShouldShowNamePlate_site_jmpback = 0x0072B2C7;
    constexpr uintptr_t CGUnit_C__ShouldShowNamePlate_site_pass = 0x0072B247;

    void(*CGWorldFrame__GetScreenCoordinates_site)() = reinterpret_cast<DummyCallback_t>(0x0071574A);

    const auto isValidObjFn = reinterpret_cast<int(*)(void*)>(0x0077F0B0);

    enum class IEClickLogic { NONE, THRU_ENEMY, THRU_FRIEND };
    template <IEClickLogic mode>
    void findBestPlate(C3Vector* pos, CGNamePlate*& prio) {
        auto& buf = g_entries.get();
        for (const auto& e : buf) {
            if ((e->ptr->m_flags & 0x100) == 0 || *g_lockedTarget == e->ptr->m_ownerGuid) continue;

            // original logic, clamped search boundaries
            const bool isFriendly = e->hasState(Entry::IEState::IS_FRIENDLY);
            const Vec2D<float> hitBox = isFriendly ? Vec2D<float>{ g_hitWidthF, g_hitHeightF } : Vec2D<float>{ g_hitWidthE, g_hitHeightE };

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
}

namespace NamePlates {
    static void __fastcall CGNamePlate__Initialize_siteWrapper(CGUnit_C* parent) {
        if (CGNamePlate* plate = parent->m_nameplate) {
            if (Entry* e = g_entries.initEntry(plate)) {
                e->clearState();
                e->setState(Entry::IEState::IS_FRIENDLY, parent->IsFriendly());
                e->unit = parent;

                g_entries.resolvePairs(e, -1, 0);
                g_entries.applyStackingState(e);
                g_entries.applyClampingState(e);
                g_entries.appendAdded(e);

                // for occlusion
                plate->SetPlateState(EFrameState::NP_IS_FRESH, true);
            }
        }
    }

    static void __fastcall CSimpleFrame__Hide_siteWrapper(CGNamePlate* plate) {
        g_entries.appendRemoved(plate->GetPlateId());
    }

    static int __cdecl CGWorldFrame__UpdateNamePlatePositionsHk(CGWorldFrame* pThis) {
        g_entries.flushRemoved(); // bulk flush now to ensure callbacks recieve a complete gapless snapshot of the previous frame
        auto& buf = g_entries.get();
        if (buf.empty()) {
            g_entries.clearPending();
            return CLayoutFrame::ResizePending();
        }

        const int n = static_cast<int>(buf.size());
        uint32_t level = static_cast<uint32_t>(n) * 10;

        g_entries.sort(ESortType::ST_IN); // no target/focus
        for (auto& e : buf) e->freshState();

        const float sceneTime = std::min(0.02f, pThis->m_sceneTime);
        const uint64_t ms = CGGameUI::OsGetAsyncTimeMsFn();

        for (int i = 0; i < n; ++i) {
            Entry* e1 = buf[i];
            if (!e1->hasState(Entry::IEState::SHOULD_STACK) || e1->hasState(Entry::IEState::IS_FRESH)) {
                e1->updVis(g_speedLower, g_speedPull, g_momMult, sceneTime, g_maxRaise, g_clampTopOffset);
                if (!e1->isAt(e1->targetOffsetX, e1->targetOffsetY, true)) g_entries.allPlaced = 0;
                e1->ptr->SetPoint(1, static_cast<CLayoutFrame*>(pThis), 6, e1->getVisX(), e1->getVisY() + (e1->ptr->m_height * g_placement), 1);
                e1->ptr->SetFrameDepth(e1->ptr->m_depthZ - pThis->m_depth, 1);
                continue;
            }

            // improvised callback handling z-losers and ambiguity
            int status = 1;
            while (status & 1) {
                status &= ~1;
                Entry::YieldNode* node = e1->getYieldHead();
                while (node) {
                    Entry* e2 = node->entry;
                    const int id2 = e2->ptr->GetPlateId();
                    if (!node->committed) {
                        float hyst = g_entries.getHystPair(e1, e2);
                        if (float reqY = e2->resolvePush(e1, g_bandY, hyst); reqY > 0.0f) {
                            float dx = e2->getReqDXFor(e1);
                            float minSepX = e1->getAvgWFor(e2, g_bandX * hyst);
                            if (dx < minSepX) { // no buffer here
                                float w = std::pow(std::clamp(1.0f - (dx / minSepX), 0.0f, 1.0f), 1.5f);
                                e1->targetOffsetY = reqY;
                                e1->targetOffsetX = e2->getReqXFor(e1) * w;
                                g_entries.commitPair(e2, e1, ms, sceneTime, g_bandY, g_bandX);
                                node->committed = true;
                                status |= 1;
                            }
                        }
                    }
                    if (node->heap) status |= 2;
                    node = node->next;
                }
            }
            if (status & 2) {
                Entry::YieldNode* node = e1->getYieldTail();
                while (node && node->heap) {
                    Entry::YieldNode* prev = node->prev;
                    if (node->heap) delete node;
                    node = prev;
                }
            }
            for (int j = i + 1; j < n; ++j) {
                Entry* e2 = buf[j];
                // skip fresh plates, UNIT_ADDED callbacks later might disable collisions
                if (e2->hasState(Entry::IEState::SHOULD_STACK) && !e2->hasState(Entry::IEState::IS_FRESH)) {
                    float minSepX = e1->getAvgWFor(e2, g_bandX * g_entries.getHystPair(e1, e2));
                    // extra x buffer in case e2 gets pulled later
                    if (e1->getReqDXFor(e2) < minSepX + e2->ptr->m_width * g_maxPull) e2->appendYield(e1);
                }
            }
        }

        g_entries.sort(ESortType::ST_OUT);
        for (auto* e : buf) {
            g_entries.resolvePairs(e, ms, sceneTime);
            e->updVis(((e->targetOffsetY - e->stackOffsetY) > 0.0f) ? g_speedRaise : g_speedLower,
                g_speedPull, g_momMult, sceneTime, g_maxRaise, g_clampTopOffset);
            // cool down after having e arrived
            if (!e->isAt(e->targetOffsetX, e->targetOffsetY, true)) {
                g_entries.allPlaced = 0; e->sortKey = 1;
                e->commitTargetY += (e->targetOffsetY - e->commitTargetY) * std::clamp(4.0f * std::abs(e->momentumY) * sceneTime, 0.0f, 1.0f);
                e->commitTargetX += (e->targetOffsetX - e->commitTargetX) * std::clamp(4.0f * std::abs(e->momentumX) * sceneTime, 0.0f, 1.0f);
            }
            else {
                e->commitTargetY = e->targetOffsetY;
                e->commitTargetX = e->targetOffsetX;
                e->momentumY *= 0.1f;
                e->momentumX *= 0.1f;
                e->sortKey = -1;
            }
            e->setState(Entry::IEState::IS_FRESH, false);
            e->ptr->SetPoint(1, static_cast<CLayoutFrame*>(pThis), 6, e->getVisX(), e->getVisY() + (e->ptr->m_height * g_placement), 1);
            e->ptr->SetFrameDepth(e->ptr->m_depthZ - pThis->m_depth, 1);
            e->ptr->SetFrameLevel(level, 1);
            level -= 10; // addons buffer
        }
        // keep the update running
        if (g_entries.allPlaced++ > 1) {
            pThis->m_renderDirtyFlags &= ~1;
            g_entries.allPlaced = 0;
        }
        else {
            pThis->m_renderDirtyFlags |= 1;
        }

        const int result = CLayoutFrame::ResizePending();
        g_entries.flushAdded(); // all set, fire callbacks
        g_entries.clearPending();
        return result;
    }

    static int __fastcall CGUnit_C__UpdateReactionHk(CGUnit_C* unit, void* edx, int updateAll) {
        // original logic
        const int result = unit->UpdateReaction(updateAll);
        if (unit->GetGUID() == ObjectMgr::GetPlayerGuid()) {
            auto& buf = g_entries.get();
            for (auto& e : buf) g_entries.applyReaction(e);
        }
        else if (CGNamePlate* plate = unit->m_nameplate) {
            if (Entry* e = g_entries.getEntry(plate->GetPlateId())) g_entries.applyReaction(e);
        }
        return result;
    }

    static void __cdecl CGUnit_C__SetNamePlateFocusHk(C3Vector* pos) {
        // original logic
        uint32_t* activeInput = CGInputControl::GetActive();
        if (activeInput && (activeInput[1] & 0x6000003) == 0) {
            CGNamePlate* prio = nullptr;
            if (g_mouseMode & EMouseMode::M_CLICK_THRU_ENEMY) findBestPlate<IEClickLogic::THRU_ENEMY>(pos, prio);
            else if (g_mouseMode & EMouseMode::M_CLICK_THRU_FRIEND) findBestPlate<IEClickLogic::THRU_FRIEND>(pos, prio);
            else findBestPlate<IEClickLogic::NONE>(pos, prio);

            CGNamePlate* focus = *g_nameplateFocus;
            if (prio != focus) {
                if (focus) focus->OnLoseFocus();
                *g_nameplateFocus = prio;
                if (prio) prio->OnGainFocus();
                CGWorldFrame::GetWorldFrame()->m_renderDirtyFlags |= 1;
            }
        }
    }

    static void __fastcall CGPlayer_C__NotifyCombatChangeHk(CGPlayer_C* pThis, void* edx, int offs, int val) {
        // just in case everything is perfectly stationary (render flags won't get set)
        g_inCombat = val; CGWorldFrame::GetWorldFrame()->m_renderDirtyFlags |= 1;
        return pThis->NotifyCombatChange(offs, val);
    }

    static void __cdecl CGGameUI__TargetHk(guid_t guid) {
        CGGameUI::TargetFn(guid); CGWorldFrame::GetWorldFrame()->m_renderDirtyFlags |= 1;
    }

    static void __cdecl CGGameUI__ClearTargetHk(guid_t guid, int flag) {
        CGGameUI::ClearTargetFn(guid, flag); CGWorldFrame::GetWorldFrame()->m_renderDirtyFlags |= 1;
    }

    static bool __fastcall CGUnit_C__ShouldShowNamePlate_siteWrapper(CGUnit_C* unit) {
        return g_clampTop != EClampMode::C_BOSS || unit->GetCreatureRank() == ECreatureRank::RANK_WORLDBOSS || isValidObjFn(unit->m_worldObject);
    }

    static bool __fastcall CGWorldFrame__GetScreenCoordinates_siteWrapper(CGWorldFrame* pThis, void* edx, C3Vector* pos, Vec2D<float>* out, int* res) {
        int result; bool r = pThis->GetScreenCoordinates(pos, out, &result);
        return (result == 7 || r); // 7 = unit is outside of the top edge of the viewport
    }

    static uint8_t __fastcall CSimpleFrame__SetFrameAlpha_siteWrapper(CGUnit_C* unit) {
        // original logic + traceline + alpha blending
        if (!unit || !unit->m_nameplate) return 255;
        uint8_t targetAlpha = (*g_lockedTarget)
            ? ((unit->GetEntry()->m_guid == *g_lockedTarget) ? 255 : 255 * g_nonTargetAlpha)
            : 255;
        float current = static_cast<float>(unit->m_nameplate->m_alpha);
        if (g_occlusionAlpha < 1.0f) {
            if (CGCamera* cam = CGCamera::GetActiveCamera()) {
                C3Vector hitPoint; float dist = 1.0f;
                C3Vector start = cam->m_pos; C3Vector end;
                unit->GetPosition(end); end.Z += unit->m_unitHeight / 2.0f;
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

    static guid_t* __cdecl CGGameUI__WipeActivePlatesHk() {
        g_entries.clearAll();
        return CGGameUI::WipeActivePlatesFn();
    }

    static int __cdecl CGGameUI__DestroyPlatePoolHk() {
        g_entries.clearAll();
        return CGGameUI::DestroyPlatePoolFn();
    }

    guid_t GetTokenGuid(int index) {
        return g_entries.getTokenGuid(index);
    }

    int GetTokenId(guid_t guid) {
        return g_entries.getTokenId(guid);
    }

    static CGNamePlate* GetNameplateByGuid(guid_t guid) {
        Entry* e = g_entries.getEntry(guid);
        return (e && e->guid != 0) ? e->ptr : nullptr;
    }

    static void __declspec(naked) CGNamePlate__OnUpdate_siteHk() {
        __asm {
            push edi;
            jmp CGNamePlate__OnUpdate_site_jmpback;
        }
    }

    static void __declspec(naked) CSimpleFrame__Hide_siteHk() {
        __asm {
            pushad;
            mov ecx, [esi + 0C38h];
            call CSimpleFrame__Hide_siteWrapper;
            popad;
            mov dword ptr[esi + 0C38h], 0;
            mov ecx, esi;
            pop esi;
            jmp edx;
        }
    }

    static void __declspec(naked) CGNamePlate__Initialize_siteHk() {
        __asm {
            mov ecx, esi;
            call CGNamePlate__Initialize_siteWrapper;
            pop edi;
            pop esi;
            mov esp, ebp;
            pop ebp;
            retn 8;
        }
    }

    static void __declspec(naked) CGUnit_C__ShouldShowNamePlate_siteHk() {
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

    static void __declspec(naked) CGWorldFrame__GetScreenCoordinates_siteHk() {
        __asm {
            call CGWorldFrame__GetScreenCoordinates_siteWrapper;
            mov esp, ebp;
            pop ebp;
            retn;
        }
    }

    static void __declspec(naked) CGWorldFrame__UpdateNamePlatePositions_siteHk() {
        __asm {
            jmp CGWorldFrame__UpdateNamePlatePositions_site_jmpback; // skip flag cleanup
        }
    }

    static void __declspec(naked) CSimpleFrame__SetFrameAlpha_siteHk() {
        __asm {
            mov ecx, edi;
            call CSimpleFrame__SetFrameAlpha_siteWrapper;
            push eax;
            jmp CSimpleFrame__SetFrameAlpha_site_jmpback;
        }
    }
}

namespace NamePlates {
    static int C_NamePlate_GetNamePlates(lua_State* L) {
        Lua::lua_createtable(L, 0, 0);
        int id = 1;
        const auto& buf = g_entries.get();
        for (const auto& e : buf) {
            Lua::lua_pushframe(L, reinterpret_cast<CSimpleFrame*>(e->ptr));
            Lua::lua_rawseti(L, -2, id++);
        }
        return 1;
    }

    static int C_NamePlate_GetNamePlateForUnit(lua_State* L) {
        const char* token = Lua::luaL_checkstring(L, 1);
        if (!token) return 0;
        guid_t guid = ObjectMgr::String2Guid(token);
        if (!guid) return 0;
        CGNamePlate* nameplate = GetNameplateByGuid(guid);
        if (!nameplate) return 0;
        Lua::lua_pushframe(L, reinterpret_cast<CSimpleFrame*>(nameplate));
        return 1;
    }

    static int C_NamePlate_GetNamePlateByGUID(lua_State* L) {
        const char* guidStr = Lua::luaL_checkstring(L, 1);
        if (!guidStr) return 0;
        guid_t guid = static_cast<guid_t>(strtoull(guidStr, nullptr, 0));
        if (!guid) return 0;
        CGNamePlate* nameplate = GetNameplateByGuid(guid);
        if (!nameplate) return 0;
        Lua::lua_pushframe(L, reinterpret_cast<CSimpleFrame*>(nameplate));
        return 1;
    }

    static int C_NamePlate_GetNamePlateTokenByGUID(lua_State* L) {
        const char* guidStr = Lua::luaL_checkstring(L, 1);
        if (!guidStr) return 0;
        guid_t guid = static_cast<guid_t>(strtoull(guidStr, nullptr, 0));
        if (!guid) return 0;
        if (const char* token = g_entries.getToken(guid)) {
            Lua::lua_pushstring(L, token);
            return 1;
        }
        return 0;
    }

    static int lua_openlibnameplates(lua_State* L) {
        Lua::luaL_Reg methods[] = {
            {"GetNamePlates", C_NamePlate_GetNamePlates},
            {"GetNamePlateForUnit", C_NamePlate_GetNamePlateForUnit},
            {"GetNamePlateByGUID", C_NamePlate_GetNamePlateByGUID},
            {"GetNamePlateTokenByGUID", C_NamePlate_GetNamePlateTokenByGUID},
        };

        Lua::lua_createtable(L, 0, std::size(methods));
        for (size_t i = 0; i < std::size(methods); i++) {
            Lua::lua_pushcfunction(L, methods[i].func);
            Lua::lua_setfield(L, -2, methods[i].name);
        }
        Lua::lua_setglobal(L, "C_NamePlate");
        return 0;
    }

    static int CVarHandler_NameplateDistance(CVar* cvar, const char*, const char* value, void*) {
        float f;
        const int result = cvar->Sync(value, &f, 41.0f, 100.0f, "%.2f");
        *reinterpret_cast<float*>(0x00ADAA7C) = f * f;
        if (CGWorldFrame* wf = CGWorldFrame::GetWorldFrame()) wf->m_renderDirtyFlags |= 1; return result;
    }
    static int CVarHandler_NameplatePlacement(CVar* cvar, const char*, const char* value, void*) {
        const int result = cvar->Sync(value, &g_placement, -1.0f, 2.0f, "%.4f");
        if (CGWorldFrame* wf = CGWorldFrame::GetWorldFrame()) wf->m_renderDirtyFlags |= 1; return result;
    }
    static int CVarHandler_NameplateBandX(CVar* cvar, const char*, const char* value, void*) {
        const int result = cvar->Sync(value, &g_bandX, 0.1f, 1.0f, "%.2f");
        if (CGWorldFrame* wf = CGWorldFrame::GetWorldFrame()) wf->m_renderDirtyFlags |= 1; return result;
    }
    static int CVarHandler_NameplateBandY(CVar* cvar, const char*, const char* value, void*) {
        const int result = cvar->Sync(value, &g_bandY, 0.1f, 1.5f, "%.2f");
        if (CGWorldFrame* wf = CGWorldFrame::GetWorldFrame()) wf->m_renderDirtyFlags |= 1; return result;
    }
    static int CVarHandler_NameplateHitboxWidthE(CVar* cvar, const char*, const char* value, void*) {
        const int result = cvar->Sync(value, &g_hitWidthE, 0.0f, 1.0f, "%.2f");
        if (CGWorldFrame* wf = CGWorldFrame::GetWorldFrame()) wf->m_renderDirtyFlags |= 1; return result;
    }
    static int CVarHandler_NameplateHitboxHeightE(CVar* cvar, const char*, const char* value, void*) {
        const int result = cvar->Sync(value, &g_hitHeightE, 0.0f, 1.0f, "%.2f");
        if (CGWorldFrame* wf = CGWorldFrame::GetWorldFrame()) wf->m_renderDirtyFlags |= 1; return result;
    }
    static int CVarHandler_NameplateHitboxWidthF(CVar* cvar, const char*, const char* value, void*) {
        const int result = cvar->Sync(value, &g_hitWidthF, 0.0f, 1.0f, "%.2f");
        if (CGWorldFrame* wf = CGWorldFrame::GetWorldFrame()) wf->m_renderDirtyFlags |= 1; return result;
    }
    static int CVarHandler_NameplateHitboxHeightF(CVar* cvar, const char*, const char* value, void*) {
        const int result = cvar->Sync(value, &g_hitHeightF, 0.0f, 1.0f, "%.2f");
        if (CGWorldFrame* wf = CGWorldFrame::GetWorldFrame()) wf->m_renderDirtyFlags |= 1; return result;
    }
    static int CVarHandler_NameplateRaiseSpeed(CVar* cvar, const char*, const char* value, void*) {
        const int result = cvar->Sync(value, &g_speedRaise, 1.0f, 30.0f, "%.2f");
        if (CGWorldFrame* wf = CGWorldFrame::GetWorldFrame()) wf->m_renderDirtyFlags |= 1; return result;
    }
    static int CVarHandler_NameplateLowerSpeed(CVar* cvar, const char*, const char* value, void*) {
        const int result = cvar->Sync(value, &g_speedLower, 1.0f, 30.0f, "%.2f");
        if (CGWorldFrame* wf = CGWorldFrame::GetWorldFrame()) wf->m_renderDirtyFlags |= 1; return result;
    }
    static int CVarHandler_NameplatePullSpeed(CVar* cvar, const char*, const char* value, void*) {
        const int result = cvar->Sync(value, &g_speedPull, 1.0f, 30.0f, "%.2f");
        if (CGWorldFrame* wf = CGWorldFrame::GetWorldFrame()) wf->m_renderDirtyFlags |= 1; return result;
    }
    static int CVarHandler_NameplateMomentum(CVar* cvar, const char*, const char* value, void*) {
        const int result = cvar->Sync(value, &g_momMult, 1.0f, 8.0f, "%.2f");
        if (CGWorldFrame* wf = CGWorldFrame::GetWorldFrame()) wf->m_renderDirtyFlags |= 1; return result;
    }
    static int CVarHandler_NameplateRaiseDistance(CVar* cvar, const char*, const char* value, void*) {
        const int result = cvar->Sync(value, &g_maxRaise, 1.0f, 8.0f, "%.2f");
        if (CGWorldFrame* wf = CGWorldFrame::GetWorldFrame()) wf->m_renderDirtyFlags |= 1; return result;
    }
    static int CVarHandler_NameplatePullDistance(CVar* cvar, const char*, const char* value, void*) {
        const int result = cvar->Sync(value, &g_maxPull, 0.0f, 0.75f, "%.2f");
        if (CGWorldFrame* wf = CGWorldFrame::GetWorldFrame()) wf->m_renderDirtyFlags |= 1; return result;
    }
    static int CVarHandler_NameplateClampTopOffset(CVar* cvar, const char*, const char* value, void*) {
        const int result = cvar->Sync(value, &g_clampTopOffset, 0.0f, NDC_Y * 0.5f, "%.2f");
        if (CGWorldFrame* wf = CGWorldFrame::GetWorldFrame()) wf->m_renderDirtyFlags |= 1; return result;
    }
    static int CVarHandler_NameplateOcclusionAlpha(CVar* cvar, const char*, const char* value, void*) {
        const int result = cvar->Sync(value, &g_occlusionAlpha, 0.0f, 1.0f, "%.2f");
        if (CGWorldFrame* wf = CGWorldFrame::GetWorldFrame()) wf->m_renderDirtyFlags |= 1; return result;
    }
    static int CVarHandler_NameplateNonTargetAlpha(CVar* cvar, const char*, const char* value, void*) {
        const int result = cvar->Sync(value, &g_nonTargetAlpha, 0.0f, 1.0f, "%.2f");
        if (CGWorldFrame* wf = CGWorldFrame::GetWorldFrame()) wf->m_renderDirtyFlags |= 1; return result;
    }
    static int CVarHandler_NameplateAlphaSpeed(CVar* cvar, const char*, const char* value, void*) {
        const int result = cvar->Sync(value, &g_alphaSpd, 0.01f, 1.00f, "%.3f");
        if (CGWorldFrame* wf = CGWorldFrame::GetWorldFrame()) wf->m_renderDirtyFlags |= 1; return result;
    }
    static int CVarHandler_NameplateMouseMode(CVar* cvar, const char*, const char* value, void*) {
        int f;
        const int result = cvar->Sync(value, &f, static_cast<int>(EMouseMode::M_DISABLED), static_cast<int>(std::size(g_mouseModeMap)) - 1, "%d");
        g_mouseMode = static_cast<EMouseMode>(g_mouseModeMap[f]);
        if (CGWorldFrame* wf = CGWorldFrame::GetWorldFrame()) wf->m_renderDirtyFlags |= 1; return result;
    }
    static int CVarHandler_NameplateClampTop(CVar* cvar, const char*, const char* value, void*) {
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
        auto& buf = g_entries.get();
        for (auto& e : buf) g_entries.applyClampingState(e);
        if (CGWorldFrame* wf = CGWorldFrame::GetWorldFrame()) wf->m_renderDirtyFlags |= 1; return result;
    }
    static int CVarHandler_NameplateStacking(CVar* cvar, const char*, const char* value, void*) {
        const int result = cvar->Sync(value, reinterpret_cast<int*>(&g_stackingMode), static_cast<int>(EStackingMode::S_DISABLED), static_cast<int>(EStackingMode::S_FRIENDLY), "%d");
        auto& buf = g_entries.get();
        for (auto& e : buf) g_entries.applyStackingState(e);
        if (CGWorldFrame* wf = CGWorldFrame::GetWorldFrame()) wf->m_renderDirtyFlags |= 1;
        return result;
    }
}

void NamePlates::initialize() {
    g_entries.reserveAll(MAX_PLATES);
    g_entries.initializeTokens(MAX_PLATES);

    Hooks::FrameXML::registerLuaLib(lua_openlibnameplates);
    Hooks::FrameXML::registerEvent(NAME_PLATE_CREATED);
    Hooks::FrameXML::registerEvent(NAME_PLATE_UNIT_ADDED);
    Hooks::FrameXML::registerEvent(NAME_PLATE_UNIT_REMOVED);
    Hooks::FrameXML::registerCVar(&s_cvar_nameplateDistance, "nameplateDistance", nullptr, "41.0", CVarHandler_NameplateDistance);
    Hooks::FrameXML::registerCVar(&s_cvar_nameplatePlacement, "nameplatePlacement", nullptr, "0.0", CVarHandler_NameplatePlacement);
    Hooks::FrameXML::registerCVar(&s_cvar_nameplateMouseMode, "nameplateMouseoverMode", nullptr, "0", CVarHandler_NameplateMouseMode);
    Hooks::FrameXML::registerCVar(&s_cvar_nameplateBandX, "nameplateBandX", nullptr, "0.7", CVarHandler_NameplateBandX);
    Hooks::FrameXML::registerCVar(&s_cvar_nameplateBandY, "nameplateBandY", nullptr, "1.0", CVarHandler_NameplateBandY);
    Hooks::FrameXML::registerCVar(&s_cvar_nameplateHitboxWidthE, "nameplateHitboxWidthE", nullptr, "1.0", CVarHandler_NameplateHitboxWidthE);
    Hooks::FrameXML::registerCVar(&s_cvar_nameplateHitboxHeightE, "nameplateHitboxHeightE", nullptr, "1.0", CVarHandler_NameplateHitboxHeightE);
    Hooks::FrameXML::registerCVar(&s_cvar_nameplateHitboxWidthF, "nameplateHitboxWidthF", nullptr, "1.0", CVarHandler_NameplateHitboxWidthF);
    Hooks::FrameXML::registerCVar(&s_cvar_nameplateHitboxHeightF, "nameplateHitboxHeightF", nullptr, "1.0", CVarHandler_NameplateHitboxHeightF);
    Hooks::FrameXML::registerCVar(&s_cvar_nameplateRaiseSpeed, "nameplateRaiseSpeed", nullptr, "8.0", CVarHandler_NameplateRaiseSpeed);
    Hooks::FrameXML::registerCVar(&s_cvar_nameplateLowerSpeed, "nameplateLowerSpeed", nullptr, "8.0", CVarHandler_NameplateLowerSpeed);
    Hooks::FrameXML::registerCVar(&s_cvar_nameplatePullSpeed, "nameplatePullSpeed", nullptr, "8.0", CVarHandler_NameplatePullSpeed);
    Hooks::FrameXML::registerCVar(&s_cvar_nameplateMomentum, "nameplateMomentum", nullptr, "1.25", CVarHandler_NameplateMomentum);
    Hooks::FrameXML::registerCVar(&s_cvar_nameplateRaiseDistance, "nameplateRaiseDistanceMax", nullptr, "6.0", CVarHandler_NameplateRaiseDistance);
    Hooks::FrameXML::registerCVar(&s_cvar_nameplatePullDistance, "nameplatePullDistanceMax", nullptr, "0.25", CVarHandler_NameplatePullDistance);
    Hooks::FrameXML::registerCVar(&s_cvar_nameplateOcclusionAlpha, "nameplateOcclusionAlpha", nullptr, "1.0", CVarHandler_NameplateOcclusionAlpha);
    Hooks::FrameXML::registerCVar(&s_cvar_nameplateNonTargetAlpha, "nameplateNonTargetAlpha", nullptr, "0.5", CVarHandler_NameplateNonTargetAlpha);
    Hooks::FrameXML::registerCVar(&s_cvar_nameplateAlphaSpeed, "nameplateAlphaSpeed", nullptr, "0.25", CVarHandler_NameplateAlphaSpeed);
    Hooks::FrameXML::registerCVar(&s_cvar_nameplateClampTop, "nameplateClampTop", nullptr, "0", CVarHandler_NameplateClampTop);
    Hooks::FrameXML::registerCVar(&s_cvar_nameplateClampTopOffset, "nameplateClampTopOffset", nullptr, "0.1", CVarHandler_NameplateClampTopOffset);
    Hooks::FrameXML::registerCVar(&s_cvar_nameplateStacking, "nameplateStacking", nullptr, "0", CVarHandler_NameplateStacking);

    Hooks::Detour(&CSimpleFrame__Hide_site, CSimpleFrame__Hide_siteHk);
    Hooks::Detour(&CSimpleFrame__SetFrameAlpha_site, CSimpleFrame__SetFrameAlpha_siteHk);

    Hooks::Detour(&CGNamePlate__OnUpdate_site, CGNamePlate__OnUpdate_siteHk);
    Hooks::Detour(&CGNamePlate__Initialize_site, CGNamePlate__Initialize_siteHk);

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