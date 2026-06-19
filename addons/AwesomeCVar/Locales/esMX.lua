-- File: esMX.lua
-- Language: Spanish (Mexico)
local _, AwesomeCVar = ...

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
	L.MINIMAP_ICON = "Icono del minimapa"
	L.MINIMAP_TOOLTIP = "Clic para abrir\nArrastrar para mover"
	L.GAME_MENU_BUTTON = "Botón del menú del juego"
	L.OPEN_ADDON = "Abrir AwesomeCVar"

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
	L.CVAR_LABEL_OCCLUSION_MODE = "Modo de oclusión de placas de nombre"
    L.CVAR_LABEL_NONTARGET_ALPHA = "Alfa de placas sin objetivo"
    L.CVAR_LABEL_ALPHA_SPEED = "Velocidad de fundido de placas"
	L.CVAR_LABEL_CLAMP_MODE = "Modo de fijación de placas de nombre"
	L.CVAR_LABEL_NAMEPLATE_DISTANCE = "Distancia de visualización de placas"
    L.CVAR_LABEL_MAX_RAISE_DISTANCE = "Apilamiento: Distancia vertical máx."
    L.CVAR_LABEL_MAX_PULL_DISTANCE = "Apilamiento: Distancia horizontal máx."
    L.CVAR_LABEL_X_SPACE = "Espaciado X de placas"
    L.CVAR_LABEL_Y_SPACE = "Espaciado Y de placas"
	L.CVAR_LABEL_V_OFFSET = "Desplazamiento vertical de placas fijadas"
	L.CVAR_LABEL_H_OFFSET = "Desplazamiento horizontal de placas fijadas"
    L.CVAR_LABEL_PLACEMENT = "Desplazamiento de posición de placas"
    L.CVAR_LABEL_SPEED_RAISE = "Velocidad de subida al apilar"
    L.CVAR_LABEL_SPEED_LOWER = "Velocidad de bajada al apilar"
    L.CVAR_LABEL_SPEED_PULL = "Velocidad de atracción horizontal"
	L.CVAR_LABEL_INERTIA = "Inercia de apilamiento de placas de nombre"
	L.CVAR_LABEL_HYST_DECAY = "Velocidad de separación de pares"
	L.CVAR_LABEL_HITBOX_ANCHOR = "Anclaje de Hitbox de Placa"
    L.CVAR_LABEL_HITBOX_HEIGHT_ENEMY = "Altura de hitbox (Enemigo)"
    L.CVAR_LABEL_HITBOX_WIDTH_ENEMY = "Ancho de hitbox (Enemigo)"
    L.CVAR_LABEL_HITBOX_HEIGHT_FRIENDLY = "Altura de hitbox (Aliado)"
    L.CVAR_LABEL_HITBOX_WIDTH_FRIENDLY = "Ancho de hitbox (Aliado)"
    L.CVAR_LABEL_INTERACTION_MODE = "Modo de interacción"
    L.CVAR_LABEL_INTERACTION_ANGLE = "Ángulo de interacción (grados)"
    L.CVAR_LABEL_STANCE_PATCH = "Parche de cambio de forma/actitud"
    L.CVAR_LABEL_SHOW_PLAYER = "Renderizado del modelo del jugador"
    L.CVAR_LABEL_MSDF_MODE = "Modo de renderizado de fuentes (Requiere Reinicio)"
	L.CVAR_LABEL_OBJ_HIGHLIGHT = "Resaltado de Objetos"
    L.CVAR_LABEL_CAMERA_INDIRECT_VISIBILITY = "Visibilidad indirecta de cámara"
    L.CVAR_LABEL_CAMERA_INDIRECT_ALPHA = "Alfa indirecto de cámara"
    L.CVAR_LABEL_CAMERA_DISTANCE_MAX = "Distancia de cámara"
	L.CVAR_LABEL_PORTRAIT = "Resolución de retrato"

    L.DESC_INFO = "Todas las placas de nombre incluyen métodos adicionales:\n- SetStackingEnabled(bool)\n- GetStackingEnabled()\n\nEjemplo de uso:\nif UnitExists('target') then\n C_NamePlate.GetNamePlateForUnit('target'):SetStackingEnabled(false)\nend\n\nNota: Es demasiado pronto\npara llamar a estos métodos en NAME_PLATE_CREATED,\nusa NAME_PLATE_UNIT_ADDED en su lugar.\n"
	L.DESC_STACKING_MODE = "El modo 'Inteligente' permite que las placas omitan el empilado si hay suficiente espacio debajo, resultando en un diseño más compacto pero con reajustes más frecuentes."
    L.DESC_MOUSEOVER = "Las opciones de 'Resaltar' elevan el nivel del marco de la placa bajo el ratón para que aparezca sobre las demás.\nEl objetivo actual siempre tendrá prioridad de altura."
	L.DESC_INERTIA = "Controla el peso físico del movimiento de las placas durante el apilamiento.\nLos valores más altos aumentan la respuesta; los valores más bajos producen un movimiento más pesado y amortiguado."
	L.DESC_HYST_DECAY = "Controla qué tan rápido se disuelven los pares de apilamiento cuando las placas ya no se superponen.\nValores más altos causan una separación más rápida; valores más bajos mantienen los pares unidos por más tiempo."
	L.DESC_PLACEMENT = "Un valor de desplazamiento vertical que aleja las placas de su punto de anclaje predeterminado."
	L.DESC_HITBOX_ANCHOR = "Establece el punto de origen vertical del área interactiva de la placa.\nAjústalo para que coincida con cómo tu addon ancla los marcos de las placas (p. ej., anclaje por borde superior o inferior)."
    L.DESC_ALPHA_BLEND = "Controla qué tan rápido las placas cambian de opacidad (1 = Instantáneo)."
    L.DESC_STANCE_PATCH = "Permite cambiar de forma/actitud y lanzar una habilidad en un solo clic al usar macros."
    L.DESC_OCCLUSION_ALPHA = "Controla la opacidad de las placas de nombre cuando están bloqueadas por obstáculos o el terreno. Los valores positivos multiplican la opacidad actual de no-objetivo, los negativos imponen un límite máximo estricto sobre la opacidad ocluida."
	L.DESC_OCCLUSION_MODE = "Controla cuándo se ocultan o se desvanecen las placas de nombre al ser bloqueadas por el terreno o las paredes."
    L.DESC_CAMERA_INDIRECT_VISIBILITY = "Permite que la cámara atraviese ciertos objetos del mundo en lugar de ser bloqueada."
    L.DESC_CAMERA_INDIRECT_ALPHA = "Establece el nivel de transparencia de los objetos que se interponen entre la cámara y el jugador."
    L.DESC_CAMERA_DISTANCE_MAX = "Establece la distancia máxima que la cámara puede alejarse del jugador."
    L.DESC_MSDF = "Activa el renderizado de fuentes basado en vectores, mejorando drásticamente la calidad."
	L.DESC_OBJ_HIGHLIGHT = "Fuerza destellos brillantes en recursos (hierbas/menas) y objetos interactivos como cajas o tableros de anuncios."
	L.DESC_PORTRAIT = "Aumenta la resolución de la textura de renderizado para todos los retratos del juego."

	-- CVar Mode Options
	L.MODE_DISABLED = "Desactivado"
	L.MODE_ENABLED = "Activado"

    L.MODE_STACKING_DISABLED = "Superpuesto"
    L.MODE_STACKING_ALL = "Apilado (Todo)"
    L.MODE_STACKING_ENEMY = "Apilado (Enemigos)"
    L.MODE_STACKING_FRIENDLY = "Apilado (Aliados)"
    L.MODE_STACKING_SMART_ALL = "Apilado Inteligente (Todo)"
    L.MODE_STACKING_SMART_ENEMY = "Apilado Inteligente (Enemigos)"
    L.MODE_STACKING_SMART_FRIENDLY = "Apilado Inteligente (Aliados)"

    L.MODE_MOUSE_DISABLED = "Predeterminado"
    L.MODE_MOUSE_CLICKTHROUGH_ENEMY = "Clic a través (Enemigos)"
    L.MODE_MOUSE_CLICKTHROUGH_ENEMY_RAISE_FRIENDLY = "Clic Enm + Resaltar Aliado"
    L.MODE_MOUSE_CLICKTHROUGH_ENEMY_RAISE_FRIENDLY_COMBAT = "Clic Enm + Resaltar Aliado (Combate)"
    L.MODE_MOUSE_CLICKTHROUGH_FRIENDLY = "Clic a través (Aliados)"
    L.MODE_MOUSE_CLICKTHROUGH_FRIENDLY_RAISE_ENEMY = "Clic Aliado + Resaltar Enm"
    L.MODE_MOUSE_CLICKTHROUGH_FRIENDLY_RAISE_ENEMY_COMBAT = "Clic Aliado + Resaltar Enm (Combate)"
    L.MODE_MOUSE_RAISE_OCCLUDED = "Siempre resaltar ocluidos"
    L.MODE_MOUSE_RAISE_OCCLUDED_COMBAT = "Resaltar ocluidos (Solo combate)"

	L.MODE_CLAMP_ALL = "Fijar todo (solo arriba)"
	L.MODE_CLAMP_BOSSES = "Fijar solo jefes (solo arriba)"
	L.MODE_CLAMP_ALL_EDGES = "Fijar todo (todos los lados)"
	L.MODE_CLAMP_BOSSES_EDGES = "Fijar solo jefes (todos los lados)"

	L.MODE_OCCLUSION_ALWAYS = "Ocluir siempre"
	L.MODE_OCCLUSION_NOCOMBAT = "Solo fuera de combate"

    L.MODE_HITBOX_TOP = "Arriba"
    L.MODE_HITBOX_CENTER = "Centro"
    L.MODE_HITBOX_BOTTOM = "Abajo"

    L.MODE_LABEL_PLAYER_RADIUS = "Radio del jugador 20yd"
    L.MODE_LABEL_CONE_ANGLE = "Ángulo de cono (grados) dentro de 20yd"

	L.MODE_MSDF_ENABLED_UNSAFE = "Activado (fuentes no seguras): debido a cómo se calculan los campos de distancia, algunas fuentes con contornos que se cruzan a sí mesmos (por ejemplo, 'diediedie') pueden fallar."

	L.MODE_HIGHLIGHTS_TRACKED = "Solo rastreados"
end