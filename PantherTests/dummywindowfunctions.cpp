
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

void RenderRectangle(PGRendererHandle handle, PGRect rectangle, PGColor color) {

}

void RenderLine(PGRendererHandle handle, PGLine line, PGColor color) {

}

void RenderImage(PGRendererHandle window, void* image, int x, int y) {

}

void RenderText(PGRendererHandle renderer, const char* text, size_t length, PGScalar x, PGScalar y) {

}

void RenderCaret(PGRendererHandle renderer, const char *text, size_t len, int x, int y, lng characternr) {

}

void RenderSelection(PGRendererHandle renderer, const char *text, size_t len, int x, int y, lng start, lng end) {

}


void SetTextColor(PGRendererHandle window, PGColor color) {

}


void SetTextFont(PGRendererHandle window, PGFontHandle font) {

}

void SetTextAlign(PGRendererHandle window, PGTextAlign alignment) {

}

void GetRenderOffsets(PGRendererHandle renderer, const char* text, lng length, std::vector<short>& offsets) {

}

int GetRenderWidth(PGRendererHandle renderer, const char* text, lng length) {
	return 5;
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

lng GetTime(void) {
	return 0;
}

void RefreshWindow(PGWindowHandle window, PGIRect rect) {

}

void SetCursor(PGWindowHandle window, PGCursorType type) {

}

void RenderTriangle(PGRendererHandle handle, PGPoint a, PGPoint b, PGPoint c, PGColor color) {

}

PGScalar MeasureTextWidth(PGRendererHandle renderer, const char* text, size_t length) {
	return 0;
}

PGScalar GetTextHeight(PGRendererHandle renderer) {
	return 0;
}

void RenderCaret(PGRendererHandle renderer, const char *text, size_t len, PGScalar x, PGScalar y, lng characternr, PGScalar line_height) {

}

void RenderSelection(PGRendererHandle renderer, const char *text, size_t len, PGScalar x, PGScalar y, lng start, lng end, PGColor selection_color, PGScalar line_height) {

}

void SetTextFont(PGRendererHandle window, PGFontHandle font, PGScalar height) {

}

bool WindowHasFocus(PGWindowHandle window) {
	return true;
}

PGPoint GetMousePosition(PGWindowHandle window) {
	return PGPoint(0, 0);
}

void RenderTriangle(PGRendererHandle handle, PGPoint a, PGPoint b, PGPoint c, PGColor color, PGStyle drawStyle) {}
void RenderRectangle(PGRendererHandle handle, PGRect rectangle, PGColor color, PGStyle style) {}
void RenderSquiggles(PGRendererHandle renderer, PGScalar width, PGScalar x, PGScalar y, PGColor color) {}