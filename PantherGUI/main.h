#pragma once

#include "windowfunctions.h"

#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>

int WINAPI WinMain(HINSTANCE hInstance,	HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

PGWindowHandle GetHWNDHandle(HWND hwnd);
void SetHWNDHandle(HWND hwnd, PGWindowHandle);
size_t UCS2Length(char* data);
std::string UCS2toUTF8(PWSTR data);
std::string UTF8toUCS2(std::string text);