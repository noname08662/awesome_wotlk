#include "UnitAPI.h"
#include "Lua.h"
#include "Hooks.h"
#include "NamePlates.h"

namespace {
    bool checkToken(lua_State* L, const char* token, guid_t guid) {
        if (const guid_t guid_t = ObjectMgr::GetGuidByUnitID(token); guid_t == guid) {
            Lua::lua_pushstring(L, token);
            return true;
        }
        return false;
    }

    bool checkIndexedTokens(lua_State* L, const char* base, int start, int end, guid_t guid) {
        char token[16];
        for (int i = start; i <= end; ++i) {
            snprintf(token, sizeof(token), "%s%d", base, i);
            if (checkToken(L, token, guid)) return true;
        }
        return false;
    }

    int unitHasFlag(lua_State* L, uint32_t flag) {
        CGUnit_C* unit = ObjectMgr::Get<CGUnit_C>(ObjectMgr::GetGuidByUnitID(Lua::luaL_checkstring(L, 1)), TYPEMASK_UNIT);
        if (unit && (unit->GetEntry()->m_flags & flag)) {
            Lua::lua_pushnumber(L, 1);
            return 1;
        }
        return 0;
    }

    int lua_UnitIsControlled(lua_State* L) {
        return unitHasFlag(L, UNIT_FLAG_FLEEING | UNIT_FLAG_CONFUSED | UNIT_FLAG_STUNNED | UNIT_FLAG_PACIFIED);
    }

    int lua_UnitIsDisarmed(lua_State* L) {
        return unitHasFlag(L, UNIT_FLAG_DISARMED);
    }

    int lua_UnitIsSilenced(lua_State* L) {
        return unitHasFlag(L, UNIT_FLAG_SILENCED);
    }

    /*
    enum NPCFlags : uint32_t
    {
        UNIT_NPC_FLAG_NONE = 0x00000000,       // SKIP
        UNIT_NPC_FLAG_GOSSIP = 0x00000001,       // TITLE has gossip menu DESCRIPTION 100%
        UNIT_NPC_FLAG_QUESTGIVER = 0x00000002,       // TITLE is quest giver DESCRIPTION guessed, probably ok
        UNIT_NPC_FLAG_UNK1 = 0x00000004,
        UNIT_NPC_FLAG_UNK2 = 0x00000008,
        UNIT_NPC_FLAG_TRAINER = 0x00000010,       // TITLE is trainer DESCRIPTION 100%
        UNIT_NPC_FLAG_TRAINER_CLASS = 0x00000020,       // TITLE is class trainer DESCRIPTION 100%
        UNIT_NPC_FLAG_TRAINER_PROFESSION = 0x00000040,       // TITLE is profession trainer DESCRIPTION 100%
        UNIT_NPC_FLAG_VENDOR = 0x00000080,       // TITLE is vendor (generic) DESCRIPTION 100%
        UNIT_NPC_FLAG_VENDOR_AMMO = 0x00000100,       // TITLE is vendor (ammo) DESCRIPTION 100%, general goods vendor
        UNIT_NPC_FLAG_VENDOR_FOOD = 0x00000200,       // TITLE is vendor (food) DESCRIPTION 100%
        UNIT_NPC_FLAG_VENDOR_POISON = 0x00000400,       // TITLE is vendor (poison) DESCRIPTION guessed
        UNIT_NPC_FLAG_VENDOR_REAGENT = 0x00000800,       // TITLE is vendor (reagents) DESCRIPTION 100%
        UNIT_NPC_FLAG_REPAIR = 0x00001000,       // TITLE can repair DESCRIPTION 100%
        UNIT_NPC_FLAG_FLIGHTMASTER = 0x00002000,       // TITLE is flight master DESCRIPTION 100%
        UNIT_NPC_FLAG_SPIRITHEALER = 0x00004000,       // TITLE is spirit healer DESCRIPTION guessed
        UNIT_NPC_FLAG_SPIRITGUIDE = 0x00008000,       // TITLE is spirit guide DESCRIPTION guessed
        UNIT_NPC_FLAG_INNKEEPER = 0x00010000,       // TITLE is innkeeper
        UNIT_NPC_FLAG_BANKER = 0x00020000,       // TITLE is banker DESCRIPTION 100%
        UNIT_NPC_FLAG_PETITIONER = 0x00040000,       // TITLE handles guild/arena petitions DESCRIPTION 100% 0xC0000 = guild petitions, 0x40000 = arena team petitions
        UNIT_NPC_FLAG_TABARDDESIGNER = 0x00080000,       // TITLE is guild tabard designer DESCRIPTION 100%
        UNIT_NPC_FLAG_BATTLEMASTER = 0x00100000,       // TITLE is battlemaster DESCRIPTION 100%
        UNIT_NPC_FLAG_AUCTIONEER = 0x00200000,       // TITLE is auctioneer DESCRIPTION 100%
        UNIT_NPC_FLAG_STABLEMASTER = 0x00400000,       // TITLE is stable master DESCRIPTION 100%
        UNIT_NPC_FLAG_GUILD_BANKER = 0x00800000,       // TITLE is guild banker DESCRIPTION cause client to send 997 opcode
        UNIT_NPC_FLAG_SPELLCLICK = 0x01000000,       // TITLE has spell click enabled DESCRIPTION cause client to send 1015 opcode (spell click)
        UNIT_NPC_FLAG_PLAYER_VEHICLE = 0x02000000,       // TITLE is player vehicle DESCRIPTION players with mounts that have vehicle data should have it set
        UNIT_NPC_FLAG_MAILBOX = 0x04000000        // TITLE is mailbox
    };
    */
    int lua_UnitOccupations(lua_State* L) {
        CGUnit_C* unit = ObjectMgr::Get<CGUnit_C>(ObjectMgr::GetGuidByUnitID(Lua::luaL_checkstring(L, 1)), TYPEMASK_UNIT);
        if (!unit) return 0;
        Lua::lua_pushnumber(L, unit->GetEntry()->m_npc_flags);
        return 1;
    }

    int lua_UnitOwner(lua_State* L) {
        CGUnit_C* unit = ObjectMgr::Get<CGUnit_C>(ObjectMgr::GetGuidByUnitID(Lua::luaL_checkstring(L, 1)), TYPEMASK_UNIT);
        if (!unit) return 0;
        guid_t ownerGuid = unit->GetEntry()->m_summonedBy ? unit->GetEntry()->m_summonedBy : unit->GetEntry()->m_createdBy;
        if (!ownerGuid) return 0;

        char guidStr[32];
        snprintf(guidStr, sizeof(guidStr), "0x%016llX", ownerGuid);
        CGUnit_C* owner = ObjectMgr::Get<CGUnit_C>(ownerGuid, TYPEMASK_UNIT);
        if (!owner) return 0;
        const char* name = owner->GetObjectName();
        Lua::lua_pushstring(L, name ? name : "UNKNOWN");
        Lua::lua_pushstring(L, guidStr);
        return 2;
    }

    int lua_UnitTokenFromGUID(lua_State* L) {
        guid_t guid = ObjectMgr::HexString2Guid(Lua::luaL_checkstring(L, 1));
        if (!guid || !(ObjectMgr::Get<CGUnit_C>(guid, TYPEMASK_UNIT))) return 0;

        for (const char* token : { "player", "vehicle", "pet", "target", "focus", "mouseover" }) {
            if (checkToken(L, token, guid)) return 1;
        }

        if (checkIndexedTokens(L, "party", 1, 4, guid)) return 1;
        if (checkIndexedTokens(L, "partypet", 1, 4, guid)) return 1;
        if (checkIndexedTokens(L, "raid", 1, 40, guid)) return 1;
        if (checkIndexedTokens(L, "raidpet", 1, 40, guid)) return 1;
        if (checkIndexedTokens(L, "arena", 1, 5, guid)) return 1;
        if (checkIndexedTokens(L, "arenapet", 1, 5, guid)) return 1;
        if (checkIndexedTokens(L, "boss", 1, 5, guid)) return 1;

        int tokenId = NamePlates::GetTokenId(guid);
        if (tokenId >= 0) {
            char token[16];
            snprintf(token, sizeof(token), "nameplate%d", tokenId + 1);
            Lua::lua_pushstring(L, token);
            return 1;
        }
        return 0;
    }

    int lua_openunitlib(lua_State* L) {
        Lua::luaL_Reg funcs[] = {
            { "UnitIsControlled", lua_UnitIsControlled },
            { "UnitIsDisarmed", lua_UnitIsDisarmed },
            { "UnitIsSilenced", lua_UnitIsSilenced },
            { "UnitOccupations", lua_UnitOccupations },
            { "UnitOwner", lua_UnitOwner },
            { "UnitTokenFromGUID", lua_UnitTokenFromGUID },
        };
        for (auto& [name, func] : funcs) {
            Lua::lua_pushcfunction(L, func);
            Lua::lua_setglobal(L, name);
        }
        return 0;
    }
}

void UnitAPI::initialize() {
    Hooks::FrameXML::registerLuaLib(lua_openunitlib);
}
