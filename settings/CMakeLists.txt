add_library(panther_settings OBJECT default-settings.h default-styles.h globalsettings.cpp globalsettings.h keybindings.h keybindings.cpp settings.cpp settings.h style.cpp style.h workspace.cpp workspace.h)
set(ALL_OBJECT_FILES ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:panther_settings> PARENT_SCOPE)
