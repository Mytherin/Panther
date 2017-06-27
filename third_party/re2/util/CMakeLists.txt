add_library(panther_re2_util OBJECT flags.h logging.h mix.h mutex.h rune.cc sparse_array.h sparse_set.h strutil.cc strutil.h utf.h util.h)
set(ALL_OBJECT_FILES ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:panther_re2_util> PARENT_SCOPE)
