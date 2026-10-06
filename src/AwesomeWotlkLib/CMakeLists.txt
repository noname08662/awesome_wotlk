project( AwesomeWotlkLib )

file(GLOB_RECURSE HEADERS "${CMAKE_SOURCE_DIR}/include/*.h")

add_library(
	${PROJECT_NAME} SHARED
		${HEADERS}
		"Entry.cpp"

		"Camera.h" "Camera.cpp"
		"BugFixes.h" "BugFixes.cpp"
        "BugFixesShaders.h"
        "D3D.h" "D3D.cpp"
        "Extensions.h" "Extensions.cpp"
        "CommandLine.h" "CommandLine.cpp"
        "Inventory.h" "Inventory.cpp"
        "Item.h" "Item.cpp"
        "Spell.h" "Spell.cpp"
        "Misc.h" "Misc.cpp"
        "MiscCaptureFrame.h"
        "MiscCaptureFrameLua.h"
        "MSDF.h" "MSDF.cpp"
        "MSDFCache.h" "MSDFCache.cpp"
        "MSDFFaceSweep.h" "MSDFFaceSweep.cpp"
        "MSDFFont.h" "MSDFFont.cpp"
        "MSDFManager.h" "MSDFManager.cpp"
        "MSDFPregen.h" "MSDFPregen.cpp"
        "MSDFShaders.h"
        "MSDFUtils.h"
        "MSDFValidator.h"
        "NamePlates.h" "NamePlates.cpp"
        "NamePlateSettings.h"
        "NamePlateEntry.h" "NamePlateEntry.cpp"
        "NamePlateManager.h" "NamePlateManager.cpp"
        "UnitAPI.h" "UnitAPI.cpp"
        "VoiceChat.h" "VoiceChat.cpp")

target_include_directories(
    ${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(
    ${PROJECT_NAME} PRIVATE
        AwesomeWotLK_Common

        ole32
        oleaut32
        uuid
        sapi
)

target_compile_definitions(
    ${PROJECT_NAME} PRIVATE
        APP_NAME="${PROJECT_NAME}"
)
