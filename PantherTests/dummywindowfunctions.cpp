
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

PGSize RenderText(PGRendererHandle window, const char* text, size_t length, int x, int y) {
	return PGSize(0, 0);
}

void RenderCaret(PGRendererHandle renderer, const char *text, size_t len, int x, int y, ssize_t characternr) {

}

void RenderSelection(PGRendererHandle renderer, const char *text, size_t len, int x, int y, ssize_t start, ssize_t end) {

}


void SetTextColor(PGRendererHandle window, PGColor color) {

}


void SetTextFont(PGRendererHandle window, PGFontHandle font) {

}

void SetTextAlign(PGRendererHandle window, PGTextAlign alignment) {

}

void GetRenderOffsets(PGRendererHandle renderer, const char* text, ssize_t length, std::vector<short>& offsets) {

}

int GetRenderWidth(PGRendererHandle renderer, const char* text, ssize_t length) {
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

ssize_t GetTime(void) {
	return 0;
}
