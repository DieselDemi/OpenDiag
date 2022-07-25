#include "dummy_callbacks.h"

INT_PTR CALLBACK DummyCallback(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    switch(message) {
        case WM_INITDIALOG:
            return (INT_PTR) TRUE;

        case WM_COMMAND:
        {
            if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
            {
                EndDialog(hDlg, LOWORD(wParam));
                return (INT_PTR)TRUE;
            }
        }
            break;

        default:
            return (INT_PTR)FALSE;
    }
    return (INT_PTR)FALSE;
}

INT_PTR CALLBACK ProcessFlashTool(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) { return DummyCallback(hDlg, message, wParam, lParam); }
INT_PTR CALLBACK ProcessVLSMonitor(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) { return DummyCallback(hDlg, message, wParam, lParam); }
INT_PTR CALLBACK ProcessDieselFunctions(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) { return DummyCallback(hDlg, message, wParam, lParam); }
INT_PTR CALLBACK ProcessVBFFileData(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) { return DummyCallback(hDlg, message, wParam, lParam); }
INT_PTR CALLBACK ProcessSpecialConfig(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) { return DummyCallback(hDlg, message, wParam, lParam); }
INT_PTR CALLBACK ProcessPTimers(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) { return DummyCallback(hDlg, message, wParam, lParam); }
INT_PTR CALLBACK ProcessHexFileData(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) { return DummyCallback(hDlg, message, wParam, lParam); }
INT_PTR CALLBACK ProcessFaultInjection(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) { return DummyCallback(hDlg, message, wParam, lParam); }
INT_PTR CALLBACK ProcessSaveRestoreKAMTool(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) { return DummyCallback(hDlg, message, wParam, lParam); }
INT_PTR CALLBACK ProcessEngineeringPanel(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) { return DummyCallback(hDlg, message, wParam, lParam); }
INT_PTR CALLBACK ProcessDataLogging(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) { return DummyCallback(hDlg, message, wParam, lParam); }
INT_PTR CALLBACK ProcessBinaryData(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) { return DummyCallback(hDlg, message, wParam, lParam); }
INT_PTR CALLBACK ProcessPIDDMRMonitor(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) { return DummyCallback(hDlg, message, wParam, lParam); }
INT_PTR CALLBACK ProcessDTCMonitor(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) { return DummyCallback(hDlg, message, wParam, lParam); }
INT_PTR CALLBACK ProcessViewInitFileOptions(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) { return DummyCallback(hDlg, message, wParam, lParam); }
INT_PTR CALLBACK ProcessViewSpecialStats(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) { return DummyCallback(hDlg, message, wParam, lParam); }
INT_PTR CALLBACK ProcessStressTests(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) { return DummyCallback(hDlg, message, wParam, lParam); }
INT_PTR CALLBACK ProcessIPConfig(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) { return DummyCallback(hDlg, message, wParam, lParam); }
INT_PTR CALLBACK ProcessViewPCConfig(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) { return DummyCallback(hDlg, message, wParam, lParam); }
INT_PTR CALLBACK ProcessFuelMultiplier(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) { return DummyCallback(hDlg, message, wParam, lParam); }
INT_PTR CALLBACK ProcessOBDIIMonitor(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) { return DummyCallback(hDlg, message, wParam, lParam); }
INT_PTR CALLBACK ProcessPatsFunctions(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) { return DummyCallback(hDlg, message, wParam, lParam); }
INT_PTR CALLBACK ProcessCANMonitor(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) { return DummyCallback(hDlg, message, wParam, lParam); }
INT_PTR CALLBACK ProcessActiveCommands(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) { return DummyCallback(hDlg, message, wParam, lParam); }
INT_PTR CALLBACK ProcessBlockReadWrite(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) { return DummyCallback(hDlg, message, wParam, lParam); }
INT_PTR CALLBACK ProcessSpecialTests(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) { return DummyCallback(hDlg, message, wParam, lParam); }
INT_PTR CALLBACK ProcessRAMDownload(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) { return DummyCallback(hDlg, message, wParam, lParam); }
INT_PTR CALLBACK ProcessDisplayMemorySegment(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) { return DummyCallback(hDlg, message, wParam, lParam); }
INT_PTR CALLBACK ProcessOutputTestMode(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) { return DummyCallback(hDlg, message, wParam, lParam); }
INT_PTR CALLBACK ProcessGenericFlash(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) { return DummyCallback(hDlg, message, wParam, lParam); }
INT_PTR CALLBACK ProcessPeriodicMsg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) { return DummyCallback(hDlg, message, wParam, lParam); }
INT_PTR CALLBACK ProcessBrakeFunctions(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) { return DummyCallback(hDlg, message, wParam, lParam); }
INT_PTR CALLBACK ProcessReadTranslatedTable(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) { return DummyCallback(hDlg, message, wParam, lParam); }
INT_PTR CALLBACK ProcessDialogBox1(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) { return DummyCallback(hDlg, message, wParam, lParam); }
INT_PTR CALLBACK ProcessTextPopup1(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) { return DummyCallback(hDlg, message, wParam, lParam); }
INT_PTR CALLBACK ProcessTextPopup2(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) { return DummyCallback(hDlg, message, wParam, lParam); }
INT_PTR CALLBACK ProcessTextPopup3(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) { return DummyCallback(hDlg, message, wParam, lParam); }
INT_PTR CALLBACK ProcessGenLogWindow(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) { return DummyCallback(hDlg, message, wParam, lParam); }
INT_PTR CALLBACK ProcessUsageNotes(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) { return DummyCallback(hDlg, message, wParam, lParam); }