[C_NamePlates](#c_nameplates) - [C_VoiceChat](#c_voicechat) - [Unit](#unit) - [Inventory](#inventory) - [Spell](#spell) - [Item](#item) - [Misc](#misc)

# C_NamePlates
Backported C-Lua interfaces from retail

## C_NamePlate.GetNamePlateForUnit `API`
**Arguments:** `unitId` (string)  
**Returns:** `namePlate` (frame)

Get nameplate by unitId.

```lua
frame = C_NamePlate.GetNamePlateForUnit("target")
```

## C_NamePlate.GetNamePlates `API`
**Arguments:** none  
**Returns:** `namePlateList` (table)

Get all visible nameplates.

```lua
for _, nameplate in pairs(C_NamePlate.GetNamePlates()) do
  -- something
end
```

## C_NamePlate.GetNamePlateByGUID `API`
**Arguments:** `guid` (string)  
**Returns:** `namePlate` (frame)

Get nameplate from UnitGUID, for example from combat log.

```lua
local nameplate = C_NamePlate.GetNamePlateByGUID(destGUID)
```

## C_NamePlate.GetNamePlateTokenByGUID `API`
**Arguments:** `guid` (string)  
**Returns:** `token` (string)

Get nameplate token from UnitGUID, for example from combat log.

```lua
local token = C_NamePlate.GetNamePlateTokenByGUID(destGUID)
local frame = C_NamePlate.GetNamePlateForUnit(token)
```

## NAME_PLATE_CREATED `Event`
**Parameters:** `namePlateBase` (frame)

Fires when a nameplate is created.

## NAME_PLATE_UNIT_ADDED `Event`
**Parameters:** `unitId` (string)

Fires when a new nameplate appears.

## NAME_PLATE_UNIT_REMOVED `Event`
**Parameters:** `unitId` (string)

Fires when a nameplate is about to be hidden.

## NAME_PLATE_OWNER_CHANGED `Event`
**Parameters:** `unitId` (string)

Fires when the nameplate owner changes (workaround for [this issue](https://github.com/FrostAtom/awesome_wotlk/blob/main/src/AwesomeWotlkLib/NamePlates.cpp#L170)).

## nameplateDistance `CVar`
**Arguments:** `distance` (number)  
**Default:** 41

Sets the display distance of nameplates in yards.

## nameplateStacking `CVar`
**Arguments:** `enabled` (boolean)  
**Default:** 0

Enables or disables the nameplate stacking feature.

## nameplateXSpace `CVar`
**Arguments:** `width` (number)  
**Default:** 130

Sets the effective width of a nameplate used in collision/stacking calculations.

## nameplateYSpace `CVar`
**Arguments:** `height` (number)  
**Default:** 20

Sets the effective height of a nameplate used in stacking collision calculations.

## nameplateUpperBorder `CVar`
**Arguments:** `offset` (number)  
**Default:** 30

Defines the vertical offset from the top of the screen where nameplates stop stacking upward.

## nameplateOriginPos `CVar`
**Arguments:** `offset` (number)  
**Default:** 20

Offset used to push nameplates slightly higher than their default position.

## nameplateSpeedRaise `CVar`
**Arguments:** `speed` (number)  
**Default:** 1

Speed at which nameplates move **upward** during stacking resolution.

## nameplateSpeedReset `CVar`
**Arguments:** `speed` (number)  
**Default:** 1

Speed at which nameplates **reset** during stacking resolution.

## nameplateSpeedLower `CVar`
**Arguments:** `speed` (number)  
**Default:** 1

Speed at which nameplates move **downward** during stacking resolution.

## nameplateFriendlyHitboxHeight `CVar`
**Arguments:** `height` (number)  
**Default:** 0

Height of a clickable **friendly** nameplate hitbox. Addons may override or break this; reload or disable/enable nameplates afterward.  
Use 0 to disable and use default values.  
Affected by the `nameplateStackFriendlyMode` setting, which has an unintuitive name.

## nameplateFriendlyHitboxWidth `CVar`
**Arguments:** `width` (number)  
**Default:** 0

Width of a clickable **friendly** nameplate hitbox. Addons may override or break this; reload or disable/enable nameplates afterward.  
Use 0 to disable and use default values.

## nameplateHitboxHeight `CVar`
**Arguments:** `height` (number)  
**Default:** 0

Height of a clickable nameplate hitbox. Addons may override or break this; reload or disable/enable nameplates afterward.  
Use 0 to disable and use default values.

## nameplateHitboxWidth `CVar`
**Arguments:** `width` (number)  
**Default:** 0

Width of a clickable nameplate hitbox. Addons may override or break this; reload or disable/enable nameplates afterward.  
Use 0 to disable and use default values.

## nameplateStackFriendly `CVar`
**Arguments:** `toggle` (boolean)  
**Default:** 1

Toggles whether friendly nameplates stack or overlap.  
- **0** = overlapping  
- **1** = stacking

## nameplateStackFriendlyMode `CVar`
**Arguments:** `mode` (number)  
**Default:** 1

Changes how mob friendliness is determined.  
- **0** = Uses `UnitReaction("player", "nameplate%") >= 5` with CanAttack check for reaction 4  
- **1** = Uses healthbar color parsing (same method as WeakAuras)

## nameplateMaxRaiseDistance `CVar`
**Arguments:** `height` (number)  
**Default:** 200

Sets the maximum height a nameplate can rise before deciding to move down or stay in place.

## nameplateExtendWorldFrameHeight `CVar`
**Arguments:** `enabled` (boolean)  
**Default:** 0

When enabled, extends the height of the WorldFrame, allowing nameplates that would normally be out of view to remain visible.

**Note:** This may interfere with some UI elements or addons that rely on the original WorldFrame height. It's recommended to use a WeakAura or addon to toggle this setting only during raids or boss encounters.

## nameplateUpperBorderOnlyBoss `CVar`
**Arguments:** `enabled` (boolean)  
**Default:** 0

When enabled, only boss creature nameplates will stick to the top of the screen; all other nameplates will overflow.

---

# C_VoiceChat
Windows SAPI-backed Text-to-Speech backport from retail

## C_VoiceChat.GetTtsVoices `API`
**Arguments:** none  
**Returns:** `voiceList` (table) → `{ { voiceID = number, name = string }, ... }`

Returns all locally available TTS voices.

```lua
for _, v in ipairs(C_VoiceChat.GetTtsVoices()) do
  print(v.voiceID, v.name)
end
```

## C_VoiceChat.GetRemoteTtsVoices `API`
**Arguments:** none  
**Returns:** `voiceList` (table)

Same as `GetTtsVoices()`.

## C_VoiceChat.SpeakText `API`
**Arguments:**
- `voiceID` (number)
- `text` (string)
- `destination` (number, optional, default=1)
- `rate` (number, optional)
- `volume` (number, optional)

**Returns:** `utteranceID` (number)

Speaks text asynchronously.
- `destination = 1` → speak immediately (FIFO)
- `destination = 4` → accepted, no special handling (async)

```lua
C_VoiceChat.SpeakText(1, "Hello World", 1, 0, 100)
```

## C_VoiceChat.StopSpeakingText `API`
**Arguments:** none  
**Returns:** none

Stops all queued or currently playing utterances.

## C_TTSSettings.GetSpeechRate `API`
**Arguments:** none  
**Returns:** `rate` (number) [-10..10]

## C_TTSSettings.GetSpeechVolume `API`
**Arguments:** none  
**Returns:** `volume` (number) [0..100]

## C_TTSSettings.GetSpeechVoiceID `API`
**Arguments:** none  
**Returns:** `voiceID` (number)

## C_TTSSettings.GetVoiceOptionName `API`
**Arguments:** none  
**Returns:** `voiceName` (string)

## C_TTSSettings.SetDefaultSettings `API`
**Arguments:** none  
**Returns:** none

Resets to defaults: voice=1 (if available), rate=0, volume=100.

## C_TTSSettings.SetSpeechRate `API`
**Arguments:** `rate` (number) [-10..10]  
**Returns:** none

## C_TTSSettings.SetSpeechVolume `API`
**Arguments:** `volume` (number) [0..100]  
**Returns:** none

## C_TTSSettings.SetVoiceOption `API`
**Arguments:** `voiceID` (number)  
**Returns:** none

## C_TTSSettings.SetVoiceOptionByName `API`
**Arguments:** `voiceName` (string)  
**Returns:** none

## C_TTSSettings.RefreshVoices `API`
**Arguments:** none  
**Returns:** none

Refreshes the voice list.  
Fires `VOICE_CHAT_TTS_VOICES_UPDATE` if the list has changed.

## VOICE_CHAT_TTS_PLAYBACK_STARTED `Event`
**Parameters:** `numConsumers` (number), `utteranceID` (number), `durationMS` (number), `destination` (number)

Fired when SAPI starts playback.  
`durationMS` is always **0**.

## VOICE_CHAT_TTS_PLAYBACK_FINISHED `Event`
**Parameters:** `numConsumers` (number), `utteranceID` (number), `destination` (number)

Fired when SAPI finishes playback.

## VOICE_CHAT_TTS_PLAYBACK_FAILED `Event`
**Parameters:** `status` (string), `utteranceID` (number), `destination` (number)

Fired if `SpeakText()` or setup fails (e.g., no voice/device available).

## VOICE_CHAT_TTS_SPEAK_TEXT_UPDATE `Event`
**Parameters:** `status` (string), `utteranceID` (number)

Unused placeholder.

## VOICE_CHAT_TTS_VOICES_UPDATE `Event`
**Parameters:** none

Fired when the enumerated voice list changes.

## ttsVoice `CVar`
**Arguments:** `voiceID` (number)  
**Default:** 1

Sets the active voice.

## ttsSpeed `CVar`
**Arguments:** `rate` (number) [-10..10]  
**Default:** 0

Controls the speech rate.

## ttsVolume `CVar`
**Arguments:** `volume` (number) [0..100]  
**Default:** 100

Controls the speech volume.

---

# Unit

## UnitIsControlled `API`
**Arguments:** `unitId` (string)  
**Returns:** `isControlled` (boolean)

Returns true if the unit is under hard crowd control.

## UnitIsDisarmed `API`
**Arguments:** `unitId` (string)  
**Returns:** `isDisarmed` (boolean)

Returns true if the unit is disarmed.

## UnitIsSilenced `API`
**Arguments:** `unitId` (string)  
**Returns:** `isSilenced` (boolean)

Returns true if the unit is silenced.

## UnitOccupations `API`
**Arguments:** `unitID` (string)  
**Returns:** `npcFlags` (number)

Returns [npcFlags bitmask](https://github.com/someweirdhuman/awesome_wotlk/blob/7ab28cea999256d4c769b8a1e335a7d93c5cac32/src/AwesomeWotlkLib/UnitAPI.cpp#L37) if passed a valid unitID, otherwise returns nothing.

## UnitOwner `API`
**Arguments:** `unitID` (string)  
**Returns:** `ownerName` (string), `ownerGuid` (string)

Returns owner name and GUID if passed a valid unitID, otherwise returns nothing.

## UnitTokenFromGUID `API`
**Arguments:** `GUID` (string)  
**Returns:** `UnitToken` (string)

Returns unit token if passed a valid GUID, otherwise returns nothing.

---

# Inventory

## GetInventoryItemTransmog `API`
**Arguments:** `unitId` (string), `slot` (number)  
**Returns:** `itemId` (number), `enchantId` (number)

Returns information about item transmogrification.

---

# Spell

## enableStancePatch `CVar`
**Arguments:** `enabled` (boolean)  
**Default:** 0

Enables a patch that allows you to swap stance or form and cast the next ability that's not on GCD in a single click.

## GetSpellBaseCooldown `API`
**Arguments:** `spellId` (number or string)  
**Returns:** `cdMs` (number), `gcdMs` (number)

Returns cooldown and global cooldown in milliseconds if passed a valid spellId, otherwise returns nothing.

---

# Item

## GetItemInfoInstant `API`
**Arguments:** `itemId/itemName/itemHyperlink` (string or number)  
**Returns:** `itemID` (number), `itemType` (string), `itemSubType` (string), `itemEquipLoc` (string), `icon` (string), `classID` (number), `subclassID` (number)

Returns ID, type, subtype, equipment slot, icon, class ID, and subclass ID if passed a valid argument, otherwise returns nothing.

---

# Misc

## cameraIndirectVisibility `CVar`
**Arguments:** `enabled` (number)  
**Default:** 0

Toggles camera behavior when obstructed by objects in the world.  
- **0** = Default client behavior  
- **1** = Allows camera to move freely through some world objects without being blocked

## cameraIndirectAlpha `CVar`
**Arguments:** `alpha` (number)  
**Default:** 0.6

Controls the transparency level of objects between the camera and the player character when `cameraIndirectVisibility` is enabled.  
Limited to [0.6 - 1] range.

## interactionMode `CVar`
**Arguments:** `mode` (boolean)  
**Default:** 1

Toggles behavior of interaction keybind or macro.  
- **1** = Interaction is limited to entities in front of the player within the angle defined by `interactionAngle` and within 20 yards  
- **0** = Interaction occurs with the nearest entity within 20 yards, regardless of direction

## interactionAngle `CVar`
**Arguments:** `angle` (number)  
**Default:** 60

The size of the cone-shaped area in front of the player (in degrees) within which a mob or entity must be located to be eligible for interaction.  
Only used if `interactionMode` is set to 1 (default).

## MSDFMode `CVar`
**Arguments:** `mode` (number)  
**Default:** 1

MSDF-based font rendering utilizes vector distance data instead of rasterized textures, allowing crisp, high-quality text at any scale with minimal blurring or aliasing. Applies to all in-game text.

- **0** = Disabled  
- **1** = Enabled  
- **2** = Enabled (unsafe fonts) — Due to how distance fields are calculated, some fonts with self-intersecting contours (e.g., 'diediedie') may break.

## cursor `macro`

Backported `cursor` macro conditional for quick-casting AoE spells at cursor position.

```
/cast [@cursor] Blizzard
/cast [target=cursor] Flare
```

## playerlocation `macro`

Implemented `playerlocation` macro conditional for quick-casting AoE spells at player location.

```
/cast [@playerlocation] Blizzard
/cast [target=playerlocation] Flare
```

## FlashWindow `API`
**Arguments:** none  
**Returns:** none

Starts flashing the game window icon in the taskbar.

## IsWindowFocused `API`
**Arguments:** none  
**Returns:** `focused` (boolean)

Returns 1 if the game window is focused, otherwise returns nil.

## FocusWindow `API`
**Arguments:** none  
**Returns:** none

Brings the game window to the foreground.

## CopyToClipboard `API`
**Arguments:** `text` (string)  
**Returns:** none

Copies text to the clipboard.

## cameraFov `CVar`
**Arguments:** `value` (number)  
**Default:** 100

Changes the camera field of view (fisheye effect), in range **1**-**200**.
