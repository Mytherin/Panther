/// windowfunctions.h
/// This is a set of functions that provide simple wrappers for native OS functionality
/// The actual GUI library is implemented on top of these simple wrappers
#pragma once

#include "json.h"
#include "utils.h"

#include <string>
#include <vector>

class PGWorkspace;

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

struct PGTooltip;
typedef struct PGTooltip* PGTooltipHandle;

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
	PGPoint& operator*=(const PGScalar& rhs) {
		this->x *= rhs;
		this->y *= rhs;
		return *this;
	}
	PGPoint() : x(0), y(0) { }
	PGPoint(PGScalar x, PGScalar y) : x(x), y(y) { }

	double GetDistance(PGPoint b);
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
	PGSize& operator*=(const PGScalar& rhs) {
		this->width *= rhs;
		this->height *= rhs;
		return *this;
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
	PGRect& operator*=(const PGScalar& rhs) {
		this->x *= rhs;
		this->y *= rhs;
		this->width *= rhs;
		this->height *= rhs;
		return *this;
	}
};

extern PGScalar PGMarginAuto;

struct PGPadding {
	PGScalar left;
	PGScalar right;
	PGScalar top;
	PGScalar bottom;

	PGPadding() : left(0), right(0), top(0), bottom(0) {}
	PGPadding(PGScalar padding) : left(padding), right(padding), top(padding), bottom(padding) {}
	PGPadding(PGScalar left, PGScalar right, PGScalar top, PGScalar bottom) : left(left), right(right), top(top), bottom(bottom) {}
};

struct PGCircle {
	PGScalar x;
	PGScalar y;
	PGScalar radius;

	PGCircle() : x(0), y(0), radius(0) { }
	PGCircle(PGScalar x, PGScalar y, PGScalar radius) : x(x), y(y), radius(radius) { }
	PGCircle& operator*=(const PGScalar& rhs) {
		this->x *= rhs;
		this->y *= rhs;
		this->radius *= rhs;
		return *this;
	}
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
	PGLine& operator*=(const PGScalar& rhs) {
		this->start *= rhs;
		this->end *= rhs;
		return *this;
	}
};

struct PGPolygon {
	std::vector<PGPoint> points;
	bool closed = true;

	PGPolygon() : points(), closed(true) { }
	PGPolygon(std::vector<PGPoint> points) : points(points), closed(true) { }

	PGPolygon& operator*=(const PGScalar& rhs) {
		for(size_t i = 0; i < points.size(); i++) {
			points[i] *= rhs;
		}
		return *this;
	}
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

std::string PGButtonToString(PGButton);

typedef long long PGTime;

PGTime PGGetTime();
PGTime PGGetTimeOS();

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
class TextView;

// Window Functions
PGWindowHandle PGCreateWindow(PGWorkspace* workspace, PGPoint position, std::vector<std::shared_ptr<TextView>> initial_files);
PGWindowHandle PGCreateWindow(PGWorkspace* workspace, std::vector<std::shared_ptr<TextView>> initial_files);
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
	PGCursorWait,
	PGCursorResizeHorizontal,
	PGCursorResizeVertical
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
void RenderImage(PGRendererHandle window, PGBitmapHandle image, int x, int y);
void RenderImage(PGRendererHandle renderer, PGBitmapHandle image, PGRect rect);
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

enum PGFontType {
	PGFontTypeTextField,
	PGFontTypeUI,
	PGFontTypePopup
};

PGFontHandle PGCreateFont(PGFontType);
PGFontHandle PGCreateFont(const char* fontname, bool italic, bool bold);
PGFontHandle PGCreateFont(const char* filename);
PGFontHandle PGCreateFont();
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
void SetWindowTitle(PGWindowHandle window, std::string title);

class PGWorkspace;
void PGInitializeGlobals();
void PGInitialize();
PGWorkspace* PGInitializeFirstWorkspace();
PGWorkspace* PGGetWorkspace(PGWindowHandle window);
void PGCloseWorkspace(PGWorkspace* workspace);
void PGLoadWorkspace(PGWindowHandle window, nlohmann::json& j);
void PGWriteWorkspace(PGWindowHandle window, nlohmann::json& j);

class ControlManager;
std::shared_ptr<ControlManager> PGCreateControlManager(PGWindowHandle handle, std::vector<std::shared_ptr<TextView>> initial_files);
ControlManager* GetWindowManager(PGWindowHandle window);
void SetWindowManager(PGWindowHandle window, std::shared_ptr<ControlManager> manager);


void SetClipboardTextOS(PGWindowHandle window, std::string);
void SetClipboardText(PGWindowHandle window, std::string);
std::string GetClipboardText(PGWindowHandle window);
std::string GetClipboardTextOS(PGWindowHandle window);
std::vector<std::string> GetClipboardTextHistory();


typedef byte PGPopupMenuFlags;

extern const PGPopupMenuFlags PGPopupMenuFlagsNone;
extern const PGPopupMenuFlags PGPopupMenuChecked;
extern const PGPopupMenuFlags PGPopupMenuGrayed;
extern const PGPopupMenuFlags PGPopupMenuSelected;
extern const PGPopupMenuFlags PGPopupMenuSubmenu;
extern const PGPopupMenuFlags PGPopupMenuHighlighted;

typedef void(*PGControlCallback)(Control* control);
typedef void(*PGControlDataCallback)(Control* control, void* data);

struct PGFunctionData {
	PGControlDataCallback function = nullptr;
	void* data = nullptr;

	PGFunctionData() : function(nullptr), data(nullptr) {}
};

enum PGPopupType {
	PGPopupTypeEntry,
	PGPopupTypeSeparator,
	PGPopupTypeMenu,
	PGPopupTypeSubmenu
};

struct PGPopupInformation {
	PGPopupMenuHandle handle;
	PGPopupMenuHandle menu_handle = nullptr;
	std::string text;
	std::string hotkey;
	std::string data;
	union {
		double ddata;
		lng ldata;
		void* pdata;
	};
	PGPopupType type;
	PGBitmapHandle image = nullptr;

	PGPopupInformation() : handle(nullptr) {}
	PGPopupInformation(PGPopupMenuHandle handle) : handle(handle), type(PGPopupTypeEntry), pdata(nullptr), image(nullptr), menu_handle(nullptr) { }
	PGPopupInformation(PGPopupMenuHandle handle, std::string data) : handle(handle), type(PGPopupTypeEntry), data(data), image(nullptr), menu_handle(nullptr) {}
	PGPopupInformation(PGPopupMenuHandle handle, double data) : handle(handle), type(PGPopupTypeEntry), ddata(data), image(nullptr), menu_handle(nullptr) {}
	PGPopupInformation(PGPopupMenuHandle handle, lng data) : handle(handle), type(PGPopupTypeEntry), ldata(data), image(nullptr), menu_handle(nullptr) {}
	PGPopupInformation(PGPopupMenuHandle handle, void* data) : handle(handle), type(PGPopupTypeEntry), pdata(data), image(nullptr), menu_handle(nullptr) {}
	PGPopupInformation(PGPopupMenuHandle handle, std::string text, std::string hotkey) : handle(handle), text(text), hotkey(hotkey), data(), type(PGPopupTypeEntry), pdata(nullptr), image(nullptr), menu_handle(nullptr) { }
	PGPopupInformation(PGPopupMenuHandle handle, std::string text, std::string hotkey, PGBitmapHandle image) : handle(handle), text(text), hotkey(hotkey), data(), type(PGPopupTypeEntry), pdata(nullptr), image(image), menu_handle(nullptr) {}
	PGPopupInformation(PGPopupMenuHandle handle, std::string text, std::string hotkey, void* data) : handle(handle), text(text), hotkey(hotkey), data(), type(PGPopupTypeEntry), pdata(data), image(nullptr), menu_handle(nullptr) {}
	PGPopupInformation(PGPopupMenuHandle handle, std::string text, std::string hotkey, PGBitmapHandle image, void* data) : handle(handle), text(text), hotkey(hotkey), data(), type(PGPopupTypeEntry), pdata(data), image(image), menu_handle(nullptr) {}
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
PGSize PGMeasurePopupItem(PGFontHandle font, PGPopupInformation* information, PGScalar text_width, PGScalar hotkey_width, PGPopupType type);
void PGRenderPopupItem(PGRendererHandle renderer, PGPoint point, PGFontHandle font, PGPopupInformation* information, PGSize size, PGPopupMenuFlags flags, PGScalar text_width, PGScalar hotkey_width, PGPopupType type);

PGPopupMenuHandle PGCreateMenu(PGWindowHandle window, Control* control);
void PGSetWindowMenu(PGWindowHandle, PGPopupMenuHandle);

void OpenFolderInExplorer(std::string path);
void OpenFolderInTerminal(std::string path);

std::vector<std::string> ShowOpenFileDialog(bool allow_files, bool allow_directories, bool allow_multiple_selection);
std::string ShowSaveFileDialog();

PGPoint ConvertWindowToScreen(PGWindowHandle, PGPoint); 

void DropFile(PGWindowHandle handle, std::string filename);

enum PGDragDropType {
	PGDragDropNone,
	PGDragDropTabs
};

typedef void(*PGDropCallback)(PGPoint mouse, void* data);

void PGStartDragDrop(PGWindowHandle window, PGBitmapHandle image, PGDropCallback, void* data, size_t data_length);
void PGCancelDragDrop(PGWindowHandle window);


void PGMessageBox(PGWindowHandle window, std::string title, std::string message);

enum PGResponse {
	PGResponseYes,
	PGResponseNo,
	PGResponseCancel
};

enum PGConfirmationBoxType {
	PGConfirmationBoxYesNoCancel,
	PGConfirmationBoxYesNo
};

typedef void(*PGConfirmationCallback)(PGWindowHandle window, Control* control, void* data, PGResponse response);

PGResponse PGConfirmationBox(PGWindowHandle window, std::string title, std::string message, PGConfirmationBoxType type = PGConfirmationBoxYesNoCancel);
void PGConfirmationBox(PGWindowHandle window, std::string title, std::string message, PGConfirmationCallback callback, Control* control, void* data, PGConfirmationBoxType type = PGConfirmationBoxYesNoCancel);

std::string GetOSName();

struct PGVerticalScroll {
	lng linenumber = 0;
	lng inner_line = 0;
	PGScalar line_fraction = 0;

	PGVerticalScroll() : linenumber(0), inner_line(0), line_fraction(0) { }
	PGVerticalScroll(lng linenumber, lng inner_line) : linenumber(linenumber), inner_line(inner_line), line_fraction(0) { }

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
	bool is_directory;
};

PGFileInformation PGGetFileFlags(std::string path);

enum PGIOError {
	PGIOSuccess,
	PGIOErrorPermissionDenied,
	PGIOErrorFileNotFound,
	PGIOErrorOther
};

PGIOError PGRenameFile(std::string source, std::string dest);
// permanently delete a file
PGIOError PGRemoveFile(std::string source);
// delete a file by sending it to the recycle bin
PGIOError PGTrashFile(std::string source);


struct PGFile {
	std::string path;

	std::string Directory();
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

PGDirectoryFlags PGGetDirectoryFiles(std::string directory, std::vector<PGFile>& directories, std::vector<PGFile>& files, void* glob = nullptr);
PGDirectoryFlags PGGetDirectoryFilesOS(std::string directory, std::vector<PGFile>& directories, std::vector<PGFile>& files, void* glob = nullptr);

std::string PGCurrentDirectory();
std::string PGPathJoin(std::string path_one, std::string path_two);
std::string PGRootPath(std::string path);


enum PGFindTextType {
	PGFindSingleFile,
	PGFindReplaceSingleFile,
	PGFindReplaceManyFiles
};

void PGLogMessage(std::string text);


typedef void(*PGMouseCallback)(Control* control, bool, void*);

struct PGMouseRegion {
	Control* control = nullptr;

	virtual void MouseMove(PGPoint mouse) = 0;

	PGMouseRegion(Control* control) : control(control) {}
};

struct PGSingleMouseRegion : public PGMouseRegion {
	bool mouse_inside = false;
	PGIRect* rect = nullptr;
	void* data = nullptr;
	PGMouseCallback mouse_event;

	void MouseMove(PGPoint mouse);

	PGSingleMouseRegion(PGIRect* rect, Control* control, PGMouseCallback mouse_event, void* data = nullptr) : rect(rect), PGMouseRegion(control), mouse_event(mouse_event), data(data) {}
};

typedef std::vector<std::unique_ptr<PGSingleMouseRegion>> MouseRegionSet;

struct PGMouseRegionContainer : public PGMouseRegion {
	MouseRegionSet* regions;
	bool currently_contained;

	void MouseMove(PGPoint mouse);

	PGMouseRegionContainer(MouseRegionSet* regions, Control* control) : regions(regions), PGMouseRegion(control), currently_contained(false) {}
};

PGTooltipHandle PGCreateTooltip(PGWindowHandle, PGRect rect, std::string text);
void PGUpdateTooltipRegion(PGTooltipHandle, PGRect rect);
void PGDestroyTooltip(PGTooltipHandle);

std::string PGApplicationPath();

struct PGCommandLineSettings {
	int exit_code = -1;
	bool new_window = false;
	bool wait = false;
	bool background = false;
	std::vector<std::string> files;

	PGCommandLineSettings() : exit_code(-1), new_window(false), wait(false), background(false) { }
};

PGCommandLineSettings PGHandleCommandLineArguments(int argc, const char** argv);

