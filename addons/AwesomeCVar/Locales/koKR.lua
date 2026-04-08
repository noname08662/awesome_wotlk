-- File: koKR.lua
-- Language: Korean
local addonName, AwesomeCVar = ...

if not AwesomeCVar.L then
    AwesomeCVar.L = {}
end

local L = AwesomeCVar.L

if GetLocale() == "koKR" then
    -- 일반
    L.ADDON_NAME = "AwesomeCVar"
    L.ADDON_NAME_SHORT = "Awesome CVar"
    L.MAIN_FRAME_TITLE = "Awesome CVar 관리자"
    L.RESET_TO = "%s(으)로 초기화"

    -- 팝업
    L.RELOAD_POPUP_TITLE = "UI 재시작 필요"
    L.RELOAD_POPUP_TEXT = "변경 사항 중 하나 이상을 적용하려면 UI 재시작(/reload)이 필요합니다."
    L.RESET_POPUP_TITLE = "기본값 초기화 확인"
    L.RESET_POPUP_TEXT = "모든 설정값을 기본값으로 초기화하시겠습니까?"

    -- 채팅 메시지
    L.MSG_LOADED = "Awesome CVar가 로드되었습니다! /awesome을 입력하여 관리자를 여세요."
    L.MSG_FRAME_RESET = "프레임 위치가 화면 중앙으로 초기화되었습니다."
    L.MSG_SET_VALUE = "%s을(를) %s(으)로 설정했습니다."
    L.MSG_FRAME_CREATE_ERROR = "AwesomeCVar 프레임을 생성할 수 없습니다!"
    L.MSG_UNKNOWN_COMMAND = "알 수 없는 명령어입니다. /awesome help를 입력하여 사용 가능한 명령어를 확인하세요."
    L.MSG_HELP_HEADER = "Awesome CVar 명령어:"
    L.MSG_HELP_TOGGLE = "/awesome - CVar 관리자 열기/닫기"
    L.MSG_HELP_SHOW = "/awesome show - CVar 관리자 표시"
    L.MSG_HELP_HIDE = "/awesome hide - CVar 관리자 숨기기"
    L.MSG_HELP_RESET = "/awesome reset - 프레임 위치 초기화"
    L.MSG_HELP_HELP = "/awesome help - 도움말 메시지 표시"

    -- CVar 카테고리
    L.CATEGORY_CAMERA = "카메라"
    L.CATEGORY_NAMEPLATES = "이름표"
    L.CATEGORY_TEXT_TO_SPEECH = "음성 변환(TTS)"
    L.CATEGORY_INTERACTION = "상호작용"
    L.CATEGORY_OTHER = "기타"

    -- CVar 라벨 및 설명
    L.CVAR_LABEL_INFO = "참고"
    L.CVAR_LABEL_TTS_VOICE = "TTS 음성"
    L.CVAR_LABEL_TTS_VOLUME = "TTS 음량"
    L.CVAR_LABEL_TTS_SPEED = "TTS 속도"
    L.CVAR_LABEL_CAMERA_FOV = "카메라 시야각(FoV)"
    L.CVAR_LABEL_STACKING_MODE = "이름표 배열 모드"
    L.CVAR_LABEL_MOUSEOVER = "이름표 마우스오버 모드"
    L.CVAR_LABEL_OCCLUSION_ALPHA = "이름표 가려짐 투명도"
    L.CVAR_LABEL_NONTARGET_ALPHA = "비대상 이름표 투명도"
    L.CVAR_LABEL_ALPHA_SPEED = "이름표 투명도 변화 속도"
    L.CVAR_LABEL_CLAMP_TOP = "이름표 상단 고정 모드"
    L.CVAR_LABEL_NAMEPLATE_DISTANCE = "이름표 표시 거리"
    L.CVAR_LABEL_MAX_RAISE_DISTANCE = "상하 배열 최대 간격"
    L.CVAR_LABEL_MAX_PULL_DISTANCE = "좌우 배열 최대 간격"
    L.CVAR_LABEL_X_SPACE = "이름표 가로 간격"
    L.CVAR_LABEL_Y_SPACE = "이름표 세로 간격"
    L.CVAR_LABEL_UPPER_BORDER = "이름표 상단 고정 오프셋"
    L.CVAR_LABEL_PLACEMENT = "이름표 배치 오프셋"
    L.CVAR_LABEL_SPEED_RAISE = "이름표 상단 이동 속도"
    L.CVAR_LABEL_SPEED_LOWER = "이름표 하단 이동 속도"
    L.CVAR_LABEL_SPEED_PULL = "이름표 가로 이동 속도"
    L.CVAR_LABEL_HITBOX_HEIGHT_ENEMY = "적군 이름표 히트박스 높이"
    L.CVAR_LABEL_HITBOX_WIDTH_ENEMY = "적군 이름표 히트박스 너비"
    L.CVAR_LABEL_HITBOX_HEIGHT_FRIENDLY = "아군 이름표 히트박스 높이"
    L.CVAR_LABEL_HITBOX_WIDTH_FRIENDLY = "아군 이름표 히트박스 너비"
    L.CVAR_LABEL_INTERACTION_MODE = "상호작용 모드"
    L.CVAR_LABEL_INTERACTION_ANGLE = "상호작용 원뿔 각도(도)"
    L.CVAR_STANCE_PATCH = "태세/형태 변환 패치"
    L.CVAR_SHOW_PLAYER = "내 캐릭터 모델 렌더링"
    L.CVAR_MSDF_MODE = "글꼴 렌더링 모드 (재시작 필요)"
    L.CVAR_LABEL_CAMERA_INDIRECT_VISIBILITY = "카메라 간접 가시성"
    L.CVAR_LABEL_CAMERA_INDIRECT_ALPHA = "카메라 간접 투명도"
    L.CVAR_LABEL_CAMERA_DISTANCE_MAX = "카메라 최대 거리"

    L.DESC_INFO = "\n모든 이름표에 다음 추가 메서드가 포함됩니다:\n- SetStackingEnabled(bool)\n- GetStackingEnabled()\n\n사용 예시:\nif UnitExists('target') then\n C_NamePlate.GetNamePlateForUnit('target'):SetStackingEnabled(false)\nend\n\n참고: NAME_PLATE_CREATED 시점은\n메서드 호출에 너무 이릅니다.\n대신 NAME_PLATE_UNIT_ADDED를 사용하세요.\n"
    L.DESC_STACKING_MODE = "0 = 비활성; 1 = 모두 활성; 2 = 적군만; 3 = 아군만."
    L.DESC_MOUSEOVER = "0 = 비활성;\n1 = 적군 클릭 무시;\n2 = 적군 클릭 무시, 가려진 아군 이름표 마우스오버 시 항상 표시;\n3 = 적군 클릭 무시, 전투 중 가려진 아군 이름표 표시;\n4 = 아군 클릭 무시;\n5 = 아군 클릭 무시, 가려진 적군 이름표 마우스오버 시 항상 표시;\n6 = 아군 클릭 무시, 전투 중 가려진 적군 이름표 표시;\n7 = 마우스오버된 가려진 이름표 항상 앞으로 가져오기;\n8 = 전투 중 마우스오버된 가려진 이름표 앞으로 가져오기."
    L.DESC_ALPHA_BLEND = "이름표가 새로운 투명도에 도달하는 속도를 조절합니다 (1 = 즉시)."
    L.DESC_CLAMP_TOP = "0 = 비활성; 1 = 모두 고정; 2 = 우두머리만 고정."
    L.DESC_STANCE_PATCH = "매크로 사용 시 단 한 번의 클릭으로 태세/형태 변환과 기술 시전을 동시에 가능하게 합니다."
    L.DESC_OCCLUSION_ALPHA = "장애물이나 지형에 가려진 이름표의 투명도를 조절합니다."
    L.DESC_CAMERA_INDIRECT_VISIBILITY = "카메라가 지형지물에 막히지 않고 통과할 수 있게 합니다."
    L.DESC_CAMERA_INDIRECT_ALPHA = "카메라와 캐릭터 사이를 가리는 물체의 투명도를 설정합니다."
    L.DESC_CAMERA_DISTANCE_MAX = "카메라가 시점에서 멀어질 수 있는 최대 거리를 설정합니다."
    L.DESC_MSDF = "벡터 기반 글꼴 렌더링을 활성화하여 글자 품질을 획기적으로 향상시킵니다.\n0 = 비활성; 1 = 활성;\n2 = 활성 (불안정한 글꼴) — 거리 필드 계산 방식 때문에\n획이 겹치는 일부 글꼴은 깨질 수 있습니다."

    -- CVar 모드 옵션
    L.MODE_LABEL_PLAYER_RADIUS = "플레이어 반경 20미터"
    L.MODE_LABEL_CONE_ANGLE = "20미터 내 원뿔 각도(도)"
end