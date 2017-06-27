
add_definitions(-DU_IO_IMPLEMENTATION)

add_library(icu_io OBJECT locbund.cpp locbund.h sprintf.c sscanf.c ucln_io.cpp ucln_io.h ufile.c ufile.h ufmt_cmn.c ufmt_cmn.h uprintf.cpp uprintf.h uprntf_p.c uscanf.c uscanf.h uscanf_p.c ustdio.c ustream.cpp)

set(ICU_OBJECT_FILES ${ICU_OBJECT_FILES} $<TARGET_OBJECTS:icu_io> PARENT_SCOPE)
