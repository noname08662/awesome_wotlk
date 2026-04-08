-- File: zhCN.lua
-- Language: Simplified Chinese
local addonName, AwesomeCVar = ...

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
    L.CVAR_LABEL_NONTARGET_ALPHA = "非目标姓名板透明度"
    L.CVAR_LABEL_ALPHA_SPEED = "姓名板透明度渐变速度"
    L.CVAR_LABEL_CLAMP_TOP = "姓名板顶部锁定模式"
    L.CVAR_LABEL_NAMEPLATE_DISTANCE = "姓名板渲染距离"
    L.CVAR_LABEL_MAX_RAISE_DISTANCE = "堆叠最大垂直提升距离"
    L.CVAR_LABEL_MAX_PULL_DISTANCE = "堆叠最大水平拉伸距离"
    L.CVAR_LABEL_X_SPACE = "姓名板横向间距"
    L.CVAR_LABEL_Y_SPACE = "姓名板纵向间距"
    L.CVAR_LABEL_UPPER_BORDER = "姓名板顶部锁定偏移"
    L.CVAR_LABEL_PLACEMENT = "姓名板布局偏移"
    L.CVAR_LABEL_SPEED_RAISE = "姓名板堆叠上升速度"
    L.CVAR_LABEL_SPEED_LOWER = "姓名板堆叠下降速度"
    L.CVAR_LABEL_SPEED_PULL = "姓名板堆叠水平拉伸速度"
    L.CVAR_LABEL_HITBOX_HEIGHT_ENEMY = "敌方姓名板点击判定高度"
    L.CVAR_LABEL_HITBOX_WIDTH_ENEMY = "敌方姓名板点击判定宽度"
    L.CVAR_LABEL_HITBOX_HEIGHT_FRIENDLY = "友好姓名板点击判定高度"
    L.CVAR_LABEL_HITBOX_WIDTH_FRIENDLY = "友好姓名板点击判定宽度"
    L.CVAR_LABEL_INTERACTION_MODE = "交互模式"
    L.CVAR_LABEL_INTERACTION_ANGLE = "交互锥形角度 (度)"
    L.CVAR_STANCE_PATCH = "姿态/形态切换补丁"
    L.CVAR_SHOW_PLAYER = "自身角色模型渲染"
    L.CVAR_MSDF_MODE = "字体渲染模式 (需要重启游戏)"
    L.CVAR_LABEL_CAMERA_INDIRECT_VISIBILITY = "镜头间接可见性"
    L.CVAR_LABEL_CAMERA_INDIRECT_ALPHA = "镜头间接透明度"
    L.CVAR_LABEL_CAMERA_DISTANCE_MAX = "镜头最大距离"

    L.DESC_INFO = "\n所有姓名板现在包含以下额外方法：\n- SetStackingEnabled(bool)\n- GetStackingEnabled()\n\n用法示例：\nif UnitExists('target') then\n C_NamePlate.GetNamePlateForUnit('target'):SetStackingEnabled(false)\nend\n\n注意：在 NAME_PLATE_CREATED 事件下\n调用这些方法过早，\n请改用 NAME_PLATE_UNIT_ADDED。\n"
    L.DESC_STACKING_MODE = "0 = 禁用；1 = 开启全部；2 = 仅敌方；3 = 仅友好。"
    L.DESC_MOUSEOVER = "0 = 禁用；\n1 = 点击穿透敌人；\n2 = 点击穿透敌人，总是提升被遮挡的鼠标悬停友好姓名板层级；\n3 = 点击穿透敌人，战斗中提升被遮挡的友好姓名板层级；\n4 = 点击穿透盟友；\n5 = 点击穿透盟友，总是提升被遮挡的鼠标悬停敌方姓名板层级；\n6 = 点击穿透盟友，战斗中提升被遮挡的敌方姓名板层级；\n7 = 总是提升鼠标悬停的被遮挡姓名板层级；\n8 = 仅在战斗中提升鼠标悬停的被遮挡姓名板层级。"
    L.DESC_ALPHA_BLEND = "控制姓名板切换到新透明度的动画速度 (1 = 瞬间)。"
    L.DESC_CLAMP_TOP = "0 = 禁用；1 = 全部锁定；2 = 仅锁定首领。"
    L.DESC_STANCE_PATCH = "允许在使用宏时，通过单次点击完成姿态/形态切换并施放技能。"
    L.DESC_OCCLUSION_ALPHA = "控制当姓名板被障碍物或地形阻挡时的透明度。"
    L.DESC_CAMERA_INDIRECT_VISIBILITY = "允许镜头穿过某些世界物体而非被阻挡。"
    L.DESC_CAMERA_INDIRECT_ALPHA = "设置镜头与玩家角色之间的遮挡物透明度。"
    L.DESC_CAMERA_DISTANCE_MAX = "设置镜头可以从玩家身上拉远的最大距离。"
    L.DESC_MSDF = "开启基于矢量的字体渲染，大幅提升字型质量。\n0 = 禁用；1 = 开启；\n2 = 开启（不安全字体）——由于距离场计算方式，\n某些具有自交轮廓的字体可能会显示异常。"

    -- CVar 模式选项
    L.MODE_LABEL_PLAYER_RADIUS = "玩家半径 20码"
    L.MODE_LABEL_CONE_ANGLE = "20码内的锥形角度 (度)"
end