[C_NamePlates](#c_nameplates) - [C_VoiceChat](#c_voicechat) - [Unit](#unit) - [Inventory](#inventory) - [Spell](#spell) - [Item](#item) - [Misc](#misc)

# C_NamePlates
Backported C-Lua interfaces from retail  
All nameplates come equipped with a `unit` field holding relevant `nameplateN` token string

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

## GetStackingEnabled `Method`
**Arguments:** none  
**Returns:** `enabled` (boolean)

Returns the per-nameplate stacking override flag. Defaults to true for all relevant units.

```lua
print(string.format("Target nameplate is %s", C_NamePlate.GetNamePlateForUnit('target'):GetStackingEnabled() and "stacking" or "not stacking"))
```

## SetStackingEnabled `Method`
**Arguments:** `enabled` (boolean)  
**Returns:** none

Sets the per-nameplate stacking override flag.

```lua
for _, nameplate in pairs(C_NamePlate.GetNamePlates()) do
  nameplate:SetStackingEnabled(UnitName(nameplate.unit) == "Thatguy")
end
```

## NAME_PLATE_CREATED `Event`
**Parameters:** `namePlateBase` (frame)

Fires when a nameplate object is initially created.

## NAME_PLATE_UNIT_ADDED `Event`
**Parameters:** `unitId` (string)

Fires when a nameplate becomes active and is attached to a unit.

## NAME_PLATE_UNIT_REMOVED `Event`
**Parameters:** `unitId` (string)

Fires when a nameplate is detached from a unit and is about to be hidden.

## nameplateDistance `CVar`
**Arguments:** `distance` (number)  
**Default:** 41

Sets the display distance of nameplates in yards.

## nameplateStacking `CVar`
**Arguments:** `mode` (number)  
**Default:** 0

Defines the nameplate stacking behavior.
- **0** = Disabled (Overlapping)
- **1** = Enable All 
- **2** = Enemy Only
- **3** = Friendly Only

## nameplateMouseMode `CVar`
**Arguments:** `mode` (number)  
**Default:** 0

Defines nameplate mouse interaction and depth behavior.
- **0** = Disabled/No Changes  
- **1** = Click-through enemies
- **2** = Click-through enemies; always raise the frame level of occluded friendly plates on mouseover
- **3** = Click-through enemies; raise the frame level of occluded friendly plates on mouseover (In Combat Only)
- **4** = Click-through friendlies
- **5** = Click-through friendlies; always raise the frame level of occluded enemy plates on mouseover
- **6** = Click-through friendlies; raise the frame level of occluded enemy plates on mouseover (In Combat Only)
- **7** = Always raise the frame level of any occluded plate on mouseover
- **8** = Raise the frame level of any occluded plate on mouseover (In Combat Only)

## nameplateBandX `CVar`
**Arguments:** `width` (number)  
**Default:** 0.7

Sets the horizontal overlap tolerance. Represents the maximum combined width ratio nameplates are allowed to overlap during stacking.

## nameplateBandY `CVar`
**Arguments:** `height` (number)  
**Default:** 1

Sets the vertical separation margin. Represents the minimum combined height ratio required between nameplates during stacking.

## nameplatePlacement `CVar`
**Arguments:** `offset` (number)  
**Default:** 0

A vertical offset ratio used to displace nameplates from their original anchor points.

## nameplateRaiseSpeed `CVar`
**Arguments:** `speed` (number)  
**Default:** 100

The velocity at which nameplates shift **upward** to resolve stacking conflicts.

## nameplateLowerSpeed `CVar`
**Arguments:** `speed` (number)  
**Default:** 100

The velocity at which nameplates shift **downward** to resolve stacking conflicts.

## nameplatePullSpeed `CVar`
**Arguments:** `speed` (number)  
**Default:** 50

The velocity at which nameplates shift **horizontally** to resolve stacking conflicts.

## nameplateHitboxHeightF / nameplateHitboxWidthF `CVar`
**Arguments:** `scale` (number)  
**Default:** 1

Multipliers for the clickable hitbox dimensions of **friendly** nameplates.

## nameplateHitboxHeightE / nameplateHitboxWidthE `CVar`
**Arguments:** `scale` (number)  
**Default:** 1

Multipliers for the clickable hitbox dimensions of **enemy** nameplates.

## nameplateRaiseDistance `CVar`
**Arguments:** `distance` (number)  
**Default:** 8

Sets the maximum vertical distance (as a ratio of plate height) a nameplate can be pushed from its origin.

## nameplatePullDistance `CVar`
**Arguments:** `distance` (number)  
**Default:** 0.25

Sets the maximum horizontal distance (as a ratio of plate width) a nameplate can be pulled from its origin.

## nameplateClampTop `CVar`
**Arguments:** `mode` (number)  
**Default:** 0

Restricts nameplates from moving beyond the top edge of the screen.
- **0** = Disabled  
- **1** = Enabled (All)
- **2** = Bosses Only

## nameplateClampTopOffset `CVar`
**Arguments:** `offset` (number)  
**Default:** 0.1

Sets the vertical screen boundary for clamping (0.0 is the very top). Requires `nameplateClampTop` to be enabled.

## nameplateOcclusionAlpha `CVar`
**Arguments:** `alpha` (number)  
**Default:** 1

Sets the transparency level for nameplates blocked by line-of-sight (objects or terrain).

## nameplateNonTargetAlpha `CVar`
**Arguments:** `alpha` (number)  
**Default:** 0.5

Sets the transparency level for all nameplates except the current target.

## nameplateAlphaSpeed `CVar`
**Arguments:** `speed` (number)  
**Default:** 0.25

Determines the transition speed for `nameplateOcclusionAlpha` and `nameplateNonTargetAlpha` alpha state changes.

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
