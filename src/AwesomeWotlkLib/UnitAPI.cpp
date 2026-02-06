#include "UnitAPI.h"
#include "GameClient.h"
#include "Hooks.h"
#include "NamePlates.h"

extern int getTokenId(guid_t guid);

static int lua_UnitHasFlag(lua_State* L, uint32_t flag) {
    Unit* unit = (Unit*)ObjectMgr::Get(luaL_checkstring(L, 1), TYPEMASK_UNIT);
    if (unit && (unit->entry->flags & flag)) {
        lua_pushnumber(L, 1);
        return 1;
    }
    return 0;
}

static int lua_UnitIsControlled(lua_State* L) {
    static constexpr uint32_t controlledFlags = UNIT_FLAG_FLEEING | UNIT_FLAG_CONFUSED | UNIT_FLAG_STUNNED | UNIT_FLAG_PACIFIED;
    return lua_UnitHasFlag(L, controlledFlags);
}

static int lua_UnitIsDisarmed(lua_State* L) {
    return lua_UnitHasFlag(L, UNIT_FLAG_DISARMED);
}

static int lua_UnitIsSilenced(lua_State* L) {
    return lua_UnitHasFlag(L, UNIT_FLAG_SILENCED);
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
static int lua_UnitOccupations(lua_State* L)
{
    Unit* unit = (Unit*)ObjectMgr::Get(luaL_checkstring(L, 1), TYPEMASK_UNIT);
    if (!unit)
        return 0;
    lua_pushnumber(L, unit->entry->npc_flags);
    return 1;
}

static int lua_UnitOwner(lua_State* L)
{
    Unit* unit = (Unit*)ObjectMgr::Get(luaL_checkstring(L, 1), TYPEMASK_UNIT);
    if (!unit)
        return 0;
    guid_t ownerGuid = unit->entry->summonedBy ? unit->entry->summonedBy : unit->entry->createdBy;
    if (!ownerGuid)
        return 0;

    char guidStr[32];
    snprintf(guidStr, sizeof(guidStr), "0x%llx", ownerGuid);
    lua_pushstring(L, ObjectMgr::UnitNameFromGuid(ownerGuid));
    lua_pushstring(L, guidStr);

    return 2;
}

static int lua_UnitTokenFromGUID(lua_State* L) {
    guid_t guid = ObjectMgr::HexString2Guid(luaL_checkstring(L, 1));
    if (!guid || !((Unit*)ObjectMgr::Get(guid, TYPEMASK_UNIT)))
        return 0;

    auto checkToken = [&](const char* token) -> bool {
        Unit* unit = (Unit*)ObjectMgr::Get(token, TYPEMASK_UNIT);
        return unit && ObjectMgr::GetGuidByUnitID(token) == guid;
    };

    const char* singleTokens[] = { "player", "vehicle", "pet", "target", "focus", "mouseover" };
    for (const char* token : singleTokens) {
        if (checkToken(token)) {
            lua_pushstring(L, token);
            return 1;
        }
    }

    auto checkIndexedTokens = [&](const char* base, int start, int end) -> bool {
        char token[16];
        for (int i = start; i <= end; ++i) {
            snprintf(token, sizeof(token), "%s%d", base, i);
            if (checkToken(token)) {
                lua_pushstring(L, token);
                return true;
            }
        }
        return false;
    };

    if (checkIndexedTokens("party", 1, 4)) return 1;
    if (checkIndexedTokens("partypet", 1, 4)) return 1;
    if (checkIndexedTokens("raid", 1, 40)) return 1;
    if (checkIndexedTokens("raidpet", 1, 40)) return 1;
    if (checkIndexedTokens("arena", 1, 5)) return 1;
    if (checkIndexedTokens("arenapet", 1, 5)) return 1;
    if (checkIndexedTokens("boss", 1, 5)) return 1;

    int tokenId = getTokenId(guid);
    if (tokenId >= 0) {
        char token[16];
        snprintf(token, sizeof(token), "nameplate%d", tokenId + 1);
        lua_pushstring(L, token);
        return 1;
    }

    return 0;
}
    /*
    Unit* npc = (Unit*)ObjectMgr::Get("npc", TYPEMASK_UNIT);
    if (npc && ObjectMgr::GetGuidByUnitID("npc") == guid) {
        lua_pushstring(L, "npc");
        return 1;
    }
    */

    /*
    Unit* softenemy = (Unit*)ObjectMgr::Get("softenemy", TYPEMASK_UNIT);
    if (softenemy && ObjectMgr::GetGuidByUnitID("softenemy") == guid) {
        lua_pushstring(L, "softenemy");
        return 1;
    }

    Unit* softfriend = (Unit*)ObjectMgr::Get("softfriend", TYPEMASK_UNIT);
    if (softfriend && ObjectMgr::GetGuidByUnitID("softfriend") == guid) {
        lua_pushstring(L, "softfriend");
        return 1;
    }

    Unit* softinteract = (Unit*)ObjectMgr::Get("softinteract", TYPEMASK_UNIT);
    if (softinteract && ObjectMgr::GetGuidByUnitID("softinteract") == guid) {
        lua_pushstring(L, "softinteract");
        return 1;
    }
    */


static int lua_openunitlib(lua_State* L)
{
    luaL_Reg funcs[] = {
        { "UnitIsControlled", lua_UnitIsControlled },
        { "UnitIsDisarmed", lua_UnitIsDisarmed },
        { "UnitIsSilenced", lua_UnitIsSilenced },
        { "UnitOccupations", lua_UnitOccupations },
        { "UnitOwner", lua_UnitOwner },
        { "UnitTokenFromGUID", lua_UnitTokenFromGUID },
    };

    for (auto& [name, func] : funcs) {
        lua_pushcfunction(L, func);
        lua_setglobal(L, name);
    }

    return 0;
}

void UnitAPI::initialize()
{
    Hooks::FrameXML::registerLuaLib(lua_openunitlib);
}
