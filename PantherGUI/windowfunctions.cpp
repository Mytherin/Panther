
#include "windowfunctions.h"

PGTextAlign PGTextAlignBottom = 0x01;
PGTextAlign PGTextAlignTop = 0x02;
PGTextAlign PGTextAlignLeft = 0x04;
PGTextAlign PGTextAlignRight = 0x08;
PGTextAlign PGTextAlignHorizontalCenter = 0x10;
PGTextAlign PGTextAlignVerticalCenter = 0x20;

const PGModifier PGModifierNone = 0x00;
const PGModifier PGModifierCmd = 0x01;
const PGModifier PGModifierCtrl = 0x02;
const PGModifier PGModifierShift = 0x04;
const PGModifier PGModifierAlt = 0x08;
const PGModifier PGModifierCtrlShift = PGModifierCtrl | PGModifierShift;
const PGModifier PGModifierCtrlAlt = PGModifierCtrl | PGModifierAlt;
const PGModifier PGModifierShiftAlt = PGModifierShift | PGModifierAlt;
const PGModifier PGModifierCtrlShiftAlt = PGModifierCtrl | PGModifierShift | PGModifierAlt;
const PGModifier PGModifierCmdCtrl = PGModifierCmd | PGModifierCtrl;
const PGModifier PGModifierCmdShift = PGModifierCmd | PGModifierShift;
const PGModifier PGModifierCmdAlt = PGModifierCmd | PGModifierAlt;
const PGModifier PGModifierCmdCtrlShift = PGModifierCmd | PGModifierCtrl | PGModifierShift;
const PGModifier PGModifierCmdShiftAlt = PGModifierCmd | PGModifierShift | PGModifierAlt;
const PGModifier PGModifierCmdCtrlAlt = PGModifierCmd | PGModifierCtrl | PGModifierAlt;
const PGModifier PGModifierCmdCtrlShiftAlt = PGModifierCmd | PGModifierCtrl | PGModifierShift | PGModifierAlt;


const PGMouseButton PGMouseButtonNone = 0x00;
const PGMouseButton PGLeftMouseButton = 0x01;
const PGMouseButton PGRightMouseButton = 0x02;
const PGMouseButton PGMiddleMouseButton = 0x04;
const PGMouseButton PGXButton1 = 0x08;
const PGMouseButton PGXButton2 = 0x10;
