/// windowfunctions.h
/// This is a set of functions that provide simple wrappers for native OS functionality
/// The actual GUI library is implemented on top of these simple wrappers
#pragma once

#include "json.h"
#include "utils.h"

#include <string>
#include <vector>

struct PGWindow;
typedef struct PGWindow* PGWindowHandle;

struct PGRenderer;
typedef struct PGRenderer* PGRendererHandle;

struct PGTimer;
typedef struct PGTimer* PGTimerHandle;

struct PGFont;
typedef struct PGFont* PGFontHandle;

struct PGPopupMenu;
typedef struct PGPopupMenu* PGPopupMenuHandle;

struct PGBitmap;
typedef struct PGBitmap* PGBitmapHandle;

struct PGDropData;
typedef struct PGDropData* PGDropHandle;

typedef float PGScalar;

class Control;

struct PGPoint {
	PGScalar x;
	PGScalar y;

	friend PGPoint operator+(PGPoint lhs, const PGPoint& rhs) {
		return PGPoint(lhs.x + rhs.x, lhs.y + rhs.y);
	}
	friend PGPoint operator-(PGPoint lhs, const PGPoint& rhs) {
		return PGPoint(lhs.x - rhs.x, lhs.y - rhs.y);
	}
	PGPoint() : x(0), y(0) { }
	PGPoint(PGScalar x, PGScalar y) : x(x), y(y) { }
};

struct PGSize {
	PGScalar width;
	PGScalar height;

	friend PGSize operator+(PGSize lhs, const PGSize& rhs) {
		return PGSize(lhs.width + rhs.width, lhs.height + rhs.height);
	}
	friend PGSize operator-(PGSize lhs, const PGSize& rhs) {
		return PGSize(lhs.width - rhs.width, lhs.height - rhs.height);
	}
	PGSize() : width(0), height(0) { }
	PGSize(PGScalar width, PGScalar height) : width(width), height(height) { }
};

struct PGIRect;

struct PGRect {
	PGScalar x;
	PGScalar y;
	PGScalar width;
	PGScalar height;

	PGRect() : x(0), y(0), width(0), height(0) { }
	PGRect(PGScalar x, PGScalar y, PGScalar width, PGScalar height) : x(x), y(y), width(width), height(height) { }
	PGRect(PGIRect rect);
};

struct PGCircle {
	PGScalar x;
	PGScalar y;
	PGScalar radius;

	PGCircle() : x(0), y(0), radius(0) { }
	PGCircle(PGScalar x, PGScalar y, PGScalar radius) : x(x), y(y), radius(radius) { }
};

struct PGIRect {
	int x;
	int y;
	int width;
	int height;

	PGIRect() : x(0), y(0), width(0), height(0) { }
	PGIRect(int x, int y, int width, int height) : x(x), y(y), width(width), height(height) { }
	PGIRect(PGRect rect) : x((int) rect.x), y((int) rect.y), width((int) rect.width), height((int) rect.height) { }
};

struct PGLine {
	PGPoint start;
	PGPoint end;

	PGLine(PGPoint start, PGPoint end) : start(start), end(end) { }
	PGLine(PGScalar startx, PGScalar starty, PGScalar endx, PGScalar endy) : start(startx, starty), end(endx, endy) { }
};

struct PGPolygon {
	std::vector<PGPoint> points;
	bool closed = true;

	PGPolygon() : points(), closed(true) { }
	PGPolygon(std::vector<PGPoint> points) : points(points), closed(true) { }
};

bool PGRectangleContains(PGRect, PGPoint);
bool PGRectangleContains(PGIRect, PGPoint);
bool PGIRectanglesOverlap(PGIRect, PGIRect);

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

	PGColor() : r(0), g(0), b(0), a(0) { }
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

typedef long long PGTime;

PGTime GetTime();

class MouseClickInstance {
public:
	PGScalar x;
	PGScalar y;
	PGTime time;
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

class TextFile;

// Window Functions
PGWindowHandle PGCreateWindow(PGPoint position, std::vector<std::shared_ptr<TextFile>> initial_files);
PGWindowHandle PGCreateWindow(std::vector<std::shared_ptr<TextFile>> initial_files);
void PGCloseWindow(PGWindowHandle window);
void ShowWindow(PGWindowHandle window);
void HideWindow(PGWindowHandle window);
void RefreshWindow(PGWindowHandle window, bool redraw_now = false);
void RefreshWindow(PGWindowHandle window, PGIRect, bool redraw_now = false);
void RedrawWindow(PGWindowHandle window);
void RedrawWindow(PGWindowHandle window, PGIRect);
PGSize GetWindowSize(PGWindowHandle window);
PGPoint PGGetWindowPosition(PGWindowHandle window);

typedef enum {
	PGCursorNone,
	PGCursorStandard,
	PGCursorCrosshair,
	PGCursorHand,
	PGCursorIBeam,
	PGCursorWait
} PGCursorType;

typedef enum {
	PGDrawStyleFill,
	PGDrawStyleStroke
} PGDrawStyle;

void SetCursor(PGWindowHandle window, PGCursorType type);

typedef enum {
	PGTimerFlagsNone = 0x00,
	PGTimerExecuteOnce = 0x01,
} PGTimerFlags;

typedef void (*PGTimerCallback)(PGWindowHandle);

PGTimerHandle CreateTimer(PGWindowHandle handle, int ms, PGTimerCallback, PGTimerFlags);
void DeleteTimer(PGTimerHandle);

PGRendererHandle GetRendererHandle(PGWindowHandle window);
lng GetPositionInLine(PGFontHandle font, PGScalar x, const char* text, size_t length);

// set render bounds of the current canvas
// note that every call to SetRenderBounds MUST be followed by a call to ClearRenderBounds
void SetRenderBounds(PGRendererHandle, PGRect rectangle);
// clear the render bounds
void ClearRenderBounds(PGRendererHandle);

void RenderGradient(PGRendererHandle handle, PGRect rectangle, PGColor left, PGColor right);
void RenderTriangle(PGRendererHandle handle, PGPoint a, PGPoint b, PGPoint c, PGColor color, PGDrawStyle drawStyle);
void RenderRectangle(PGRendererHandle handle, PGRect rectangle, PGColor color, PGDrawStyle style, PGScalar width = 1.0f);
void RenderCircle(PGRendererHandle handle, PGCircle circle, PGColor color, PGDrawStyle style);
void RenderPolygon(PGRendererHandle handle, PGPolygon polygon, PGColor color, double stroke_width = -1);
void RenderLine(PGRendererHandle handle, PGLine line, PGColor color, int width = 2);
void RenderDashedLine(PGRendererHandle handle, PGLine line, PGColor color, PGScalar line_width, PGScalar spacing_width, int width = 2);
void RenderImage(PGRendererHandle window, PGBitmapHandle image, int x, int y, PGScalar max_position = INT_MAX);
// Render text at the specified location with the specified alignment, returns the width of the rendered text
PGScalar RenderText(PGRendererHandle renderer, PGFontHandle font, const char *text, size_t len, PGScalar x, PGScalar y, PGTextAlign);
PGScalar RenderString(PGRendererHandle renderer, PGFontHandle font, const std::string& text, PGScalar x, PGScalar y, PGTextAlign alignment);
// Render text at the specified location
void RenderText(PGRendererHandle renderer, PGFontHandle font, const char* text, size_t length, PGScalar x, PGScalar y, PGScalar max_position = INT_MAX);
void RenderString(PGRendererHandle renderer, PGFontHandle font, const std::string& text, PGScalar x, PGScalar y, PGScalar max_position = INT_MAX);
// Render squiggles under text at the specified location
void RenderSquiggles(PGRendererHandle renderer, PGScalar width, PGScalar x, PGScalar y, PGColor color);
void RenderFileIcon(PGRendererHandle renderer, PGFontHandle font, const char *text, PGScalar x, PGScalar y, PGScalar width, PGScalar height, PGColor text_color, PGColor page_color, PGColor edge_color);

PGBitmapHandle CreateBitmapFromSize(PGScalar width, PGScalar height);
PGBitmapHandle CreateBitmapForText(PGFontHandle font, const char* text, size_t length);
PGRendererHandle CreateRendererForBitmap(PGBitmapHandle handle);
PGBitmapHandle PGLoadImage(std::string path);
void DeleteRenderer(PGRendererHandle renderer);
void DeleteImage(PGBitmapHandle handle);

PGScalar MeasureTextWidth(PGFontHandle font, std::string& text);
PGScalar MeasureTextWidth(PGFontHandle font, const char* text);
PGScalar MeasureTextWidth(PGFontHandle font, const char* text, size_t length);
std::vector<PGScalar> CumulativeCharacterWidths(PGFontHandle font, const char* text, size_t length, PGScalar xoffset, PGScalar maximum_width, lng& render_start, lng& render_end);
PGScalar GetTextHeight(PGFontHandle font);
void RenderCaret(PGRendererHandle renderer, PGFontHandle font, PGScalar selection_offset, PGScalar x, PGScalar y, PGColor color);
void RenderSelection(PGRendererHandle renderer, PGFontHandle font, const char *text, size_t len, PGScalar x, PGScalar y, lng start, lng end, lng render_start, lng render_end, std::vector<PGScalar>& character_widths, PGColor selection_color);

enum PGTextStyle {
	PGTextStyleNormal,
	PGTextStyleBold,
	PGTextStyleItalic
};

PGFontHandle PGCreateFont(char* fontname, bool italic, bool bold);
PGFontHandle PGCreateFont(char* filename);
PGFontHandle PGCreateFont(bool italic = false, bool bold = false);
void PGDestroyFont(PGFontHandle font);
// Sets the color of the text rendered with the RenderText method
void SetTextColor(PGFontHandle font, PGColor color);
PGColor GetTextColor(PGFontHandle font);
// Sets the font used by the RenderText method
void SetTextFontSize(PGFontHandle font, PGScalar height);
PGScalar GetTextFontSize(PGFontHandle font);
void SetTextStyle(PGFontHandle font, PGTextStyle style);
void SetTextTabWidth(PGFontHandle font, int tabwidth);

Control* GetFocusedControl(PGWindowHandle window);
bool WindowHasFocus(PGWindowHandle window);

PGMouseButton GetMouseState(PGWindowHandle window);
PGPoint GetMousePosition(PGWindowHandle window);
PGPoint GetMousePosition(PGWindowHandle window, Control* c);
void SetWindowTitle(PGWindowHandle window, char* title);
void RegisterControl(PGWindowHandle window, Control *control);

class PGWorkspace;
PGWorkspace* PGGetWorkspace(PGWindowHandle window);
void PGLoadWorkspace(PGWindowHandle window, nlohmann::json& j);
void PGWriteWorkspace(PGWindowHandle window, nlohmann::json& j);


void* GetWindowManager(PGWindowHandle window);

void SetClipboardTextOS(PGWindowHandle window, std::string);
void SetClipboardText(PGWindowHandle window, std::string);
std::string GetClipboardText(PGWindowHandle window);
std::vector<std::string> GetClipboardTextHistory();


typedef byte PGPopupMenuFlags;

extern const PGPopupMenuFlags PGPopupMenuFlagsNone;
extern const PGPopupMenuFlags PGPopupMenuChecked;
extern const PGPopupMenuFlags PGPopupMenuGrayed;
extern const PGPopupMenuFlags PGPopupMenuSelected;
extern const PGPopupMenuFlags PGPopupMenuSubmenu;

typedef void(*PGControlCallback)(Control* control);
typedef void(*PGControlDataCallback)(Control* control, void* data);


struct PGPopupInformation {
	std::string text;
	std::string hotkey;
	std::string data;

	PGPopupInformation() { }
	PGPopupInformation(std::string text, std::string hotkey) : text(text), hotkey(hotkey), data() { }
};

typedef void(*PGPopupCallback)(Control* control, PGPopupInformation* popup);

PGPopupMenuHandle PGCreatePopupMenu(PGWindowHandle window, Control* control);
void PGPopupMenuInsertSubmenu(PGPopupMenuHandle, PGPopupMenuHandle submenu, std::string submenu_name);
void PGPopupMenuInsertEntry(PGPopupMenuHandle handle, std::string text, PGPopupCallback callback, PGPopupMenuFlags flags = PGPopupMenuFlagsNone);
void PGPopupMenuInsertEntry(PGPopupMenuHandle, PGPopupInformation info, PGPopupCallback callback, PGPopupMenuFlags flags = PGPopupMenuFlagsNone);
void PGPopupMenuInsertSeparator(PGPopupMenuHandle);
// Displays the menu next to the mouse
void PGDisplayPopupMenu(PGPopupMenuHandle, PGTextAlign align);
// Displays the menu at the specified point
void PGDisplayPopupMenu(PGPopupMenuHandle, PGPoint, PGTextAlign align);
PGSize PGMeasurePopupItem(PGFontHandle font, PGPopupInformation* information);
void PGRenderPopupItem(PGRendererHandle renderer, PGFontHandle font, PGPopupInformation* information, PGSize size, PGPopupMenuFlags flags);

void OpenFolderInExplorer(std::string path);
void OpenFolderInTerminal(std::string path);

std::vector<std::string> ShowOpenFileDialog(bool allow_files, bool allow_directories, bool allow_multiple_selection);
std::string ShowSaveFileDialog();

PGPoint ConvertWindowToScreen(PGWindowHandle, PGPoint); 

std::string OpenFileMenu();

void DropFile(PGWindowHandle handle, std::string filename);

enum PGDragDropType {
	PGDragDropNone,
	PGDragDropTabs
};

typedef void(*PGDropCallback)(PGPoint mouse, void* data);

void PGStartDragDrop(PGWindowHandle window, PGBitmapHandle image, PGDropCallback, void* data, size_t data_length);

void PGMessageBox(PGWindowHandle window, std::string title, std::string message);

enum PGResponse {
	PGResponseYes,
	PGResponseNo,
	PGResponseCancel
};

typedef void(*PGConfirmationCallback)(PGWindowHandle window, Control* control, void* data, PGResponse response);

PGResponse PGConfirmationBox(PGWindowHandle window, std::string title, std::string message);
void PGConfirmationBox(PGWindowHandle window, std::string title, std::string message, PGConfirmationCallback callback, Control* control, void* data);

std::string GetOSName();

struct PGVerticalScroll {
	lng linenumber = 0;
	lng inner_line = 0;

	PGVerticalScroll() : linenumber(0), inner_line(0) { }
	PGVerticalScroll(lng linenumber, lng inner_line) : linenumber(linenumber), inner_line(inner_line) { }

	friend bool operator< (const PGVerticalScroll& lhs, const PGVerticalScroll& rhs) {
		return lhs.linenumber < rhs.linenumber ||
			(lhs.linenumber == rhs.linenumber && lhs.inner_line < rhs.inner_line);
	}
	friend bool operator> (const PGVerticalScroll& lhs, const PGVerticalScroll& rhs){ return rhs < lhs; }
	friend bool operator<=(const PGVerticalScroll& lhs, const PGVerticalScroll& rhs){ return !(lhs > rhs); }
	friend bool operator>=(const PGVerticalScroll& lhs, const PGVerticalScroll& rhs){ return !(lhs < rhs); }
	friend bool operator==(const PGVerticalScroll& lhs, const PGVerticalScroll& rhs){ return lhs.linenumber == rhs.linenumber && lhs.inner_line == rhs.inner_line; }
	friend bool operator!=(const PGVerticalScroll& lhs, const PGVerticalScroll& rhs){ return !(lhs == rhs); }
};


enum PGFileFlags {
	PGFileFlagsEmpty,
	PGFileFlagsFileNotFound,
	PGFileFlagsErrorOpeningFile
};

struct PGFileInformation {
	PGFileFlags flags;
	lng creation_time;
	lng modification_time;
	lng file_size;
};

PGFileInformation PGGetFileFlags(std::string path);

struct PGFile {
	std::string path;

	std::string Filename();
	std::string Extension();
	PGFile() { }
	PGFile(std::string path) : path(path) { }
};

enum PGDirectoryFlags {
	PGDirectorySuccess,
	PGDirectoryNotFound,
	PGDirectoryUnknown
};

PGDirectoryFlags PGGetDirectoryFiles(std::string directory, std::vector<PGFile>& directories, std::vector<PGFile>& files);

std::string PGPathJoin(std::string path_one, std::string path_two);

enum PGFindTextType {
	PGFindSingleFile,
	PGFindReplaceSingleFile,
	PGFindReplaceManyFiles
};
