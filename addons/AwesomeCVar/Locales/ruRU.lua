-- File: ruRU.lua
-- Language: Russian
local addonName, AwesomeCVar = ...

if not AwesomeCVar.L then
    AwesomeCVar.L = {}
end

local L = AwesomeCVar.L

if GetLocale() == "ruRU" then
    -- Общее
    L.ADDON_NAME = "AwesomeCVar"
    L.ADDON_NAME_SHORT = "Awesome CVar"
    L.MAIN_FRAME_TITLE = "Менеджер Awesome CVar"
    L.RESET_TO = "Сбросить на %s"

    -- Всплывающие окна
    L.RELOAD_POPUP_TITLE = "Требуется перезагрузка интерфейса"
    L.RELOAD_POPUP_TEXT = "Одно или несколько внесенных изменений требуют перезагрузки интерфейса (ReloadUI) для вступления в силу."
    L.RESET_POPUP_TITLE = "Подтверждение сброса"
    L.RESET_POPUP_TEXT = "Вы уверены, что хотите сбросить все значения до настроек по умолчанию?"

    -- Сообщения в чате
    L.MSG_LOADED = "Awesome CVar загружен! Введите /awesome, чтобы открыть менеджер."
    L.MSG_FRAME_RESET = "Положение окна было сброшено в центр."
    L.MSG_SET_VALUE = "%s установлено на %s."
    L.MSG_FRAME_CREATE_ERROR = "Не удалось создать окно AwesomeCVar!"
    L.MSG_UNKNOWN_COMMAND = "Неизвестная команда. Введите /awesome help для списка команд."
    L.MSG_HELP_HEADER = "Команды Awesome CVar:"
    L.MSG_HELP_TOGGLE = "/awesome - Показать/скрыть менеджер CVar"
    L.MSG_HELP_SHOW = "/awesome show - Показать менеджер CVar"
    L.MSG_HELP_HIDE = "/awesome hide - Скрыть менеджер CVar"
    L.MSG_HELP_RESET = "/awesome reset - Сбросить положение окна в центр"
    L.MSG_HELP_HELP = "/awesome help - Показать это справочное сообщение"

    -- Категории CVar
    L.CATEGORY_CAMERA = "Камера"
    L.CATEGORY_NAMEPLATES = "Индикаторы здоровья"
    L.CATEGORY_TEXT_TO_SPEECH = "Синтез речи (TTS)"
    L.CATEGORY_INTERACTION = "Взаимодействие"
    L.CATEGORY_OTHER = "Прочее"

    -- Метки и описания CVar
    L.CVAR_LABEL_INFO = "Заметки"
    L.CVAR_LABEL_TTS_VOICE = "Голос TTS"
    L.CVAR_LABEL_TTS_VOLUME = "Громкость TTS"
    L.CVAR_LABEL_TTS_SPEED = "Скорость TTS"
    L.CVAR_LABEL_CAMERA_FOV = "Угол обзора камеры (FoV)"
    L.CVAR_LABEL_STACKING_MODE = "Режим наложения индикаторов"
    L.CVAR_LABEL_MOUSEOVER = "Режим наведения (Mouseover)"
    L.CVAR_LABEL_OCCLUSION_ALPHA = "Прозрачность индикаторов за препятствиями"
    L.CVAR_LABEL_NONTARGET_ALPHA = "Прозрачность индикаторов не-цели"
    L.CVAR_LABEL_ALPHA_SPEED = "Скорость изменения прозрачности"
    L.CVAR_LABEL_CLAMP_TOP = "Прижатие индикаторов к верхнему краю"
    L.CVAR_LABEL_NAMEPLATE_DISTANCE = "Дистанция отрисовки индикаторов"
    L.CVAR_LABEL_MAX_RAISE_DISTANCE = "Макс. верт. расстояние при наложении"
    L.CVAR_LABEL_MAX_PULL_DISTANCE = "Макс. гор. расстояние при наложении"
    L.CVAR_LABEL_X_SPACE = "Межстрочный интервал X"
    L.CVAR_LABEL_Y_SPACE = "Межстрочный интервал Y"
    L.CVAR_LABEL_UPPER_BORDER = "Смещение прижатия к верхнему краю"
    L.CVAR_LABEL_PLACEMENT = "Смещение размещения"
    L.CVAR_LABEL_SPEED_RAISE = "Скорость подъема при наложении"
    L.CVAR_LABEL_SPEED_LOWER = "Скорость опускания при наложении"
    L.CVAR_LABEL_SPEED_PULL = "Скорость гор. сближения при наложении"
    L.CVAR_LABEL_HITBOX_HEIGHT_ENEMY = "Высота хитбокса (Враги)"
    L.CVAR_LABEL_HITBOX_WIDTH_ENEMY = "Ширина хитбокса (Враги)"
    L.CVAR_LABEL_HITBOX_HEIGHT_FRIENDLY = "Высота хитбокса (Союзники)"
    L.CVAR_LABEL_HITBOX_WIDTH_FRIENDLY = "Ширина хитбокса (Союзники)"
    L.CVAR_LABEL_INTERACTION_MODE = "Режим взаимодействия"
    L.CVAR_LABEL_INTERACTION_ANGLE = "Угол конуса взаимодействия (град.)"
    L.CVAR_STANCE_PATCH = "Исправление смены облика/стойки"
    L.CVAR_SHOW_PLAYER = "Отрисовка модели игрока"
    L.CVAR_MSDF_MODE = "Режим отрисовки шрифтов (Нужен перезапуск)"
    L.CVAR_LABEL_CAMERA_INDIRECT_VISIBILITY = "Косвенная видимость камеры"
    L.CVAR_LABEL_CAMERA_INDIRECT_ALPHA = "Косвенная прозрачность камеры"
    L.CVAR_LABEL_CAMERA_DISTANCE_MAX = "Макс. дистанция камеры"

    L.DESC_INFO = "\nВсе индикаторы включают дополнительные методы:\n- SetStackingEnabled(bool)\n- GetStackingEnabled()\n\nПример использования:\nif UnitExists('target') then\n C_NamePlate.GetNamePlateForUnit('target'):SetStackingEnabled(false)\nend\n\nПримечание: Эти методы нельзя вызывать\nпри NAME_PLATE_CREATED,\nиспользуйте NAME_PLATE_UNIT_ADDED.\n"
    L.DESC_STACKING_MODE = "0 = Выключено; 1 = Включить все; 2 = Только враги; 3 = Только союзники."
    L.DESC_MOUSEOVER = "0 = Выключено;\n1 = Клик сквозь врагов;\n2 = Клик сквозь врагов, всегда поднимать уровень союзных индикаторов за препятствиями;\n3 = Клик сквозь врагов, поднимать уровень союзных индикаторов в бою;\n4 = Клик сквозь союзников;\n5 = Клик сквозь союзников, всегда поднимать уровень вражеских индикаторов;\n6 = Клик сквозь союзников, поднимать уровень вражеских индикаторов в бою;\n7 = Всегда поднимать уровень индикатора под курсором за препятствием;\n8 = Поднимать уровень индикатора под курсором в бою."
    L.DESC_ALPHA_BLEND = "Управляет скоростью изменения прозрачности индикаторов (1 = Мгновенно)."
    L.DESC_CLAMP_TOP = "0 = Выключено; 1 = Прижимать все; 2 = Прижимать только боссов."
    L.DESC_STANCE_PATCH = "Позволяет менять стойку/облик и применять способность одним кликом при использовании макросов."
    L.DESC_OCCLUSION_ALPHA = "Прозрачность индикаторов, когда они перекрыты препятствиями или ландшафтом."
    L.DESC_CAMERA_INDIRECT_VISIBILITY = "Позволяет камере проходить сквозь некоторые объекты мира вместо блокировки."
    L.DESC_CAMERA_INDIRECT_ALPHA = "Задает уровень прозрачности объектов между камерой и персонажем."
    L.DESC_CAMERA_DISTANCE_MAX = "Устанавливает максимальную дистанцию отдаления камеры."
    L.DESC_MSDF = "Включает векторную отрисовку шрифтов, значительно улучшая четкость.\n0 = Выключено; 1 = Включено;\n2 = Включено (небезопасные шрифты) — из-за особенностей расчета полей расстояний,\nнекоторые шрифты с самопересекающимися контурами могут ломаться."

    -- Опции режима CVar
    L.MODE_LABEL_PLAYER_RADIUS = "Радиус игрока (20 ярдов)"
    L.MODE_LABEL_CONE_ANGLE = "Угол конуса (град.) в пределах 20 ярдов"
end