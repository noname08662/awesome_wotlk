# Awesome WotLK
## World of Warcraft 3.3.5a 12340 Improvements Library
### Fork of https://github.com/FrostAtom/awesome_wotlk/

## [Details](#details) - [Installation](#installation) - [Docs](https://github.com/noname08662/awesome_wotlk/blob/main/docs/api_reference.md) - [3rd Party Libraries](#3rd-party-libraries)

___

## Details

### Features
- **MSDF Font Rendering:** Optionally enables smooth, vector-based font rendering, reducing pixelation across all in-game text.
- **Clipboard Fix:** Resolves issue where non-English text would appear as "???" when copied to clipboard.
- **Auto Login:** Launch with credentials via command line/shortcuts.  
  Usage: `Wow.exe -login "LOGIN" -password "PASSWORD" -realmlist "REALMLIST" -realmname "REALMNAME"`
- **Camera FOV Control:** Adjust the camera's field of view.
- **Improved Nameplate Sorting:** Enhanced nameplate stacking and collision logic.
- **Macro Conditionals:**
  - Backported `cursor` conditional
  - Implemented `playerlocation` conditional
- **Stance/Form Bug Fix:** Fixed client bug preventing casting a second ability after changing form or stance. Now `/cast battle stance /cast charge` works in a single click.
- ...a few other miscellaneous fixes/tweaks.

### New API Functions
- **C_NamePlate:**
  - `C_NamePlate.GetNamePlates`
  - `C_NamePlate.GetNamePlateForUnit`
  - `C_NamePlate.GetNamePlateByGUID`
  - `C_NamePlate.GetNamePlateTokenByGUID`
  - `GetStackingEnabled` nameplate method
  - `SetStackingEnabled` nameplate method
  - `GetOcclusionEnabled` nameplate method
  - `SetOcclusionEnabled` nameplate method
- **C_VoiceChat (TTS):**
  - `C_VoiceChat.SpeakText`
  - `C_VoiceChat.StopSpeakingText`
  - `C_VoiceChat.GetTtsVoices`
  - `C_VoiceChat.GetRemoteTtsVoices`
- **C_TTSSettings:**
  - `C_TTSSettings.GetSpeechRate`
  - `C_TTSSettings.GetSpeechVolume`
  - `C_TTSSettings.GetSpeechVoiceID`
  - `C_TTSSettings.GetVoiceOptionName`
  - `C_TTSSettings.SetDefaultSettings`
  - `C_TTSSettings.SetSpeechRate`
  - `C_TTSSettings.SetSpeechVolume`
  - `C_TTSSettings.SetVoiceOption`
  - `C_TTSSettings.SetVoiceOptionByName`
  - `C_TTSSettings.RefreshVoices`
- **Unit Functions:**
  - `UnitIsControlled`
  - `UnitIsDisarmed`
  - `UnitIsSilenced`
  - `UnitOccupations`
  - `UnitOwner`
  - `UnitTokenFromGUID`
- **Inventory & Items:**
  - `GetInventoryItemTransmog`
  - `GetItemInfoInstant`
- **Spell:**
  - `GetSpellBaseCooldown`
- **Miscellaneous:**
  - `FlashWindow`
  - `IsWindowFocused`
  - `FocusWindow`
  - `CopyToClipboard`
  - `QueueInteract`

### New Events
- **Nameplate Events:**
  - `NAME_PLATE_CREATED`
  - `NAME_PLATE_UNIT_ADDED`
  - `NAME_PLATE_UNIT_REMOVED`
- **TTS Events:**
  - `VOICE_CHAT_TTS_PLAYBACK_STARTED`
  - `VOICE_CHAT_TTS_PLAYBACK_FINISHED`
  - `VOICE_CHAT_TTS_PLAYBACK_FAILED`
  - `VOICE_CHAT_TTS_SPEAK_TEXT_UPDATE` _(unused)_
  - `VOICE_CHAT_TTS_VOICES_UPDATE`

### New CVars
- **Nameplate CVars:**
  - `nameplateDistance`
  - `nameplatePlacement`
  - `nameplateMouseMode`
  - `nameplateInertia`
  - `nameplateHysteresisDecay`
  - `nameplateBandX`
  - `nameplateBandY`
  - `nameplateHitboxAnchor`
  - `nameplateHitboxWidthE`
  - `nameplateHitboxHeightE`
  - `nameplateHitboxWidthF`
  - `nameplateHitboxHeightF`
  - `nameplateRaiseSpeed`
  - `nameplateLowerSpeed`
  - `nameplatePullSpeed`
  - `nameplateRaiseDistance`
  - `nameplatePullDistance`
  - `nameplateOcclusionMode`
  - `nameplateOcclusionAlpha`
  - `nameplateNonTargetAlpha`
  - `nameplateAlphaSpeed`
  - `nameplateClampMode`
  - `nameplateClampModeVOffset`
  - `nameplateClampModeHOffset`
  - `nameplateStacking`
- **Interaction CVars:**
  - `interactionMode`
  - `interactionAngle`
- **Camera CVars:**
  - `cameraIndirectVisibility`
  - `cameraIndirectAlpha`
  - `cameraFov`
- **TTS CVars:**
  - `ttsVoice`
  - `ttsSpeed`
  - `ttsVolume`
- **Miscellaneous:**
  - `enableStancePatch`
  - `MSDFMode`
  - `objectHighlightMode`
  - `portraitResolution`

### New Interaction Keybind
A new keybind for smart interaction with the game world:
- Loots mobs
- Skins mobs
- Interacts with nearby objects (ore veins, chairs, doors, mailboxes, etc.)
- Can be bound in the Options menu like any other keybind
- Can be used in macros: `/interact` or `/interact [@mouseover]` (standard Blizzard modifiers apply)

### Nameplate Stacking
New nameplate stacking system to prevent overlapping:
- Enable stacking: `/console nameplateStacking %mode%` (consult [docs](https://github.com/noname08662/awesome_wotlk/blob/main/docs/api_reference.md) for available modes)
- **Important:** If using [this WeakAura](https://wago.io/AQdGXNEBH), delete it and restart the client before using this feature
- See [Docs](https://github.com/noname08662/awesome_wotlk/blob/main/docs/api_reference.md) for detailed CVar information
- All settings are configurable in-game via `/awesome` command (requires AwesomeCVar addon)

**Recommended:** Use the AwesomeCVar addon and the `/awesome` command to configure all CVars through an intuitive in-game interface.

### Font Blacklisting
You can exempt specific fonts from vector-based (MSDF) rendering by blacklisting them. Follow these steps:
1. Locate the target folder: Go to your game directory and find the `Fonts_AwesomeWotLK` folder. (If it does not exist, create it manually or launch the game once to let it generate automatically.)
2. Exempt the font: Choose one of the following two methods:
   * Method A: Locate the `.ttf` or `.otf` file of the font you want to exclude (typically found within your `./Interface/AddOns/...` addon directories) and copy-paste it directly into the `Fonts_AwesomeWotLK` folder.
   * Method B: Create an empty file with no extension inside the `Fonts_AwesomeWotLK` folder, and name it after the font.
   
   Note for Method B: The file name must match the font's internal name, not the display name shown by your addons (e.g., 'Homespun TT BRK'). You can find the real internal name by double-clicking the font file to open it in Windows Font Viewer (or a similar tool) and checking the font title.
3. Apply changes: Relaunch the game. The target font will now bypass the MSDF pipeline and render normally.

### AwesomeCVar Addon
![AwesomeCVar Preview](https://raw.githubusercontent.com/noname08662/awesome_wotlk/refs/heads/main/docs/assets/preview_v5.png)

### Nameplate Features
Please note that in order to benefit from some of the new nameplate functionality (such as castbars on all targets or class-colored bars), you must use AwesomeWotLK-aware addons. 
If you are using [ElvUI](https://github.com/ElvUI-WotLK/ElvUI) (or any of its forks), you will need [this plugin](https://github.com/noname08662/ElvUI_Extras), [this plugin](https://github.com/Zidras/ElvUI_ProjectZidras), or both to take full advantage of this mod.

___

## Installation
1. Download the latest [release](https://github.com/noname08662/awesome_wotlk/releases)
2. Extract all files to your game's root folder
3. Run `AwesomeWotlkPatch.exe` (you should see a confirmation message), or drag `Wow.exe` onto `AwesomeWotlkPatch.exe`
4. To update, simply download the new version and replace the `AwesomeWotlkLib.dll` file (unless release notes specify something extra)

___

## 3rd Party Libraries
- [microsoft-Detours](https://github.com/microsoft/Detours) - [License](https://github.com/microsoft/Detours/blob/6782fe6e6ab11ae34ae66182aa5a73b5fdbcd839/LICENSE.md)
- [freetype](https://github.com/freetype/freetype) - [License](https://github.com/freetype/freetype/blob/master/docs/FTL.TXT)
- [msdfgen](https://github.com/Chlumsky/msdfgen) - [License](https://github.com/Chlumsky/msdfgen/blob/master/LICENSE.txt)
- [skia](https://github.com/google/skia) - [License](https://github.com/google/skia/blob/main/LICENSE)
- [unordered_dense](https://github.com/martinus/unordered_dense) - [License](https://github.com/martinus/unordered_dense/blob/main/LICENSE)
