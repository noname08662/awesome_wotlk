-- File: Constants.lua
-- Holds all static definitions for the addon.

local addonName, ACVar = ...
local L = ACVar.L
_G["AwesomeCVar"] = {} -- Public API table

-- Awesome WotLK version this addon was made for: the AwesomeWotlk global set in Entry.cpp.
-- Releases ship the mod and the addon together; bump this with Entry.cpp.
ACVar.MOD_VERSION = 38
ACVar.URLS = {
    GITHUB = "https://github.com/noname08662/awesome_wotlk",
    RELEASES = "https://github.com/noname08662/awesome_wotlk/releases/latest",
    DOCS = "https://github.com/noname08662/awesome_wotlk/blob/main/docs/api_reference.md",
}

-- Returns "ok", "addon_outdated", "mod_outdated" or "missing", and the running mod version.
function ACVar.GetVersionStatus()
    local modVersion = tonumber(_G.AwesomeWotlk)
    if not modVersion then return "missing" end
    if modVersion > ACVar.MOD_VERSION then return "addon_outdated", modVersion end
    if modVersion < ACVar.MOD_VERSION then return "mod_outdated", modVersion end
    return "ok", modVersion
end

local VERSION_STATUS_TEXT = {
    ok = "|cff00ff00"..L.VERSION_OK.."|r",
    addon_outdated = "|cffff8000"..L.VERSION_ADDON_OUTDATED.."|r",
    mod_outdated = "|cffff8000"..L.VERSION_MOD_OUTDATED.."|r",
    missing = "|cffff2020"..L.VERSION_MISSING.."|r",
}

function ACVar.GetVersionStatusText()
    return VERSION_STATUS_TEXT[ACVar.GetVersionStatus()]
end

local function versionText()
    local _, modVersion = ACVar.GetVersionStatus()
    local addonVersion = GetAddOnMetadata(addonName, "Version") or "?"
    return string.format(L.ABOUT_VERSION_MOD, modVersion and tostring(modVersion) or L.ABOUT_NOT_LOADED).."\n"
        ..string.format(L.ABOUT_VERSION_ADDON, addonVersion).."\n\n"
        ..ACVar.GetVersionStatusText()
end

ACVar.CONSTANTS = {
    COLORS = {
        SUCCESS = "|cff00ff00",
        HIGHLIGHT = "|cffffd100",
        VALUE = "|cff00ccff",
        ERROR = "|cffff0000",
        RESET = "|r",
        DESC_TEXT = {0.6, 0.6, 0.6},
        PERF_TEXT = {0.85, 0.7, 0.45},
        INACTIVE_TEXT = {0.75, 0.75, 0.75},
    },
    DIMMED_ALPHA = 0.4,
    FRAME = {
        MAIN_WIDTH = 768,
        MAIN_HEIGHT = 580,
        POPUP_WIDTH = 350,
        POPUP_HEIGHT = 120,
        BUTTON_WIDTH = 100,
        BUTTON_HEIGHT = 25,
        TAB_HEIGHT = 25,
        REASON_LINE_HEIGHT = 16,
    }
}

-- Text-to-speech voices by voice id, filled from the client and refreshed when its voice list changes.
ACVar.TTS_VOICES = {}

local function updateTts()
    wipe(ACVar.TTS_VOICES)
    for _, voiceInfo in pairs(C_VoiceChat and C_VoiceChat.GetTtsVoices() or {}) do
        ACVar.TTS_VOICES[voiceInfo.voiceID] = voiceInfo.name
    end
end

updateTts()

local TtsUpdateFrame = CreateFrame("Frame")
TtsUpdateFrame:RegisterEvent("VOICE_CHAT_TTS_VOICES_UPDATE")
TtsUpdateFrame:SetScript("OnEvent", updateTts)

-- Dependencies: test(get) is false while a setting has no effect, mirroring the mod's own checks.
-- The card is dimmed and shows the reason then. get(cvarName) returns the CVar's current value.
local REQUIRES = {
    fade = { reason = L.REASON_FADE_OFF, test = function(get) return get("cameraIndirectVisibility") ~= 0 end },
    msdf = { reason = L.REASON_MSDF_OFF, test = function(get) return get("MSDFMode") ~= 0 end },
    occlusion = { reason = L.REASON_OCCLUSION_OFF,
        test = function(get) return math.abs(tonumber(get("nameplateOcclusionAlpha")) or 0) < 1 end },
    stacking = { reason = L.REASON_STACKING_OFF, test = function(get) return get("nameplateStacking") ~= 0 end },
    clamping = { reason = L.REASON_CLAMPING_OFF, test = function(get) return get("nameplateClampMode") ~= 0 end },
    clampAllSides = { reason = L.REASON_CLAMP_NOT_ALL_SIDES,
        test = function(get) local mode = get("nameplateClampMode") return mode == 3 or mode == 4 end },
    freeze = { reason = L.REASON_FREEZE_OFF, test = function(get) return get("nameplateMouseFreeze") ~= 0 end },
    shortHitbox = { reason = L.REASON_HITBOX_FULL, test = function(get)
        return (tonumber(get("nameplateHitboxHeightE")) or 1) < 1 or (tonumber(get("nameplateHitboxHeightF")) or 1) < 1
    end },
    cone = { reason = L.REASON_INTERACTION_RADIUS, test = function(get) return get("interactionMode") ~= 0 end },
}

-- Every control shown in the window, keyed by category id.
-- Category ids are stable and locale-independent: frame names, tab lookup and the public API use them.
ACVar.CVARS = {
    Rendering = {
        { name = "headerCamera", label = L.SECTION_CAMERA, type = "header" },
        { name = "cameraFov", label = L.CVAR_LABEL_CAMERA_FOV, desc = L.DESC_CAMERA_FOV, perf = L.PERF_CAMERA_FOV, type = "slider", min = 90, max = 150, default = 100 },
        { name = "cameraDistanceMax", label = L.CVAR_LABEL_CAMERA_DISTANCE_MAX, desc = L.DESC_CAMERA_DISTANCE_MAX, perf = L.PERF_CAMERA_DISTANCE, type = "slider", min = 0, max = 50, step = 1, default = 15 },
        { name = "cameraIndirectVisibility", label = L.CVAR_LABEL_CAMERA_INDIRECT_VISIBILITY, desc = L.DESC_CAMERA_INDIRECT_VISIBILITY, perf = L.PERF_CAMERA_INDIRECT, type = "toggle", min = 0, max = 1, default = 0 },
        { name = "cameraIndirectAlpha", label = L.CVAR_LABEL_CAMERA_INDIRECT_ALPHA, desc = L.DESC_CAMERA_INDIRECT_ALPHA, perf = L.PERF_NONE, type = "slider", min = 0.6, max = 1, step = 0.05, default = 0.6,
            requires = REQUIRES.fade },
        { name = "showPlayer", label = L.CVAR_LABEL_SHOW_PLAYER, desc = L.DESC_SHOW_PLAYER, perf = L.PERF_NONE, type = "toggle", min = 0, max = 1, default = 1 },
        { name = "headerInterface", label = L.SECTION_INTERFACE, type = "header" },
        { name = "uiHalfPixelFix", label = L.CVAR_LABEL_UI_HALF_PIXEL, desc = L.DESC_UI_HALF_PIXEL, perf = L.PERF_NONE, type = "toggle", min = 0, max = 1, default = 1 },
        { name = "uiPixelSnap", label = L.CVAR_LABEL_UI_PIXEL_SNAP, desc = L.DESC_UI_PIXEL_SNAP, perf = L.PERF_NEGLIGIBLE, type = "toggle", min = 0, max = 1, default = 1 },
        { name = "uiTextureSampling", label = L.CVAR_LABEL_UI_SAMPLING, desc = L.DESC_UI_SAMPLING, perf = L.PERF_UI_SAMPLING, type = "mode", default = 1, modes = {
            { value = 0, label = L.MODE_DISABLED },
            { value = 1, label = L.MODE_SAMPLING_BOX },
            { value = 2, label = L.MODE_SAMPLING_TENT },
        }},
        { name = "portraitResolution", label = L.CVAR_LABEL_PORTRAIT, desc = L.DESC_PORTRAIT, perf = L.PERF_PORTRAIT, type = "dropdown", default = 64, options = {
            [64] = "64", [128] = "128", [256] = "256", [512] = "512", [1024] = "1024", [2048] = "2048",
        }},
        { name = "headerFonts", label = L.SECTION_FONTS, type = "header" },
        { name = "MSDFMode", label = L.CVAR_LABEL_MSDF_MODE, desc = L.DESC_MSDF, perf = L.PERF_MSDF, type = "mode", default = 1, modes = {
            { value = 0, label = L.MODE_DISABLED },
            { value = 1, label = L.MODE_ENABLED },
            { value = 2, label = L.MODE_MSDF_ENABLED_UNSAFE },
        }},
        { name = "MSDFOutlinePass", label = L.CVAR_LABEL_MSDF_OUTLINE, desc = L.DESC_MSDF_OUTLINE, perf = L.PERF_MSDF_OUTLINE, type = "toggle", min = 0, max = 1, default = 1,
            requires = REQUIRES.msdf },
        { name = "noteMSDFPregen", label = L.CVAR_LABEL_MSDF_PREGEN, desc = L.DESC_MSDF_PREGEN, type = "description", requires = REQUIRES.msdf },
        { name = "noteMSDFBlacklist", label = L.CVAR_LABEL_MSDF_BLACKLIST, desc = L.DESC_MSDF_BLACKLIST, type = "description", requires = REQUIRES.msdf },
        { name = "headerAlwaysOn", label = L.SECTION_ALWAYS_ON, type = "header" },
        { name = "noteProjectedTextures", label = L.CVAR_LABEL_PROJECTED_TEXTURES, desc = L.DESC_PROJECTED_TEXTURES, type = "description" },
    },
    Nameplates = {
        { name = "headerNotes", label = L.SECTION_NOTES, type = "header" },
        { name = "noteNameplateAddons", label = L.CVAR_LABEL_NAMEPLATE_ADDONS, desc = L.DESC_NAMEPLATE_ADDONS, type = "description" },
        { name = "headerDisplay", label = L.SECTION_DISPLAY, type = "header" },
        { name = "nameplateDistance", label = L.CVAR_LABEL_NAMEPLATE_DISTANCE, desc = L.DESC_NAMEPLATE_DISTANCE, perf = L.PERF_NAMEPLATE_DISTANCE, type = "slider", min = 41, max = 200, step = 1, default = 41 },
        { name = "nameplatePlacement", label = L.CVAR_LABEL_PLACEMENT, desc = L.DESC_PLACEMENT, perf = L.PERF_NONE, type = "slider", min = -1, max = 2, step = 0.01, default = 0 },
        { name = "nameplateNonTargetAlpha", label = L.CVAR_LABEL_NONTARGET_ALPHA, desc = L.DESC_NONTARGET_ALPHA, perf = L.PERF_NONE, type = "slider", min = 0, max = 1, step = 0.01, default = 0.5 },
        { name = "nameplateAlphaSpeed", label = L.CVAR_LABEL_ALPHA_SPEED, desc = L.DESC_ALPHA_BLEND, perf = L.PERF_NONE, type = "slider", min = 0.01, max = 1, step = 0.01, default = 0.25 },
        { name = "headerOcclusion", label = L.SECTION_OCCLUSION, type = "header" },
        { name = "nameplateOcclusionAlpha", label = L.CVAR_LABEL_OCCLUSION_ALPHA, desc = L.DESC_OCCLUSION_ALPHA, perf = L.PERF_OCCLUSION_ALPHA, type = "slider", min = -1, max = 1, step = 0.01, default = 1 },
        { name = "nameplateOcclusionMode", label = L.CVAR_LABEL_OCCLUSION_MODE, desc = L.DESC_OCCLUSION_MODE, perf = L.PERF_OCCLUSION_MODE, type = "mode", default = 0,
            requires = REQUIRES.occlusion, modes = {
            { value = 0, label = L.MODE_OCCLUSION_ALWAYS },
            { value = 1, label = L.MODE_OCCLUSION_NOCOMBAT },
        }},
        { name = "headerStacking", label = L.SECTION_STACKING, type = "header" },
        { name = "nameplateStacking", label = L.CVAR_LABEL_STACKING_MODE, desc = L.DESC_STACKING_MODE, perf = L.PERF_STACKING, type = "mode", default = 0, modes = {
            { value =  0, label = L.MODE_STACKING_DISABLED },
            { value =  1, label = L.MODE_STACKING_ALL },
            { value =  2, label = L.MODE_STACKING_ENEMY },
            { value =  3, label = L.MODE_STACKING_FRIENDLY },
            { value = -1, label = L.MODE_STACKING_SMART_ALL },
            { value = -2, label = L.MODE_STACKING_SMART_ENEMY },
            { value = -3, label = L.MODE_STACKING_SMART_FRIENDLY },
        }},
        { name = "nameplateBandY", label = L.CVAR_LABEL_Y_SPACE, desc = L.DESC_Y_SPACE, perf = L.PERF_NONE, type = "slider", min = 0.1, max = 1.5, step = 0.01, default = 1, requires = REQUIRES.stacking },
        { name = "nameplateBandX", label = L.CVAR_LABEL_X_SPACE, desc = L.DESC_X_SPACE, perf = L.PERF_NONE, type = "slider", min = 0.1, max = 1, step = 0.01, default = 0.7, requires = REQUIRES.stacking },
        { name = "nameplateRaiseDistance", label = L.CVAR_LABEL_MAX_RAISE_DISTANCE, desc = L.DESC_MAX_RAISE_DISTANCE, perf = L.PERF_NONE, type = "slider", min = 1, max = 20, step = 0.25, default = 8, requires = REQUIRES.stacking },
        { name = "nameplatePullDistance", label = L.CVAR_LABEL_MAX_PULL_DISTANCE, desc = L.DESC_MAX_PULL_DISTANCE, perf = L.PERF_NONE, type = "slider", min = 0, max = 0.75, step = 0.01, default = 0.25, requires = REQUIRES.stacking },
        { name = "nameplateRaiseSpeed", label = L.CVAR_LABEL_SPEED_RAISE, desc = L.DESC_SPEED_RAISE, perf = L.PERF_NONE, type = "slider", min = 1, max = 250, step = 1, default = 100, requires = REQUIRES.stacking },
        { name = "nameplateLowerSpeed", label = L.CVAR_LABEL_SPEED_LOWER, desc = L.DESC_SPEED_LOWER, perf = L.PERF_NONE, type = "slider", min = 1, max = 250, step = 1, default = 100, requires = REQUIRES.stacking },
        { name = "nameplatePullSpeed", label = L.CVAR_LABEL_SPEED_PULL, desc = L.DESC_SPEED_PULL, perf = L.PERF_NONE, type = "slider", min = 1, max = 250, step = 1, default = 50, requires = REQUIRES.stacking },
        { name = "nameplateInertia", label = L.CVAR_LABEL_INERTIA, desc = L.DESC_INERTIA, perf = L.PERF_NONE, type = "slider", min = 0, max = 20, step = 0.1, default = 1, requires = REQUIRES.stacking },
        { name = "nameplateHysteresisDecay", label = L.CVAR_LABEL_HYST_DECAY, desc = L.DESC_HYST_DECAY, perf = L.PERF_NONE, type = "slider", min = 0.25, max = 30, step = 0.05, default = 1, requires = REQUIRES.stacking },
        { name = "headerClamping", label = L.SECTION_CLAMPING, type = "header" },
        { name = "nameplateClampMode", label = L.CVAR_LABEL_CLAMP_MODE, desc = L.DESC_CLAMP_MODE, perf = L.PERF_NEGLIGIBLE, type = "mode", default = 0, modes = {
            { value = 0, label = L.MODE_DISABLED },
            { value = 1, label = L.MODE_CLAMP_ALL },
            { value = 2, label = L.MODE_CLAMP_BOSSES },
            { value = 3, label = L.MODE_CLAMP_ALL_EDGES },
            { value = 4, label = L.MODE_CLAMP_BOSSES_EDGES },
        }},
        { name = "nameplateClampModeFilter", label = L.CVAR_LABEL_CLAMP_FILTER, desc = L.DESC_CLAMP_FILTER, perf = L.PERF_NEGLIGIBLE, type = "mode", default = 0, requires = REQUIRES.clamping, modes = {
            { value = 0, label = L.MODE_CLAMP_FILTER_NONE },
            { value = 1, label = L.MODE_CLAMP_FILTER_TARGET },
            { value = 2, label = L.MODE_CLAMP_FILTER_COMBAT },
            { value = 3, label = L.MODE_CLAMP_FILTER_TARGET_COMBAT },
        }},
        { name = "nameplateClampModeVOffset", label = L.CVAR_LABEL_V_OFFSET, desc = L.DESC_V_OFFSET, perf = L.PERF_NONE, type = "slider", min = 0, max = 0.05, step = 0.01, default = 0.01, requires = REQUIRES.clamping },
        { name = "nameplateClampModeHOffset", label = L.CVAR_LABEL_H_OFFSET, desc = L.DESC_H_OFFSET, perf = L.PERF_NONE, type = "slider", min = 0, max = 0.1, step = 0.01, default = 0.01,
            requires = REQUIRES.clampAllSides },
        { name = "headerMouse", label = L.SECTION_MOUSE, type = "header" },
        { name = "nameplateMouseMode", label = L.CVAR_LABEL_MOUSEOVER, desc = L.DESC_MOUSEOVER, perf = L.PERF_NEGLIGIBLE, type = "mode", default = 0, modes = {
            { value = 0, label = L.MODE_DISABLED },
            { value = 1, label = L.MODE_MOUSE_CLICKTHROUGH_ENEMY },
            { value = 2, label = L.MODE_MOUSE_CLICKTHROUGH_ENEMY_RAISE_FRIENDLY },
            { value = 3, label = L.MODE_MOUSE_CLICKTHROUGH_ENEMY_RAISE_FRIENDLY_COMBAT },
            { value = 4, label = L.MODE_MOUSE_CLICKTHROUGH_FRIENDLY },
            { value = 5, label = L.MODE_MOUSE_CLICKTHROUGH_FRIENDLY_RAISE_ENEMY },
            { value = 6, label = L.MODE_MOUSE_CLICKTHROUGH_FRIENDLY_RAISE_ENEMY_COMBAT },
            { value = 7, label = L.MODE_MOUSE_RAISE_OCCLUDED },
            { value = 8, label = L.MODE_MOUSE_RAISE_OCCLUDED_COMBAT },
        }},
        { name = "nameplateMouseFreeze", label = L.CVAR_LABEL_MOUSE_FREEZE, desc = L.DESC_MOUSE_FREEZE, perf = L.PERF_NEGLIGIBLE, type = "mode", default = 0, modes = {
            { value = 0, label = L.MODE_DISABLED },
            { value = 1, label = L.MODE_FREEZE_ALWAYS },
            { value = 2, label = L.MODE_FREEZE_COMBAT },
        }},
        { name = "nameplateMouseFreezeGrace", label = L.CVAR_LABEL_FREEZE_GRACE, desc = L.DESC_FREEZE_GRACE, perf = L.PERF_NONE, type = "slider", min = 0, max = 1, step = 0.01, default = 0.15, requires = REQUIRES.freeze },
        { name = "nameplateMouseFreezeTime", label = L.CVAR_LABEL_FREEZE_TIME, desc = L.DESC_FREEZE_TIME, perf = L.PERF_NONE, type = "slider", min = 0, max = 2, step = 0.05, default = 0.3, requires = REQUIRES.freeze },
        { name = "headerClickableArea", label = L.SECTION_CLICKABLE_AREA, type = "header" },
        { name = "nameplateHitboxAnchor", label = L.CVAR_LABEL_HITBOX_ANCHOR, desc = L.DESC_HITBOX_ANCHOR, perf = L.PERF_NONE, type = "mode", default = 1,
            requires = REQUIRES.shortHitbox, modes = {
            { value = 0, label = L.MODE_HITBOX_TOP },
            { value = 1, label = L.MODE_HITBOX_CENTER },
            { value = 2, label = L.MODE_HITBOX_BOTTOM },
        }},
        { name = "nameplateHitboxHeightE", label = L.CVAR_LABEL_HITBOX_HEIGHT_ENEMY, desc = L.DESC_HITBOX_HEIGHT_ENEMY, perf = L.PERF_NONE, type = "slider", min = 0, max = 1, step = 0.01, default = 1 },
        { name = "nameplateHitboxWidthE", label = L.CVAR_LABEL_HITBOX_WIDTH_ENEMY, desc = L.DESC_HITBOX_WIDTH_ENEMY, perf = L.PERF_NONE, type = "slider", min = 0, max = 1, step = 0.01, default = 1 },
        { name = "nameplateHitboxHeightF", label = L.CVAR_LABEL_HITBOX_HEIGHT_FRIENDLY, desc = L.DESC_HITBOX_HEIGHT_FRIENDLY, perf = L.PERF_NONE, type = "slider", min = 0, max = 1, step = 0.01, default = 1 },
        { name = "nameplateHitboxWidthF", label = L.CVAR_LABEL_HITBOX_WIDTH_FRIENDLY, desc = L.DESC_HITBOX_WIDTH_FRIENDLY, perf = L.PERF_NONE, type = "slider", min = 0, max = 1, step = 0.01, default = 1 },
        { name = "headerAddonAuthors", label = L.SECTION_ADDON_AUTHORS, type = "header" },
        { name = "info", label = L.CVAR_LABEL_INFO, desc = L.DESC_INFO, type = "description" },
    },
    TextToSpeech = {
        { name = "ttsVoice", label = L.CVAR_LABEL_TTS_VOICE, desc = L.DESC_TTS_VOICE, perf = L.PERF_TTS, type = "dropdown", default = 1, options = ACVar.TTS_VOICES },
        { name = "ttsVolume", label = L.CVAR_LABEL_TTS_VOLUME, desc = L.DESC_TTS_VOLUME, perf = L.PERF_TTS, type = "slider", min = 0, max = 100, step = 1, default = 100 },
        { name = "ttsSpeed", label = L.CVAR_LABEL_TTS_SPEED, desc = L.DESC_TTS_SPEED, perf = L.PERF_TTS, type = "slider", min = -10, max = 10, step = 1, default = 0 },
    },
    Interaction = {
        { name = "noteInteractKeybind", label = L.CVAR_LABEL_INTERACT_KEYBIND, desc = L.DESC_INTERACT_KEYBIND, type = "description" },
        { name = "interactionMode", label = L.CVAR_LABEL_INTERACTION_MODE, desc = L.DESC_INTERACTION_MODE, perf = L.PERF_NEGLIGIBLE, type = "mode", default = 1, modes = {
            { value = 0, label = L.MODE_LABEL_PLAYER_RADIUS },
            { value = 1, label = L.MODE_LABEL_CONE_ANGLE },
        }},
        { name = "interactionAngle", label = L.CVAR_LABEL_INTERACTION_ANGLE, desc = L.DESC_INTERACTION_ANGLE, perf = L.PERF_NONE, type = "slider", min = 15, max = 160, step = 1, default = 60,
            requires = REQUIRES.cone },
        { name = "interactionHighlight", label = L.CVAR_LABEL_INTERACTION_HIGHLIGHT, desc = L.DESC_INTERACTION_HIGHLIGHT, perf = L.PERF_NEGLIGIBLE, type = "toggle", min = 0, max = 1, default = 1 },
    },
    Other = {
        { name = "headerWorldObjects", label = L.SECTION_WORLD_OBJECTS, type = "header" },
        { name = "objectHighlightMode", label = L.CVAR_LABEL_OBJ_HIGHLIGHT, desc = L.DESC_OBJ_HIGHLIGHT, perf = L.PERF_OBJ_HIGHLIGHT, type = "mode", default = 0, modes = {
            { value = 0, label = L.MODE_DISABLED },
            { value = 1, label = L.MODE_ENABLED },
            { value = 2, label = L.MODE_HIGHLIGHTS_TRACKED },
        }},
        { name = "headerLogs", label = L.SECTION_LOGS, type = "header" },
        { name = "chatLogSessionKey", label = L.CVAR_LABEL_CHAT_LOG, desc = L.DESC_SESSION_LOG, perf = L.PERF_NONE, type = "toggle", min = 0, max = 1, default = 1 },
        { name = "combatLogSessionKey", label = L.CVAR_LABEL_COMBAT_LOG, desc = L.DESC_SESSION_LOG, perf = L.PERF_NONE, type = "toggle", min = 0, max = 1, default = 1 },
        { name = "headerMacros", label = L.SECTION_MACROS, type = "header" },
        { name = "noteMacroConditionals", label = L.CVAR_LABEL_MACRO_CONDITIONALS, desc = L.DESC_MACRO_CONDITIONALS, type = "description" },
        { name = "headerScreenshots", label = L.SECTION_SCREENSHOTS, type = "header" },
        { name = "noteFrameCapture", label = L.CVAR_LABEL_FRAME_CAPTURE, desc = L.DESC_FRAME_CAPTURE, type = "description" },
        { name = "headerAlwaysOnOther", label = L.SECTION_ALWAYS_ON, type = "header" },
        { name = "noteClipboard", label = L.CVAR_LABEL_CLIPBOARD, desc = L.DESC_CLIPBOARD, type = "description" },
    },
    -- Read-only entries: "header" starts a section, "description" is text, "link" is a copyable URL
    About = {
        { name = "headerOverview", label = L.SECTION_OVERVIEW, type = "header" },
        { name = "aboutMod", label = L.ABOUT_TITLE, desc = L.ABOUT_DESC, type = "description" },
        { name = "aboutVersion", label = L.ABOUT_VERSION, desc = versionText(), type = "description" },
        { name = "headerLinks", label = L.SECTION_LINKS, type = "header" },
        { name = "linkGitHub", label = L.LINK_GITHUB, desc = L.LINK_GITHUB_DESC, type = "link", url = ACVar.URLS.GITHUB },
        { name = "linkReleases", label = L.LINK_RELEASES, desc = L.LINK_RELEASES_DESC, type = "link", url = ACVar.URLS.RELEASES },
        { name = "linkDocs", label = L.LINK_DOCS, desc = L.LINK_DOCS_DESC, type = "link", url = ACVar.URLS.DOCS },
    },
}

-- Entries that only display information and hold no CVar.
function ACVar.IsReadOnly(cvarDef)
    return cvarDef.type == "header" or cvarDef.type == "description" or cvarDef.type == "link"
end

-- Tab order; every category id in CVARS must be listed here.
ACVar.CATEGORY_ORDER = { "Rendering", "Nameplates", "TextToSpeech", "Interaction", "Other", "About" }

-- Tab selected when the window is first built; later opens keep the last selected tab.
ACVar.DEFAULT_CATEGORY = "About"

-- Localized tab names, by category id.
ACVar.CATEGORY_NAMES = {
    Rendering = L.CATEGORY_RENDERING,
    Nameplates = L.CATEGORY_NAMEPLATES,
    TextToSpeech = L.CATEGORY_TEXT_TO_SPEECH,
    Interaction = L.CATEGORY_INTERACTION,
    Other = L.CATEGORY_OTHER,
    About = L.CATEGORY_ABOUT,
}

-- Former tab names that now live in another tab, so external callers keep working.
local CATEGORY_ALIASES = { camera = "Rendering", ui = "Rendering" }

local function normalizeTabName(name)
    return (name:gsub("%s+", "")):lower()
end

-- Resolves a tab name to its category id: the id itself, case and spaces ignored ("Text to Speech"),
-- the English or localized tab name, or a former tab name. Returns nil when nothing matches.
function ACVar.ResolveCategory(name)
    if type(name) ~= "string" then return nil end
    if ACVar.CVARS[name] then return name end
    local key = normalizeTabName(name)
    for _, id in ipairs(ACVar.CATEGORY_ORDER) do
        if key == normalizeTabName(id) or key == normalizeTabName(ACVar.CATEGORY_NAMES[id]) then
            return id
        end
    end
    return CATEGORY_ALIASES[key]
end
