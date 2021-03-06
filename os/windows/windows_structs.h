
#pragma once

#include "controlmanager.h"
#include "workspace.h"
#include "renderer.h"

#include <map>
#include <windows.h>


struct PGWindow {
public:
	PGModifier modifier;
	HWND hwnd = nullptr;
	std::shared_ptr<ControlManager> manager;
	PGRendererHandle renderer = nullptr;
	IDropTarget* drop_target;
	PGDropSource* source = nullptr;
	PGTimerHandle timer;
	HCURSOR cursor;
	PGWorkspace* workspace;
	PGPopupMenuHandle menu;
	SkBitmap bitmap;
	
	bool pending_popup_menu = false;
	bool pending_drag_drop = false;
	bool pending_confirmation_box = false;
	bool pending_destroy = false;
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

	struct ConfirmationBoxData {
		PGWindowHandle window;
		std::string title;
		std::string message;
		PGConfirmationCallback callback;
		Control* control;
		void* data;
		PGConfirmationBoxType type;
	} confirmation_box_data;

	PGWindow(PGWorkspace* workspace) : modifier(PGModifierNone), pending_drag_drop(false), dragging(false), 
		pending_popup_menu(false), workspace(workspace), pending_destroy(false),
		pending_confirmation_box(false) {}
};

struct PGTooltip {
	PGWindowHandle window;
	std::string tooltip_text;
	HWND tooltip;
};

struct PGTimerParameter {
	PGTimerCallback callback;
	PGWindowHandle window;
};

struct PGTimer {
	HANDLE timer;
	PGTimerParameter *parameter;
};

#define BASE_INDEX 1000
struct PGPopupMenu {
	PGWindowHandle window = nullptr;
	PGScalar text_size = 0;
	PGScalar hotkey_size = 0;
	HMENU menu;
	bool is_popupmenu = false;
	int index = BASE_INDEX;
	std::vector<PGPopupInformation*> data;
	std::map<int, PGPopupCallback> callbacks;
	Control* control;
	PGFontHandle font;

	~PGPopupMenu() {
		for (auto it = data.begin(); it != data.end(); it++) {
			delete *it;
		}
	}
};
