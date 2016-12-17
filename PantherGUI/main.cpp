

#include "main.h"
#include "windowfunctions.h"
#include "control.h"
#include "textfield.h"
#include "scheduler.h"
#include "filemanager.h"
#include "tabcontrol.h"
#include "controlmanager.h"
#include "container.h"
#include "droptarget.h"

#include <malloc.h>

#include <windowsx.h>
#include <vector>

#include "renderer.h"

struct PGWindow {
public:
	PGModifier modifier;
	HWND hwnd;
	ControlManager* manager;
	HCURSOR cursor;

	PGWindow() : modifier(PGModifierNone) {}
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
	global_handle->manager->PeriodicRender();
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	static TCHAR szWindowClass[] = _T("Panther");
	static TCHAR szTitle[] = _T("Panther");

	OleInitialize(nullptr);

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
	wcex.hCursor = NULL;
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

	IDropTarget* drop_target;
	RegisterDropWindow(hWnd, &drop_target);

	PGWindowHandle res = new PGWindow();
	res->hwnd = hWnd;
	res->cursor = cursor_standard;
	global_handle = res;

	ControlManager* manager = new ControlManager(res);
	manager->SetPosition(PGPoint(0, 0));
	manager->SetSize(PGSize(1000, 700));
	manager->SetAnchor(PGAnchorLeft | PGAnchorRight | PGAnchorTop | PGAnchorBottom);
	res->manager = manager;

	CreateTimer(MAX_REFRESH_FREQUENCY, PeriodicWindowRedraw, PGTimerFlagsNone);

	Scheduler::Initialize();
	Scheduler::SetThreadCount(8);

	// "E:\\Github Projects\\Tibialyzer4\\Database Scan\\tibiawiki_pages_current.xml"
	// "E:\\killinginthenameof.xml"
	// "C:\\Users\\wieis\\Desktop\\syntaxtest.py"
	// "C:\\Users\\wieis\\Desktop\\syntaxtest.c"
	TextFile* textfile = FileManager::OpenFile("E:\\Github Projects\\Tibialyzer4\\Database Scan\\tibiawiki_pages_current.xml");
	TextFile* textfile2 = FileManager::OpenFile("C:\\Users\\wieis\\Desktop\\syntaxtest.c");
	PGContainer* tabbed = new PGContainer(res, textfile);
	tabbed->SetAnchor(PGAnchorLeft | PGAnchorRight | PGAnchorTop | PGAnchorBottom);
	tabbed->UpdateParentSize(PGSize(0, 0), manager->GetSize());
	/*
	TabControl* tabs = new TabControl(res);
	tabs->width = 1000;
	tabs->height = 20;
	tabs->x = 0;
	tabs->y = 0;
	tabs->anchor = PGAnchorLeft | PGAnchorRight;

	TextField* textField = new TextField(res, textfile);
	textField->width = 1000;
	textField->height = 670;
	textField->x = 0;
	textField->y = 20;
	textField->anchor = PGAnchorBottom | PGAnchorLeft | PGAnchorRight;*/

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
	UnregisterDropWindow(hWnd, drop_target);

	//GdiplusShutdown(gdiplusToken);
	return (int)msg.wParam;
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
	case WM_PAINT: {
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);
		SkBitmap bitmap;
		PGIRect rect(
			ps.rcPaint.left,
			ps.rcPaint.top,
			ps.rcPaint.right - ps.rcPaint.left,
			ps.rcPaint.bottom - ps.rcPaint.top);

		RenderControlsToBitmap(bitmap, rect, global_handle->manager, "Consolas");

		BITMAPINFO bmi;
		memset(&bmi, 0, sizeof(bmi));
		bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		bmi.bmiHeader.biWidth = bitmap.width();
		bmi.bmiHeader.biHeight = -bitmap.height(); // top-down image
		bmi.bmiHeader.biPlanes = 1;
		bmi.bmiHeader.biBitCount = 32;
		bmi.bmiHeader.biCompression = BI_RGB;
		bmi.bmiHeader.biSizeImage = 0;

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
		case VK_TAB:
			button = PGButtonTab;
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
			global_handle->manager->KeyboardButton(button, global_handle->modifier);
		} else if (wParam >= 0x41 && wParam <= 0x5A) {
			character = (char)wParam;
			if (global_handle->modifier != PGModifierNone) {
				global_handle->manager->KeyboardCharacter((char)wParam, global_handle->modifier);
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
		global_handle->manager->KeyboardCharacter((char)wParam, PGModifierNone);
		break;
	case WM_UNICHAR:
		assert(0);
		break;
	case WM_MOUSEWHEEL: {
		// FIXME: control over which the mouse is
		POINT point;
		point.x = GET_X_LPARAM(lParam);
		point.y = GET_Y_LPARAM(lParam);
		ScreenToClient(global_handle->hwnd, &point);
		PGModifier modifier = 0;
		if (wParam & MK_CONTROL) modifier |= PGModifierCtrl;
		if (wParam & MK_SHIFT) modifier |= PGModifierShift;
		int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
		global_handle->manager->MouseWheel(point.x, point.y, zDelta, modifier);
		break;
	}
	case WM_LBUTTONDOWN: {
		// FIXME: control over which the mouse is
		int x = GET_X_LPARAM(lParam);
		int y = GET_Y_LPARAM(lParam);
		PGModifier modifier = 0;
		if (wParam & MK_CONTROL) modifier |= PGModifierCtrl;
		if (wParam & MK_SHIFT) modifier |= PGModifierShift;
		global_handle->manager->MouseDown(x, y, PGLeftMouseButton, modifier);
		break;
	}
	case WM_LBUTTONUP: {
		// FIXME: control over which the mouse is
		int x = GET_X_LPARAM(lParam);
		int y = GET_Y_LPARAM(lParam);
		PGModifier modifier = 0;
		if (wParam & MK_CONTROL) modifier |= PGModifierCtrl;
		if (wParam & MK_SHIFT) modifier |= PGModifierShift;
		global_handle->manager->MouseUp(x, y, PGLeftMouseButton, modifier);
		break;
	}
	case WM_MBUTTONDOWN: {
		// FIXME: control over which the mouse is
		int x = GET_X_LPARAM(lParam);
		int y = GET_Y_LPARAM(lParam);
		PGModifier modifier = 0;
		if (wParam & MK_CONTROL) modifier |= PGModifierCtrl;
		if (wParam & MK_SHIFT) modifier |= PGModifierShift;
		global_handle->manager->MouseDown(x, y, PGMiddleMouseButton, modifier);
		break;
	}
	case WM_MBUTTONUP: {
		// FIXME: control over which the mouse is
		int x = GET_X_LPARAM(lParam);
		int y = GET_Y_LPARAM(lParam);
		PGModifier modifier = 0;
		if (wParam & MK_CONTROL) modifier |= PGModifierCtrl;
		if (wParam & MK_SHIFT) modifier |= PGModifierShift;
		global_handle->manager->MouseUp(x, y, PGMiddleMouseButton, modifier);
		break;
	}
	case WM_SETCURSOR:
		SetCursor(global_handle->cursor);
		return true;
	case WM_ERASEBKGND:
		return 1;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_SIZE: {
		if (wParam != SIZE_MINIMIZED) {
			int width = LOWORD(lParam);
			int height = HIWORD(lParam);
			PGSize old_size = GetWindowSize(global_handle);
			global_handle->manager->UpdateParentSize(old_size, PGSize(width, height));
			RedrawWindow(global_handle);
		}
		break;
	}
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
	return PGPoint((PGScalar)point.x, (PGScalar)point.y);
}

PGMouseButton GetMouseState(PGWindowHandle window) {
	PGMouseButton button = PGMouseButtonNone;
	if ((GetKeyState(VK_LBUTTON) & 0x80) != 0) {
		button |= PGLeftMouseButton;
	}
	if ((GetKeyState(VK_RBUTTON) & 0x80) != 0) {
		button |= PGRightMouseButton;
	}
	if ((GetKeyState(VK_MBUTTON) & 0x80) != 0) {
		button |= PGMiddleMouseButton;
	}
	return button;
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
	window->manager->RefreshWindow();
}

void RefreshWindow(PGWindowHandle window, PGIRect rectangle) {
	window->manager->RefreshWindow(rectangle);
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
	RECT rect;
	GetWindowRect(window->hwnd, &rect);
	return PGSize(rect.right - rect.left, rect.bottom - rect.top);
}

Control* GetFocusedControl(PGWindowHandle window) {
	return window->manager->GetFocusedControl();
}

void RegisterControl(PGWindowHandle window, Control *control) {
	if (window->manager != nullptr)
		window->manager->AddControl(control);
}

PGTime GetTime() {
	return (PGTime)GetTickCount();
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

char GetSystemPathSeparator() {
	return '\\';
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
	window->cursor = cursor;
}

void* GetControlManager(PGWindowHandle window) {
	return window->manager;
}
