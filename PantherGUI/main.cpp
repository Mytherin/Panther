

#include "main.h"
#include "windowfunctions.h"
#include "control.h"
#include "scheduler.h"
#include "filemanager.h"
#include "controlmanager.h"
#include "container.h"
#include "droptarget.h"
#include "encoding.h"
#include "unicode.h"
#include "language.h"
#include "logger.h"

#include "statusbar.h"
#include "textfield.h"
#include "tabcontrol.h"

#include "c.h"
#include "xml.h"

#include <malloc.h>

#include <windowsx.h>
#include <vector>

#include "renderer.h"

struct PGWindow {
public:
	PGModifier modifier;
	HWND hwnd = nullptr;
	ControlManager* manager = nullptr;
	PGRendererHandle renderer = nullptr;
	PGPopupMenuHandle popup = nullptr;
	HCURSOR cursor;

	PGWindow() : modifier(PGModifierNone) {}
};

struct PGTimer {
	HANDLE timer;
};

struct PGPopupMenu {
	PGWindowHandle window;
	HMENU menu;
	int index = 1000;
	std::map<int, PGControlCallback> callbacks;
	Control* control;
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
	res->renderer = InitializeRenderer();
	global_handle = res;

	ControlManager* manager = new ControlManager(res);
	manager->SetPosition(PGPoint(0, 0));
	manager->SetSize(PGSize(1000, 700));
	manager->SetAnchor(PGAnchorLeft | PGAnchorRight | PGAnchorTop | PGAnchorBottom);
	res->manager = manager;
	
	CreateTimer(res, MAX_REFRESH_FREQUENCY, PeriodicWindowRedraw, PGTimerFlagsNone);

	PGLanguageManager::AddLanguage(new CLanguage());
	PGLanguageManager::AddLanguage(new XMLLanguage());

	Scheduler::Initialize();
	Scheduler::SetThreadCount(8);

	// "E:\\Github Projects\\Tibialyzer4\\Database Scan\\tibiawiki_pages_current.xml"
	// "E:\\killinginthenameof.xml"
	// "C:\\Users\\wieis\\Desktop\\syntaxtest.py"
	// "C:\\Users\\wieis\\Desktop\\syntaxtest.c"
	TextFile* textfile = FileManager::OpenFile("C:\\Users\\wieis\\Desktop\\syntaxtest.c");
	TextFile* textfile2 = FileManager::OpenFile("E:\\Github Projects\\Tibialyzer4\\Database Scan\\tibiawiki_pages_small.xml");
	PGContainer* tabbed = new PGContainer(res);
	tabbed->width = 0;
	tabbed->height = TEXT_TAB_HEIGHT;
	TextField* textfield = new TextField(res, textfile);
	textfield->SetAnchor(PGAnchorBottom | PGAnchorLeft | PGAnchorRight);
	textfield->SetPosition(PGPoint(tabbed->x, tabbed->y + TEXT_TAB_HEIGHT));
	textfield->SetSize(PGSize(tabbed->width, tabbed->height - TEXT_TAB_HEIGHT - STATUSBAR_HEIGHT));
	TabControl* tabs = new TabControl(res, textfield);
	tabs->SetAnchor(PGAnchorLeft | PGAnchorRight);
	tabs->SetPosition(PGPoint(tabbed->x, tabbed->y));
	tabs->SetSize(PGSize(tabbed->width, TEXT_TAB_HEIGHT));
	tabbed->AddControl(tabs);
	tabbed->AddControl(textfield);

	StatusBar* bar = new StatusBar(res, textfield);
	bar->SetAnchor(PGAnchorLeft | PGAnchorRight);
	bar->SetPosition(PGPoint(manager->x, manager->y + manager->height - STATUSBAR_HEIGHT));
	bar->SetSize(PGSize(manager->width, STATUSBAR_HEIGHT));

	tabbed->SetAnchor(PGAnchorLeft | PGAnchorRight | PGAnchorTop | PGAnchorBottom);
	//tabbed->SetPosition(PGPoint(50, 50));
	//tabbed->SetSize(manager->GetSize() - PGSize(100, 100));
	tabbed->UpdateParentSize(PGSize(0, 0), manager->GetSize());

	manager->AddControl(tabbed);
	manager->AddControl(bar);
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

		RenderControlsToBitmap(global_handle->renderer, bitmap, rect, global_handle->manager);

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
		case VK_OEM_PLUS:
			character = '+';
			break;
		case VK_OEM_COMMA:
			character = ',';
			break;
		case VK_OEM_MINUS:
			character = '-';
			break;
		case VK_OEM_PERIOD:
			character = '.';
			break;
		case VK_ESCAPE:
			button = PGButtonEscape;
			break;
		default:
			break;
		}
		if (button != PGButtonNone) {
			global_handle->manager->KeyboardButton(button, global_handle->modifier);
			return 0;
		} else if ((wParam >= 0x41 && wParam <= 0x5A) || character != '\0') {
			if (character == '\0')
				character = (char)wParam;
			if (global_handle->modifier != PGModifierNone) {
				global_handle->manager->KeyboardCharacter(character, global_handle->modifier);
				return 0;
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
	case WM_CHAR: {
		// FIXME: more efficient UTF16 -> UTF8 conversion
		wchar_t w = (wchar_t)wParam;
		std::string str = std::string((char*) &w, 2);
		char* output;
		lng size = PGConvertText(str, &output, PGEncodingUTF16Platform, PGEncodingUTF8);

		if (size == 1) {
			if (wParam < 0x20 || wParam >= 0x7F)
				break;
			global_handle->manager->KeyboardCharacter(output[0], PGModifierNone);
		} else {
			PGUTF8Character u;
			u.length = size;
			memcpy(u.character, output, size);

			global_handle->manager->KeyboardUnicode(u, PGModifierNone);
		}
		free(output);
		break;
	}
	case WM_UNICHAR:
		// we handle WM_CHAR (UTF16), not WM_UNICHAR (UTF32)
		return false;
	case WM_MOUSEWHEEL: {
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
		int x = GET_X_LPARAM(lParam);
		int y = GET_Y_LPARAM(lParam);
		PGModifier modifier = 0;
		if (wParam & MK_CONTROL) modifier |= PGModifierCtrl;
		if (wParam & MK_SHIFT) modifier |= PGModifierShift;
		global_handle->manager->MouseDown(x, y, PGLeftMouseButton, modifier);
		break;
	}
	case WM_LBUTTONUP: {
		int x = GET_X_LPARAM(lParam);
		int y = GET_Y_LPARAM(lParam);
		PGModifier modifier = 0;
		if (wParam & MK_CONTROL) modifier |= PGModifierCtrl;
		if (wParam & MK_SHIFT) modifier |= PGModifierShift;
		global_handle->manager->MouseUp(x, y, PGLeftMouseButton, modifier);
		break;
	}
	case WM_MBUTTONDOWN: {
		int x = GET_X_LPARAM(lParam);
		int y = GET_Y_LPARAM(lParam);
		PGModifier modifier = 0;
		if (wParam & MK_CONTROL) modifier |= PGModifierCtrl;
		if (wParam & MK_SHIFT) modifier |= PGModifierShift;
		global_handle->manager->MouseDown(x, y, PGMiddleMouseButton, modifier);
		break;
	}
	case WM_MBUTTONUP: {
		int x = GET_X_LPARAM(lParam);
		int y = GET_Y_LPARAM(lParam);
		PGModifier modifier = 0;
		if (wParam & MK_CONTROL) modifier |= PGModifierCtrl;
		if (wParam & MK_SHIFT) modifier |= PGModifierShift;
		global_handle->manager->MouseUp(x, y, PGMiddleMouseButton, modifier);
		break;
	}

	case WM_RBUTTONDOWN: {
		int x = GET_X_LPARAM(lParam);
		int y = GET_Y_LPARAM(lParam);
		PGModifier modifier = 0;
		if (wParam & MK_CONTROL) modifier |= PGModifierCtrl;
		if (wParam & MK_SHIFT) modifier |= PGModifierShift;
		global_handle->manager->MouseDown(x, y, PGRightMouseButton, modifier);
		break;
	}
	case WM_RBUTTONUP: {
		int x = GET_X_LPARAM(lParam);
		int y = GET_Y_LPARAM(lParam);
		PGModifier modifier = 0;
		if (wParam & MK_CONTROL) modifier |= PGModifierCtrl;
		if (wParam & MK_SHIFT) modifier |= PGModifierShift;
		global_handle->manager->MouseUp(x, y, PGRightMouseButton, modifier);
		break;
	}
	case WM_SETCURSOR:
		SetCursor(global_handle->popup ? cursor_standard : global_handle->cursor);
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
	case WM_COMMAND: {
		int index = LOWORD(wParam);
		if (global_handle->popup) {
			PGControlCallback callback = global_handle->popup->callbacks[index];
			if (callback) {
				callback(global_handle->popup->control);
			}
			delete global_handle->popup;
			global_handle->popup = nullptr;
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
	return window->manager->GetActiveControl();
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
		char* result = nullptr;
		size_t length = PGConvertText(text, &result, PGEncodingUTF8, PGEncodingUTF16Platform);
		assert(length > 0);
		HGLOBAL clipbuffer;
		char * buffer;
		EmptyClipboard();
		clipbuffer = GlobalAlloc(GMEM_DDESHARE, length + 2);
		buffer = (char*)GlobalLock(clipbuffer);
		memcpy(buffer, result, length * sizeof(char));
		buffer[length] = '\0';
		buffer[length + 1] = '\0';
		GlobalUnlock(clipbuffer);
		SetClipboardData(CF_UNICODETEXT, clipbuffer);
		CloseClipboard();
		free(result);
	}
}

size_t ucs2length(char* data) {
	int i = 0;
	while (true) {
		if (!data[i] && !data[i + 1]) {
			return i;
		}
		i += 2;
	}
}

std::string GetClipboardText(PGWindowHandle window) {
	if (OpenClipboard(window->hwnd)) {
		char* result = nullptr;

		// get the text from the clipboard
		HANDLE data = GetClipboardData(CF_UNICODETEXT);
		std::string text = std::string(((char*)data), ucs2length((char*)data));
		
		CloseClipboard();
		// on Windows we assume the text on the clipboard is encoded in UTF16: convert to UTF8
		size_t length = PGConvertText(text, &result, PGEncodingUTF16Platform, PGEncodingUTF8);
		assert(length > 0);
		text = std::string(result, length);
		free(result);
		assert(utf8_strlen(text) >= 0);
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

PGTimerHandle CreateTimer(PGWindowHandle wnd, int ms, PGTimerCallback callback, PGTimerFlags flags) {
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

PGRendererHandle GetRendererHandle(PGWindowHandle window) {
	return window->renderer;
}

PGPopupMenuHandle PGCreatePopupMenu(PGWindowHandle window, Control* control) {
	PGPopupMenuHandle handle = new PGPopupMenu();
	handle->menu = CreatePopupMenu();
	handle->window = window;
	handle->control = control;
	return handle;
}

void PGPopupMenuInsertEntry(PGPopupMenuHandle handle, std::string text, PGControlCallback callback, PGPopupMenuFlags flags) {
	int append_flags = MF_STRING;
	if (flags & PGPopupMenuChecked) append_flags |= MF_CHECKED;
	if (flags & PGPopupMenuGrayed) append_flags |= MF_GRAYED;
	int index = handle->index;
	AppendMenu(handle->menu, append_flags, index, text.c_str());
	handle->callbacks[index] = callback;
	handle->index++;
}

void PGPopupMenuInsertSeparator(PGPopupMenuHandle handle) {
	AppendMenu(handle->menu, MF_SEPARATOR, 0, NULL);
}

void PGPopupMenuInsertSubmenu(PGPopupMenuHandle handle, PGPopupMenuHandle submenu, std::string name) {
	AppendMenu(handle->menu, MF_POPUP, (UINT_PTR) submenu->menu, name.c_str());
}

void PGDisplayPopupMenu(PGPopupMenuHandle handle, PGTextAlign align) {
	POINT pt;
	GetCursorPos(&pt);
	PGDisplayPopupMenu(handle, PGPoint(pt.x, pt.y), align);
}


void PGDisplayPopupMenu(PGPopupMenuHandle handle, PGPoint point, PGTextAlign align) {
	UINT alignment = 0;
	if (align & PGTextAlignLeft) {
		alignment |= TPM_LEFTALIGN;
	} else if (align & PGTextAlignHorizontalCenter) {
		alignment |= TPM_CENTERALIGN;
	} else {
		alignment |= TPM_RIGHTALIGN;
	}
	if (align & PGTextAlignTop) {
		alignment |= TPM_TOPALIGN;
	} else if (align & PGTextAlignVerticalCenter) {
		alignment |= TPM_VCENTERALIGN;
	} else {
		alignment |= TPM_BOTTOMALIGN;
	}
	SetCursor(cursor_standard);
	handle->window->popup = handle;
	TrackPopupMenu(handle->menu, alignment, point.x, point.y, 0, handle->window->hwnd, NULL);
	DestroyMenu(handle->menu);
}

void OpenFolderInExplorer(std::string path) {
	std::string parameter = "explorer.exe /select,\"" + path + "\"";
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	if (CreateProcess(NULL, (LPSTR) parameter.c_str(), NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
		WaitForSingleObject(pi.hProcess, INFINITE);
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
	} else {
		DWORD dw = GetLastError();
		LPVOID lpMsgBuf;

		FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			dw,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR) &lpMsgBuf,
			0, NULL );

		Logger::WriteLogMessage(std::string((char*)lpMsgBuf));
	}
}

void OpenFolderInTerminal(std::string path) {
}

PGPoint ConvertWindowToScreen(PGWindowHandle window, PGPoint point) {
	POINT p;
	p.x = point.x; p.y = point.y;
	ClientToScreen(window->hwnd, &p);
	return PGPoint(p.x, p.y);
}
