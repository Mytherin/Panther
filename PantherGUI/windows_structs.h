
#pragma once

#include "controlmanager.h"
#include <map>
#include <windows.h>


struct PGWindow {
public:
	PGModifier modifier;
	HWND hwnd = nullptr;
	ControlManager* manager = nullptr;
	PGRendererHandle renderer = nullptr;
	PGPopupMenuHandle popup = nullptr;
	IDropTarget* drop_target;
	HCURSOR cursor;

	PGWindow() : modifier(PGModifierNone) {}
};

struct PGTimerParameter {
	PGTimerCallback callback;
	PGWindowHandle window;
};

struct PGTimer {
	HANDLE timer;
	PGTimerParameter *parameter;
};

struct PGPopupMenu {
	PGWindowHandle window;
	HMENU menu;
	int index = 1000;
	std::map<int, PGControlCallback> callbacks;
	Control* control;
};
