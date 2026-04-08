-- File: deDE.lua
-- Language: German
local addonName, AwesomeCVar = ...

if not AwesomeCVar.L then
    AwesomeCVar.L = {}
end

local L = AwesomeCVar.L

if GetLocale() == "deDE" then
    -- Allgemein
    L.ADDON_NAME = "AwesomeCVar"
    L.ADDON_NAME_SHORT = "Awesome CVar"
    L.MAIN_FRAME_TITLE = "Awesome CVar Manager"
    L.RESET_TO = "Zurücksetzen auf %s"

    -- Popups
    L.RELOAD_POPUP_TITLE = "Interface-Reload erforderlich"
    L.RELOAD_POPUP_TEXT = "Eine oder mehrere Änderungen erfordern ein Neuladen des Interfaces (ReloadUI), um wirksam zu werden."
    L.RESET_POPUP_TITLE = "Standardwerte wiederherstellen"
    L.RESET_POPUP_TEXT = "Bist du sicher, dass du alle Werte auf die Standardeinstellungen zurücksetzen möchtest?"

    -- Chat-Nachrichten
    L.MSG_LOADED = "Awesome CVar geladen! Gib /awesome ein, um den Manager zu öffnen."
    L.MSG_FRAME_RESET = "Fensterposition wurde auf die Mitte zurückgesetzt."
    L.MSG_SET_VALUE = "%s auf %s gesetzt."
    L.MSG_FRAME_CREATE_ERROR = "AwesomeCVar-Fenster konnte nicht erstellt werden!"
    L.MSG_UNKNOWN_COMMAND = "Unbekannter Befehl. Gib /awesome help für eine Übersicht ein."
    L.MSG_HELP_HEADER = "Awesome CVar Befehle:"
    L.MSG_HELP_TOGGLE = "/awesome - CVar-Manager umschalten"
    L.MSG_HELP_SHOW = "/awesome show - CVar-Manager anzeigen"
    L.MSG_HELP_HIDE = "/awesome hide - CVar-Manager ausblenden"
    L.MSG_HELP_RESET = "/awesome reset - Fensterposition zentrieren"
    L.MSG_HELP_HELP = "/awesome help - Diese Hilfe anzeigen"

    -- CVar-Kategorien
    L.CATEGORY_CAMERA = "Kamera"
    L.CATEGORY_NAMEPLATES = "Namensplaketten"
    L.CATEGORY_TEXT_TO_SPEECH = "Text-zu-Sprache (TTS)"
    L.CATEGORY_INTERACTION = "Interaktion"
    L.CATEGORY_OTHER = "Sonstiges"

    -- CVar-Beschriftungen & Beschreibungen
    L.CVAR_LABEL_INFO = "Notizen"
    L.CVAR_LABEL_TTS_VOICE = "TTS-Stimme"
    L.CVAR_LABEL_TTS_VOLUME = "TTS-Lautstärke"
    L.CVAR_LABEL_TTS_SPEED = "TTS-Geschwindigkeit"
    L.CVAR_LABEL_CAMERA_FOV = "Kamera-Sichtfeld (FoV)"
    L.CVAR_LABEL_STACKING_MODE = "Plaketten-Stapelmodus"
    L.CVAR_LABEL_MOUSEOVER = "Plaketten-Mouseover-Modus"
    L.CVAR_LABEL_OCCLUSION_ALPHA = "Plaketten-Verdeckungs-Alpha"
    L.CVAR_LABEL_NONTARGET_ALPHA = "Plaketten-Nicht-Ziel-Alpha"
    L.CVAR_LABEL_ALPHA_SPEED = "Plaketten-Transparenz-Übergang"
    L.CVAR_LABEL_CLAMP_TOP = "Plaketten am oberen Rand fixieren"
    L.CVAR_LABEL_NAMEPLATE_DISTANCE = "Plaketten-Sichtweite"
    L.CVAR_LABEL_MAX_RAISE_DISTANCE = "Stapeln: Max. vertikaler Abstand"
    L.CVAR_LABEL_MAX_PULL_DISTANCE = "Stapeln: Max. horizontaler Abstand"
    L.CVAR_LABEL_X_SPACE = "Plaketten: X-Abstand"
    L.CVAR_LABEL_Y_SPACE = "Plaketten: Y-Abstand"
    L.CVAR_LABEL_UPPER_BORDER = "Plaketten: Oberer Rand-Offset"
    L.CVAR_LABEL_PLACEMENT = "Plaketten: Platzierungs-Offset"
    L.CVAR_LABEL_SPEED_RAISE = "Stapeln: Geschwindigkeit Anheben"
    L.CVAR_LABEL_SPEED_LOWER = "Stapeln: Geschwindigkeit Absenken"
    L.CVAR_LABEL_SPEED_PULL = "Stapeln: Geschwindigkeit Horizontale"
    L.CVAR_LABEL_HITBOX_HEIGHT_ENEMY = "Hitbox-Höhe (Gegner)"
    L.CVAR_LABEL_HITBOX_WIDTH_ENEMY = "Hitbox-Breite (Gegner)"
    L.CVAR_LABEL_HITBOX_HEIGHT_FRIENDLY = "Hitbox-Höhe (Freundlich)"
    L.CVAR_LABEL_HITBOX_WIDTH_FRIENDLY = "Hitbox-Breite (Freundlich)"
    L.CVAR_LABEL_INTERACTION_MODE = "Interaktionsmodus"
    L.CVAR_LABEL_INTERACTION_ANGLE = "Interaktionswinkel (Grad)"
    L.CVAR_STANCE_PATCH = "Haltungs-/Formwechsel-Patch"
    L.CVAR_SHOW_PLAYER = "Eigener Charaktermodell rendern"
    L.CVAR_MSDF_MODE = "Schrift-Rendering-Modus (Neustart erforderlich)"
    L.CVAR_LABEL_CAMERA_INDIRECT_VISIBILITY = "Indirekte Kamera-Sichtbarkeit"
    L.CVAR_LABEL_CAMERA_INDIRECT_ALPHA = "Indirekter Kamera-Alpha"
    L.CVAR_LABEL_CAMERA_DISTANCE_MAX = "Max. Kameradistanz"

    L.DESC_INFO = "\nAlle Namensplaketten enthalten zusätzliche Methoden:\n- SetStackingEnabled(bool)\n- GetStackingEnabled()\n\nBeispiel:\nif UnitExists('target') then\n C_NamePlate.GetNamePlateForUnit('target'):SetStackingEnabled(false)\nend\n\nHinweis: Es ist zu früh,\ndiese Methoden bei NAME_PLATE_CREATED aufzurufen,\nnutze stattdessen NAME_PLATE_UNIT_ADDED.\n"
    L.DESC_STACKING_MODE = "0 = Deaktiviert; 1 = Alle aktivieren; 2 = Nur Gegner; 3 = Nur Freundlich."
    L.DESC_MOUSEOVER = "0 = Deaktiviert;\n1 = Durch Gegner durchklicken;\n2 = Durch Gegner durchklicken, verdeckte Mouseover-Freundesplaketten immer hervorheben;\n3 = Durch Gegner durchklicken, verdeckte Freundesplaketten im Kampf hervorheben;\n4 = Durch Freunde durchklicken;\n5 = Durch Freunde durchklicken, verdeckte Mouseover-Gegnerplaketten immer hervorheben;\n6 = Durch Freunde durchklicken, verdeckte Gegnerplaketten im Kampf hervorheben;\n7 = Ebene verdeckter Mouseover-Plaketten immer anheben;\n8 = Ebene verdeckter Mouseover-Plaketten im Kampf anheben."
    L.DESC_ALPHA_BLEND = "Steuert, wie schnell Plaketten zu neuen Transparenzwerten animieren (1 = Sofort)."
    L.DESC_CLAMP_TOP = "0 = Deaktiviert; 1 = Alle fixieren; 2 = Nur Bosse fixieren."
    L.DESC_STANCE_PATCH = "Ermöglicht den Wechsel der Haltung/Form und das Wirken einer Fähigkeit mit einem Klick in Makros."
    L.DESC_OCCLUSION_ALPHA = "Steuert die Transparenz von Plaketten, wenn sie durch Hindernisse oder Gelände verdeckt sind."
    L.DESC_CAMERA_INDIRECT_VISIBILITY = "Ermöglicht es der Kamera, durch bestimmte Objekte hindurchzugehen, anstatt blockiert zu werden."
    L.DESC_CAMERA_INDIRECT_ALPHA = "Legt die Transparenz von Objekten fest, die sich zwischen Kamera und Spieler befinden."
    L.DESC_CAMERA_DISTANCE_MAX = "Legt die maximale Entfernung fest, die die Kamera herauszoomen kann."
    L.DESC_MSDF = "Aktiviert vektorbasiertes Schrift-Rendering, was die Glyphenqualität drastisch verbessert.\n0 = Deaktiviert; 1 = Aktiviert;\n2 = Aktiviert (unsichere Schriften) — aufgrund der Berechnung der Distance Fields\nkönnen einige Schriften mit sich selbst überschneidenden Konturen fehlerhaft sein."

    -- CVar Mode Optionen
    L.MODE_LABEL_PLAYER_RADIUS = "Spieler-Radius 20m"
    L.MODE_LABEL_CONE_ANGLE = "Kegelwinkel (Grad) innerhalb 20m"
end