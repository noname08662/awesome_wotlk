project(AwesomeWotlkPatch)

add_executable(
    ${PROJECT_NAME}
    Main.cpp
    Utils.h
    Patch.h
)

target_compile_definitions(
    ${PROJECT_NAME} PRIVATE
    APP_NAME="${PROJECT_NAME}"
)

target_link_libraries(
    ${PROJECT_NAME} PRIVATE
    AwesomeWotLK_Common
)

if(MSVC)
    target_link_options(${PROJECT_NAME} PRIVATE
        "/MANIFESTUAC:level='requireAdministrator' uiAccess='false'"
        "/SUBSYSTEM:WINDOWS" 
        "/ENTRY:mainCRTStartup" 
    )
endif()