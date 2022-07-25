#include "utils.h"
#include "globals.h"

void DisplayTextPopup2(HWND hSizeWindow, PUCHAR pszHeaderText, PUCHAR pszDisplayText)
{
    HWND hWnd;
    int iVar1;
    int iVar2;
    int cx;
    int cy;
    RECT rTextPopup2;
    RECT rOwnerWindow;
    int wWindowTop;
    int wWindowLeft;
    int wOffsetY;
    int wOffsetX;
    int wWindowHeight;
    int wWindowWidth;
    int wOwnerHeight;
    int wOwnerWidth;
    HWND hTextBox;

    hWnd = GetDlgItem((HWND)hWndTextPop2, 0x3ed);
    GetWindowRect((HWND)hSizeWindow, (LPRECT)&rOwnerWindow);
    iVar1 = rOwnerWindow.right - rOwnerWindow.left;
    iVar2 = rOwnerWindow.bottom - rOwnerWindow.top;
    GetWindowRect((HWND)hWndTextPop2, (LPRECT)&rTextPopup2);
    cx = rTextPopup2.right - rTextPopup2.left;
    cy = rTextPopup2.bottom - rTextPopup2.top;
    if ((cx < iVar1) && (cy < iVar2))
    {
        SetWindowPos((HWND)hWndTextPop2, (HWND)0x0, rOwnerWindow.left + (iVar1 - cx) / 2,
                         rOwnerWindow.top + (iVar2 - cy) / 2, cx, cy, 5);
    }
    SetWindowTextA((HWND)hWndTextPop2, (LPCSTR)pszHeaderText);
    SetWindowTextA(hWnd, (LPCSTR)pszDisplayText);
    UpdateWindow(hWnd);
    if (usDisplayStandardPopups != 0)
    {
        ShowWindow((HWND)hWndTextPop2, 5);
        UpdateWindow((HWND)hWndTextPop2);
    }
}

void HideTextPopup2(void)
{
    ShowWindow((HWND)hWndTextPop2, 0);
    UpdateWindow((HWND)hWndTextPop2);
}

USHORT TermIndex(PUCHAR pszString)
{
    unsigned char uVar1;
    unsigned short uVar2;
    int iVar3;
    PUCHAR puVar4;
    USHORT usCheckIndex;
    USHORT usStringLength;
    USHORT usTermIndex;

    iVar3 = -1;
    puVar4 = pszString;
    do
    {
        if (iVar3 == 0)
            break;
        iVar3 = iVar3 + -1;
        uVar1 = *puVar4;
        puVar4 = puVar4 + 1;
    } while (uVar1 != '\0');
    usTermIndex = ~(unsigned short)iVar3 - 1;
    if ((usTermIndex != 0) && (uVar2 = ~(unsigned short)iVar3 - 2, pszString[uVar2] == '\n'))
    {
        usTermIndex = uVar2;
    }
    return usTermIndex;
}

void SwapDataByte(UCHAR bByte1, USHORT bByte2) {

}
