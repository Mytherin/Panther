
include_directories(${SKIA_SOURCE_DIR}/include/core ${SKIA_SOURCE_DIR}/include/config ${SKIA_SOURCE_DIR}/include/codec ${SKIA_SOURCE_DIR}/include/effects ${SKIA_SOURCE_DIR}/src/core)

add_library(panther_os_specific OBJECT assert.h dirent.h droptarget.cpp droptarget.h mmap.cpp windows_structs.h default-keybindings.h main.cpp main.h)

set(ALL_OBJECT_FILES ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:panther_os_specific> PARENT_SCOPE)
