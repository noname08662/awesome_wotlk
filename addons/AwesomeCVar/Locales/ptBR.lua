-- File: ptBR.lua
-- Language: Portuguese (Brazil)
local addonName, AwesomeCVar = ...

if not AwesomeCVar.L then
    AwesomeCVar.L = {}
end

local L = AwesomeCVar.L

if GetLocale() == "ptBR" then
    -- Geral
    L.ADDON_NAME = "AwesomeCVar"
    L.ADDON_NAME_SHORT = "Awesome CVar"
    L.MAIN_FRAME_TITLE = "Gerenciador Awesome CVar"
    L.RESET_TO = "Redefinir para %s"

    -- Popups
    L.RELOAD_POPUP_TITLE = "Recarga de IU Necessária"
    L.RELOAD_POPUP_TEXT = "Uma ou mais alterações feitas exigem uma recarga da interface (ReloadUI) para entrar em vigor."
    L.RESET_POPUP_TITLE = "Confirmar Redefinição"
    L.RESET_POPUP_TEXT = "Tem certeza de que deseja redefinir todos os valores para os padrões?"

    -- Mensagens de Chat
    L.MSG_LOADED = "Awesome CVar carregado! Digite /awesome para abrir o gerenciador."
    L.MSG_FRAME_RESET = "A posição da janela foi redefinida para o centro."
    L.MSG_SET_VALUE = "%s definido para %s."
    L.MSG_FRAME_CREATE_ERROR = "Não foi possível criar a janela do AwesomeCVar!"
    L.MSG_UNKNOWN_COMMAND = "Comando desconhecido. Digite /awesome help para comandos disponíveis."
    L.MSG_HELP_HEADER = "Comandos Awesome CVar:"
    L.MSG_HELP_TOGGLE = "/awesome - Alternar o gerenciador CVar"
    L.MSG_HELP_SHOW = "/awesome show - Mostrar o gerenciador CVar"
    L.MSG_HELP_HIDE = "/awesome hide - Ocultar o gerenciador CVar"
    L.MSG_HELP_RESET = "/awesome reset - Centralizar posição da janela"
    L.MSG_HELP_HELP = "/awesome help - Mostrar esta mensagem de ajuda"

    -- Categorias de CVar
    L.CATEGORY_CAMERA = "Câmera"
    L.CATEGORY_NAMEPLATES = "Placas de Nome"
    L.CATEGORY_TEXT_TO_SPEECH = "Voz de Texto (TTS)"
    L.CATEGORY_INTERACTION = "Interação"
    L.CATEGORY_OTHER = "Outros"

    -- Rótulos e Descrições de CVar
    L.CVAR_LABEL_INFO = "Notas"
    L.CVAR_LABEL_TTS_VOICE = "Voz do TTS"
    L.CVAR_LABEL_TTS_VOLUME = "Volume do TTS"
    L.CVAR_LABEL_TTS_SPEED = "Velocidade do TTS"
    L.CVAR_LABEL_CAMERA_FOV = "Campo de Visão (FoV)"
    L.CVAR_LABEL_STACKING_MODE = "Modo de Empilhamento de Placas"
    L.CVAR_LABEL_MOUSEOVER = "Modo de Mouseover de Placas"
    L.CVAR_LABEL_OCCLUSION_ALPHA = "Alfa de Oclusão das Placas"
    L.CVAR_LABEL_NONTARGET_ALPHA = "Alfa de Placas (Sem Alvo)"
    L.CVAR_LABEL_ALPHA_SPEED = "Velocidade de Transição de Alfa"
    L.CVAR_LABEL_CLAMP_TOP = "Fixar Placas no Topo"
	L.CVAR_LABEL_NAMEPLATE_DISTANCE = "Distância de Exibição das Placas"
    L.CVAR_LABEL_MAX_RAISE_DISTANCE = "Empilhamento: Distância Vertical Máx."
    L.CVAR_LABEL_MAX_PULL_DISTANCE = "Empilhamento: Distância Horizontal Máx."
    L.CVAR_LABEL_X_SPACE = "Espaçamento X das Placas"
    L.CVAR_LABEL_Y_SPACE = "Espaçamento Y das Placas"
    L.CVAR_LABEL_UPPER_BORDER = "Margem Superior de Placas Fixas"
    L.CVAR_LABEL_PLACEMENT = "Deslocamento de Posição"
    L.CVAR_LABEL_SPEED_RAISE = "Velocidade de Subida (Empilhamento)"
    L.CVAR_LABEL_SPEED_LOWER = "Velocidade de Descida (Empilhamento)"
    L.CVAR_LABEL_SPEED_PULL = "Velocidade de Atração Horizontal"
	L.CVAR_LABEL_INERTIA = "Inércia de Empilhamento de Placas"
	L.CVAR_HYST_DECAY = "Velocidade de Separação de Pares"
	L.CVAR_LABEL_HITBOX_ANCHOR = "Âncora de Hitbox da Placa"
    L.CVAR_LABEL_HITBOX_HEIGHT_ENEMY = "Altura da Hitbox (Inimigo)"
    L.CVAR_LABEL_HITBOX_WIDTH_ENEMY = "Largura da Hitbox (Inimigo)"
    L.CVAR_LABEL_HITBOX_HEIGHT_FRIENDLY = "Altura da Hitbox (Aliado)"
    L.CVAR_LABEL_HITBOX_WIDTH_FRIENDLY = "Largura da Hitbox (Aliado)"
    L.CVAR_LABEL_INTERACTION_MODE = "Modo de Interação"
    L.CVAR_LABEL_INTERACTION_ANGLE = "Ângulo de Interação (graus)"
    L.CVAR_STANCE_PATCH = "Correção de Troca de Postura/Forma"
    L.CVAR_SHOW_PLAYER = "Renderizar Modelo do Jogador"
    L.CVAR_MSDF_MODE = "Modo de Renderização de Fonte (Requer Reinício)"
    L.CVAR_LABEL_CAMERA_INDIRECT_VISIBILITY = "Visibilidade Indireta da Câmera"
    L.CVAR_LABEL_CAMERA_INDIRECT_ALPHA = "Alfa Indireto da Câmera"
    L.CVAR_LABEL_CAMERA_DISTANCE_MAX = "Distância Máxima da Câmera"

    L.DESC_INFO = "Todas as placas de nome incluem métodos adicionais:\n- SetStackingEnabled(bool)\n- GetStackingEnabled()\n\nExemplo de uso:\nif UnitExists('target') then\n C_NamePlate.GetNamePlateForUnit('target'):SetStackingEnabled(false)\nend\n\nNota: É cedo demais para chamar\nesses métodos em NAME_PLATE_CREATED,\nuse NAME_PLATE_UNIT_ADDED em vez disso.\n"
	L.DESC_STACKING_MODE = "O modo 'Inteligente' permite que as placas ignorem o empilhamento se houver espaço suficiente abaixo, resultando em um layout mais compacto, porém com rearranjos mais frequentes."
    L.DESC_MOUSEOVER = "As opções de 'Elevar' aumentam o nível do quadro da placa sob o mouse, fazendo-a aparecer sobre as outras.\nO alvo atual continua tendo o nível mais alto."
	L.DESC_INERTIA = "Controla o peso físico do movimento das placas durante o empilhamento.\nValores mais altos aumentam a responsividade; valores mais baixos produzem um movimento mais pesado e amortecido."
	L.DESC_HYST_DECAY = "Controla a rapidez com que os pares de empilhamento se dissolvem quando as placas não se sobrepõem mais.\nValores mais altos causam separação mais rápida; valores mais baixos mantêm os pares unidos por mais tempo."
	L.DESC_PLACEMENT = "Um valor de deslocamento vertical que afasta as placas do seu ponto de ancoragem padrão."
	L.DESC_HITBOX_ANCHOR = "Define o ponto de origem vertical da área clicável da placa.\nAjuste de acordo com como seu addon ancora os quadros das placas (ex.: ancoragem pela borda superior ou inferior)."
    L.DESC_ALPHA_BLEND = "Controla a velocidade com que as placas animam para novos alvos de opacidade (1 = Instantâneo)."
    L.DESC_STANCE_PATCH = "Permite trocar de postura/forma e usar uma habilidade com um único clique ao usar macros."
    L.DESC_OCCLUSION_ALPHA = "Controla a opacidade das placas quando bloqueadas por obstáculos ou terreno."
    L.DESC_CAMERA_INDIRECT_VISIBILITY = "Permite que a câmera atravesse certos objetos em vez de ser bloqueada."
    L.DESC_CAMERA_INDIRECT_ALPHA = "Define o nível de transparência de objetos que ficam entre a câmera e o personagem."
    L.DESC_CAMERA_DISTANCE_MAX = "Define a distância máxima que a câmera pode afastar do jogador."
    L.DESC_MSDF = "Ativa a renderização de fonte baseada em vetores, melhorando drasticamente a qualidade.\n0 = Desativado; 1 = Ativado;\n2 = Ativado (fontes inseguras) — devido ao cálculo de campos de distância,\nalgumas fontes com contornos sobrepostos podem falhar."

    -- Opções de Modo CVar
    L.MODE_STACKING_DISABLED = "Sobreposto"
    L.MODE_STACKING_ALL = "Empilhado (Todos)"
    L.MODE_STACKING_ENEMY = "Empilhado (Inimigos)"
    L.MODE_STACKING_FRIENDLY = "Empilhado (Aliados)"
    L.MODE_STACKING_SMART_ALL = "Empilhamento Inteligente (Todos)"
    L.MODE_STACKING_SMART_ENEMY = "Empilhamento Inteligente (Inimigos)"
    L.MODE_STACKING_SMART_FRIENDLY = "Empilhamento Inteligente (Aliados)"

    L.MODE_MOUSE_DISABLED = "Padrão"
    L.MODE_MOUSE_CLICKTHROUGH_ENEMY = "Clicar através (Inimigos)"
    L.MODE_MOUSE_CLICKTHROUGH_ENEMY_RAISE_FRIENDLY = "Clicar Inim + Elevar Aliado"
    L.MODE_MOUSE_CLICKTHROUGH_ENEMY_RAISE_FRIENDLY_COMBAT = "Clicar Inim + Elevar Aliado (Combate)"
    L.MODE_MOUSE_CLICKTHROUGH_FRIENDLY = "Clicar através (Aliados)"
    L.MODE_MOUSE_CLICKTHROUGH_FRIENDLY_RAISE_ENEMY = "Clicar Aliado + Elevar Inim"
    L.MODE_MOUSE_CLICKTHROUGH_FRIENDLY_RAISE_ENEMY_COMBAT = "Clicar Aliado + Elevar Inim (Combate)"
    L.MODE_MOUSE_RAISE_OCCLUDED = "Sempre elevar ocluídos"
    L.MODE_MOUSE_RAISE_OCCLUDED_COMBAT = "Elevar ocluídos (Apenas combate)"

    L.MODE_CLAMP_DISABLED = "Desativado"
    L.MODE_CLAMP_ALL = "Fixar Todas"
    L.MODE_CLAMP_BOSSES = "Fixar Apenas Chefes"

    L.MODE_HITBOX_TOP = "Topo"
    L.MODE_HITBOX_CENTER = "Centro"
    L.MODE_HITBOX_BOTTOM = "Base"

    L.MODE_LABEL_PLAYER_RADIUS = "Raio do Jogador 20yd"
    L.MODE_LABEL_CONE_ANGLE = "Ângulo do Cone (graus) dentro de 20yd"
end