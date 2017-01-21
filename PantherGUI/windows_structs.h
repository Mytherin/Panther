
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
	PGTimerHandle timer;
	HCURSOR cursor;
	
	bool pending_popup_menu = false;
	bool pending_drag_drop = false;
	bool dragging = false;

	struct DragDropData {
		PGBitmapHandle image;
		PGDropCallback callback;
		void* data;
		size_t data_length;
	} drag_drop_data;

	struct PopupData {
		UINT alignment;
		PGPoint point;
		PGPopupMenuHandle menu;
	} popup_data;

	PGWindow() : modifier(PGModifierNone), pending_drag_drop(false), dragging(false), pending_popup_menu(false) {}
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
