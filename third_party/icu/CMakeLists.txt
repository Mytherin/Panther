
include_directories(common common/unicode i18n i18n/unicode io io/unicode stubdata tools tools/toolutil)

add_subdirectory(common)
add_subdirectory(i18n)
add_subdirectory(io)
add_subdirectory(stubdata)
add_subdirectory(tools)

add_library(panther_icu_library STATIC ${ICU_OBJECT_FILES})
