-- File: zhCN.lua
-- Language: Simplified Chinese
local _, AwesomeCVar = ...

if not AwesomeCVar.L then
    AwesomeCVar.L = {}
end

local L = AwesomeCVar.L

if GetLocale() == "zhCN" then
    -- 常规
    L.ADDON_NAME = "AwesomeCVar"
    L.ADDON_NAME_SHORT = "Awesome CVar"
    L.MAIN_FRAME_TITLE = "Awesome CVar 管理器"
    L.RESET_TO = "重置为 %s"
	L.MINIMAP_ICON = "小地图图标"
	L.MINIMAP_TOOLTIP = "点击打开\n拖动移动"
	L.GAME_MENU_BUTTON = "游戏菜单按钮"
	L.OPEN_ADDON = "打开 AwesomeCVar"

    -- 弹出窗口
    L.RELOAD_POPUP_TITLE = "需要重载界面"
    L.RELOAD_POPUP_TEXT = "你所做的一项或多项修改需要重载界面 (ReloadUI) 才能生效。"
    L.RESET_POPUP_TITLE = "确认重置默认值"
    L.RESET_POPUP_TEXT = "你确定要将所有数值重置回默认设置吗？"

    -- 聊天消息
    L.MSG_LOADED = "Awesome CVar 已加载！输入 /awesome 打开管理器。"
    L.MSG_FRAME_RESET = "框架位置已重置至屏幕中心。"
    L.MSG_SET_VALUE = "已将 %s 设置为 %s。"
    L.MSG_FRAME_CREATE_ERROR = "无法创建 AwesomeCVar 框架！"
    L.MSG_UNKNOWN_COMMAND = "未知命令。输入 /awesome help 查看可用命令。"
    L.MSG_HELP_HEADER = "Awesome CVar 命令列表:"
    L.MSG_HELP_TOGGLE = "/awesome - 切换显示/隐藏管理器"
    L.MSG_HELP_SHOW = "/awesome show - 显示管理器"
    L.MSG_HELP_HIDE = "/awesome hide - 隐藏管理器"
    L.MSG_HELP_RESET = "/awesome reset - 重置框架位置到中心"
    L.MSG_HELP_HELP = "/awesome help - 显示此帮助信息"

    -- CVar 分类
    L.CATEGORY_CAMERA = "镜头"
    L.CATEGORY_NAMEPLATES = "姓名板"
    L.CATEGORY_TEXT_TO_SPEECH = "文字转语音 (TTS)"
    L.CATEGORY_INTERACTION = "交互"
    L.CATEGORY_OTHER = "其他"

    -- CVar 标签与描述
    L.CVAR_LABEL_INFO = "备注"
    L.CVAR_LABEL_TTS_VOICE = "TTS 语音"
    L.CVAR_LABEL_TTS_VOLUME = "TTS 音量"
    L.CVAR_LABEL_TTS_SPEED = "TTS 语速"
    L.CVAR_LABEL_CAMERA_FOV = "镜头视野 (FoV)"
    L.CVAR_LABEL_STACKING_MODE = "姓名板堆叠模式"
    L.CVAR_LABEL_MOUSEOVER = "姓名板鼠标悬停模式"
    L.CVAR_LABEL_OCCLUSION_ALPHA = "姓名板被遮挡透明度"
	L.CVAR_LABEL_OCCLUSION_MODE = "姓名板 translucent/遮挡模式"
    L.CVAR_LABEL_NONTARGET_ALPHA = "非目标姓名板透明度"
    L.CVAR_LABEL_ALPHA_SPEED = "姓名板透明度渐变速度"
    L.CVAR_LABEL_CLAMP_MODE = "姓名板贴边模式"
	L.CVAR_LABEL_NAMEPLATE_DISTANCE = "姓名板显示距离"
    L.CVAR_LABEL_MAX_RAISE_DISTANCE = "堆叠最大垂直提升距离"
    L.CVAR_LABEL_MAX_PULL_DISTANCE = "堆叠最大水平拉伸距离"
    L.CVAR_LABEL_X_SPACE = "姓名板横向间距"
    L.CVAR_LABEL_Y_SPACE = "姓名板纵向间距"
	L.CVAR_LABEL_V_OFFSET = "贴边姓名板垂直偏移"
	L.CVAR_LABEL_H_OFFSET = "贴边姓名板水平偏移"
    L.CVAR_LABEL_PLACEMENT = "姓名板布局偏移"
    L.CVAR_LABEL_SPEED_RAISE = "姓名板堆叠上升速度"
    L.CVAR_LABEL_SPEED_LOWER = "姓名板堆叠下降速度"
    L.CVAR_LABEL_SPEED_PULL = "姓名板堆叠水平拉伸速度"
	L.CVAR_LABEL_INERTIA = "姓名板堆叠惯性"
	L.CVAR_LABEL_HYST_DECAY = "姓名板配对分离速度"
	L.CVAR_LABEL_HITBOX_ANCHOR = "姓名板碰撞盒锚点"
    L.CVAR_LABEL_HITBOX_HEIGHT_ENEMY = "敌方姓名板点击判定高度"
    L.CVAR_LABEL_HITBOX_WIDTH_ENEMY = "敌方姓名板点击判定宽度"
    L.CVAR_LABEL_HITBOX_HEIGHT_FRIENDLY = "友好姓名板点击判定高度"
    L.CVAR_LABEL_HITBOX_WIDTH_FRIENDLY = "友好姓名板点击判定宽度"
    L.CVAR_LABEL_INTERACTION_MODE = "交互模式"
    L.CVAR_LABEL_INTERACTION_ANGLE = "交互锥形角度 (度)"
    L.CVAR_LABEL_SHOW_PLAYER = "自身角色模型渲染"
    L.CVAR_LABEL_MSDF_MODE = "字体渲染模式 (需要重启游戏)"
	L.CVAR_LABEL_OBJ_HIGHLIGHT = "物体高亮显示"
    L.CVAR_LABEL_CAMERA_INDIRECT_VISIBILITY = "镜头间接可见性"
    L.CVAR_LABEL_CAMERA_INDIRECT_ALPHA = "镜头间接透明度"
    L.CVAR_LABEL_CAMERA_DISTANCE_MAX = "镜头最大距离"
	L.CVAR_LABEL_PORTRAIT = "头像分辨率"
	L.CVAR_LABEL_CHAT_LOG = "聊天记录会话时间戳"
	L.CVAR_LABEL_COMBAT_LOG = "战斗记录会话时间戳"

    L.DESC_INFO = "所有姓名板现在包含以下额外方法：\n- SetStackingEnabled(bool)\n- GetStackingEnabled()\n\n用法示例：\nif UnitExists('target') then\n C_NamePlate.GetNamePlateForUnit('target'):SetStackingEnabled(false)\nend\n\n注意：在 NAME_PLATE_CREATED 事件下\n调用这些方法过早，\n请改用 NAME_PLATE_UNIT_ADDED。\n"
	L.DESC_STACKING_MODE = "“智能”模式允许姓名板在下方有足够空间时跳过堆叠推挤，从而实现更紧凑的布局，但会导致更频繁的重新排列。"
    L.DESC_MOUSEOVER = "“提升”选项会提高鼠标悬停姓名板的层级，使其显示在其他姓名板之上。\n当前目标的姓名板层级依然是最高的。"
	L.DESC_INERTIA = "控制姓名板在堆叠时移动的物理重量感。\n数值越高响应越灵敏；数值越低移动越沉重、阻尼感越强。"
	L.DESC_HYST_DECAY = "控制姓名板不再重叠后堆叠配对解除的速度。\n数值越高分离越快；数值越低配对维持时间越长。"
	L.DESC_PLACEMENT = "垂直偏移比例值，用于将姓名板从其默认锚点位置移动。"
	L.DESC_HITBOX_ANCHOR = "设置姓名板可点击区域的垂直原点。\n根据你的UI插件锚定姓名板框架的方式进行调整（例如顶部或底部锚定）。"
    L.DESC_ALPHA_BLEND = "控制姓名板切换到新透明度的动画速度 (1 = 瞬间)。"
    L.DESC_OCCLUSION_ALPHA = "控制姓名板被障碍物或地形遮挡时的透明度。正值会与当前非目标透明度相乘，负值则对被遮挡的透明度强制应用严格的最高上限。"
	L.DESC_OCCLUSION_MODE = "控制当姓名板被地形或墙壁遮挡时，何时隐藏或淡出。"
    L.DESC_CAMERA_INDIRECT_VISIBILITY = "允许镜头穿过某些世界物体而非被阻挡。"
    L.DESC_CAMERA_INDIRECT_ALPHA = "设置镜头与玩家角色之间的遮挡物透明度。"
    L.DESC_CAMERA_DISTANCE_MAX = "设置镜头可以从玩家身上拉远的最大距离。"
    L.DESC_MSDF = "开启基于矢量的字体渲染，大幅提升字型质量。"
	L.DESC_OBJ_HIGHLIGHT = "强制在资源（草药/矿石）和可交互的物体（如箱子或悬赏通缉令告示牌）上显示闪烁光芒效果。"
	L.DESC_PORTRAIT = "提高游戏内所有头像材质的渲染分辨率。"
	L.DESC_SESSION_LOG = "自动在日志文件名后添加时间戳。"

	-- CVar 模式选项
	L.MODE_DISABLED = "已禁用"
	L.MODE_ENABLED = "已启用"

    L.MODE_STACKING_DISABLED = "重叠"
    L.MODE_STACKING_ALL = "堆叠 (全部)"
    L.MODE_STACKING_ENEMY = "堆叠 (敌人)"
    L.MODE_STACKING_FRIENDLY = "堆叠 (友方)"
    L.MODE_STACKING_SMART_ALL = "智能堆叠 (全部)"
    L.MODE_STACKING_SMART_ENEMY = "智能堆叠 (敌人)"
    L.MODE_STACKING_SMART_FRIENDLY = "智能堆叠 (友方)"

    L.MODE_MOUSE_DISABLED = "默认"
    L.MODE_MOUSE_CLICKTHROUGH_ENEMY = "敌人点击穿透"
    L.MODE_MOUSE_CLICKTHROUGH_ENEMY_RAISE_FRIENDLY = "敌人穿透 + 提升友方层级"
    L.MODE_MOUSE_CLICKTHROUGH_ENEMY_RAISE_FRIENDLY_COMBAT = "敌人穿透 + 提升友方 (战斗中)"
    L.MODE_MOUSE_CLICKTHROUGH_FRIENDLY = "友方点击穿透"
    L.MODE_MOUSE_CLICKTHROUGH_FRIENDLY_RAISE_ENEMY = "友方穿透 + 提升敌人层级"
    L.MODE_MOUSE_CLICKTHROUGH_FRIENDLY_RAISE_ENEMY_COMBAT = "友方穿透 + 提升敌人 (战斗中)"
    L.MODE_MOUSE_RAISE_OCCLUDED = "总是提升被遮挡的姓名板"
    L.MODE_MOUSE_RAISE_OCCLUDED_COMBAT = "提升被遮挡姓名板 (仅战斗)"

	L.MODE_CLAMP_ALL = "全部贴边 (仅限顶部)"
	L.MODE_CLAMP_BOSSES = "仅首领贴边 (仅限顶部)"
	L.MODE_CLAMP_ALL_EDGES = "全部贴边 (所有边缘)"
	L.MODE_CLAMP_BOSSES_EDGES = "仅首领贴边 (所有边缘)"

	L.MODE_OCCLUSION_ALWAYS = "总是遮挡"
	L.MODE_OCCLUSION_NOCOMBAT = "仅在非战斗状态"

    L.MODE_HITBOX_TOP = "顶部"
    L.MODE_HITBOX_CENTER = "居中"
    L.MODE_HITBOX_BOTTOM = "底部"

    L.MODE_LABEL_PLAYER_RADIUS = "玩家半径 20码"
    L.MODE_LABEL_CONE_ANGLE = "20码内的锥形角度 (度)"

	L.MODE_MSDF_ENABLED_UNSAFE = "启用（不安全字体）— 由于距离场（distance fields）的计算方式，某些具有自相交轮廓的字体（例如 'diediedie'）可能会发生错乱。"

	L.MODE_HIGHLIGHTS_TRACKED = "仅已追踪"
end