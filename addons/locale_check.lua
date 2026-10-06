-- Offline check of AwesomeCVar's locales with stock Lua 5.1 (no game needed).
-- Usage: lua locale_check.lua [addonDir]   (default: AwesomeCVar next to this script)
-- For every locale: keys missing from or unknown to enUS, %s/%d specifier mismatches, then a full load in
-- TOC order (all locales, then Constants.lua) with that locale active, checking that every tab name, label,
-- description, performance note, reason and mode label is a string and that loading writes no CVar.
-- Any of these fails the run (exit 1). Run by the pre-commit hook and the Luacheck workflow; the release
-- copies only the addon folders, so this file never ships.
local dir = arg[1] or ((arg[0]:match("^(.*)[/\\]") or ".") .. "/AwesomeCVar")
local LOCALES = { "enUS", "deDE", "esMX", "frFR", "koKR", "ptBR", "ruRU", "zhCN", "zhTW" }

local function readFile(path)
    local f = assert(io.open(path, "rb")); local s = f:read("*a"); f:close()
    return (s:gsub("^\239\187\191", ""))
end

local function loadChunk(path, ns)
    local fn = assert(loadstring(readFile(path), "@" .. path))
    fn("AwesomeCVar", ns)
end

-- WoW stubs
local cur
function GetLocale() return cur end
function wipe(t) for k in pairs(t) do t[k] = nil end return t end
function CreateFrame() return { RegisterEvent = function() end, SetScript = function() end } end
function GetAddOnMetadata() return "3.0" end
local cvarWrites = 0
function SetCVar() cvarWrites = cvarWrites + 1 end
C_VoiceChat = { GetTtsVoices = function() return { { voiceID = 0, name = "V" } } end }

local function specs(s)
    local t = {}
    for spec in s:gsub("%%%%", ""):gmatch("%%[-%d.]*([sdif])") do t[#t + 1] = spec end
    return table.concat(t, ",")
end

local failures = 0
local function fail(fmt, ...) failures = failures + 1; print("  FAIL " .. string.format(fmt, ...)) end

-- enUS reference
local enNs = {}
cur = "enUS"; loadChunk(dir .. "/Locales/enUS.lua", enNs)
local EN = enNs.L

for _, loc in ipairs(LOCALES) do
    cur = loc
    -- raw locale table on its own, to diff keys
    local raw = { L = {} }
    loadChunk(dir .. "/Locales/" .. loc .. ".lua", raw)

    local missing, obsolete, specDiff, nonString = {}, {}, {}, {}
    for k in pairs(EN) do if raw.L[k] == nil and loc ~= "enUS" then missing[#missing + 1] = k end end
    for k, v in pairs(raw.L) do
        if EN[k] == nil then obsolete[#obsolete + 1] = k
        elseif type(v) ~= "string" then nonString[#nonString + 1] = k
        elseif specs(v) ~= specs(EN[k]) then specDiff[#specDiff + 1] = k .. " [" .. specs(v) .. " vs " .. specs(EN[k]) .. "]" end
    end
    table.sort(missing); table.sort(obsolete); table.sort(specDiff)

    -- full load order as in the TOC: enUS, then every locale (guarded), then Constants
    local ns = {}
    for _, l in ipairs(LOCALES) do loadChunk(dir .. "/Locales/" .. l .. ".lua", ns) end
    cvarWrites = 0
    loadChunk(dir .. "/Constants.lua", ns)

    local n = 0
    for _ in pairs(raw.L) do n = n + 1 end
    print(string.format("%s: %d keys, %d missing, %d obsolete, %d format mismatch", loc, n, #missing, #obsolete, #specDiff))
    if #missing > 0 and #missing <= 20 then print("  missing: " .. table.concat(missing, " ")) end
    if #obsolete > 0 then print("  obsolete: " .. table.concat(obsolete, " ")) end
    if #specDiff > 0 then print("  format: " .. table.concat(specDiff, "; ")) end
    if #missing > 0 then fail("%d missing key(s)", #missing) end
    if #obsolete > 0 then fail("%d obsolete key(s)", #obsolete) end
    if #specDiff > 0 then fail("%d format mismatch(es)", #specDiff) end
    for _, k in ipairs(nonString) do fail("%s is not a string", k) end
    if cvarWrites > 0 then fail("%d CVar writes during load", cvarWrites) end

    for _, id in ipairs(ns.CATEGORY_ORDER) do
        if type(ns.CATEGORY_NAMES[id]) ~= "string" then fail("CATEGORY_NAMES.%s", id) end
        if not ns.CVARS[id] then fail("CVARS.%s missing", id) end
        if ns.ResolveCategory(id) ~= id then fail("ResolveCategory(%s)", id) end
        if ns.ResolveCategory(ns.CATEGORY_NAMES[id]) ~= id then fail("ResolveCategory(localized %s)", id) end
        for _, d in ipairs(ns.CVARS[id]) do
            if type(d.label) ~= "string" then fail("%s.label", d.name) end
            if d.type ~= "header" and type(d.desc) ~= "string" then fail("%s.desc", d.name) end
            if d.perf ~= nil and type(d.perf) ~= "string" then fail("%s.perf", d.name) end
            if d.requires and type(d.requires.reason) ~= "string" then fail("%s.requires.reason", d.name) end
            if not ns.IsReadOnly(d) and d.type ~= "header" and d.type ~= "description" and d.type ~= "link" and not d.perf then
                fail("%s has no perf", d.name)
            end
            for _, m in ipairs(d.modes or {}) do
                if type(m.label) ~= "string" then fail("%s mode %s label", d.name, tostring(m.value)) end
            end
        end
    end
end
print(failures == 0 and "OK" or (failures .. " failure(s)"))
if failures > 0 then os.exit(1) end
