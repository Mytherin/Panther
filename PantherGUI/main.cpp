

#include "main.h"
#include "windowfunctions.h"
#include "control.h"
#include "textfield.h"
#include "time.h"
#include "scheduler.h"

#include <malloc.h>

#include <windowsx.h>
#include <vector>

#include <SkBitmap.h>
#include <SkDevice.h>
#include <SkPaint.h>
#include <SkRect.h>
#include <SkData.h>
#include <SkImage.h>
#include <SkStream.h>
#include <SkSurface.h>
#include <SkPath.h>
#include <SkTypeface.h>

struct PGWindow {
public:
	PGModifier modifier;
	HWND hwnd;
	std::vector<Control*> registered_refresh;
	Control *focused_control;
	PGIRect invalidated_area;
	PGCursorType cursor_type;
	bool invalidated;

	PGWindow() : modifier(PGModifierNone), invalidated_area(0, 0, 0, 0), invalidated(false) {}
};

struct PGRenderer {
	HDC hdc;
	SkCanvas* canvas;
	SkPaint* paint;
	SkPaint* textpaint;
	PGScalar character_width;
	PGScalar text_offset;

	PGRenderer(HDC hdc) : hdc(hdc), canvas(nullptr), paint(nullptr) {}
};

struct PGFont {
	HFONT font;
};

struct PGTimer {
	HANDLE timer;
};

PGWindowHandle global_handle;

HCURSOR cursor_standard = nullptr;
HCURSOR cursor_crosshair = nullptr;
HCURSOR cursor_hand = nullptr;
HCURSOR cursor_ibeam = nullptr;
HCURSOR cursor_wait = nullptr;

#define MAX_REFRESH_FREQUENCY 1000/30

void PeriodicWindowRedraw(void) {
	for (auto it = global_handle->registered_refresh.begin(); it != global_handle->registered_refresh.end(); it++) {
		(*it)->PeriodicRender();
	}
	if (global_handle->invalidated) {
		RedrawWindow(global_handle);
	} else if (global_handle->invalidated_area.width != 0) {
		RedrawWindow(global_handle, global_handle->invalidated_area);
	}
	global_handle->invalidated = 0;
	global_handle->invalidated_area.width = 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	static TCHAR szWindowClass[] = _T("Panther");
	static TCHAR szTitle[] = _T("Panther");

	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = (HICON)LoadImage( // returns a HANDLE so we have to cast to HICON
		nullptr,             // hInstance must be NULL when loading from a file
		"logo.ico",       // the icon file name
		IMAGE_ICON,       // specifies that the file is an icon
		0,                // width of the image (we'll specify default later on)
		0,                // height of the image
		LR_LOADFROMFILE |  // we want to load a file (as opposed to a resource)
		LR_DEFAULTSIZE |   // default metrics based on the type (IMAGE_ICON, 32x32)
		LR_SHARED         // let the system release the handle when it's no longer used
		);
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = nullptr;
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, IDI_APPLICATION);
	
	if (!RegisterClassEx(&wcex)) {
		MessageBox(nullptr,
			_T("Call to RegisterClassEx failed!"),
			_T("Win32 Guided Tour"),
			0);

		return 1;
	}

	cursor_standard = LoadCursor(nullptr, IDC_ARROW);
	cursor_crosshair = LoadCursor(nullptr, IDC_CROSS);
	cursor_hand = LoadCursor(nullptr, IDC_HAND);
	cursor_ibeam = LoadCursor(nullptr, IDC_IBEAM);
	cursor_wait = LoadCursor(nullptr, IDC_WAIT);

	// The parameters to CreateWindow explained:
	// szWindowClass: the name of the application
	// szTitle: the text that appears in the title bar
	// WS_OVERLAPPEDWINDOW: the type of window to create
	// CW_USEDEFAULT, CW_USEDEFAULT: initial position (x, y)
	// 500, 100: initial size (width, length)
	// NULL: the parent of this window
	// NULL: this application does not have a menu bar
	// hInstance: the first parameter from WinMain
	// NULL: not used in this application
	HWND hWnd = CreateWindow(
		szWindowClass,
		szTitle,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		1016, 738,
		nullptr,
		nullptr,
		hInstance,
		nullptr
		);
	if (!hWnd) {
		MessageBox(nullptr,
			_T("Call to CreateWindow failed!"),
			_T("Win32 Guided Tour"),
			0);

		return 1;
	}

	SetWindowLong(hWnd, GWL_EXSTYLE, WS_EX_COMPOSITED);

	PGWindowHandle res = new PGWindow();
	res->hwnd = hWnd;
	res->cursor_type = PGCursorStandard;
	global_handle = res;

	CreateTimer(MAX_REFRESH_FREQUENCY, PeriodicWindowRedraw, PGTimerFlagsNone);
	
	Scheduler::Initialize();
	Scheduler::SetThreadCount(8);

	// "E:\\Github Projects\\Tibialyzer4\\Database Scan\\tibiawiki_pages_current.xml"
	// "E:\\killinginthenameof.xml"
	// "C:\\Users\\wieis\\Desktop\\syntaxtest.py"
	// "C:\\Users\\wieis\\Desktop\\syntaxtest.c"
	TextField* textField = new TextField(res, "C:\\Users\\wieis\\Desktop\\syntaxtest.c");
	textField->width = 1000;
	textField->height = 700;
	textField->x = 0;
	textField->y = 0;


	// The parameters to ShowWindow explained:
	// hWnd: the value returned from CreateWindow
	// nCmdShow: the fourth parameter from WinMain
	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	//GdiplusShutdown(gdiplusToken);
	return (int)msg.wParam;
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
	case WM_PAINT: {
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);
		SkBitmap bitmap;
		SkPaint paint;
		SkPaint textpaint;
		PGRenderer renderer(hdc);
		PGIRect rect(
			ps.rcPaint.left,
			ps.rcPaint.top,
			ps.rcPaint.right - ps.rcPaint.left,
			ps.rcPaint.bottom - ps.rcPaint.top);

		bitmap.allocN32Pixels(rect.width, rect.height);
		bitmap.allocPixels();

		SkCanvas canvas(bitmap);
		textpaint.setTextSize(SkIntToScalar(15));
		textpaint.setAntiAlias(true);
		textpaint.setStyle(SkPaint::kFill_Style);
		textpaint.setTextEncoding(SkPaint::kUTF8_TextEncoding);
		textpaint.setTextAlign(SkPaint::kLeft_Align);
		SkFontStyle style(SkFontStyle::kNormal_Weight, SkFontStyle::kNormal_Width, SkFontStyle::kUpright_Slant);
		auto font = SkTypeface::MakeFromName("Consolas", style);
		textpaint.setTypeface(font);
		paint.setStyle(SkPaint::kFill_Style);
		canvas.clear(SkColorSetRGB(30, 30, 30));

		renderer.canvas = &canvas;
		renderer.paint = &paint;
		renderer.textpaint = &textpaint;

		for (auto it = global_handle->registered_refresh.begin(); it != global_handle->registered_refresh.end(); it++) {
			(*it)->Draw(&renderer, &rect);
		}

		BITMAPINFO bmi;
		memset(&bmi, 0, sizeof(bmi));
		bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
		bmi.bmiHeader.biWidth       = bitmap.width();
		bmi.bmiHeader.biHeight      = -bitmap.height(); // top-down image
		bmi.bmiHeader.biPlanes      = 1;
		bmi.bmiHeader.biBitCount    = 32;
		bmi.bmiHeader.biCompression = BI_RGB;
		bmi.bmiHeader.biSizeImage   = 0;

		assert(bitmap.width() * bitmap.bytesPerPixel() == bitmap.rowBytes());
		bitmap.lockPixels();
		int ret = SetDIBitsToDevice(hdc,
			rect.x, rect.y,
			bitmap.width(), bitmap.height(),
			0, 0,
			0, bitmap.height(),
			bitmap.getPixels(),
			&bmi,
			DIB_RGB_COLORS);
		(void)ret; // we're ignoring potential failures for now.
		bitmap.unlockPixels();
		EndPaint(hWnd, &ps);
		break;
	}
	case WM_KEYDOWN: {
		char character = '\0';
		PGButton button = PGButtonNone;
		switch (wParam) {
		case VK_SHIFT:
			global_handle->modifier |= PGModifierShift;
			break;
		case VK_CONTROL:
			global_handle->modifier |= PGModifierCtrl;
			break;
		case VK_MENU:
			global_handle->modifier |= PGModifierAlt;
			break;
		case VK_LEFT:
			button = PGButtonLeft;
			break;
		case VK_RIGHT:
			button = PGButtonRight;
			break;
		case VK_UP:
			button = PGButtonUp;
			break;
		case VK_DOWN:
			button = PGButtonDown;
			break;
		case VK_HOME:
			button = PGButtonHome;
			break;
		case VK_END:
			button = PGButtonEnd;
			break;
		case VK_INSERT:
			button = PGButtonInsert;
			break;
		case VK_DELETE:
			button = PGButtonDelete;
			break;
		case VK_F1:
			button = PGButtonF1;
			break;
		case VK_F2:
			button = PGButtonF2;
			break;
		case VK_F3:
			button = PGButtonF3;
			break;
		case VK_F4:
			button = PGButtonF4;
			break;
		case VK_F5:
			button = PGButtonF5;
			break;
		case VK_F6:
			button = PGButtonF6;
			break;
		case VK_F7:
			button = PGButtonF7;
			break;
		case VK_F8:
			button = PGButtonF8;
			break;
		case VK_F9:
			button = PGButtonF9;
			break;
		case VK_F10:
			button = PGButtonF10;
			break;
		case VK_F11:
			button = PGButtonF11;
			break;
		case VK_F12:
			button = PGButtonF12;
			break;
		case VK_BACK:
			button = PGButtonBackspace;
			break;
		case VK_RETURN:
			button = PGButtonEnter;
			break;
		case VK_PRIOR:
			button = PGButtonPageUp;
			break;
		case VK_NEXT:
			button = PGButtonPageDown;
			break;
		default:
			break;
		}
		if (button != PGButtonNone) {
			// FIXME modifier
			global_handle->focused_control->KeyboardButton(button, global_handle->modifier);
		} else if (wParam >= 0x41 && wParam <= 0x5A) {
			character = (char) wParam;
			if (global_handle->modifier != PGModifierNone) {
				global_handle->focused_control->KeyboardCharacter((char)wParam, global_handle->modifier);
			}
		}
		break;
	}
	case WM_KEYUP:
		switch (wParam) {
		case VK_SHIFT:
			global_handle->modifier &= ~PGModifierShift;
			break;
		case VK_CONTROL:
			global_handle->modifier &= ~PGModifierCtrl;
			break;
		case VK_MENU:
			global_handle->modifier &= ~PGModifierAlt;
			break;
		}
		break;

	case WM_CHAR:
		if (wParam < 0x20 || wParam >= 0x7F) break;
		global_handle->focused_control->KeyboardCharacter((char)wParam, PGModifierNone);
		break;
	case WM_UNICHAR:
		assert(0);
		break;
	case WM_MOUSEWHEEL: {
		// FIXME: control over which the mouse is
		int x = GET_X_LPARAM(lParam);
		int y = GET_Y_LPARAM(lParam);
		PGModifier modifier = 0;
		if (wParam & MK_CONTROL) modifier |= PGModifierCtrl;
		if (wParam & MK_SHIFT) modifier |= PGModifierShift;
		int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
		for (auto it = global_handle->registered_refresh.begin(); it != global_handle->registered_refresh.end(); it++) {
			(*it)->MouseWheel(x, y, zDelta, modifier);
		}
		break;
	}
	case WM_LBUTTONDOWN: {
		// FIXME: control over which the mouse is
		int x = GET_X_LPARAM(lParam);
		int y = GET_Y_LPARAM(lParam);
		PGModifier modifier = 0;
		if (wParam & MK_CONTROL) modifier |= PGModifierCtrl;
		if (wParam & MK_SHIFT) modifier |= PGModifierShift;
		global_handle->focused_control->MouseDown(x, y, PGLeftMouseButton, modifier);
		break;
	}
	case WM_LBUTTONUP: {
		// FIXME: control over which the mouse is
		int x = GET_X_LPARAM(lParam);
		int y = GET_Y_LPARAM(lParam);
		PGModifier modifier = 0;
		if (wParam & MK_CONTROL) modifier |= PGModifierCtrl;
		if (wParam & MK_SHIFT) modifier |= PGModifierShift;
		global_handle->focused_control->MouseUp(x, y, PGLeftMouseButton, modifier);
		break;
	}
	case WM_MBUTTONDOWN: {
		// FIXME: control over which the mouse is
		int x = GET_X_LPARAM(lParam);
		int y = GET_Y_LPARAM(lParam);
		PGModifier modifier = 0;
		if (wParam & MK_CONTROL) modifier |= PGModifierCtrl;
		if (wParam & MK_SHIFT) modifier |= PGModifierShift;
		global_handle->focused_control->MouseDown(x, y, PGMiddleMouseButton, modifier);
		break;
	}
	case WM_MBUTTONUP: {
		// FIXME: control over which the mouse is
		int x = GET_X_LPARAM(lParam);
		int y = GET_Y_LPARAM(lParam);
		PGModifier modifier = 0;
		if (wParam & MK_CONTROL) modifier |= PGModifierCtrl;
		if (wParam & MK_SHIFT) modifier |= PGModifierShift;
		global_handle->focused_control->MouseUp(x, y, PGMiddleMouseButton, modifier);
		break;
	}
	case WM_MOUSEMOVE: {
		int x = GET_X_LPARAM(lParam);
		int y = GET_Y_LPARAM(lParam);
		PGModifier modifier = 0;
		PGMouseButton buttons = PGMouseButtonNone;
		if (wParam & MK_CONTROL) modifier |= PGModifierCtrl;
		if (wParam & MK_SHIFT) modifier |= PGModifierShift;
		if (wParam & MK_LBUTTON) buttons |= PGLeftMouseButton;
		if (wParam & MK_RBUTTON) buttons |= PGRightMouseButton;
		if (wParam & MK_MBUTTON) buttons |= PGMiddleMouseButton;
		if (wParam & MK_XBUTTON1) buttons |= PGXButton1;
		if (wParam & MK_XBUTTON2) buttons |= PGXButton2;
		global_handle->focused_control->MouseMove(x, y, buttons);
		break;
	}
	case WM_SETCURSOR:
		return 1;
	case WM_ERASEBKGND:
		return 1;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
		break;
	}

	return 0;
}

PGPoint GetMousePosition(PGWindowHandle window) {
	POINT point;
	if (!GetCursorPos(&point)) {
		return PGPoint(-1, -1);
	}
	if (!ScreenToClient(window->hwnd, &point)) {
		return PGPoint(-1, -1);
	}
	return PGPoint((PGScalar) point.x, (PGScalar) point.y);
}

PGWindowHandle PGCreateWindow(void) {
	PGWindowHandle res = new PGWindow();
	if (!res) {
		return nullptr;
	}
	HINSTANCE hInstance = GetModuleHandle(nullptr);

	HWND hWnd = CreateWindow(
		"Panther",
		"Panther GUI",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		500, 100,
		nullptr,
		nullptr,
		hInstance,
		nullptr
		);
	if (!hWnd) {
		return nullptr;
	}
	res->hwnd = hWnd;
	return res;
}

void CloseWindow(PGWindowHandle window) {
	if (!window) return;
	if (window->hwnd) CloseWindow(window->hwnd);
	free(window);
}

void ShowWindow(PGWindowHandle window) {
	if (!window) return;
	ShowWindow(window->hwnd, SW_SHOWDEFAULT);
}

void HideWindow(PGWindowHandle window) {

}

void RefreshWindow(PGWindowHandle window) {
	window->invalidated = true;
}

void RefreshWindow(PGWindowHandle window, PGIRect rectangle) {
	// FIXME do not invalidate entire window here
	window->invalidated = true;
	if (window->invalidated_area.width == 0) {
		window->invalidated_area = rectangle;
	} else {
		window->invalidated_area.x = min(window->invalidated_area.x, rectangle.x);
		window->invalidated_area.y = min(window->invalidated_area.y, rectangle.y);
		window->invalidated_area.width = max(window->invalidated_area.x + window->invalidated_area.width, rectangle.x + rectangle.width) - window->invalidated_area.x;
		window->invalidated_area.height = max(window->invalidated_area.y + window->invalidated_area.height, rectangle.y + rectangle.height) - window->invalidated_area.y;
		assert(window->invalidated_area.x <= rectangle.x &&
			window->invalidated_area.y <= rectangle.y &&
			window->invalidated_area.x + window->invalidated_area.width >= rectangle.x + rectangle.width &&
			window->invalidated_area.y + window->invalidated_area.height >= rectangle.y + rectangle.height);
	}
}

void RedrawWindow(PGWindowHandle window) {
	InvalidateRect(window->hwnd, nullptr, true);
}

void RedrawWindow(PGWindowHandle window, PGIRect rectangle) {
	RECT rect;
	rect.top = rectangle.y;
	rect.left = rectangle.x;
	rect.right = rect.left + rectangle.width;
	rect.bottom = rect.top + rectangle.height;
	InvalidateRect(window->hwnd, &rect, true);
}

PGSize GetWindowSize(PGWindowHandle window) {
	// FIXME
	PGSize size(0, 0);
	return size;
}

static SkPaint::Style PGStyleConvert(PGStyle style) {
	if (style == PGStyleStroke) {
		return SkPaint::kStroke_Style;
	}
	return SkPaint::kFill_Style;
}

void RenderTriangle(PGRendererHandle handle, PGPoint a, PGPoint b, PGPoint c, PGColor color, PGStyle drawStyle) {
	SkPath path;
	SkPoint points[] = {{a.x, a.y}, {b.x, b.y}, {c.x, c.y}}; // triangle
	path.addPoly(points, 3, true);
	handle->paint->setColor(SkColorSetARGB(color.a, color.r, color.g, color.b));
	handle->paint->setStyle(PGStyleConvert(drawStyle));
	handle->canvas->drawPath(path, *handle->paint);
}

void RenderRectangle(PGRendererHandle handle, PGRect rectangle, PGColor color, PGStyle drawStyle) {
	SkRect rect;
	rect.fLeft = rectangle.x;
	rect.fTop = rectangle.y;
	rect.fRight = rectangle.x + rectangle.width;
	rect.fBottom = rectangle.y + rectangle.height;
	handle->paint->setStyle(PGStyleConvert(drawStyle));
	handle->paint->setColor(SkColorSetARGB(color.a, color.r, color.g, color.b));
	handle->canvas->drawRect(rect, *handle->paint);
}

void RenderLine(PGRendererHandle handle, PGLine line, PGColor color) {
	handle->paint->setColor(SkColorSetARGB(color.a, color.r, color.g, color.b));
	handle->canvas->drawLine(line.start.x, line.start.y, line.end.x, line.end.y, *handle->paint);
}

void RenderImage(PGRendererHandle renderer, void* image, int x, int y) {

}

void RenderText(PGRendererHandle renderer, const char *text, size_t len, PGScalar x, PGScalar y) {
	if (len == 0) {
		len = 1;
		text = " ";
	}
	// FIXME: render tabs correctly
	renderer->canvas->drawText(text, len, x, y + renderer->text_offset, *renderer->textpaint);
}

void RenderSquiggles(PGRendererHandle renderer, PGScalar width, PGScalar x, PGScalar y, PGColor color) {
	SkPath path;
	PGScalar offset = 3; // FIXME: depend on text height
	PGScalar end = x + width;
	path.moveTo(x, y);
	while (x < end) {
		path.quadTo(x + 1, y + offset, x + 2, y);
		offset = -offset;
		x += 2;
	}
	renderer->paint->setColor(SkColorSetARGB(color.a, color.r, color.g, color.b));
	renderer->paint->setAntiAlias(true);
	renderer->paint->setAutohinted(true);
	renderer->canvas->drawPath(path, *renderer->paint);
}

PGScalar MeasureTextWidth(PGRendererHandle renderer, const char* text, size_t length) {
	//return renderer->textpaint->measureText(text, length);
	int size = 0;
	for (size_t i = 0; i < length; i++) {
		if (text[i] == '\t') {
			size += 5; // FIXME: tab width
		} else {
			size += 1;
		}
	}
	return size * renderer->character_width;
}

PGScalar GetTextHeight(PGRendererHandle renderer) {
	SkPaint::FontMetrics metrics;
	renderer->textpaint->getFontMetrics(&metrics, 0);
	return metrics.fDescent - metrics.fAscent;
}

void RenderCaret(PGRendererHandle renderer, const char *text, size_t len, PGScalar x, PGScalar y, ssize_t characternr, PGScalar line_height) {
	PGScalar width = MeasureTextWidth(renderer, text, characternr);
	SkColor color = renderer->textpaint->getColor();
	RenderLine(renderer, PGLine(x + width, y, x + width, y + line_height), PGColor(SkColorGetR(color), SkColorGetG(color), SkColorGetB(color)));
}

void RenderSelection(PGRendererHandle renderer, const char *text, size_t len, PGScalar x, PGScalar y, ssize_t start, ssize_t end, PGColor selection_color, PGScalar line_height) {
	if (start == end) return;
	PGScalar selection_start = MeasureTextWidth(renderer, text, start);
	PGScalar selection_width = MeasureTextWidth(renderer, text, end > (ssize_t) len ? len : end);
	if (end > (ssize_t) len) {
		assert(end == len + 1);
		selection_width += renderer->character_width;
	}
	RenderRectangle(renderer, PGRect(x + selection_start, y, selection_width - selection_start, GetTextHeight(renderer)), selection_color, PGStyleFill);
}

void SetTextColor(PGRendererHandle renderer, PGColor color) {
	renderer->textpaint->setColor(SkColorSetARGB(color.a, color.r, color.g, color.b));
}

void SetTextFont(PGRendererHandle renderer, PGFontHandle font, PGScalar height) {
	renderer->textpaint->setTextSize(height);
	renderer->character_width = renderer->textpaint->measureText("x", 1);
	renderer->text_offset = renderer->textpaint->getFontBounds().height() / 2 + renderer->textpaint->getFontBounds().height() / 4;
}

void SetTextAlign(PGRendererHandle renderer, PGTextAlign alignment) {
	if (alignment & PGTextAlignBottom) {
		assert(0);
	} else if (alignment & PGTextAlignTop) {
		assert(0);
	} else if (alignment & PGTextAlignVerticalCenter) {
		assert(0);
	}

	if (alignment & PGTextAlignLeft) {
		renderer->textpaint->setTextAlign(SkPaint::kLeft_Align);
	} else if (alignment & PGTextAlignRight) {
		renderer->textpaint->setTextAlign(SkPaint::kRight_Align);
	} else if (alignment & PGTextAlignHorizontalCenter) {
		renderer->textpaint->setTextAlign(SkPaint::kCenter_Align);
	}
}

Control* GetFocusedControl(PGWindowHandle window) {
	return window->focused_control;
}

void RegisterRefresh(PGWindowHandle window, Control *control) {
	window->focused_control = control;
	window->registered_refresh.push_back(control);
}

void RegisterMouseClick(PGWindowHandle window, PGClickFunction callback) {

}

void RegisterMouseDown(PGWindowHandle window, PGClickFunction callback) {

}

void RegisterButtonPress(PGWindowHandle window, PGButtonPress callback) {

}

void RegisterMouseScroll(PGWindowHandle window, PGScrollFunction callback) {

}

time_t GetTime() {
	return GetTickCount();
}

void SetWindowTitle(PGWindowHandle window, char* title) {
	SetWindowText(window->hwnd, title);
}

void SetClipboardText(PGWindowHandle window, std::string text) {
	if (OpenClipboard(window->hwnd)) {
		HGLOBAL clipbuffer;
		char * buffer;
		EmptyClipboard();
		clipbuffer = GlobalAlloc(GMEM_DDESHARE, text.length() + 1);
		buffer = (char*)GlobalLock(clipbuffer);
		memcpy(buffer, text.c_str(), text.length() * sizeof(char));
		GlobalUnlock(clipbuffer);
		SetClipboardData(CF_TEXT, clipbuffer);
		CloseClipboard();
	}
}

std::string GetClipboardText(PGWindowHandle window) {
	if (OpenClipboard(window->hwnd)) {
		std::string text = std::string((char*)GetClipboardData(CF_TEXT));
		CloseClipboard();
		return text;
	}
	return nullptr;
}

PGLineEnding GetSystemLineEnding() {
	return PGLineEndingWindows;
}


VOID CALLBACK TimerRoutine(PVOID lpParam, BOOLEAN TimerOrWaitFired) {
	((PGTimerCallback)lpParam)();
}

PGTimerHandle CreateTimer(int ms, PGTimerCallback callback, PGTimerFlags flags) {
	HANDLE timer;
	ULONG timer_flags = 0;
	if (timer_flags & PGTimerExecuteOnce) timer_flags |= WT_EXECUTEONLYONCE;

	if (!CreateTimerQueueTimer(
		&timer,
		nullptr,
		TimerRoutine,
		callback,
		(DWORD)ms,
		flags & PGTimerExecuteOnce ? (DWORD)0 : (DWORD)ms,
		timer_flags))
		return nullptr;

	PGTimerHandle handle = new PGTimer();
	handle->timer = timer;
	return handle;
}

void DeleteTimer(PGTimerHandle handle) {
	DeleteTimerQueueTimer(nullptr, handle->timer, nullptr);
}

bool WindowHasFocus(PGWindowHandle window) {
	HWND hwnd = GetForegroundWindow();
	return hwnd == window->hwnd;
}

void SetCursor(PGWindowHandle window, PGCursorType type) {
	if (window->cursor_type == type) return;
	HCURSOR cursor = nullptr;
	switch (type) {
	case PGCursorStandard:
		cursor = cursor_standard;
		break;
	case PGCursorCrosshair:
		cursor = cursor_crosshair;
		break;
	case PGCursorHand:
		cursor = cursor_hand;
		break;
	case PGCursorIBeam:
		cursor = cursor_ibeam;
		break;
	case PGCursorWait:
		cursor = cursor_wait;
		break;
	default:
		return;
	}
	window->cursor_type = type;
	SetCursor(cursor);
}
