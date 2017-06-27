add_library(panther_controls_basic OBJECT button.cpp button.h decoratedscrollbar.cpp decoratedscrollbar.h label.cpp label.h scrollbar.cpp scrollbar.h splitter.cpp splitter.h togglebutton.cpp togglebutton.h)
set(ALL_OBJECT_FILES ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:panther_controls_basic> PARENT_SCOPE)
