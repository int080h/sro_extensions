enable_language(C)

set(MINHOOK_SOURCES
    src/buffer.c
    src/hook.c
    src/trampoline.c
    src/hde/hde32.c
)

add_library(minhook STATIC ${MINHOOK_SOURCES})
target_include_directories(minhook PUBLIC include)
