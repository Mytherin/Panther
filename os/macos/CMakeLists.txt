
include_directories(${SKIA_SOURCE_DIR}/include/core ${SKIA_SOURCE_DIR}/include/config ${SKIA_SOURCE_DIR}/include/codec ${SKIA_SOURCE_DIR}/include/effects)

add_library(panther_os_specific OBJECT AppDelegate.mm AppDelegate.h default-keybindings.h main.mm PGDrawBitmap.h PGView.h PGView.mm PGWindow.h PGWindow.mm)

set(ALL_OBJECT_FILES ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:panther_os_specific> PARENT_SCOPE)
