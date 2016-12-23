
#include "control.h"
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

std::string GetButtonName(PGButton button) {
	switch (button) {
		case PGButtonNone: return std::string("PGButtonNone");
		case PGButtonInsert: return std::string("PGButtonInsert");
		case PGButtonHome: return std::string("PGButtonHome");
		case PGButtonPageUp: return std::string("PGButtonPageUp");
		case PGButtonPageDown: return std::string("PGButtonPageDown");
		case PGButtonDelete: return std::string("PGButtonDelete");
		case PGButtonEnd: return std::string("PGButtonEnd");
		case PGButtonPrintScreen: return std::string("PGButtonPrintScreen");
		case PGButtonScrollLock: return std::string("PGButtonScrollLock");
		case PGButtonPauseBreak: return std::string("PGButtonPauseBreak");
		case PGButtonEscape: return std::string("PGButtonEscape");
		case PGButtonTab: return std::string("PGButtonTab");
		case PGButtonCapsLock: return std::string("PGButtonCapsLock");
		case PGButtonF1: return std::string("PGButtonF1");
		case PGButtonF2: return std::string("PGButtonF2");
		case PGButtonF3: return std::string("PGButtonF3");
		case PGButtonF4: return std::string("PGButtonF4");
		case PGButtonF5: return std::string("PGButtonF5");
		case PGButtonF6: return std::string("PGButtonF6");
		case PGButtonF7: return std::string("PGButtonF7");
		case PGButtonF8: return std::string("PGButtonF8");
		case PGButtonF9: return std::string("PGButtonF9");
		case PGButtonF10: return std::string("PGButtonF10");
		case PGButtonF11: return std::string("PGButtonF11");
		case PGButtonF12: return std::string("PGButtonF12");
		case PGButtonNumLock: return std::string("PGButtonNumLock");
		case PGButtonDown: return std::string("PGButtonDown");
		case PGButtonUp: return std::string("PGButtonUp");
		case PGButtonLeft: return std::string("PGButtonLeft");
		case PGButtonRight: return std::string("PGButtonRight");
		case PGButtonBackspace: return std::string("PGButtonBackspace");
		case PGButtonEnter: return std::string("PGButtonEnter");
	}
	return std::string("UnknownButton");
}

std::string GetModifierName(PGModifier modifier) {
	if (modifier == PGModifierNone) return std::string("PGModifierNone");
	else if (modifier == PGModifierCmd) return std::string("PGModifierCmd");
	else if (modifier == PGModifierCtrl) return std::string("PGModifierCtrl");
	else if (modifier == PGModifierShift) return std::string("PGModifierShift");
	else if (modifier == PGModifierAlt) return std::string("PGModifierAlt");
	else if (modifier == PGModifierCtrlShift) return std::string("PGModifierCtrlShift");
	else if (modifier == PGModifierCtrlAlt) return std::string("PGModifierCtrlAlt");
	else if (modifier == PGModifierShiftAlt) return std::string("PGModifierShiftAlt");
	else if (modifier == PGModifierCtrlShiftAlt) return std::string("PGModifierCtrlShiftAlt");
	else if (modifier == PGModifierCmdCtrl) return std::string("PGModifierCmdCtrl");
	else if (modifier == PGModifierCmdShift) return std::string("PGModifierCmdShift");
	else if (modifier == PGModifierCmdAlt) return std::string("PGModifierCmdAlt");
	else if (modifier == PGModifierCmdCtrlShift) return std::string("PGModifierCmdCtrlShift");
	else if (modifier == PGModifierCmdShiftAlt) return std::string("PGModifierCmdShiftAlt");
	else if (modifier == PGModifierCmdCtrlAlt) return std::string("PGModifierCmdCtrlAlt");
	else if (modifier == PGModifierCmdCtrlShiftAlt) return std::string("PGModifierCmdCtrlShiftAlt");
	return std::string("UnknownModifier");
}

std::string GetMouseButtonName(PGMouseButton modifier) {
	if (modifier == PGMouseButtonNone) return std::string("PGMouseButtonNone");
	else if (modifier == PGLeftMouseButton) return std::string("PGLeftMouseButton");
	else if (modifier == PGRightMouseButton) return std::string("PGRightMouseButton");
	else if (modifier == PGMiddleMouseButton) return std::string("PGMiddleMouseButton");
	else if (modifier == PGXButton1) return std::string("PGXButton1");
	else if (modifier == PGXButton2) return std::string("PGXButton2");
	else if (modifier == (PGLeftMouseButton | PGRightMouseButton)) return std::string("PGLeftMouseButton | PGRightMouseButton");
	else if (modifier == (PGLeftMouseButton | PGMiddleMouseButton)) return std::string("PGLeftMouseButton | PGMiddleMouseButton");
	else if (modifier == (PGRightMouseButton | PGMiddleMouseButton)) return std::string("PGRightMouseButton | PGMiddleMouseButton");
	return std::string("UnknownModifier");
}

PGPoint GetMousePosition(PGWindowHandle window, Control* c) {
	return GetMousePosition(window) - c->Position();
}