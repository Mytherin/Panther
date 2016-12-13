/// windowfunctions.h
/// This is a set of functions that provide simple wrappers for native OS functionality
/// The actual GUI library is implemented on top of these simple wrappers
#pragma once

#include "utils.h"
#include <vector>

struct PGWindow;
typedef struct PGWindow* PGWindowHandle;

struct PGRenderer;
typedef struct PGRenderer* PGRendererHandle;

struct PGTimer;
typedef struct PGTimer* PGTimerHandle;

struct PGFont;
typedef struct PGFont* PGFontHandle;

typedef float PGScalar;

class Control;

struct PGPoint {
	PGScalar x;
	PGScalar y;

	PGPoint(PGScalar x, PGScalar y) : x(x), y(y) { }
};

struct PGSize {
	PGScalar width;
	PGScalar height;

	PGSize(PGScalar width, PGScalar height) : width(width), height(height) { }
};

struct PGIRect {
	int x;
	int y;
	int width;
	int height;

	PGIRect(int x, int y, int width, int height) : x(x), y(y), width(width), height(height) { }
};

struct PGRect {
	PGScalar x;
	PGScalar y;
	PGScalar width;
	PGScalar height;

	PGRect(PGScalar x, PGScalar y, PGScalar width, PGScalar height) : x(x), y(y), width(width), height(height) { }
};

struct PGLine {
	PGPoint start;
	PGPoint end;

	PGLine(PGPoint start, PGPoint end) : start(start), end(end) { }
	PGLine(PGScalar startx, PGScalar starty, PGScalar endx, PGScalar endy) : start(startx, starty), end(endx, endy) { }
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

typedef long long time_t;

time_t GetTime();

class MouseClickInstance {
public:
	PGScalar x;
	PGScalar y;
	time_t time;
	int clicks;

	MouseClickInstance() : x(0), y(0), time(0), clicks(0) { }
};

typedef byte PGModifier;

extern const PGModifier PGModifierNone;
extern const PGModifier PGModifierCmd;
extern const PGModifier PGModifierCtrl;
extern const PGModifier PGModifierShift;
extern const PGModifier PGModifierAlt;
extern const PGModifier PGModifierCtrlShift;
extern const PGModifier PGModifierCtrlAlt;
extern const PGModifier PGModifierShiftAlt;
extern const PGModifier PGModifierCtrlShiftAlt;
extern const PGModifier PGModifierCmdCtrl;
extern const PGModifier PGModifierCmdShift;
extern const PGModifier PGModifierCmdAlt;
extern const PGModifier PGModifierCmdCtrlShift;
extern const PGModifier PGModifierCmdShiftAlt;
extern const PGModifier PGModifierCmdCtrlAlt;
extern const PGModifier PGModifierCmdCtrlShiftAlt;

typedef byte PGMouseButton;

extern const PGMouseButton PGMouseButtonNone;
extern const PGMouseButton PGLeftMouseButton;
extern const PGMouseButton PGRightMouseButton;
extern const PGMouseButton PGMiddleMouseButton;
extern const PGMouseButton PGXButton1;
extern const PGMouseButton PGXButton2;

typedef void (*PGClickFunction)(int x, int y, PGMouseButton button, PGModifier modifier);
typedef void (*PGScrollFunction)(int x, int y, int distance, PGModifier modifier);
typedef void (*PGButtonPress)(PGButton button, PGModifier modifier);
typedef void (*PGRefresh)(PGRendererHandle handle);

std::string GetButtonName(PGButton);
std::string GetMouseButtonName(PGMouseButton);
std::string GetModifierName(PGModifier);

// Window Functions
PGWindowHandle PGCreateWindow(void);
void CloseWindow(PGWindowHandle window);
void ShowWindow(PGWindowHandle window);
void HideWindow(PGWindowHandle window);
void RefreshWindow(PGWindowHandle window);
void RefreshWindow(PGWindowHandle window, PGIRect);
void RedrawWindow(PGWindowHandle window);
void RedrawWindow(PGWindowHandle window, PGIRect);
PGSize GetWindowSize(PGWindowHandle window);

typedef enum {
	PGCursorStandard,
	PGCursorCrosshair,
	PGCursorHand,
	PGCursorIBeam,
	PGCursorWait
} PGCursorType;

typedef enum {
	PGStyleFill,
	PGStyleStroke
} PGStyle;

void SetCursor(PGWindowHandle window, PGCursorType type);

typedef enum {
	PGTimerFlagsNone = 0x00,
	PGTimerExecuteOnce = 0x01,
} PGTimerFlags;

typedef void (*PGTimerCallback)(void);

PGTimerHandle CreateTimer(int ms, PGTimerCallback, PGTimerFlags);
void DeleteTimer(PGTimerHandle);

void RenderTriangle(PGRendererHandle handle, PGPoint a, PGPoint b, PGPoint c, PGColor color, PGStyle drawStyle);
void RenderRectangle(PGRendererHandle handle, PGRect rectangle, PGColor color, PGStyle style);
void RenderLine(PGRendererHandle handle, PGLine line, PGColor color);
void RenderImage(PGRendererHandle window, void* image, int x, int y);
// Render text at the specified location
void RenderText(PGRendererHandle renderer, const char* text, size_t length, PGScalar x, PGScalar y);
// Render squiggles under text at the specified location
void RenderSquiggles(PGRendererHandle renderer, PGScalar width, PGScalar x, PGScalar y, PGColor color);
PGScalar MeasureTextWidth(PGRendererHandle renderer, const char* text, size_t length);
PGScalar GetTextHeight(PGRendererHandle renderer);
void RenderCaret(PGRendererHandle renderer, const char *text, size_t len, PGScalar x, PGScalar y, ssize_t characternr, PGScalar line_height);
void RenderSelection(PGRendererHandle renderer, const char *text, size_t len, PGScalar x, PGScalar y, ssize_t start, ssize_t end, PGColor selection_color, PGScalar line_height);
// Sets the color of the text rendered with the RenderText method
void SetTextColor(PGRendererHandle window, PGColor color);
// Sets the font used by the RenderText method
void SetTextFont(PGRendererHandle window, PGFontHandle font, PGScalar height);
// Sets the text-alignment of text rendered with the RenderText method
void SetTextAlign(PGRendererHandle window, PGTextAlign alignment);

Control* GetFocusedControl(PGWindowHandle window);
bool WindowHasFocus(PGWindowHandle window);


PGPoint GetMousePosition(PGWindowHandle window);
void SetWindowTitle(PGWindowHandle window, char* title);
void RegisterRefresh(PGWindowHandle window, Control *callback);
void UnregisterRefresh(PGWindowHandle window, Control *callback);
void RegisterMouseClick(PGWindowHandle window, PGClickFunction callback);
void RegisterMouseDown(PGWindowHandle window, PGClickFunction callback);
void RegisterButtonPress(PGWindowHandle window, PGButtonPress callback);
void RegisterMouseScroll(PGWindowHandle window, PGScrollFunction callback);


void SetClipboardText(PGWindowHandle window, std::string);
std::string GetClipboardText(PGWindowHandle window);