-- File: enUS.lua
-- Language: English (US)
local addonName, AwesomeCVar = ...

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
    L.CVAR_LABEL_NONTARGET_ALPHA = "Nameplate Non-Target Alpha"
    L.CVAR_LABEL_ALPHA_SPEED = "Nameplate Alpha Blending Speed"
    L.CVAR_LABEL_CLAMP_TOP = "Nameplate Clamp Top Mode"
    L.CVAR_LABEL_NAMEPLATE_DISTANCE = "Nameplate Render Distance"
    L.CVAR_LABEL_MAX_RAISE_DISTANCE = "Nameplate Stacking Max Vertical Raise Distance"
    L.CVAR_LABEL_MAX_PULL_DISTANCE = "Nameplate Stacking Max Horizontal Pull Distance"
    L.CVAR_LABEL_X_SPACE = "Nameplate X Space"
    L.CVAR_LABEL_Y_SPACE = "Nameplate Y Space"
    L.CVAR_LABEL_UPPER_BORDER = "Nameplate Clamped Top Offset"
    L.CVAR_LABEL_PLACEMENT = "Nameplate Placement Offset"
    L.CVAR_LABEL_SPEED_RAISE = "Nameplate Stacking Vertical Raise Speed"
    L.CVAR_LABEL_SPEED_LOWER = "Nameplate Stacking Vertical Lower Speed"
    L.CVAR_LABEL_SPEED_PULL = "Nameplate Stacking Horizontal Pull Speed"
    L.CVAR_LABEL_HITBOX_HEIGHT_ENEMY = "Enemy Nameplate Hitbox Height"
    L.CVAR_LABEL_HITBOX_WIDTH_ENEMY = "Enemy Nameplate Hitbox Width"
    L.CVAR_LABEL_HITBOX_HEIGHT_FRIENDLY = "Friendly Nameplate Hitbox Height"
    L.CVAR_LABEL_HITBOX_WIDTH_FRIENDLY = "Friendly Nameplate Hitbox Width"
    L.CVAR_LABEL_INTERACTION_MODE = "Interaction Mode"
    L.CVAR_LABEL_INTERACTION_ANGLE = "Interaction Cone Angle (deg)"
    L.CVAR_STANCE_PATCH = "Stance/Form Swap Patch"
    L.CVAR_SHOW_PLAYER = "Player's Character Model Rendering"
    L.CVAR_MSDF_MODE = "Font Rendering Mode (Requires Restart)"
    L.CVAR_LABEL_CAMERA_INDIRECT_VISIBILITY = "Camera Indirect Visibility"
	L.CVAR_LABEL_CAMERA_INDIRECT_ALPHA = "Camera Indirect Alpha"
	L.CVAR_LABEL_CAMERA_DISTANCE_MAX = "Camera Distance"

    L.DESC_INFO = "\nAll nameplates include additional methods:\n- SetStackingEnabled(bool)\n- GetStackingEnabled()\n\nExample usage:\nif UnitExists('target') then\n C_NamePlate.GetNamePlateForUnit('target'):SetStackingEnabled(false)\nend\n\nNote: It is too early\nto call upon these methods at NAME_PLATE_CREATED,\nuse NAME_PLATE_UNIT_ADDED instead.\n"
    L.DESC_STACKING_MODE = "0 = Disabled; 1 = Enable All; 2 = Enemy Only; 3 = Friendly Only."
    L.DESC_MOUSEOVER = "0 = Disabled;\n1 = Click-through enemies;\n2 = Click-through enemies, always raise occluded mouseover friendly plates;\n3 = Click-through enemies, raise occluded friendly plates in combat;\n4 = Click-through friendlies;\n5 = Click-through friendlies, always raise occluded mouseover enemy plates;\n6 = Click-through friendlies, raise occluded enemy plates in combat;\n7 = Always raise occluded mouseover plate's level;\n8 = Raise occluded mouseover plate's level in combat."
    L.DESC_ALPHA_BLEND = "Controls how fast nameplates animate toward new opacity targets (1 = Instant)."
    L.DESC_CLAMP_TOP = "0 = Disabled; 1 = Clamp All; 2 = Clamp Bosses Only."
    L.DESC_STANCE_PATCH = "Allows you to change stance/form and cast an ability in a single click when using macros."
    L.DESC_OCCLUSION_ALPHA = "Controls the opacity of nameplates when they are blocked by obstacles or terrain."
    L.DESC_CAMERA_INDIRECT_VISIBILITY = "Allows the camera to pass through certain world objects instead of being blocked."
    L.DESC_CAMERA_INDIRECT_ALPHA = "Sets the transparency level of objects that come between the camera and the player character."
    L.DESC_CAMERA_DISTANCE_MAX = "Sets the maximum distance the camera can zoom out from the player."
	L.DESC_MSDF = "Enables vector-based font rendering, dramatically improving glyph quality.\n0 = Disabled; 1 = Enabled;\n2 = Enabled (unsafe fonts) — due to how distance fields are calculated,\nsome fonts with self-intersecting contours (e.g., 'diediedie') may break."

    -- CVar Mode Options
    L.MODE_LABEL_PLAYER_RADIUS = "Player Radius 20yd"
    L.MODE_LABEL_CONE_ANGLE = "Cone Angle (deg) within 20yd"
end