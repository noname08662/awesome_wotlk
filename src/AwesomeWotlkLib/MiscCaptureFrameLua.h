#pragma once

namespace misc_capture_frame_lua {
inline constexpr auto kFrameCaptureCommands = R"lua(
local tag = "|cff33ff99FrameCapture|r: "

local strings = {
    FRAMECAPTURE_CAPTURING = "capturing %s",
    FRAMECAPTURE_NO_FRAME_NAMED = "no frame named %s",
    FRAMECAPTURE_NO_MOUSE_FRAME = "no mouse-enabled frame under the cursor, name it instead: %s ChatFrame1",
    FRAMECAPTURE_WORLD_FRAME = "WorldFrame draws the 3D world and can't be captured",
    FRAMECAPTURE_INVALID_SIZE = "invalid size '%s', expected 2048 or 2x",
    FRAMECAPTURE_NO_DEVICE = "no d3d9 device",
    FRAMECAPTURE_NO_SIZE = "nothing to capture (the frame is hidden or has no size)",
    FRAMECAPTURE_NO_RENDER_TARGET = "could not allocate a %dx%d render target",
    FRAMECAPTURE_NO_READBACK_SURFACES = "could not allocate the %dx%d readback surfaces",
    FRAMECAPTURE_RENDER_FAILED = "rendering or resolving the capture failed",
    FRAMECAPTURE_NOTHING_DRAWN = "nothing was drawn (hidden or fully transparent?)",
    FRAMECAPTURE_READBACK_FAILED = "readback failed",
    FRAMECAPTURE_OUT_OF_MEMORY = "out of memory for the image, try a smaller size",
    FRAMECAPTURE_WRITE_FAILED = "could not write %s",
    FRAMECAPTURE_HOOKS_FAILED = "could not install the render hooks",
    FRAMECAPTURE_CLAMPED = ", clamped from %.2fx",
    FRAMECAPTURE_MSAA = "%dx MSAA",
    FRAMECAPTURE_NO_MSAA = "no MSAA",
}

local locales = {}
)lua"
                                              R"lua(
locales.deDE = {
    FRAMECAPTURE_CAPTURING = "erfasse %s",
    FRAMECAPTURE_NO_FRAME_NAMED = "kein Frame namens %s",
    FRAMECAPTURE_NO_MOUSE_FRAME = "kein mausaktiver Frame unter dem Cursor, gib stattdessen einen Namen an: %s ChatFrame1",
    FRAMECAPTURE_WORLD_FRAME = "WorldFrame zeichnet die 3D-Welt und kann nicht erfasst werden",
    FRAMECAPTURE_INVALID_SIZE = "ungültige Größe '%s', erwartet: 2048 oder 2x",
    FRAMECAPTURE_NO_DEVICE = "kein D3D9-Gerät",
    FRAMECAPTURE_NO_SIZE = "nichts zu erfassen (der Frame ist ausgeblendet oder hat keine Größe)",
    FRAMECAPTURE_NO_RENDER_TARGET = "Render-Target mit %dx%d konnte nicht angelegt werden",
    FRAMECAPTURE_NO_READBACK_SURFACES = "Readback-Oberflächen mit %dx%d konnten nicht angelegt werden",
    FRAMECAPTURE_RENDER_FAILED = "Rendern oder Auflösen der Aufnahme fehlgeschlagen",
    FRAMECAPTURE_NOTHING_DRAWN = "es wurde nichts gezeichnet (ausgeblendet oder vollständig transparent?)",
    FRAMECAPTURE_READBACK_FAILED = "Auslesen fehlgeschlagen",
    FRAMECAPTURE_OUT_OF_MEMORY = "nicht genug Speicher für das Bild, versuche eine kleinere Größe",
    FRAMECAPTURE_WRITE_FAILED = "%s konnte nicht geschrieben werden",
    FRAMECAPTURE_HOOKS_FAILED = "die Render-Hooks konnten nicht installiert werden",
    FRAMECAPTURE_CLAMPED = ", reduziert von %.2fx",
    FRAMECAPTURE_NO_MSAA = "kein MSAA",
}

locales.esMX = {
    FRAMECAPTURE_CAPTURING = "capturando %s",
    FRAMECAPTURE_NO_FRAME_NAMED = "no hay ningún marco llamado %s",
    FRAMECAPTURE_NO_MOUSE_FRAME = "no hay ningún marco con el mouse habilitado bajo el cursor; indica su nombre: %s ChatFrame1",
    FRAMECAPTURE_WORLD_FRAME = "WorldFrame dibuja el mundo 3D y no se puede capturar",
    FRAMECAPTURE_INVALID_SIZE = "tamaño no válido '%s', se esperaba 2048 o 2x",
    FRAMECAPTURE_NO_DEVICE = "no hay dispositivo D3D9",
    FRAMECAPTURE_NO_SIZE = "no hay nada que capturar (el marco está oculto o no tiene tamaño)",
    FRAMECAPTURE_NO_RENDER_TARGET = "no se pudo reservar un render target de %dx%d",
    FRAMECAPTURE_NO_READBACK_SURFACES = "no se pudieron reservar las superficies de lectura de %dx%d",
    FRAMECAPTURE_RENDER_FAILED = "falló el renderizado o la resolución de la captura",
    FRAMECAPTURE_NOTHING_DRAWN = "no se dibujó nada (¿oculto o totalmente transparente?)",
    FRAMECAPTURE_READBACK_FAILED = "falló la lectura",
    FRAMECAPTURE_OUT_OF_MEMORY = "no hay memoria suficiente para la imagen, prueba con un tamaño menor",
    FRAMECAPTURE_WRITE_FAILED = "no se pudo escribir %s",
    FRAMECAPTURE_HOOKS_FAILED = "no se pudieron instalar los hooks de renderizado",
    FRAMECAPTURE_CLAMPED = ", reducido desde %.2fx",
    FRAMECAPTURE_NO_MSAA = "sin MSAA",
}

locales.frFR = {
    FRAMECAPTURE_CAPTURING = "capture de %s",
    FRAMECAPTURE_NO_FRAME_NAMED = "aucun cadre nommé %s",
    FRAMECAPTURE_NO_MOUSE_FRAME = "aucun cadre réactif à la souris sous le curseur, indiquez plutôt son nom : %s ChatFrame1",
    FRAMECAPTURE_WORLD_FRAME = "WorldFrame dessine le monde 3D et ne peut pas être capturé",
    FRAMECAPTURE_INVALID_SIZE = "taille non valide '%s', attendu : 2048 ou 2x",
    FRAMECAPTURE_NO_DEVICE = "aucun périphérique D3D9",
    FRAMECAPTURE_NO_SIZE = "rien à capturer (le cadre est masqué ou n’a pas de taille)",
    FRAMECAPTURE_NO_RENDER_TARGET = "impossible d’allouer une cible de rendu de %dx%d",
    FRAMECAPTURE_NO_READBACK_SURFACES = "impossible d’allouer les surfaces de relecture de %dx%d",
    FRAMECAPTURE_RENDER_FAILED = "le rendu ou la résolution de la capture a échoué",
    FRAMECAPTURE_NOTHING_DRAWN = "rien n’a été dessiné (masqué ou entièrement transparent ?)",
    FRAMECAPTURE_READBACK_FAILED = "la relecture a échoué",
    FRAMECAPTURE_OUT_OF_MEMORY = "mémoire insuffisante pour l’image, essayez une taille plus petite",
    FRAMECAPTURE_WRITE_FAILED = "impossible d’écrire %s",
    FRAMECAPTURE_HOOKS_FAILED = "impossible d’installer les hooks de rendu",
    FRAMECAPTURE_CLAMPED = ", réduit depuis %.2fx",
    FRAMECAPTURE_NO_MSAA = "sans MSAA",
}
)lua"
                                              R"lua(
locales.koKR = {
    FRAMECAPTURE_CAPTURING = "%s 캡처 중",
    FRAMECAPTURE_NO_FRAME_NAMED = "이름이 %s인 프레임이 없습니다",
    FRAMECAPTURE_NO_MOUSE_FRAME = "커서 아래에 마우스가 활성화된 프레임이 없습니다. 대신 이름을 지정하세요: %s ChatFrame1",
    FRAMECAPTURE_WORLD_FRAME = "WorldFrame은 3D 월드를 그리므로 캡처할 수 없습니다",
    FRAMECAPTURE_INVALID_SIZE = "잘못된 크기 '%s', 2048 또는 2x 형식이어야 합니다",
    FRAMECAPTURE_NO_DEVICE = "D3D9 장치가 없습니다",
    FRAMECAPTURE_NO_SIZE = "캡처할 대상이 없습니다 (프레임이 숨겨져 있거나 크기가 없습니다)",
    FRAMECAPTURE_NO_RENDER_TARGET = "%dx%d 렌더 타깃을 할당할 수 없습니다",
    FRAMECAPTURE_NO_READBACK_SURFACES = "%dx%d 리드백 표면을 할당할 수 없습니다",
    FRAMECAPTURE_RENDER_FAILED = "캡처 렌더링 또는 리졸브에 실패했습니다",
    FRAMECAPTURE_NOTHING_DRAWN = "그려진 것이 없습니다 (숨겨져 있거나 완전히 투명한가요?)",
    FRAMECAPTURE_READBACK_FAILED = "리드백에 실패했습니다",
    FRAMECAPTURE_OUT_OF_MEMORY = "이미지를 위한 메모리가 부족합니다. 더 작은 크기를 사용해 보세요",
    FRAMECAPTURE_WRITE_FAILED = "%s 파일을 쓸 수 없습니다",
    FRAMECAPTURE_HOOKS_FAILED = "렌더 후크를 설치할 수 없습니다",
    FRAMECAPTURE_CLAMPED = ", %.2fx에서 낮춤",
    FRAMECAPTURE_NO_MSAA = "MSAA 없음",
}

locales.ptBR = {
    FRAMECAPTURE_CAPTURING = "capturando %s",
    FRAMECAPTURE_NO_FRAME_NAMED = "nenhum quadro chamado %s",
    FRAMECAPTURE_NO_MOUSE_FRAME = "nenhum quadro com mouse habilitado sob o cursor, informe o nome: %s ChatFrame1",
    FRAMECAPTURE_WORLD_FRAME = "WorldFrame desenha o mundo 3D e não pode ser capturado",
    FRAMECAPTURE_INVALID_SIZE = "tamanho inválido '%s', esperado 2048 ou 2x",
    FRAMECAPTURE_NO_DEVICE = "nenhum dispositivo D3D9",
    FRAMECAPTURE_NO_SIZE = "nada para capturar (o quadro está oculto ou não tem tamanho)",
    FRAMECAPTURE_NO_RENDER_TARGET = "não foi possível alocar um alvo de renderização de %dx%d",
    FRAMECAPTURE_NO_READBACK_SURFACES = "não foi possível alocar as superfícies de leitura de %dx%d",
    FRAMECAPTURE_RENDER_FAILED = "falha ao renderizar ou resolver a captura",
    FRAMECAPTURE_NOTHING_DRAWN = "nada foi desenhado (oculto ou totalmente transparente?)",
    FRAMECAPTURE_READBACK_FAILED = "falha na leitura",
    FRAMECAPTURE_OUT_OF_MEMORY = "memória insuficiente para a imagem, tente um tamanho menor",
    FRAMECAPTURE_WRITE_FAILED = "não foi possível gravar %s",
    FRAMECAPTURE_HOOKS_FAILED = "não foi possível instalar os hooks de renderização",
    FRAMECAPTURE_CLAMPED = ", reduzido de %.2fx",
    FRAMECAPTURE_NO_MSAA = "sem MSAA",
}

locales.ruRU = {
    FRAMECAPTURE_CAPTURING = "захват %s",
    FRAMECAPTURE_NO_FRAME_NAMED = "нет фрейма с именем %s",
    FRAMECAPTURE_NO_MOUSE_FRAME = "под курсором нет фрейма, принимающего мышь, укажите имя: %s ChatFrame1",
    FRAMECAPTURE_WORLD_FRAME = "WorldFrame рисует 3D-мир, его нельзя захватить",
    FRAMECAPTURE_INVALID_SIZE = "неверный размер '%s', ожидается 2048 или 2x",
    FRAMECAPTURE_NO_DEVICE = "нет устройства D3D9",
    FRAMECAPTURE_NO_SIZE = "нечего захватывать (фрейм скрыт или не имеет размера)",
    FRAMECAPTURE_NO_RENDER_TARGET = "не удалось выделить цель рендеринга %dx%d",
    FRAMECAPTURE_NO_READBACK_SURFACES = "не удалось выделить поверхности чтения %dx%d",
    FRAMECAPTURE_RENDER_FAILED = "ошибка при отрисовке или сведении снимка",
    FRAMECAPTURE_NOTHING_DRAWN = "ничего не отрисовано (скрыт или полностью прозрачен?)",
    FRAMECAPTURE_READBACK_FAILED = "ошибка считывания изображения",
    FRAMECAPTURE_OUT_OF_MEMORY = "недостаточно памяти для изображения, попробуйте размер поменьше",
    FRAMECAPTURE_WRITE_FAILED = "не удалось записать %s",
    FRAMECAPTURE_HOOKS_FAILED = "не удалось установить хуки отрисовки",
    FRAMECAPTURE_CLAMPED = ", уменьшено с %.2fx",
    FRAMECAPTURE_NO_MSAA = "без MSAA",
}
)lua"
                                              R"lua(
locales.zhCN = {
    FRAMECAPTURE_CAPTURING = "正在截取 %s",
    FRAMECAPTURE_NO_FRAME_NAMED = "没有名为 %s 的框体",
    FRAMECAPTURE_NO_MOUSE_FRAME = "光标下没有启用鼠标的框体，请改为指定名称：%s ChatFrame1",
    FRAMECAPTURE_WORLD_FRAME = "WorldFrame 绘制的是 3D 世界，无法截取",
    FRAMECAPTURE_INVALID_SIZE = "无效的尺寸“%s”，应为 2048 或 2x",
    FRAMECAPTURE_NO_DEVICE = "没有 D3D9 设备",
    FRAMECAPTURE_NO_SIZE = "没有可截取的内容（框体已隐藏或没有尺寸）",
    FRAMECAPTURE_NO_RENDER_TARGET = "无法分配 %dx%d 的渲染目标",
    FRAMECAPTURE_NO_READBACK_SURFACES = "无法分配 %dx%d 的回读表面",
    FRAMECAPTURE_RENDER_FAILED = "渲染或解析截图失败",
    FRAMECAPTURE_NOTHING_DRAWN = "没有绘制任何内容（已隐藏或完全透明？）",
    FRAMECAPTURE_READBACK_FAILED = "回读失败",
    FRAMECAPTURE_OUT_OF_MEMORY = "图像内存不足，请尝试更小的尺寸",
    FRAMECAPTURE_WRITE_FAILED = "无法写入 %s",
    FRAMECAPTURE_HOOKS_FAILED = "无法安装渲染钩子",
    FRAMECAPTURE_CLAMPED = ", 已从 %.2fx 下调",
    FRAMECAPTURE_NO_MSAA = "无 MSAA",
}

locales.zhTW = {
    FRAMECAPTURE_CAPTURING = "正在擷取 %s",
    FRAMECAPTURE_NO_FRAME_NAMED = "沒有名為 %s 的框架",
    FRAMECAPTURE_NO_MOUSE_FRAME = "游標下沒有啟用滑鼠的框架，請改為指定名稱：%s ChatFrame1",
    FRAMECAPTURE_WORLD_FRAME = "WorldFrame 繪製的是 3D 世界，無法擷取",
    FRAMECAPTURE_INVALID_SIZE = "無效的尺寸「%s」，應為 2048 或 2x",
    FRAMECAPTURE_NO_DEVICE = "沒有 D3D9 裝置",
    FRAMECAPTURE_NO_SIZE = "沒有可擷取的內容（框架已隱藏或沒有尺寸）",
    FRAMECAPTURE_NO_RENDER_TARGET = "無法配置 %dx%d 的算繪目標",
    FRAMECAPTURE_NO_READBACK_SURFACES = "無法配置 %dx%d 的回讀表面",
    FRAMECAPTURE_RENDER_FAILED = "算繪或解析擷取失敗",
    FRAMECAPTURE_NOTHING_DRAWN = "沒有繪製任何內容（已隱藏或完全透明？）",
    FRAMECAPTURE_READBACK_FAILED = "回讀失敗",
    FRAMECAPTURE_OUT_OF_MEMORY = "影像記憶體不足，請嘗試較小的尺寸",
    FRAMECAPTURE_WRITE_FAILED = "無法寫入 %s",
    FRAMECAPTURE_HOOKS_FAILED = "無法安裝算繪掛鉤",
    FRAMECAPTURE_CLAMPED = ", 已從 %.2fx 下調",
    FRAMECAPTURE_NO_MSAA = "無 MSAA",
}
)lua"
                                              R"lua(
for key, text in pairs(locales[GetLocale()] or {}) do strings[key] = text end
for key, text in pairs(strings) do _G[key] = text end
FRAMECAPTURE_FAILED = SCREENSHOT_FAILURE .. ": %s"
FRAMECAPTURE_SAVED = SCREENSHOT_SUCCESS .. ": %s (%dx%d, %.2fx%s, %s)"

SLASH_FRAMECAPTURE1 = "/fcapture"
SLASH_FRAMECAPTURE2 = "/framecapture"
SlashCmdList.FRAMECAPTURE = function(msg)
    local name, size
    for token in (msg or ""):gmatch("%S+") do
        if token:match("^%d+%.?%d*[xX]?$") then size = token else name = token end
    end
    local frame
    if name then
        frame = _G[name]
    else
        frame = GetMouseFocus()
        if frame == WorldFrame then frame = nil end
    end
    if type(frame) ~= "table" or not frame.IsObjectType or not frame:IsObjectType("Frame") then
        print(tag .. (name and FRAMECAPTURE_NO_FRAME_NAMED:format(name)
            or FRAMECAPTURE_NO_MOUSE_FRAME:format(SLASH_FRAMECAPTURE1)))
        return
    end
    print(tag .. FRAMECAPTURE_CAPTURING:format(frame:GetName() or tostring(frame)))
    CaptureFrame(frame, size)
end
)lua";
}  // namespace misc_capture_frame_lua
