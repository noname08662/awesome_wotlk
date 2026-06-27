-- File: deDE.lua
-- Language: German
local _, AwesomeCVar = ...

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
	L.MINIMAP_ICON = "Minimap-Symbol"
	L.MINIMAP_TOOLTIP = "Klicken zum Öffnen\nZiehen zum Bewegen"
	L.GAME_MENU_BUTTON = "Spielmenü-Button"
	L.OPEN_ADDON = "AwesomeCVar öffnen"

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
	L.CVAR_LABEL_OCCLUSION_MODE = "Namensplaketten-Verdeckungsmodus"
    L.CVAR_LABEL_NONTARGET_ALPHA = "Plaketten-Nicht-Ziel-Alpha"
    L.CVAR_LABEL_ALPHA_SPEED = "Plaketten-Transparenz-Übergang"
    L.CVAR_LABEL_CLAMP_MODE = "Namensplakettenfixierungsmodus"
    L.CVAR_LABEL_NAMEPLATE_DISTANCE = "Plaketten-Sichtweite"
    L.CVAR_LABEL_MAX_RAISE_DISTANCE = "Stapeln: Max. vertikaler Abstand"
    L.CVAR_LABEL_MAX_PULL_DISTANCE = "Stapeln: Max. horizontaler Abstand"
    L.CVAR_LABEL_X_SPACE = "Plaketten: X-Abstand"
    L.CVAR_LABEL_Y_SPACE = "Plaketten: Y-Abstand"
	L.CVAR_LABEL_V_OFFSET = "Fixierter vertikaler Versatz der Namensplakette"
	L.CVAR_LABEL_H_OFFSET = "Fixierter horizontaler Versatz der Namensplakette"
    L.CVAR_LABEL_PLACEMENT = "Plaketten: Platzierungs-Offset"
    L.CVAR_LABEL_SPEED_RAISE = "Stapeln: Geschwindigkeit Anheben"
    L.CVAR_LABEL_SPEED_LOWER = "Stapeln: Geschwindigkeit Absenken"
    L.CVAR_LABEL_SPEED_PULL = "Stapeln: Geschwindigkeit Horizontale"
	L.CVAR_LABEL_INERTIA = "Namensplaketten-Stapelungsträgheit"
	L.CVAR_LABEL_HYST_DECAY = "Namensplatten-Paartrennung"
	L.CVAR_LABEL_HITBOX_ANCHOR = "Namensplaketten-Trefferbox-Anker"
    L.CVAR_LABEL_HITBOX_HEIGHT_ENEMY = "Hitbox-Höhe (Gegner)"
    L.CVAR_LABEL_HITBOX_WIDTH_ENEMY = "Hitbox-Breite (Gegner)"
    L.CVAR_LABEL_HITBOX_HEIGHT_FRIENDLY = "Hitbox-Höhe (Freundlich)"
    L.CVAR_LABEL_HITBOX_WIDTH_FRIENDLY = "Hitbox-Breite (Freundlich)"
    L.CVAR_LABEL_INTERACTION_MODE = "Interaktionsmodus"
    L.CVAR_LABEL_INTERACTION_ANGLE = "Interaktionswinkel (Grad)"
    L.CVAR_LABEL_SHOW_PLAYER = "Eigener Charaktermodell rendern"
    L.CVAR_LABEL_MSDF_MODE = "Schrift-Rendering-Modus (Neustart erforderlich)"
	L.CVAR_LABEL_OBJ_HIGHLIGHT = "Objekthervorhebung"
    L.CVAR_LABEL_CAMERA_INDIRECT_VISIBILITY = "Indirekte Kamera-Sichtbarkeit"
    L.CVAR_LABEL_CAMERA_INDIRECT_ALPHA = "Indirekter Kamera-Alpha"
    L.CVAR_LABEL_CAMERA_DISTANCE_MAX = "Max. Kameradistanz"
	L.CVAR_LABEL_PORTRAIT = "Porträtauflösung"
	L.CVAR_LABEL_CHAT_LOG = "Chatlog mit Zeitstempel"
	L.CVAR_LABEL_COMBAT_LOG = "Kampflog mit Zeitstempel"

    L.DESC_INFO = "Alle Namensplaketten enthalten zusätzliche Methoden:\n- SetStackingEnabled(bool)\n- GetStackingEnabled()\n\nBeispiel:\nif UnitExists('target') then\n C_NamePlate.GetNamePlateForUnit('target'):SetStackingEnabled(false)\nend\n\nHinweis: Es ist zu früh,\ndiese Methoden bei NAME_PLATE_CREATED aufzurufen,\nnutze stattdessen NAME_PLATE_UNIT_ADDED.\n"
	L.DESC_STACKING_MODE = "Der 'Smart'-Modus ermöglicht es Namensplaketten, das Stapeln zu umgehen, wenn darunter genügend Platz ist. Dies führt zu einem kompakteren Layout, erfordert jedoch häufigere Neuanordnungen."
    L.DESC_MOUSEOVER = "'Hervorheben'-Optionen erhöhen die Rahmenebene der Namensplakette unter der Maus, sodass sie über anderen erscheint.\nDas aktuelle Ziel behält dennoch die höchste Ebene."
	L.DESC_INERTIA = "Steuert das physische Gewicht der Namensplaketten-Bewegung beim Stapeln.\nHöhere Werte erhöhen die Reaktionsfähigkeit; niedrigere Werte erzeugen eine schwerere, stärker gedämpfte Bewegung."
	L.DESC_HYST_DECAY = "Steuert, wie schnell Stapelpaare sich auflösen, sobald sich Namensplaketten nicht mehr überlappen.\nHöhere Werte beschleunigen die Trennung; niedrigere Werte halten Paare länger zusammen."
	L.DESC_PLACEMENT = "Ein vertikaler Versatz-Verhältniswert, der Namensplaketten von ihrem Standard-Ankerpunkt verschiebt."
	L.DESC_HITBOX_ANCHOR = "Legt den vertikalen Ursprungspunkt des anklickbaren Bereichs der Namensplakette fest.\nPasse diesen Wert an, wenn dein UI-Addon die Namensplatten-Rahmen anders verankert (z. B. am oberen oder unteren Rand)."
    L.DESC_ALPHA_BLEND = "Steuert, wie schnell Plaketten zu neuen Transparenzwerten animieren (1 = Sofort)."
    L.DESC_OCCLUSION_ALPHA = "Steuert die Transparenz von Namensplaketten, wenn sie durch Hindernisse oder Gelände blockiert sind. Positive Werte multiplizieren das aktuelle Nicht-Ziel-Alpha, negative Werte erzwingen eine strikte Obergrenze für das verdeckte Alpha."
	L.DESC_OCCLUSION_MODE = "Steuert, wann Namensplaketten ausgeblendet oder verblasst werden, wenn sie durch Gelände oder Wände blockiert sind."
    L.DESC_CAMERA_INDIRECT_VISIBILITY = "Ermöglicht es der Kamera, durch bestimmte Objekte hindurchzugehen, anstatt blockiert zu werden."
    L.DESC_CAMERA_INDIRECT_ALPHA = "Legt die Transparenz von Objekten fest, die sich zwischen Kamera und Spieler befinden."
    L.DESC_CAMERA_DISTANCE_MAX = "Legt die maximale Entfernung fest, die die Kamera herauszoomen kann."
    L.DESC_MSDF = "Aktiviert vektorbasiertes Schrift-Rendering, was die Glyphenqualität drastisch verbessert."
	L.DESC_OBJ_HIGHLIGHT = "Zwingt leuchtende Funken auf Ressourcen (Kräuter/Erz) und interaktive Objekte wie Kisten oder Steckbriefe."
	L.DESC_PORTRAIT = "Erhöht die Auflösung der Rendering-Texturen für alle Porträts im Spiel."
	L.DESC_SESSION_LOG = "Fügt dem Namen der Protokolldatei automatisch einen Zeitstempel hinzu."

    -- CVar Mode Optionen
	L.MODE_DISABLED = "Deaktiviert"
	L.MODE_ENABLED = "Aktiviert"

    L.MODE_STACKING_DISABLED = "Überlappend"
    L.MODE_STACKING_ALL = "Stapelnd (Alle)"
    L.MODE_STACKING_ENEMY = "Stapelnd (Feinde)"
    L.MODE_STACKING_FRIENDLY = "Stapelnd (Freunde)"
    L.MODE_STACKING_SMART_ALL = "Smart-Stapeln (Alle)"
    L.MODE_STACKING_SMART_ENEMY = "Smart-Stapeln (Feinde)"
    L.MODE_STACKING_SMART_FRIENDLY = "Smart-Stapeln (Freunde)"

    L.MODE_MOUSE_DISABLED = "Standard"
    L.MODE_MOUSE_CLICKTHROUGH_ENEMY = "Klick-Durchlassen (Feinde)"
    L.MODE_MOUSE_CLICKTHROUGH_ENEMY_RAISE_FRIENDLY = "Feind KD + Freund Hervorheben"
    L.MODE_MOUSE_CLICKTHROUGH_ENEMY_RAISE_FRIENDLY_COMBAT = "Feind KD + Freund Hervorh. (Kampf)"
    L.MODE_MOUSE_CLICKTHROUGH_FRIENDLY = "Klick-Durchlassen (Freunde)"
    L.MODE_MOUSE_CLICKTHROUGH_FRIENDLY_RAISE_ENEMY = "Freund KD + Feind Hervorheben"
    L.MODE_MOUSE_CLICKTHROUGH_FRIENDLY_RAISE_ENEMY_COMBAT = "Freund KD + Feind Hervorh. (Kampf)"
    L.MODE_MOUSE_RAISE_OCCLUDED = "Verdeckte immer hervorheben"
    L.MODE_MOUSE_RAISE_OCCLUDED_COMBAT = "Verdeckte hervorheben (nur Kampf)"

	L.MODE_CLAMP_ALL = "Alle fixieren (nur oben)"
	L.MODE_CLAMP_BOSSES = "Nur Bosse fixieren (nur oben)"
	L.MODE_CLAMP_ALL_EDGES = "Alle fixieren (alle Seiten)"
	L.MODE_CLAMP_BOSSES_EDGES = "Nur Bosse fixieren (alle Seiten)"

	L.MODE_OCCLUSION_ALWAYS = "Immer verdecken"
	L.MODE_OCCLUSION_NOCOMBAT = "Nur außerhalb des Kampfes"

    L.MODE_HITBOX_TOP = "Oben"
    L.MODE_HITBOX_CENTER = "Mitte"
    L.MODE_HITBOX_BOTTOM = "Unten"

	L.MODE_LABEL_PLAYER_RADIUS = "Spieler-Radius 20yd"
	L.MODE_LABEL_CONE_ANGLE = "Kegelwinkel (Grad) innerhalb 20yd"

	L.MODE_MSDF_ENABLED_UNSAFE = "Enabled (unsafe fonts) — due to how distance fields are calculated, some fonts with self-intersecting contours (e.g., 'diediedie') may break."

	L.MODE_HIGHLIGHTS_TRACKED = "Nur verfolgte"
end