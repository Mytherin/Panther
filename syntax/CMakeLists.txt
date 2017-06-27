

add_subdirectory(languages)

add_library(panther_syntax OBJECT language.cpp language.h syntax.h syntaxhighlighter.cpp syntaxhighlighter.h)
set(ALL_OBJECT_FILES ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:panther_syntax> PARENT_SCOPE)

