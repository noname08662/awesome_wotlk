-- File: Constants.lua
-- Holds all static definitions for the addon.

local _, ACVar = ...
local L = ACVar.L or {} -- Get the locale table loaded previously
_G["AwesomeCVar"] = {} -- Public API table

-- This table holds constants used throughout the addon.
ACVar.CONSTANTS = {
    ADDON_NAME = L.ADDON_NAME,
    COLORS = {
        SUCCESS = "|cff00ff00",
        HIGHLIGHT = "|cffffd100",
        VALUE = "|cff00ccff",
        ERROR = "|cffff0000",
        RESET = "|r",
        TAB_ACTIVE = {1, 1, 0},
        TAB_INACTIVE = {0.8, 0.8, 0.8},
        DESC_TEXT = {0.6, 0.6, 0.6}
    },
    FRAME = {
        MAIN_WIDTH = 768,
        MAIN_HEIGHT = 580,
        POPUP_WIDTH = 350,
        POPUP_HEIGHT = 120,
        BUTTON_WIDTH = 100,
        BUTTON_HEIGHT = 25,
        TAB_HEIGHT = 25
    }
}

-- This table holds TTS_VOICES that is populated at runtime.
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

-- This table defines every CVar control that will appear in the UI.
ACVar.CVARS = {
    [L.CATEGORY_CAMERA] = {
        { name = "cameraFov", label = L.CVAR_LABEL_CAMERA_FOV, type = "slider", min = 60, max = 150, default = 100 },
        { name = "cameraDistanceMax", label = L.CVAR_LABEL_CAMERA_DISTANCE_MAX, desc = L.DESC_CAMERA_DISTANCE_MAX, type = "slider", min = 0, max = 50, step = 1, default = 15 },
        { name = "cameraIndirectVisibility", label = L.CVAR_LABEL_CAMERA_INDIRECT_VISIBILITY, desc = L.DESC_CAMERA_INDIRECT_VISIBILITY, type = "toggle", min = 0, max = 1 },
        { name = "cameraIndirectAlpha", label = L.CVAR_LABEL_CAMERA_INDIRECT_ALPHA, desc = L.DESC_CAMERA_INDIRECT_ALPHA, type = "slider", min = 0.6, max = 1, step = 0.05, default = 0.6 },
    },
    [L.CATEGORY_NAMEPLATES] = {
        { name = "info", label = L.CVAR_LABEL_INFO, desc = L.DESC_INFO, type = "description" },
        -- Mode (radio) controls
        { name = "nameplateStacking", label = L.CVAR_LABEL_STACKING_MODE, desc = L.DESC_STACKING_MODE, type = "mode", default = 0, modes = {
            { value =  0, label = L.MODE_STACKING_DISABLED },
            { value =  1, label = L.MODE_STACKING_ALL },
            { value =  2, label = L.MODE_STACKING_ENEMY },
            { value =  3, label = L.MODE_STACKING_FRIENDLY },
            { value = -1, label = L.MODE_STACKING_SMART_ALL },
            { value = -2, label = L.MODE_STACKING_SMART_ENEMY },
            { value = -3, label = L.MODE_STACKING_SMART_FRIENDLY },
        }},
        { name = "nameplateMouseMode", label = L.CVAR_LABEL_MOUSEOVER, desc = L.DESC_MOUSEOVER, type = "mode", default = 0, modes = {
            { value = 0, label = L.MODE_MOUSE_DISABLED },
            { value = 1, label = L.MODE_MOUSE_CLICKTHROUGH_ENEMY },
            { value = 2, label = L.MODE_MOUSE_CLICKTHROUGH_ENEMY_RAISE_FRIENDLY },
            { value = 3, label = L.MODE_MOUSE_CLICKTHROUGH_ENEMY_RAISE_FRIENDLY_COMBAT },
            { value = 4, label = L.MODE_MOUSE_CLICKTHROUGH_FRIENDLY },
            { value = 5, label = L.MODE_MOUSE_CLICKTHROUGH_FRIENDLY_RAISE_ENEMY },
            { value = 6, label = L.MODE_MOUSE_CLICKTHROUGH_FRIENDLY_RAISE_ENEMY_COMBAT },
            { value = 7, label = L.MODE_MOUSE_RAISE_OCCLUDED },
            { value = 8, label = L.MODE_MOUSE_RAISE_OCCLUDED_COMBAT },
        }},
        { name = "nameplateClampMode", label = L.CVAR_LABEL_CLAMP_MODE, type = "mode", default = 0, modes = {
            { value = 0, label = L.MODE_DISABLED },
            { value = 1, label = L.MODE_CLAMP_ALL },
            { value = 2, label = L.MODE_CLAMP_BOSSES },
            { value = 3, label = L.MODE_CLAMP_ALL_EDGES },
            { value = 4, label = L.MODE_CLAMP_BOSSES_EDGES },
        }},
        { name = "nameplateHitboxAnchor", label = L.CVAR_LABEL_HITBOX_ANCHOR, desc = L.DESC_HITBOX_ANCHOR, type = "mode", default = 1, modes = {
            { value = 0, label = L.MODE_HITBOX_TOP },
            { value = 1, label = L.MODE_HITBOX_CENTER },
            { value = 2, label = L.MODE_HITBOX_BOTTOM },
		}},
        { name = "nameplateOcclusionMode", label = L.CVAR_LABEL_OCCLUSION_MODE, desc = L.DESC_OCCLUSION_MODE, type = "mode", default = 0, modes = {
            { value = 0, label = L.MODE_OCCLUSION_ALWAYS },
            { value = 1, label = L.MODE_OCCLUSION_NOCOMBAT },
		}},
        -- Sliders
        { name = "nameplatePlacement", label = L.CVAR_LABEL_PLACEMENT, desc = L.DESC_PLACEMENT, type = "slider", min = -1, max = 2, step = 0.01, default = 0.66 },
        { name = "nameplateDistance", label = L.CVAR_LABEL_NAMEPLATE_DISTANCE, type = "slider", min = 41, max = 100, step = 1, default = 41 },
        { name = "nameplateHysteresisDecay", label = L.CVAR_LABEL_HYST_DECAY, desc = L.DESC_HYST_DECAY, type = "slider", min = 0.25, max = 30, step = 0.05, default = 1 },
        { name = "nameplateOcclusionAlpha", label = L.CVAR_LABEL_OCCLUSION_ALPHA, desc = L.DESC_OCCLUSION_ALPHA, type = "slider", min = -1, max = 1, step = 0.01, default = 1 },
        { name = "nameplateNonTargetAlpha", label = L.CVAR_LABEL_NONTARGET_ALPHA, type = "slider", min = 0, max = 1, step = 0.01, default = 0.5 },
        { name = "nameplateAlphaSpeed", label = L.CVAR_LABEL_ALPHA_SPEED, desc = L.DESC_ALPHA_BLEND, type = "slider", min = 0.01, max = 1, step = 0.01, default = 1 },
        { name = "nameplateClampModeVOffset", label = L.CVAR_LABEL_V_OFFSET, type = "slider", min = 0, max = 0.15, step = 0.01, default = 0.1 },
		{ name = "nameplateClampModeHOffset", label = L.CVAR_LABEL_H_OFFSET, type = "slider", min = 0, max = 0.15, step = 0.01, default = 0.01 },
        { name = "nameplateRaiseDistance", label = L.CVAR_LABEL_MAX_RAISE_DISTANCE, type = "slider", min = 1, max = 20, step = 0.25, default = 8 },
        { name = "nameplatePullDistance", label = L.CVAR_LABEL_MAX_PULL_DISTANCE, type = "slider", min = 0, max = 0.75, step = 0.01, default = 0.25 },
        { name = "nameplateBandX", label = L.CVAR_LABEL_X_SPACE, type = "slider", min = 0.1, max = 1, step = 0.01, default = 0.7 },
        { name = "nameplateBandY", label = L.CVAR_LABEL_Y_SPACE, type = "slider", min = 0.1, max = 1.5, step = 0.01, default = 1 },
        { name = "nameplateRaiseSpeed", label = L.CVAR_LABEL_SPEED_RAISE, type = "slider", min = 1, max = 250, step = 1, default = 100 },
        { name = "nameplateLowerSpeed", label = L.CVAR_LABEL_SPEED_LOWER, type = "slider", min = 1, max = 250, step = 1, default = 100 },
        { name = "nameplatePullSpeed", label = L.CVAR_LABEL_SPEED_PULL, type = "slider", min = 1, max = 250, step = 1, default = 50 },
        { name = "nameplateInertia", label = L.CVAR_LABEL_INERTIA, desc = L.DESC_INERTIA, type = "slider", min = 0, max = 20, step = 0.1, default = 1 },
        { name = "nameplateHitboxHeightE", label = L.CVAR_LABEL_HITBOX_HEIGHT_ENEMY, type = "slider", min = 0, max = 1, step = 0.01, default = 1 },
        { name = "nameplateHitboxWidthE", label = L.CVAR_LABEL_HITBOX_WIDTH_ENEMY, type = "slider", min = 0, max = 1, step = 0.01, default = 1 },
        { name = "nameplateHitboxHeightF", label = L.CVAR_LABEL_HITBOX_HEIGHT_FRIENDLY, type = "slider", min = 0, max = 1, step = 0.01, default = 1 },
        { name = "nameplateHitboxWidthF", label = L.CVAR_LABEL_HITBOX_WIDTH_FRIENDLY, type = "slider", min = 0, max = 1, step = 0.01, default = 1 },
    },
    [L.CATEGORY_TEXT_TO_SPEECH] = {
        { name = "ttsVoice", label = L.CVAR_LABEL_TTS_VOICE, type = "dropdown", default = 1, options = ACVar.TTS_VOICES },
        { name = "ttsVolume", label = L.CVAR_LABEL_TTS_VOLUME, type = "slider", min = 0, max = 100, step = 1, default = 100 },
        { name = "ttsSpeed", label = L.CVAR_LABEL_TTS_SPEED, type = "slider", min = -10, max = 10, step = 0.25, default = 0 },
    },
    [L.CATEGORY_INTERACTION] = {
        { name = "interactionMode", label = L.CVAR_LABEL_INTERACTION_MODE, type = "mode", modes = { {value = 0, label = L.MODE_LABEL_PLAYER_RADIUS}, {value = 1, label = L.MODE_LABEL_CONE_ANGLE} }, default = 1 },
        { name = "interactionAngle", label = L.CVAR_LABEL_INTERACTION_ANGLE, type = "slider", min = 1, max = 360, step = 1, default = 90 },
    },
    [L.CATEGORY_OTHER] = {
        { name = "portraitResolution", label = L.CVAR_LABEL_PORTRAIT, desc = L.DESC_PORTRAIT, type = "slider", min = 64, max = 2048, step = 64, default = 64 },
        { name = "showPlayer", label = L.CVAR_LABEL_SHOW_PLAYER, type = "toggle", min = 0, max = 1, default = 1 },
        { name = "MSDFMode", label = L.CVAR_LABEL_MSDF_MODE, desc = L.DESC_MSDF, type = "mode", default = 1, modes = {
            { value = 0, label = L.MODE_DISABLED },
            { value = 1, label = L.MODE_ENABLED },
            { value = 2, label = L.MODE_MSDF_ENABLED_UNSAFE },
		}},
		{ name = "objectHighlightMode", label = L.CVAR_LABEL_OBJ_HIGHLIGHT, desc = L.DESC_OBJ_HIGHLIGHT, type = "mode", default = 0, modes = {
            { value = 0, label = L.MODE_DISABLED },
            { value = 1, label = L.MODE_ENABLED },
            { value = 2, label = L.MODE_HIGHLIGHTS_TRACKED },
		}},
        { name = "chatLogSessionKey", label = L.CVAR_LABEL_CHAT_LOG, desc = L.DESC_SESSION_LOG, type = "toggle", min = 0, max = 1, default = 1 },
        { name = "combatLogSessionKey", label = L.CVAR_LABEL_COMBAT_LOG, desc = L.DESC_SESSION_LOG, type = "toggle", min = 0, max = 1, default = 1 },
    }
}