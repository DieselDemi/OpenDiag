/// PTDIAG Complete reverse engineer
/// DieselDemi
/// 2022

#include "targetver.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <stdio.h>

// Resource file haeder
#include "resource.h"

#include "logfile.h"
#include "globals.h"
#include "main_dialog.h"

int APIENTRY WinMain(_In_ HINSTANCE hInstance,
            _In_opt_ HINSTANCE hPrevInstance,
            _In_ LPSTR lpCmdLine,
            _In_ int nCmdShow
) {

    //TODO Move this to the init instance method when its created
    InitLogFile();


    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_PATH);
    LoadStringW(hInstance, IDC_PTDIAG, szWindowClass, MAX_PATH);
    RegisterMainWindow(hInstance);

    if(!InitInstance(hInstance, nCmdShow)) {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_PTDIAG));

    MSG msg;

    while(GetMessage(&msg, NULL, 0, 0)) {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int)msg.wParam;
}

