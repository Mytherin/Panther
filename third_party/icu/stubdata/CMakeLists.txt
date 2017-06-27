
add_library(icu_stubdata OBJECT stubdata.c)

set(ICU_OBJECT_FILES ${ICU_OBJECT_FILES} $<TARGET_OBJECTS:icu_stubdata> PARENT_SCOPE)
