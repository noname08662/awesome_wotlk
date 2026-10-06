project(MinHook C)

add_library(
	${PROJECT_NAME} STATIC
		src/buffer.c
		src/buffer.h
		src/hde/hde32.c
		src/hde/hde32.h
		src/hde/hde64.c
		src/hde/hde64.h
		src/hde/pstdint.h
		src/hde/table32.h
		src/hde/table64.h
		src/hook.c
		src/trampoline.c
		src/trampoline.h
		include/MinHook.h
)

target_include_directories(${PROJECT_NAME} PUBLIC include)
target_include_directories(${PROJECT_NAME} PRIVATE src)
