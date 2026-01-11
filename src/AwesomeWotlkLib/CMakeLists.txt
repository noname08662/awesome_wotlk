project( AwesomeWotlkLib )

add_library(
	${PROJECT_NAME} SHARED
		"Entry.cpp"

		GameClient.h
		"D3D.h" "D3D.cpp"
        "NamePlates.h" "NamePlates.cpp"
		"Hooks.h" "Hooks.cpp"
		"Misc.h" "Misc.cpp"
		"BugFixes.h" "BugFixes.cpp"
		"Utils.h" "Utils.cpp"
		"Fonts.h" "Fonts.cpp"
		"CommandLine.cpp" "CommandLine.h"
		"Inventory.cpp" "Inventory.h"
		"Item.h" "Item.cpp"
		"UnitAPI.h" "UnitAPI.cpp"
		"Spell.h" "Spell.cpp"
		"VoiceChat.h" "VoiceChat.cpp")

target_include_directories(
    ${PROJECT_NAME} PRIVATE
		${CMAKE_SOURCE_DIR}/deps
		${CMAKE_SOURCE_DIR}/deps/skia/include
		${CMAKE_SOURCE_DIR}/deps/msdfgen
		${CMAKE_BINARY_DIR}/deps/msdfgen
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(
    ${PROJECT_NAME} PRIVATE
        Detours
		msdfgen::msdfgen-core
		msdfgen::msdfgen-ext
		freetype
        ole32
        oleaut32
        uuid
        sapi
		"${CMAKE_SOURCE_DIR}/deps/skia/skia.lib"
)

target_compile_definitions(
    ${PROJECT_NAME} PRIVATE
        APP_NAME="${PROJECT_NAME}"
)
