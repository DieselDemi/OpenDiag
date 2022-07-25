#include <stdio.h>
#include "logfile.h"
#include "resource.h"
#include "utils.h"
#include "globals.h"


void ViewLogFile(HINSTANCE hInstance, HWND hOwnerWindow) {
    DialogBoxParamA(hInstance, MAKEINTRESOURCE(IDD_VIEWLOG_FILE), (HWND) hOwnerWindow, ProcessViewLogFile, 0);
}

LRESULT CALLBACK ProcessViewLogFile(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    HWND hListBox;
    HWND hButton;

    HFONT listBoxFont;

    FILE *ptrLogFile;

    ULONG ulRecordCounter = 0;
    UCHAR bRecord[1001];


    switch (message) {
        case WM_INITDIALOG: //0x110
        {
            AdjustViewLogWindow(hDlg, 0);
            hListBox = GetDlgItem(hDlg, 1001);
            hButton = GetDlgItem(hDlg, 1009);

            //Set the font for the list box
            if ((listBoxFont = CreateFontA(0xc, 0, 0, 0, 400, 0, 0, 0, 0, 0, 0, 0, 0x31, "Courier")) != NULL) {
                SendMessageA(hListBox, WM_SETFONT, (WPARAM) listBoxFont, 1);
            }

            SendMessageA(hListBox, LB_SETHORIZONTALEXTENT, 2000, 0);
            SendMessageA(hListBox, LB_RESETCONTENT, 0, 0);

            if (!(ptrLogFile = fopen((const char *) szLogFilePath, "a+"))) {
                MessageBoxA(hDlg, "Error opening log file.", MAKEINTRESOURCE(IDC_PTDIAG), MB_ICONEXCLAMATION);
                EndDialog(hDlg, (INT_PTR) wParam);
                return TRUE;
            }

            //Count the records in the log file
            while (fgets((char *) bRecord, 1000, (FILE *) ptrLogFile) != NULL) {
                ulRecordCounter++;
            }
            rewind((FILE *) ptrLogFile);

            //Check if the log file has a record count greater than 32000
            // if it does, display a warning that its very large.
            if (32000 < ulRecordCounter) {
                MessageBoxA((HWND) hDlg, "(LPCSTR)ProcessViewLogFile::szLargeFileWarning", MAKEINTRESOURCE(IDC_PTDIAG),
                            MB_ICONEXCLAMATION);
            } else if (5000 < ulRecordCounter) {
                DisplayTextPopup2(hDlg, (PUCHAR) "View Log", (PUCHAR) "Loading...");
            }

            while (fgets((char *) bRecord, 1000, (FILE *) ptrLogFile)) {
                bRecord[TermIndex(bRecord)] = '\0';
                SendMessageA(hListBox, LB_ADDSTRING, 0, (LPARAM) bRecord);
                UpdateWindow(hListBox);
            }

            //TODO Some odd data here (LPARAM)&DAT_00a72402
            /**
             * TODO
             * This call to SendMessage adds a string to the ListBox control, however during de-compilation is points to
             * a memory address for a data to add, though I'm not sure what it's supposed to have in there.
             */
//            SendMessageA(hListBox, LB_ADDSTRING, 0, 0);
            UpdateWindow(hListBox);

            if (5000 < ulRecordCounter) {
                HideTextPopup2();
            }

            fclose((FILE *) ptrLogFile);

            //TODO This is curious
            if (usP558ECMSwapPlantMode == 0) {
                return FALSE;
            }

            EnableWindow(hButton, 0);
            UpdateWindow(hButton);

            return TRUE;
        }

        case WM_COMMAND: {
            //Windows x control
            if(LOWORD(wParam) == IDCANCEL) {
                EndDialog(hDlg, (INT_PTR)wParam);
                return TRUE;
            }

            //Close the dialog box
            if (LOWORD(wParam) != 1009) { //Clear log file button ID
                if(LOWORD(wParam) != 1047) { //Close button ID
                    return FALSE;
                }
                EndDialog(hDlg, (INT_PTR) wParam);
                return TRUE;
            }

            hListBox = GetDlgItem((HWND) hDlg,1001 /*TODO These should really be referenced in the resource header as a define*/);


            //TODO Move this string into the resource file
            const char queryClearLog[] = {0x41, 0x72, 0x65, 0x20, 0x79, 0x6f, 0x75, 0x20, 0x73, 0x75, 0x72, 0x65, 0x20,
                                          0x79, 0x6f, 0x75, 0x20, 0x77, 0x61,
                                          0x6e, 0x74, 0x20, 0x74, 0x6f, 0x20, 0x63, 0x6c, 0x65, 0x61, 0x72, 0x20, 0x74,
                                          0x68, 0x65, 0x20, 0x6c, 0x6f, 0x67,
                                          0x20, 0x66, 0x69, 0x6c, 0x65, 0x3f, 0x20, 0x20, 0x20, 0x20, 0x0a, 0x00};

            //TODO Move the lpCaption param to resource file
            if (MessageBoxA((HWND) hDlg, queryClearLog, "TODO GIVE ME RESOURCE",MB_DEFBUTTON2 | MB_ICONQUESTION | MB_YESNO) != IDYES) {
                return FALSE;
            }

            if ((ptrLogFile = fopen(szLogFilePath, "w+")) != NULL) {
                SendMessageA(hListBox, LB_RESETCONTENT, 0, 0);
                UpdateWindow(hListBox);
                MessageBoxA((HWND) hDlg, "Log file cleared.  ", "TODO GIVE ME RESOURCE", MB_ICONINFORMATION);
                fclose(ptrLogFile);
                return FALSE;
            }
            MessageBoxA((HWND) hDlg, "Error clearing log file.  ", "TODO GIVE ME RESOURCE", MB_ICONEXCLAMATION);
            return FALSE;

        }
        case WM_SIZING: {
            AdjustViewLogWindow(hDlg, 1);
        }

        default:
            return FALSE;
    }
}

//Well didn't expect that not to crash
//Just code cleanup I guess
void AdjustViewLogWindow(HWND hOwnerWindow, USHORT usAction) {
    HWND hListBoxControl;
    HWND hClearLogButtonControl;
    HWND hCloseButtonControl;

    int pxHeightOfTheTitleBar;
    int pxWidthOfClientFullscreenWindow;
    int pxHeightOfClientFullScreenWindow;

    RECT rDialogSize;

    int wListWidth;

    pxHeightOfTheTitleBar = GetSystemMetrics(SM_CYSIZE);
    pxWidthOfClientFullscreenWindow = GetSystemMetrics(SM_CXFULLSCREEN);
    pxHeightOfClientFullScreenWindow = GetSystemMetrics(SM_CYFULLSCREEN);
    hListBoxControl = GetDlgItem((HWND) hOwnerWindow, 1001);
    hClearLogButtonControl = GetDlgItem((HWND) hOwnerWindow, 1009);
    hCloseButtonControl = GetDlgItem((HWND) hOwnerWindow, 1047);

    if (usAction == 0) {
        //Not sure what this does exactly here
        if (pxWidthOfClientFullscreenWindow < 781) {
            wListWidth = 620;
        } else {
            wListWidth = 760;
        }

        SetWindowPos((HWND) hOwnerWindow, (HWND) 0x0, (pxWidthOfClientFullscreenWindow - (wListWidth + 0x1a)) / 2,
                     (pxHeightOfClientFullScreenWindow - (pxHeightOfTheTitleBar + 0xce)) / 3, wListWidth + 0x1a, pxHeightOfTheTitleBar + 0xce, 4);
        SetWindowPos(hListBoxControl, (HWND) 0x0, 8, 8, wListWidth, 0x96, 4);
        SetWindowPos(hCloseButtonControl, (HWND) 0x0, 8, 0xa6, 0x50, 0x16, 4);
        SetWindowPos(hClearLogButtonControl, (HWND) 0x0, 0x60, 0xa6, 100, 0x16, 4);
    }
    if (usAction == 1) {
        GetWindowRect((HWND) hOwnerWindow, (LPRECT) &rDialogSize);
        pxHeightOfTheTitleBar = (rDialogSize.bottom - rDialogSize.top) - pxHeightOfTheTitleBar;
        SetWindowPos(hListBoxControl, (HWND) 0x0, 0, 0, (rDialogSize.right - rDialogSize.left) + -0x1a, pxHeightOfTheTitleBar + -0x38, 6);
        SetWindowPos(hCloseButtonControl, (HWND) 0x0, 8, pxHeightOfTheTitleBar + -0x28, 0x50, 0x16, 4);
        SetWindowPos(hClearLogButtonControl, (HWND) 0x0, 0x60, pxHeightOfTheTitleBar + -0x28, 100, 0x16, 4);
    }
}

void InitLogFile(void) {
    //Get the current working directory
    if (GetCurrentDirectoryA(LOG_PATH_CHAR_LEN, szLogFilePath) != 0)
    {
        _snprintf(szLogFilePath, LOG_PATH_CHAR_LEN, "%s\\%s", szLogFilePath, "PTDIAG.LOG");
    }
    else
    {
        if (usP558ECMSwapPlantMode == 0) {
            _snprintf(szLogFilePath, LOG_PATH_CHAR_LEN, "%s\\%s", "./", "PTDIAG.LOG");
        } else {
            _snprintf(szLogFilePath, LOG_PATH_CHAR_LEN, "%s\\%s", "./", "P558.LOG");
        }
    }
}
