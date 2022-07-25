#pragma once
#include <Windows.h>

//TODO Add the about box here

//// CALLBACKS
INT_PTR CALLBACK About(HWND, UINT, WPARAM, LPARAM);

void ViewAbout(HINSTANCE, HWND);
void AdjustAboutWindow(HWND, USHORT);