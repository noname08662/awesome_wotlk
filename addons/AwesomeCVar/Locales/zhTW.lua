-- File: zhTW.lua
-- Language: Traditional Chinese
local addonName, AwesomeCVar = ...

if not AwesomeCVar.L then
    AwesomeCVar.L = {}
end

local L = AwesomeCVar.L

if GetLocale() == "zhTW" then
    -- 常規
    L.ADDON_NAME = "AwesomeCVar"
    L.ADDON_NAME_SHORT = "Awesome CVar"
    L.MAIN_FRAME_TITLE = "Awesome CVar 管理器"
    L.RESET_TO = "重設為 %s"

    -- 彈出視窗
    L.RELOAD_POPUP_TITLE = "需要重新載入介面"
    L.RELOAD_POPUP_TEXT = "你所做的一項或多項修改需要重新載入介面 (ReloadUI) 才能生效。"
    L.RESET_POPUP_TITLE = "確認重設預設值"
    L.RESET_POPUP_TEXT = "你確定要將所有數值重設回預設設定嗎？"

    -- 聊天訊息
    L.MSG_LOADED = "Awesome CVar 已載入！輸入 /awesome 打開管理器。"
    L.MSG_FRAME_RESET = "框架位置已重設至營幕中心。"
    L.MSG_SET_VALUE = "已將 %s 設置為 %s。"
    L.MSG_FRAME_CREATE_ERROR = "無法創建 AwesomeCVar 框架！"
    L.MSG_UNKNOWN_COMMAND = "未知指令。輸入 /awesome help 查看可用指令。"
    L.MSG_HELP_HEADER = "Awesome CVar 指令列表:"
    L.MSG_HELP_TOGGLE = "/awesome - 切換顯示/隱藏管理器"
    L.MSG_HELP_SHOW = "/awesome show - 顯示管理器"
    L.MSG_HELP_HIDE = "/awesome hide - 隱藏管理器"
    L.MSG_HELP_RESET = "/awesome reset - 重設框架位置到中心"
    L.MSG_HELP_HELP = "/awesome help - 顯示此幫助訊息"

    -- CVar 分類
    L.CATEGORY_CAMERA = "鏡頭"
    L.CATEGORY_NAMEPLATES = "姓名板"
    L.CATEGORY_TEXT_TO_SPEECH = "語音合成 (TTS)"
    L.CATEGORY_INTERACTION = "互動"
    L.CATEGORY_OTHER = "其他"

    -- CVar 標籤與描述
    L.CVAR_LABEL_INFO = "備註"
    L.CVAR_LABEL_TTS_VOICE = "TTS 語音"
    L.CVAR_LABEL_TTS_VOLUME = "TTS 音量"
    L.CVAR_LABEL_TTS_SPEED = "TTS 語速"
    L.CVAR_LABEL_CAMERA_FOV = "鏡頭視野 (FoV)"
    L.CVAR_LABEL_STACKING_MODE = "姓名板堆疊模式"
    L.CVAR_LABEL_MOUSEOVER = "姓名板鼠標懸停模式"
    L.CVAR_LABEL_OCCLUSION_ALPHA = "姓名板被遮擋透明度"
    L.CVAR_LABEL_NONTARGET_ALPHA = "非目標姓名板透明度"
    L.CVAR_LABEL_ALPHA_SPEED = "姓名板透明度漸變速度"
    L.CVAR_LABEL_CLAMP_TOP = "姓名板頂部鎖定模式"
    L.CVAR_LABEL_NAMEPLATE_DISTANCE = "姓名板渲染距離"
    L.CVAR_LABEL_MAX_RAISE_DISTANCE = "堆疊最大垂直提升距離"
    L.CVAR_LABEL_MAX_PULL_DISTANCE = "堆疊最大水平拉伸距離"
    L.CVAR_LABEL_X_SPACE = "姓名板橫向間距"
    L.CVAR_LABEL_Y_SPACE = "姓名板縱向間距"
    L.CVAR_LABEL_UPPER_BORDER = "姓名板頂部鎖定偏移"
    L.CVAR_LABEL_PLACEMENT = "姓名板佈局偏移"
    L.CVAR_LABEL_SPEED_RAISE = "姓名板堆疊上升速度"
    L.CVAR_LABEL_SPEED_LOWER = "姓名板堆疊下降速度"
    L.CVAR_LABEL_SPEED_PULL = "姓名板堆疊水平拉伸速度"
    L.CVAR_LABEL_HITBOX_HEIGHT_ENEMY = "敵方姓名板點擊判定高度"
    L.CVAR_LABEL_HITBOX_WIDTH_ENEMY = "敵方姓名板點擊判定寬度"
    L.CVAR_LABEL_HITBOX_HEIGHT_FRIENDLY = "友好姓名板點擊判定高度"
    L.CVAR_LABEL_HITBOX_WIDTH_FRIENDLY = "友好姓名板點擊判定寬度"
    L.CVAR_LABEL_INTERACTION_MODE = "互動模式"
    L.CVAR_LABEL_INTERACTION_ANGLE = "互動錐形角度 (度)"
    L.CVAR_STANCE_PATCH = "姿態/形態切換補丁"
    L.CVAR_SHOW_PLAYER = "自身角色模型渲染"
    L.CVAR_MSDF_MODE = "字體渲染模式 (需要重啟遊戲)"
    L.CVAR_LABEL_CAMERA_INDIRECT_VISIBILITY = "鏡頭間接可見性"
    L.CVAR_LABEL_CAMERA_INDIRECT_ALPHA = "鏡頭間接透明度"
    L.CVAR_LABEL_CAMERA_DISTANCE_MAX = "鏡頭最大距離"

    L.DESC_INFO = "\n所有姓名板現在包含以下額外方法：\n- SetStackingEnabled(bool)\n- GetStackingEnabled()\n\n用法範例：\nif UnitExists('target') then\n C_NamePlate.GetNamePlateForUnit('target'):SetStackingEnabled(false)\nend\n\n注意：在 NAME_PLATE_CREATED 事件下\n調用這些方法過早，\n請改用 NAME_PLATE_UNIT_ADDED。\n"
    L.DESC_STACKING_MODE = "0 = 禁用；1 = 開啟全部；2 = 僅敵方；3 = 僅友好。"
    L.DESC_MOUSEOVER = "0 = 禁用；\n1 = 點擊穿透敵人；\n2 = 點擊穿透敵人，總是提升被遮擋的鼠標懸停友好姓名板層級；\n3 = 點擊穿透敵人，戰鬥中提升被遮擋的友好姓名板層級；\n4 = 點擊穿透盟友；\n5 = 點擊穿透盟友，總是提升被遮擋的鼠標懸停敵方姓名板層級；\n6 = 點擊穿透盟友，戰鬥中提升被遮擋的敵方姓名板層級；\n7 = 總是提升鼠標懸停的被遮擋姓名板層級；\n8 = 僅在戰鬥中提升鼠標懸停的被遮擋姓名板層級。"
    L.DESC_ALPHA_BLEND = "控制姓名板切換到新透明度的動畫速度 (1 = 瞬間)。"
    L.DESC_CLAMP_TOP = "0 = 禁用；1 = 全部鎖定；2 = 僅鎖定首領。"
    L.DESC_STANCE_PATCH = "允許在使用巨集時，透過單次點擊完成姿態/形態切換並施放技能。"
    L.DESC_OCCLUSION_ALPHA = "控制當姓名板被障礙物或地形阻擋時的透明度。"
    L.DESC_CAMERA_INDIRECT_VISIBILITY = "允許鏡頭穿過某些世界物體而非被阻擋。"
    L.DESC_CAMERA_INDIRECT_ALPHA = "設置鏡頭與玩家角色之間的遮擋物透明度。"
    L.DESC_CAMERA_DISTANCE_MAX = "設置鏡頭可以從玩家身上拉遠的最大距離。"
    L.DESC_MSDF = "開啟基於向量的字體渲染，大幅提升字型品質。\n0 = 禁用；1 = 開啟；\n2 = 開啟（不安全字體）——由於距離場計算方式，\n某些具有自交輪廓的字體可能會顯示異常。"

    -- CVar 模式選項
    L.MODE_LABEL_PLAYER_RADIUS = "玩家半徑 20碼"
    L.MODE_LABEL_CONE_ANGLE = "20碼內的錐形角度 (度)"
end