
#include "windowfunctions.h"
#include "textfile.h"

struct PGWindow {
	int x;
};

struct PGRenderer {
	int x;
};

struct PGFont {
	int x;
};

// Window Functions
PGWindowHandle PGCreateWindow(void) {
	return NULL;
}

void CloseWindow(PGWindowHandle window) {

}

void ShowWindow(PGWindowHandle window) {

}

void HideWindow(PGWindowHandle window) {

}

void RefreshWindow(PGWindowHandle window) {

}

void RefreshWindow(PGWindowHandle window, PGRect rectangle) {

}

PGSize GetWindowSize(PGWindowHandle window) {
	return PGSize(0, 0);
}

Control* GetFocusedControl(PGWindowHandle window) {
	return NULL;
}


void SetWindowTitle(PGWindowHandle window, char* title) {

}

void RegisterRefresh(PGWindowHandle window, Control *callback) {

}


std::string clipboard = "";
void SetClipboardText(PGWindowHandle window, std::string text) {
	clipboard = text;
}

std::string GetClipboardText(PGWindowHandle window) {
	return clipboard;
}


PGLineEnding GetSystemLineEnding() {
	return PGLineEndingWindows;
}

char GetSystemPathSeparator() {
	return '\\';
}

lng GetTime(void) {
	return 0;
}

void RefreshWindow(PGWindowHandle window, PGIRect rect, bool redraw_now) {

}

void SetCursor(PGWindowHandle window, PGCursorType type) {

}

void SetTextFont(PGRendererHandle window, PGFontHandle font, PGScalar height) {

}

bool WindowHasFocus(PGWindowHandle window) {
	return true;
}

PGPoint GetMousePosition(PGWindowHandle window) {
	return PGPoint(0, 0);
}

PGPoint ConvertWindowToScreen(PGWindowHandle window, PGPoint point) {
	return PGPoint(0, 0);
}

PGPopupMenuHandle PGCreatePopupMenu(PGWindowHandle window, Control* control) {
	return nullptr;
}
void PGPopupMenuInsertSubmenu(PGPopupMenuHandle handle, PGPopupMenuHandle submenu, std::string submenu_name) {
	
}

void PGPopupMenuInsertEntry(PGPopupMenuHandle handle, std::string text, PGControlCallback callback, PGPopupMenuFlags flags) {

}

void PGPopupMenuInsertSeparator(PGPopupMenuHandle handle) {

}

void PGDisplayPopupMenu(PGPopupMenuHandle handle, PGTextAlign align) {

}

void PGDisplayPopupMenu(PGPopupMenuHandle, PGPoint, PGTextAlign align) {

}

void OpenFolderInExplorer(std::string path) {

}

void OpenFolderInTerminal(std::string path) {

}

std::string OpenFileMenu() {
	return std::string("");
}

void RedrawWindow(PGWindowHandle window) {

}

void RedrawWindow(PGWindowHandle window, PGIRect rect) {

}

PGMouseButton GetMouseState(PGWindowHandle window) {
	return PGMouseButtonNone;
}

void* GetWindowManager(PGWindowHandle window) {
	return nullptr;
}
