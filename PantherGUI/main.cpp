

#include "main.h"
#include "windowfunctions.h"
#include "control.h"
#include "textfield.h"

#include <malloc.h>

#include <vector>
#include <gdiplus.h>
#pragma comment (lib, "Gdiplus.lib")

using namespace Gdiplus;

struct PGWindow {
public:
	PGModifier modifier;
	HWND hwnd;
	std::vector<Control*> registered_refresh;
	Control *focused_control;

	PGWindow() : modifier(PGModifierNone) { }
};

struct PGRenderer {
	HDC hdc;
	Graphics graphics;

	PGRenderer(HDC hdc) : hdc(hdc), graphics(hdc) {	}
};

struct PGFont {
	HFONT font;
};

PGTextAlign PGTextAlignBottom = 0x01;
PGTextAlign PGTextAlignTop = 0x02;
PGTextAlign PGTextAlignLeft = 0x04;
PGTextAlign PGTextAlignRight = 0x08;
PGTextAlign PGTextAlignHorizontalCenter = 0x10;
PGTextAlign PGTextAlignVerticalCenter = 0x20;

PGModifier PGModifierNone = 0x00;
PGModifier PGModifierCmd = 0x01;
PGModifier PGModifierCtrl = 0x02;
PGModifier PGModifierShift = 0x04;
PGModifier PGModifierAlt = 0x08;
PGModifier PGModifierCtrlShift = PGModifierCtrl | PGModifierShift;
PGModifier PGModifierCtrlAlt = PGModifierCtrl | PGModifierAlt;
PGModifier PGModifierShiftAlt = PGModifierShift | PGModifierAlt;
PGModifier PGModifierCtrlShiftAlt = PGModifierCtrl | PGModifierShift | PGModifierAlt;
PGModifier PGModifierCmdCtrl = PGModifierCmd | PGModifierCtrl;
PGModifier PGModifierCmdShift = PGModifierCmd | PGModifierShift;
PGModifier PGModifierCmdAlt = PGModifierCmd | PGModifierAlt;
PGModifier PGModifierCmdCtrlShift = PGModifierCmd | PGModifierCtrl | PGModifierShift;
PGModifier PGModifierCmdShiftAlt = PGModifierCmd | PGModifierShift | PGModifierAlt;
PGModifier PGModifierCmdCtrlAlt = PGModifierCmd | PGModifierCtrl | PGModifierAlt;
PGModifier PGModifierCmdCtrlShiftAlt = PGModifierCmd | PGModifierCtrl | PGModifierShift | PGModifierAlt;

PGWindowHandle global_handle;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	static TCHAR szWindowClass[] = _T("Panther");
	static TCHAR szTitle[] = _T("Panther GUI");
	GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR           gdiplusToken;

	// Initialize GDI+.
	GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APPLICATION));
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_APPLICATION));

	if (!RegisterClassEx(&wcex))
	{
		MessageBox(NULL,
			_T("Call to RegisterClassEx failed!"),
			_T("Win32 Guided Tour"),
			NULL);

		return 1;
	}

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
		500, 500,
		NULL,
		NULL,
		hInstance,
		NULL
		);
	if (!hWnd)
	{
		MessageBox(NULL,
			_T("Call to CreateWindow failed!"),
			_T("Win32 Guided Tour"),
			NULL);

		return 1;
	}

	PGWindowHandle res = new PGWindow();
	res->hwnd = hWnd;
	global_handle = res;

	TextField* textField = new TextField(res);
	textField->width = 500;
	textField->height = 500;
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
	GdiplusShutdown(gdiplusToken);
	return (int)msg.wParam;
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
	case WM_PAINT: {
		PAINTSTRUCT ps;
		HDC hdc;
		hdc = BeginPaint(hWnd, &ps);
		PGRenderer renderer(hdc);
		PGRect rect(
			ps.rcPaint.left,
			ps.rcPaint.top,
			ps.rcPaint.bottom - ps.rcPaint.top,
			ps.rcPaint.right - ps.rcPaint.left);
		for (auto it = global_handle->registered_refresh.begin(); it != global_handle->registered_refresh.end(); it++) {
			(*it)->Draw(&renderer, &rect);
		}
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
		default:
			break;
		}
		if (button != PGButtonNone) {
			// FIXME modifier
			global_handle->focused_control->KeyboardButton(button, global_handle->modifier);
		} else if (wParam >= 0x41 && wParam <= 0x5A) {
			character = wParam;
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
		int x = LOWORD(lParam);
		int y = HIWORD(lParam);
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
		int x = LOWORD(lParam);
		int y = HIWORD(lParam);
		PGModifier modifier = 0;
		if (wParam & MK_CONTROL) modifier |= PGModifierCtrl;
		if (wParam & MK_SHIFT) modifier |= PGModifierShift;
		global_handle->focused_control->MouseClick(x, y, PGLeftMouseButton, modifier);
		break;
	}
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
		break;
	}

	return 0;
}

PGWindowHandle PGCreateWindow(void) {
	PGWindowHandle res = new PGWindow();
	if (!res) {
		return NULL;
	}
	HINSTANCE hInstance = GetModuleHandle(NULL);

	HWND hWnd = CreateWindow(
		"Panther",
		"Panther GUI",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		500, 100,
		NULL,
		NULL,
		hInstance,
		NULL
		);
	if (!hWnd) {
		return NULL;
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
	InvalidateRect(window->hwnd, NULL, true);
}

void RefreshWindow(PGWindowHandle window, PGRect rectangle) {
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

void RenderRectangle(PGRendererHandle handle, PGRect rectangle, PGColor color) {
	SolidBrush solidBrush(Color(color.a, color.r, color.g, color.b));
	handle->graphics.FillRectangle(&solidBrush, rectangle.x, rectangle.y, rectangle.width, rectangle.height);
}

void RenderLine(PGRendererHandle handle, PGLine line, PGColor color) {
	Pen pen(Color(color.a, color.r, color.g, color.b));
	handle->graphics.DrawLine(&pen, line.start.x, line.start.y, line.end.x, line.end.y);

}

void RenderImage(PGRendererHandle renderer, void* image, int x, int y) {

}

PGSize RenderText(PGRendererHandle renderer, const char *text, size_t len, int x, int y) {
	if (len == 0) {
		len = 1;
		text = " ";
	}

	LONG return_value = TabbedTextOut(renderer->hdc, x, y, text, len, 0, NULL, 0);
	return PGSize{ LOWORD(return_value), HIWORD(return_value) };
}

void RenderCaret(PGRendererHandle renderer, const char *text, size_t len, int x, int y, ssize_t characternr) {
	ssize_t line_height = 19;
	int width = GetRenderWidth(renderer, text, characternr);
	RenderLine(renderer, PGLine(x + width, y, x + width, y + line_height), PGColor(0, 0, 0));
}

void RenderSelection(PGRendererHandle renderer, const char *text, size_t len, int x, int y, ssize_t start, ssize_t end) {
	if (start == end) return;
	ssize_t line_height = 19;
	int selection_start = GetRenderWidth(renderer, text, start);
	int selection_width = GetRenderWidth(renderer, text, end > len ? len : end);
	if (end > len) {
		assert(end == len + 1);
		selection_width += GetRenderWidth(renderer, " ", 1);
	}
	RenderRectangle(renderer, PGRect(x + selection_start, y, selection_width - selection_start, line_height), PGColor(20, 20, 180, 125));
}

void GetRenderOffsets(PGRendererHandle renderer, const char* text, ssize_t length, std::vector<short>& offsets) {
	offsets.clear();
	for (int i = 0; i < length; i++) {	
		int result;
		UINT val = text[i];
		GetCharWidth(renderer->hdc, val, val, &result);
		offsets.push_back((short)result);
	}
}

int GetRenderWidth(PGRendererHandle renderer, const char* text, ssize_t length) {
	int width = 0;
	for (int i = 0; i < length; i++) {	
		int result;
		UINT val = text[i];
		GetCharWidth(renderer->hdc, val, val, &result);
		width += result;
	}
	return width;
}

int GetCharacterPosition(PGWindowHandle window, const char* text, ssize_t length, ssize_t x_position) {
	HDC hdc = GetDC(window->hwnd);
	int width = 0;
	int position = 0;
	for (int i = 0; i < length; i++) {	
		int result;
		UINT val = text[i];
		GetCharWidth(hdc, val, val, &result);
		if (width < x_position && width + result > x_position) {
			goto end;
		}
		width += result;
		position++;
	}
end:
	ReleaseDC(window->hwnd, hdc);
	return position;
}

void SetTextColor(PGRendererHandle renderer, PGColor color) {
	SetTextColor(renderer->hdc, RGB(color.r, color.g, color.b));
}

void SetTextFont(PGRendererHandle renderer, PGFontHandle font) {
	HFONT mfont = CreateFont(0, 0, 0, 0, FW_NORMAL, false, false, false, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FF_DONTCARE, "Consolas");
	SelectObject(renderer->hdc, mfont);
}

void SetTextAlign(PGRendererHandle renderer, PGTextAlign alignment) {
	UINT fMode = 0;
	if (alignment & PGTextAlignBottom) {
		fMode |= TA_BOTTOM;
	}
	else if (alignment & PGTextAlignTop) {
		fMode |= TA_TOP;
	}
	else if (alignment & PGTextAlignVerticalCenter) {
		fMode |= TA_BASELINE;
	}

	if (alignment & PGTextAlignLeft) {
		fMode |= TA_LEFT;
	}
	else if (alignment & PGTextAlignRight) {
		fMode |= TA_RIGHT;
	}
	else if (alignment & PGTextAlignHorizontalCenter) {
		fMode |= TA_CENTER;
	}

	SetTextAlign(renderer->hdc, fMode);
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

