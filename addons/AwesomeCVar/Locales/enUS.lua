-- File: enUS.lua
-- Language: English (US)
local _, AwesomeCVar = ...

if not AwesomeCVar.L then
    AwesomeCVar.L = {}
end

local L = AwesomeCVar.L

if GetLocale() == "enUS" then
    -- General
    L.ADDON_NAME = "AwesomeCVar"
    L.ADDON_NAME_SHORT = "Awesome CVar"
    L.MAIN_FRAME_TITLE = "Awesome CVar Manager"
    L.RESET_TO = "Reset to %s"
	L.MINIMAP_ICON = "Minimap Icon"
	L.MINIMAP_TOOLTIP = "Click to open\nDrag to move"
	L.GAME_MENU_BUTTON = "Game Menu Button"
	L.OPEN_ADDON = "Open AwesomeCVar"

    -- Popups
    L.RELOAD_POPUP_TITLE = "Reload UI Required"
    L.RELOAD_POPUP_TEXT = "One or more of the changes you have made require a ReloadUI to take effect."
    L.RESET_POPUP_TITLE = "Confirm Default Reset"
    L.RESET_POPUP_TEXT = "Are you sure you want to reset all values back to their defaults?"

    -- Chat Messages
    L.MSG_LOADED = "Awesome CVar loaded! Type /awesome to open the manager."
    L.MSG_FRAME_RESET = "Frame position has been reset to the center."
    L.MSG_SET_VALUE = "Set %s to %s."
    L.MSG_FRAME_CREATE_ERROR = "AwesomeCVar frame could not be created!"
    L.MSG_UNKNOWN_COMMAND = "Unknown command. Type /awesome help for available commands."
    L.MSG_HELP_HEADER = "Awesome CVar Commands:"
    L.MSG_HELP_TOGGLE = "/awesome - Toggle the CVar manager"
    L.MSG_HELP_SHOW = "/awesome show - Show the CVar manager"
    L.MSG_HELP_HIDE = "/awesome hide - Hide the CVar manager"
    L.MSG_HELP_RESET = "/awesome reset - Reset frame position to center"
    L.MSG_HELP_HELP = "/awesome help - Show this help message"

    -- CVar Categories
    L.CATEGORY_CAMERA = "Camera"
    L.CATEGORY_NAMEPLATES = "Nameplates"
    L.CATEGORY_TEXT_TO_SPEECH = "Text to Speech"
    L.CATEGORY_INTERACTION = "Interaction"
    L.CATEGORY_OTHER = "Other"

    -- CVar Labels & Descriptions
    L.CVAR_LABEL_INFO = "Notes"
    L.CVAR_LABEL_TTS_VOICE = "TTS Voice"
    L.CVAR_LABEL_TTS_VOLUME = "TTS Volume"
    L.CVAR_LABEL_TTS_SPEED = "TTS Rate"
    L.CVAR_LABEL_CAMERA_FOV = "Camera FoV"
    L.CVAR_LABEL_STACKING_MODE = "Nameplate Stacking Mode"
    L.CVAR_LABEL_MOUSEOVER = "Nameplate Mouseover Mode"
    L.CVAR_LABEL_OCCLUSION_ALPHA = "Nameplate Occlusion Alpha"
    L.CVAR_LABEL_OCCLUSION_MODE = "Nameplate Occlusion Mode"
    L.CVAR_LABEL_NONTARGET_ALPHA = "Nameplate Non-Target Alpha"
    L.CVAR_LABEL_ALPHA_SPEED = "Nameplate Alpha Blending Speed"
    L.CVAR_LABEL_CLAMP_MODE = "Nameplate Clamp Mode"
    L.CVAR_LABEL_NAMEPLATE_DISTANCE = "Nameplate Display Distance"
    L.CVAR_LABEL_MAX_RAISE_DISTANCE = "Nameplate Stacking Max Vertical Push Distance"
    L.CVAR_LABEL_MAX_PULL_DISTANCE = "Nameplate Stacking Max Horizontal Pull Distance"
    L.CVAR_LABEL_X_SPACE = "Nameplate X Space"
    L.CVAR_LABEL_Y_SPACE = "Nameplate Y Space"
    L.CVAR_LABEL_V_OFFSET = "Nameplate Clamped Vertical Offset"
    L.CVAR_LABEL_H_OFFSET = "Nameplate Clamped Horizontal Offset"
    L.CVAR_LABEL_PLACEMENT = "Nameplate Placement Offset"
    L.CVAR_LABEL_SPEED_RAISE = "Nameplate Stacking Vertical Raise Speed"
    L.CVAR_LABEL_SPEED_LOWER = "Nameplate Stacking Vertical Lower Speed"
    L.CVAR_LABEL_SPEED_PULL = "Nameplate Stacking Horizontal Pull Speed"
    L.CVAR_LABEL_INERTIA = "Nameplate Stacking Inertia"
	L.CVAR_LABEL_HYST_DECAY = "Nameplate Pair Breakup Rate"
    L.CVAR_LABEL_HITBOX_ANCHOR = "Nameplate Hitbox Anchor"
    L.CVAR_LABEL_HITBOX_HEIGHT_ENEMY = "Enemy Nameplate Hitbox Height"
    L.CVAR_LABEL_HITBOX_WIDTH_ENEMY = "Enemy Nameplate Hitbox Width"
    L.CVAR_LABEL_HITBOX_HEIGHT_FRIENDLY = "Friendly Nameplate Hitbox Height"
    L.CVAR_LABEL_HITBOX_WIDTH_FRIENDLY = "Friendly Nameplate Hitbox Width"
    L.CVAR_LABEL_INTERACTION_MODE = "Interaction Mode"
    L.CVAR_LABEL_INTERACTION_ANGLE = "Interaction Cone Angle (deg)"
    L.CVAR_LABEL_SHOW_PLAYER = "Player's Character Model Rendering"
    L.CVAR_LABEL_MSDF_MODE = "Font Rendering Mode (Requires Restart)"
    L.CVAR_LABEL_OBJ_HIGHLIGHT = "Object Highlighting"
    L.CVAR_LABEL_CAMERA_INDIRECT_VISIBILITY = "Camera Indirect Visibility"
	L.CVAR_LABEL_CAMERA_INDIRECT_ALPHA = "Camera Indirect Alpha"
	L.CVAR_LABEL_CAMERA_DISTANCE_MAX = "Camera Distance"
	L.CVAR_LABEL_PORTRAIT = "Portrait Resolution"
    L.CVAR_LABEL_CHAT_LOG = "Chat Log Session Keying"
    L.CVAR_LABEL_COMBAT_LOG = "Combat Log Session Keying"

    L.DESC_INFO = "All nameplates include additional methods:\n- SetStackingEnabled(bool)\n- GetStackingEnabled()\n\nExample usage:\nif UnitExists('target') then\n C_NamePlate.GetNamePlateForUnit('target'):SetStackingEnabled(false)\nend\n\nNote: It is too early\nto call upon these methods at NAME_PLATE_CREATED,\nuse NAME_PLATE_UNIT_ADDED instead.\n"
    L.DESC_STACKING_MODE = "'Smart' mode allows nameplates to bypass the stacking push if there is sufficient space below, resulting in a tighter layout at the cost of more frequent rearrangements."
	L.DESC_MOUSEOVER = "'Raise' options elevate the mouseover nameplate's frame level, making it appear above all nameplates with lower frame levels.\nThe current target's nameplate frame level remains higher than any raised mouseover nameplate."
	L.DESC_INERTIA = "Controls the physical weight of nameplate movement during stacking.\nHigher values increase responsiveness; lower values produce heavier, more damped motion."
	L.DESC_HYST_DECAY = "Controls how quickly stacking pairs dissolve once nameplates are no longer overlapping.\nHigher values cause faster separation; lower values keep pairs committed longer."
	L.DESC_PLACEMENT = "A vertical offset ratio that displaces nameplates from their default anchor point."
	L.DESC_HITBOX_ANCHOR = "Sets the vertical origin point of the nameplate's clickable area.\nAdjust this to match how your UI addon anchors its nameplate frames (e.g. top-edge or bottom-edge anchoring)."
    L.DESC_ALPHA_BLEND = "Controls how fast nameplates animate toward new opacity targets (1 = Instant)."
    L.DESC_OCCLUSION_ALPHA = "Controls the opacity of nameplates when they are blocked by obstacles or terrain. Positive values multiply the current non-target alpha, negative values enforce a strict maximum ceiling on the occluded alpha."
	L.DESC_OCCLUSION_MODE = "Controls when nameplates are hidden or faded when blocked by obstacles or terrain."
	L.DESC_CAMERA_INDIRECT_VISIBILITY = "When enabled, the camera can move freely through certain world objects rather than being blocked by them."
    L.DESC_CAMERA_INDIRECT_ALPHA = "Sets the transparency level of objects that come between the camera and the player character."
    L.DESC_CAMERA_DISTANCE_MAX = "Sets the maximum distance the camera can zoom out from the player."
	L.DESC_MSDF = "Enables vector-based font rendering, dramatically improving glyph quality."
	L.DESC_OBJ_HIGHLIGHT = "Forces glowing sparkles on resources (herbs/ore) and interactive objects like crates or bounty boards."
	L.DESC_PORTRAIT = "Increases the rendering texture resolution for all portraits in the game."
    L.DESC_SESSION_LOG = "Automatically appends a timestamp to the log file name."

    -- CVar Mode Labels
	L.MODE_DISABLED = "Disabled"
	L.MODE_ENABLED = "Enabled"

    L.MODE_STACKING_DISABLED = "Overlapping"
    L.MODE_STACKING_ALL = "Stacking (All)"
    L.MODE_STACKING_ENEMY = "Stacking (Enemies)"
    L.MODE_STACKING_FRIENDLY = "Stacking (Friends)"
    L.MODE_STACKING_SMART_ALL = "Smart Stacking (All)"
    L.MODE_STACKING_SMART_ENEMY = "Smart Stacking (Enemies)"
    L.MODE_STACKING_SMART_FRIENDLY = "Smart Stacking (Friends)"

    L.MODE_MOUSE_DISABLED = "Default"
    L.MODE_MOUSE_CLICKTHROUGH_ENEMY = "Click-through Enemies"
    L.MODE_MOUSE_CLICKTHROUGH_ENEMY_RAISE_FRIENDLY = "CT Enemy + Raise Friend"
    L.MODE_MOUSE_CLICKTHROUGH_ENEMY_RAISE_FRIENDLY_COMBAT = "CT Enemy + Raise Friend (Combat)"
    L.MODE_MOUSE_CLICKTHROUGH_FRIENDLY = "Click-through Friends"
    L.MODE_MOUSE_CLICKTHROUGH_FRIENDLY_RAISE_ENEMY = "CT Friend + Raise Enemy"
    L.MODE_MOUSE_CLICKTHROUGH_FRIENDLY_RAISE_ENEMY_COMBAT = "CT Friend + Raise Enemy (Combat)"
    L.MODE_MOUSE_RAISE_OCCLUDED = "Always Raise Occluded"
    L.MODE_MOUSE_RAISE_OCCLUDED_COMBAT = "Raise Occluded (Combat Only)"

    L.MODE_CLAMP_ALL = "Clamp All (Top Only)"
    L.MODE_CLAMP_BOSSES = "Clamp Bosses Only (Top Only)"
    L.MODE_CLAMP_ALL_EDGES = "Clamp All (All Sides)"
    L.MODE_CLAMP_BOSSES_EDGES = "Clamp Bosses Only (All Sides)"

	L.MODE_OCCLUSION_ALWAYS = "Always Occlude"
	L.MODE_OCCLUSION_NOCOMBAT = "Only Out of Combat"

    L.MODE_HITBOX_TOP = "Top"
    L.MODE_HITBOX_CENTER = "Center"
    L.MODE_HITBOX_BOTTOM = "Bottom"

    L.MODE_LABEL_PLAYER_RADIUS = "Player Radius 20yd"
    L.MODE_LABEL_CONE_ANGLE = "Cone Angle (deg) within 20yd"

	L.MODE_MSDF_ENABLED_UNSAFE = "Enabled (unsafe fonts) — due to how distance fields are calculated, some fonts with self-intersecting contours (e.g., 'diediedie') may break."

	L.MODE_HIGHLIGHTS_TRACKED = "Tracked Only"
end