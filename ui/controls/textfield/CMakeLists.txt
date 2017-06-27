add_library(panther_controls_textfield OBJECT basictextfield.cpp basictextfield.h codecompletion.cpp codecompletion.h simpletextfield.cpp simpletextfield.h tabcontrol.cpp tabcontrol.h textfield.cpp textfield.h textfieldcontainer.cpp textfieldcontainer.h)
set(ALL_OBJECT_FILES ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:panther_controls_textfield> PARENT_SCOPE)
