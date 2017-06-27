
include_directories(${SKIA_SOURCE_DIR}/include/core ${SKIA_SOURCE_DIR}/include/config ${SKIA_SOURCE_DIR}/include/codec ${SKIA_SOURCE_DIR}/include/effects)


add_subdirectory(controls)

add_library(panther_ui OBJECT renderer.cpp renderer.h)
set(ALL_OBJECT_FILES ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:panther_ui> PARENT_SCOPE)
