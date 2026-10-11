local root = (arg[0]:match("^(.*)[/\\]") or ".") .. "/.."
local entryPath = arg[1] or (root .. "/src/AwesomeWotlkLib/Entry.cpp")
local constantsPath = arg[2] or (root .. "/addons/AwesomeCVar/Constants.lua")

local function readFile(path)
    local f = assert(io.open(path, "rb")); local s = f:read("*a"); f:close()
    return s
end

local failures = 0
local function fail(fmt, ...) failures = failures + 1; print("  FAIL " .. string.format(fmt, ...)) end

-- A missing match fails too: a refactor must not turn the check into a silent pass.
local function extract(path, pattern, what)
    local value = readFile(path):match(pattern)
    if not value then fail("%s not found in %s", what, path) end
    return tonumber(value)
end

local mod = extract(entryPath, 'pushNumber%(%s*l%s*,%s*(%d+)%s*%);%s*lua::setGlobal%(%s*l%s*,%s*"AwesomeWotlk"%s*%)',
    'pushNumber(l, N) before setGlobal(l, "AwesomeWotlk")')
local addon = extract(constantsPath, "ACVar%.MOD_VERSION%s*=%s*(%d+)", "ACVar.MOD_VERSION = N")

if mod and addon then
    print(string.format("Entry.cpp: %d, Constants.lua: %d", mod, addon))
    if mod ~= addon then fail("versions differ; bump both together") end
end
print(failures == 0 and "OK" or (failures .. " failure(s)"))
if failures > 0 then os.exit(1) end
