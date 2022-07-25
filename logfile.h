#pragma once
#include <Windows.h>

#define LOG_PATH_CHAR_LEN 400

char szLogFilePath[LOG_PATH_CHAR_LEN];

/// General Functions
void InitLogFile(void);
void ViewLogFile(HINSTANCE, HWND);
void AdjustViewLogWindow(HWND, USHORT);

/// CALLBACKS
LRESULT CALLBACK ProcessViewLogFile(HWND, UINT, WPARAM, LPARAM);
/// END CALLBACKS