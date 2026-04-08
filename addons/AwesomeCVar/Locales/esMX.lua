-- File: esMX.lua
-- Language: Spanish (Mexico)
local addonName, AwesomeCVar = ...

if not AwesomeCVar.L then
    AwesomeCVar.L = {}
end

local L = AwesomeCVar.L

if GetLocale() == "esMX" then
    -- General
    L.ADDON_NAME = "AwesomeCVar"
    L.ADDON_NAME_SHORT = "Awesome CVar"
    L.MAIN_FRAME_TITLE = "Gestor de Awesome CVar"
    L.RESET_TO = "Restablecer a %s"

    -- Popups
    L.RELOAD_POPUP_TITLE = "Requiere Recarga de IU"
    L.RELOAD_POPUP_TEXT = "Uno o más cambios que has realizado requieren recargar la interfaz (ReloadUI) para aplicarse."
    L.RESET_POPUP_TITLE = "Confirmar Restablecimiento"
    L.RESET_POPUP_TEXT = "¿Estás seguro de que deseas restablecer todos los valores a sus valores predeterminados?"

    -- Chat Messages
    L.MSG_LOADED = "¡Awesome CVar cargado! Escribe /awesome para abrir el gestor."
    L.MSG_FRAME_RESET = "La posición del marco se ha restablecido al centro."
    L.MSG_SET_VALUE = "%s establecido en %s."
    L.MSG_FRAME_CREATE_ERROR = "¡No se pudo crear el marco de AwesomeCVar!"
    L.MSG_UNKNOWN_COMMAND = "Comando desconocido. Escribe /awesome help para ver los comandos disponibles."
    L.MSG_HELP_HEADER = "Comandos de Awesome CVar:"
    L.MSG_HELP_TOGGLE = "/awesome - Alternar el gestor de CVar"
    L.MSG_HELP_SHOW = "/awesome show - Mostrar el gestor de CVar"
    L.MSG_HELP_HIDE = "/awesome hide - Ocultar el gestor de CVar"
    L.MSG_HELP_RESET = "/awesome reset - Restablecer posición del marco al centro"
    L.MSG_HELP_HELP = "/awesome help - Mostrar este mensaje de ayuda"

    -- CVar Categories
    L.CATEGORY_CAMERA = "Cámara"
    L.CATEGORY_NAMEPLATES = "Placas de nombre"
    L.CATEGORY_TEXT_TO_SPEECH = "Texto a voz"
    L.CATEGORY_INTERACTION = "Interacción"
    L.CATEGORY_OTHER = "Otros"

    -- CVar Labels & Descriptions
    L.CVAR_LABEL_INFO = "Notas"
    L.CVAR_LABEL_TTS_VOICE = "Voz de TTS"
    L.CVAR_LABEL_TTS_VOLUME = "Volumen de TTS"
    L.CVAR_LABEL_TTS_SPEED = "Velocidad de TTS"
    L.CVAR_LABEL_CAMERA_FOV = "Campo de visión (FoV)"
    L.CVAR_LABEL_STACKING_MODE = "Modo de apilamiento de placas"
    L.CVAR_LABEL_MOUSEOVER = "Modo de mouseover de placas"
    L.CVAR_LABEL_OCCLUSION_ALPHA = "Alfa de oclusión de placas"
    L.CVAR_LABEL_NONTARGET_ALPHA = "Alfa de placas sin objetivo"
    L.CVAR_LABEL_ALPHA_SPEED = "Velocidad de fundido de placas"
    L.CVAR_LABEL_CLAMP_TOP = "Fijar placas al borde superior"
    L.CVAR_LABEL_NAMEPLATE_DISTANCE = "Distancia de renderizado de placas"
    L.CVAR_LABEL_MAX_RAISE_DISTANCE = "Apilamiento: Distancia vertical máx."
    L.CVAR_LABEL_MAX_PULL_DISTANCE = "Apilamiento: Distancia horizontal máx."
    L.CVAR_LABEL_X_SPACE = "Espaciado X de placas"
    L.CVAR_LABEL_Y_SPACE = "Espaciado Y de placas"
    L.CVAR_LABEL_UPPER_BORDER = "Margen superior de placas fijas"
    L.CVAR_LABEL_PLACEMENT = "Desplazamiento de posición de placas"
    L.CVAR_LABEL_SPEED_RAISE = "Velocidad de subida al apilar"
    L.CVAR_LABEL_SPEED_LOWER = "Velocidad de bajada al apilar"
    L.CVAR_LABEL_SPEED_PULL = "Velocidad de atracción horizontal"
    L.CVAR_LABEL_HITBOX_HEIGHT_ENEMY = "Altura de hitbox (Enemigo)"
    L.CVAR_LABEL_HITBOX_WIDTH_ENEMY = "Ancho de hitbox (Enemigo)"
    L.CVAR_LABEL_HITBOX_HEIGHT_FRIENDLY = "Altura de hitbox (Aliado)"
    L.CVAR_LABEL_HITBOX_WIDTH_FRIENDLY = "Ancho de hitbox (Aliado)"
    L.CVAR_LABEL_INTERACTION_MODE = "Modo de interacción"
    L.CVAR_LABEL_INTERACTION_ANGLE = "Ángulo de interacción (grados)"
    L.CVAR_STANCE_PATCH = "Parche de cambio de forma/actitud"
    L.CVAR_SHOW_PLAYER = "Renderizado del modelo del jugador"
    L.CVAR_MSDF_MODE = "Modo de renderizado de fuentes (Requiere Reinicio)"
    L.CVAR_LABEL_CAMERA_INDIRECT_VISIBILITY = "Visibilidad indirecta de cámara"
    L.CVAR_LABEL_CAMERA_INDIRECT_ALPHA = "Alfa indirecto de cámara"
    L.CVAR_LABEL_CAMERA_DISTANCE_MAX = "Distancia de cámara"

    L.DESC_INFO = "\nTodas las placas de nombre incluyen métodos adicionales:\n- SetStackingEnabled(bool)\n- GetStackingEnabled()\n\nEjemplo de uso:\nif UnitExists('target') then\n C_NamePlate.GetNamePlateForUnit('target'):SetStackingEnabled(false)\nend\n\nNota: Es demasiado pronto\npara llamar a estos métodos en NAME_PLATE_CREATED,\nusa NAME_PLATE_UNIT_ADDED en su lugar.\n"
    L.DESC_STACKING_MODE = "0 = Desactivado; 1 = Activar todo; 2 = Solo enemigos; 3 = Solo aliados."
    L.DESC_MOUSEOVER = "0 = Desactivado;\n1 = Clic traspasa enemigos;\n2 = Clic traspasa enemigos, resaltar siempre placas aliadas ocluidas;\n3 = Clic traspasa enemigos, resaltar aliadas ocluidas en combate;\n4 = Clic traspasa aliados;\n5 = Clic traspasa aliados, resaltar siempre placas enemigas ocluidas;\n6 = Clic traspasa aliados, resaltar enemigas ocluidas en combate;\n7 = Siempre resaltar nivel de placa ocluida bajo el mouse;\n8 = Resaltar nivel de placa ocluida bajo el mouse en combate."
    L.DESC_ALPHA_BLEND = "Controla qué tan rápido las placas cambian de opacidad (1 = Instantáneo)."
    L.DESC_CLAMP_TOP = "0 = Desactivado; 1 = Fijar todas; 2 = Solo jefes."
    L.DESC_STANCE_PATCH = "Permite cambiar de forma/actitud y lanzar una habilidad en un solo clic al usar macros."
    L.DESC_OCCLUSION_ALPHA = "Controla la opacidad de las placas cuando están bloqueadas por obstáculos o terreno."
    L.DESC_CAMERA_INDIRECT_VISIBILITY = "Permite que la cámara atraviese ciertos objetos del mundo en lugar de ser bloqueada."
    L.DESC_CAMERA_INDIRECT_ALPHA = "Establece el nivel de transparencia de los objetos que se interponen entre la cámara y el jugador."
    L.DESC_CAMERA_DISTANCE_MAX = "Establece la distancia máxima que la cámara puede alejarse del jugador."
    L.DESC_MSDF = "Activa el renderizado de fuentes basado en vectores, mejorando drásticamente la calidad.\n0 = Desactivado; 1 = Activado;\n2 = Activado (fuentes inseguras) — debido al cálculo de campos de distancia,\nalgunas fuentes con contornos que se cruzan pueden fallar."

    -- CVar Mode Options
    L.MODE_LABEL_PLAYER_RADIUS = "Radio del jugador 20yd"
    L.MODE_LABEL_CONE_ANGLE = "Ángulo de cono (grados) dentro de 20yd"
end