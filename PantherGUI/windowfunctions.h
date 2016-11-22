/// windowfunctions.h
/// This is a set of functions that provide simple wrappers for native OS functionality
/// The actual GUI library is implemented on top of these simple wrappers
#pragma once

#include "utils.h"

struct PGWindow;
typedef struct PGWindow* PGWindowHandle;

struct PGRenderer;
typedef struct PGRenderer* PGRendererHandle;

struct PGFont;
typedef struct PGFont* PGFontHandle;

class Control;

struct PGPoint {
	int x;
	int y;

	PGPoint(int x, int y) : x(x), y(y) { }
};

struct PGSize {
	int width;
	int height;

	PGSize(int width, int height) : width(width), height(height) { }
};

struct PGRect {
	int x;
	int y;
	int width;
	int height;

	PGRect(int x, int y, int width, int height) : x(x), y(y), width(width), height(height) { }
};

struct PGLine {
	PGPoint start;
	PGPoint end;

	PGLine(PGPoint start, PGPoint end) : start(start), end(end) { }
	PGLine(int startx, int starty, int endx, int endy) : start(startx, starty), end(endx, endy) { }
};

typedef int PGTextAlign;

extern PGTextAlign PGTextAlignBottom;
extern PGTextAlign PGTextAlignTop;
extern PGTextAlign PGTextAlignLeft;
extern PGTextAlign PGTextAlignRight;
extern PGTextAlign PGTextAlignHorizontalCenter;
extern PGTextAlign PGTextAlignVerticalCenter;

typedef unsigned char byte;

struct PGColor {
	byte r;
	byte g;
	byte b;
	byte a;

	PGColor(byte r, byte g, byte b) : r(r), g(g), b(b), a(255) { }
	PGColor(byte r, byte g, byte b, byte a) : r(r), g(g), b(b), a(a) { }
};

typedef enum {
	PGButtonNone,
	PGButtonInsert,
	PGButtonHome,
	PGButtonPageUp,
	PGButtonPageDown,
	PGButtonDelete,
	PGButtonEnd,
	PGButtonPrintScreen,
	PGButtonScrollLock,
	PGButtonPauseBreak,
	PGButtonEscape,
	PGButtonTab,
	PGButtonCapsLock,
	PGButtonF1,
	PGButtonF2,
	PGButtonF3,
	PGButtonF4,
	PGButtonF5,
	PGButtonF6,
	PGButtonF7,
	PGButtonF8,
	PGButtonF9,
	PGButtonF10,
	PGButtonF11,
	PGButtonF12,
	PGButtonNumLock,
	PGButtonDown,
	PGButtonUp,
	PGButtonLeft,
	PGButtonRight,
	PGButtonBackspace,
	PGButtonEnter
} PGButton;

typedef byte PGModifier;

extern PGModifier PGModifierNone;
extern PGModifier PGModifierCmd;
extern PGModifier PGModifierCtrl;
extern PGModifier PGModifierShift;
extern PGModifier PGModifierAlt;
extern PGModifier PGModifierCtrlShift;
extern PGModifier PGModifierCtrlAlt;
extern PGModifier PGModifierShiftAlt;
extern PGModifier PGModifierCtrlShiftAlt;
extern PGModifier PGModifierCmdCtrl;
extern PGModifier PGModifierCmdShift;
extern PGModifier PGModifierCmdAlt;
extern PGModifier PGModifierCmdCtrlShift;
extern PGModifier PGModifierCmdShiftAlt;
extern PGModifier PGModifierCmdCtrlAlt;
extern PGModifier PGModifierCmdCtrlShiftAlt;

typedef enum {
	PGLeftMouseButton,
	PGRightMouseButton
} PGMouseButton;

typedef void (*PGClickFunction)(int x, int y, PGMouseButton button, PGModifier modifier);
typedef void (*PGScrollFunction)(int x, int y, int distance, PGModifier modifier);
typedef void (*PGButtonPress)(PGButton button, PGModifier modifier);
typedef void (*PGRefresh)(PGRendererHandle handle);

// Window Functions
PGWindowHandle PGCreateWindow(void);
void CloseWindow(PGWindowHandle window);
void ShowWindow(PGWindowHandle window);
void HideWindow(PGWindowHandle window);
void RefreshWindow(PGWindowHandle window);
void RefreshWindow(PGWindowHandle window, PGRect);
PGSize GetWindowSize(PGWindowHandle window);


void RenderRectangle(PGRendererHandle handle, PGRect rectangle, PGColor color);
void RenderLine(PGRendererHandle handle, PGLine line, PGColor color);
void RenderImage(PGRendererHandle window, void* image, int x, int y);
// Render text at the specified location; returns the width and height of the rendered text
PGSize RenderText(PGRendererHandle window, const char* text, size_t length, int x, int y);
void RenderCaret(PGRendererHandle renderer, const char *text, size_t len, int x, int y, ssize_t characternr);
void RenderSelection(PGRendererHandle renderer, const char *text, size_t len, int x, int y, ssize_t start, ssize_t end);
// Sets the color of the text rendered with the RenderText method
void SetTextColor(PGRendererHandle window, PGColor color);
// Sets the font used by the RenderText method
void SetTextFont(PGRendererHandle window, PGFontHandle font);
// Sets the text-alignment of text rendered with the RenderText method
void SetTextAlign(PGRendererHandle window, PGTextAlign alignment);

int GetRenderWidth(PGRendererHandle renderer, const char* text, ssize_t length);

Control* GetFocusedControl(PGWindowHandle window);


void SetWindowTitle(PGWindowHandle window, char* title);
void RegisterRefresh(PGWindowHandle window, Control *callback);
void UnregisterRefresh(PGWindowHandle window, Control *callback);
void RegisterMouseClick(PGWindowHandle window, PGClickFunction callback);
void RegisterMouseDown(PGWindowHandle window, PGClickFunction callback);
void RegisterButtonPress(PGWindowHandle window, PGButtonPress callback);
void RegisterMouseScroll(PGWindowHandle window, PGScrollFunction callback);




