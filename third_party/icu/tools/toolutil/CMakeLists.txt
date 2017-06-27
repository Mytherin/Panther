
add_definitions(-DU_TOOLUTIL_IMPLEMENTATION)

add_library(icu_toolutil OBJECT collationinfo.cpp collationinfo.h dbgutil.cpp dbgutil.h denseranges.cpp denseranges.h filestrm.c filestrm.h filetools.cpp filetools.h flagparser.c flagparser.h package.cpp package.h pkg_genc.c pkg_genc.h pkg_gencmn.c pkg_gencmn.h pkg_icu.cpp pkg_icu.h pkg_imp.h pkgitems.cpp ppucd.cpp ppucd.h swapimpl.cpp swapimpl.h toolutil.cpp toolutil.h ucbuf.cpp ucbuf.h ucln_tu.cpp ucm.c ucm.h ucmstate.c udbgutil.cpp udbgutil.h unewdata.c unewdata.h uoptions.c uoptions.h uparse.c uparse.h writesrc.c writesrc.h xmlparser.cpp xmlparser.h)

set(ICU_OBJECT_FILES ${ICU_OBJECT_FILES} $<TARGET_OBJECTS:icu_toolutil> PARENT_SCOPE)
