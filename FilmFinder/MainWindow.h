#pragma once

#include <windows.h>
#include <string>

// Объявления функций
ATOM RegisterMainWindowClass(HINSTANCE hInstance);
LRESULT CALLBACK MainWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
void CreateMainWindow(HINSTANCE hInstance, const std::wstring& username);