#pragma once
#include <Windows.h>


//// TODO Move these to the J2534 connection
void VerifyMcS1Build();
USHORT BoxConnect();
PUCHAR PingModule(ULONG);
////

///  TODO Move to appropriate file
void DisplayConfigItems();
void InitialTextPopAdjust1();
void InitialTextPopAdjust2();
void InitialTextPopAdjust3();
void GenericLogAdjust(int);
void AdjustUsageNotesWindow(int);

void InitialWindowAdjust(HWND);
void UpdateMainStatusLine(PUCHAR, PUCHAR, HWND);
void ExitProgram(int);

BOOL InitInstance(HINSTANCE, int);
ATOM RegisterMainWindow(HINSTANCE hInstance);

//// CALLBACKS
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK ProcessMainDialog(HWND, UINT, WPARAM, LPARAM);