

#include <stdio.h>
#include "main_dialog.h"
#include "globals.h"
#include "resource.h"

//Dialog headers
#include "logfile.h"
#include "about.h"
#include "utils.h"
#include "init_file.h"
#include "configuration.h"

//TODO
#include "dummy_callbacks.h"

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow) {
    hInst = hInstance; // Store instance handle in our global variable

    hWndMain = CreateWindowW(szWindowClass,
                             szTitle,
                             WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
                             CW_USEDEFAULT,
                             0,
                             505,
                             298,
                             NULL,
                             NULL,
                             hInstance,
                             NULL);

    if (!hWndMain) {
        return FALSE;
    }

    ShowWindow(hWndMain, nCmdShow);
    UpdateWindow(hWndMain);
//// This is to set up text
    if (usBuildVersion == 2) {
////        pUVar10 = (UCHAR *) "Version ";
////        pUVar13 = szPtdiagVersion;
////        for (iVar7 = 9; iVar7 != 0; iVar7 = iVar7 + -1) {
////            *pUVar13 = *pUVar10;
////            pUVar10 = pUVar10 + 1;
////            pUVar13 = pUVar13 + 1;
////        }
////        uVar8 = 0xffffffff;
////        pUVar10 = szPtdiagVersion;
////        do {
////            if (uVar8 == 0) break;
////            uVar8 = uVar8 - 1;
////            UVar1 = *pUVar10;
////            pUVar10 = pUVar10 + 1;
////        } while (UVar1 != '\0');
//////            *(undefined4 * )(~uVar8 + 0xac4c3f) = 0x2e316d;
////        uVar8 = 0xffffffff;
////        pUVar10 = szPtdiagVersion;
////        do {
////            if (uVar8 == 0) break;
////            uVar8 = uVar8 - 1;
////            UVar1 = *pUVar10;
////            pUVar10 = pUVar10 + 1;
////        } while (UVar1 != '\0');
//////            *(undefined4 * )(~uVar8 + 0xac4c3f) = 0x302e3232;
//////            *(undefined2 * )(szPtdiagVersion + ~uVar8 + 3) = 0x30;
//////            szPtdiagVersionShort._0_4_ = 0x2e316d;
////        uVar8 = 0xffffffff;
//////            pUVar10 = szPtdiagVersionShort;
////        do {
////            if (uVar8 == 0) break;
////            uVar8 = uVar8 - 1;
////            UVar1 = *pUVar10;
////            pUVar10 = pUVar10 + 1;
////        } while (UVar1 != '\0');
//////            *(undefined4 * )(~uVar8 + 0xac4a1f) = 0x302e3232;
//////            *(undefined2 * )(szPtdiagVersionShort + ~uVar8 + 3) = 0x30;
//////            szPassThruToolType._0_4_ = &DAT_3153634d;
//////            szPassThruToolType[4] = '\0';
//////            usConnectionType = 2;
    } else {
////        pUVar10 = (UCHAR *) "Version ";
////        pUVar13 = szPtdiagVersion;
////        for (iVar7 = 9; iVar7 != 0; iVar7 = iVar7 + -1) {
////            *pUVar13 = *pUVar10;
////            pUVar10 = pUVar10 + 1;
////            pUVar13 = pUVar13 + 1;
////        }
////        uVar8 = 0xffffffff;
////        pUVar10 = szPtdiagVersion;
////        do {
////            if (uVar8 == 0) break;
////            uVar8 = uVar8 - 1;
////            UVar1 = *pUVar10;
////            pUVar10 = pUVar10 + 1;
////        } while (UVar1 != '\0');
//////            *(undefined4 * )(~uVar8 + 0xac4c3f) = 0x2e3163;
////        uVar8 = 0xffffffff;
////        pUVar10 = szPtdiagVersion;
////        do {
////            if (uVar8 == 0) break;
////            uVar8 = uVar8 - 1;
////            UVar1 = *pUVar10;
////            pUVar10 = pUVar10 + 1;
////        } while (UVar1 != '\0');
//////            *(undefined4 * )(~uVar8 + 0xac4c3f) = 0x302e3232;
//////            *(undefined2 * )(szPtdiagVersion + ~uVar8 + 3) = 0x30;
//////            szPtdiagVersionShort._0_4_ = 0x2e3163;
////        uVar8 = 0xffffffff;
////        pUVar10 = szPtdiagVersionShort;
////
////        do {
////            if (uVar8 == 0) break;
////            uVar8 = uVar8 - 1;
////            UVar1 = *pUVar10;
////            pUVar10 = pUVar10 + 1;
////        } while (UVar1 != '\0');
//////            *(undefined4 * )(~uVar8 + 0xac4a1f) = 0x302e3232;
//////            *(undefined2 * )(szPtdiagVersionShort + ~uVar8 + 3) = 0x30;
//////            pUVar10 = (UCHAR *) "CarDAQ";
//////            pUVar13 = szPassThruToolType;
////        for (iVar7 = 7; iVar7 != 0; iVar7 = iVar7 + -1) {
////            *pUVar13 = *pUVar10;
////            pUVar10 = pUVar10 + 1;
////            pUVar13 = pUVar13 + 1;
////        }
//////            usConnectionType = 0;
    }

    BOOL bIsWIndow;
    HWND hWndCurrentFocus;
    HMENU hMenuItem;


    if ((!IsWindow(hWndDialog1)) &&
        (hWndDialog1 = (HWND) CreateDialogParamA((HINSTANCE) hInst, MAKEINTRESOURCE(IDD_PTDIAG), (HWND) hWndMain, ProcessMainDialog, 0), hWndDialog1 == (HWND) 0x0)) {
        MessageBoxA(GetFocus(), "Error creating PTDIAG dialog box 1.    ", (LPCSTR) szTitle, 0x10);
        PostMessageA((HWND) hWndMain, 0x10, 0, 0);
    }

    bIsWIndow = IsWindow((HWND) hWndTextPop1);
    if ((bIsWIndow == 0) &&
        (hWndTextPop1 = (HWND) CreateDialogParamA
                ((HINSTANCE) hInst, MAKEINTRESOURCE(IDD_PTDIAG_TEXT_POPUP_1), (HWND) hWndMain,
                 ProcessTextPopup1, 0), hWndTextPop1 == (HWND) 0x0)) {
        hWndCurrentFocus = GetFocus();
        MessageBoxA(hWndCurrentFocus, "Error creating PTDIAG text popup box 1.    ", (LPCSTR) szTitle, 0x10);
        PostMessageA((HWND) hWndMain, 0x10, 0, 0);
    }

    bIsWIndow = IsWindow((HWND) hWndTextPop2);
    if ((bIsWIndow == 0) &&
        (hWndTextPop2 = (HWND) CreateDialogParamA
                ((HINSTANCE) hInst, MAKEINTRESOURCE(IDD_PTDIAG_TEXT_POPUP_2), (HWND) hWndMain,
                 ProcessTextPopup2, 0), hWndTextPop2 == (HWND) 0x0)) {
        hWndCurrentFocus = GetFocus();
        MessageBoxA(hWndCurrentFocus, "Error creating PTDIAG text popup box 2.    ", (LPCSTR) szTitle, 0x10);
        PostMessageA((HWND) hWndMain, 0x10, 0, 0);
    }

    bIsWIndow = IsWindow((HWND) hWndTextPop3);
    if ((bIsWIndow == 0) &&
        (hWndTextPop3 = (HWND) CreateDialogParamA
                ((HINSTANCE) hInst, MAKEINTRESOURCE(IDD_PTDIAG_TEXT_POPUP_3), (HWND) hWndMain,
                 ProcessTextPopup3, 0), hWndTextPop3 == (HWND) 0x0)) {
        hWndCurrentFocus = GetFocus();
        MessageBoxA(hWndCurrentFocus, "Error creating PTDIAG text popup box 3.    ", (LPCSTR) szTitle, 0x10);
        PostMessageA((HWND) hWndMain, 0x10, 0, 0);
    }
    bIsWIndow = IsWindow((HWND) hWndGenLog);
    //TODO General Log window error
    if ((bIsWIndow == 0) &&
        (hWndGenLog = (HWND) CreateDialogParamA
                ((HINSTANCE) hInst, MAKEINTRESOURCE(IDD_PTDIAG_GENERAL_LOG), (HWND) hWndMain,
                 ProcessGenLogWindow, 0), hWndGenLog == (HWND) 0x0)) {
        hWndCurrentFocus = GetFocus();
        MessageBoxA(hWndCurrentFocus, "Error creating PTDIAG generic log window.    ", (LPCSTR) szTitle, 0x10);
        //PostMessageA((HWND) hWndMain, 0x10, 0, 0);
    }
    bIsWIndow = IsWindow((HWND) hWndUsageWin);
    if ((bIsWIndow == 0) &&
        (hWndUsageWin = (HWND) CreateDialogParamA
                ((HINSTANCE) hInst, MAKEINTRESOURCE(IDD_PTDIAG_USAGE_NOTES), (HWND) hWndMain,
                 ProcessUsageNotes, 0), hWndUsageWin == (HWND) 0x0)) {
        hWndCurrentFocus = GetFocus();
        MessageBoxA(hWndCurrentFocus, "Error creating PTDIAG usage notes window.    ", (LPCSTR) szTitle, 0x10);
        PostMessageA((HWND) hWndMain, 0x10, 0, 0);
    }
    if (wWindowAdjustFlag == 0) {
        InitialWindowAdjust(hWndMain);
        InitialTextPopAdjust1();
        InitialTextPopAdjust2();
        InitialTextPopAdjust3();
        GenericLogAdjust(0);
        AdjustUsageNotesWindow(0);
        wWindowAdjustFlag = 1;
    }
    ShowWindow((HWND) hWndMain, nCmdShow);
    UpdateWindow((HWND) hWndMain);
    ShowWindow((HWND) hWndDialog1, 5);
    UpdateWindow((HWND) hWndDialog1);
    if (
            (
                    (((((((((usGlobalTestMode == 0) && (usLogOtherError == 0)) && (usProgThreadTestMode == 0)) &&
                          ((usUploadThreadTestMode == 0 && (usMonitorTestMode == 0)))) && (usDTCTestMode == 0)) &&
                        ((usSpecialTestMode1 == 0 && (usManualEPATSInstall == 0)))) && ((usETCTestMode == 0 &&
                                                                                         (((usFLITestMode == 0 &&
                                                                                            (usFaultInjTestMode ==
                                                                                             0)) &&
                                                                                           (usDataAcqPowerCycle ==
                                                                                            0))))))
                      && ((((usForceNoVIDSave == 0 && (usDGB1SimulationMode == 0)) && (usDGB1SimulationVLS == 0)
                           ) && ((usLogPokeTableTestData == 0 && (usEraseRegionOverride == 0)))))) &&
                     (((usVBFToolTestMode == 0 &&
                        (((usCCPTestMode == 0 && (usCCPSimulationMode == 0)) && (usTridUpdateTestMode == 0)))) &&
                       (((usHEVFuelTestMode == 0 && (usCD33xClusterEmulation == 0)) &&
                         ((usFuelEconEmulation == 0 &&
                           (((usAdaptiveCruiseEmulation == 0 && (usFuelEconOBDIITestMode == 0)) &&
                             ((usFuelEconOBDIISimulation == 0 &&
                               ((((usMyCANICStressTest1 == 0 && (usBypassCRCMismatchErrors == 0)) &&
                                  (usAllowFTCFunctionsWindow == 0)) &&
                                 ((usMEMReadTestMode == 0 && (usEONVTestMode == 0)))))))))))))))) &&
                    ((usTimeStampDebugMode == 0 &&
                      ((usShowCGEA_PATS_Msgs_Only == 0 && (usKOEO_6R_Plant_Test == 0)))))) &&
            (((((((usShow11and29bitCANData == 0 &&
                   (((usProgStressTestMode == 0 && (usSnakeTestMode == 0)) && (usMAFSensorTesting == 0))))
                  && (((usPCVTestSwitch == 0 && (usAutoOSCEnableTestMode == 0)) &&
                       (usCRCTestModeSwitch == 0)))) &&
                 (((usFileBrowserTestMode == 0 && (us10msChronoTestMode == 0)) &&
                   ((usCLSDataAcquisitionEnable == 0 &&
                     (((usCLSDataAcquisitionEnable2 == 0 && (usProCalUploadTestMode == 0)) &&
                       (usFuelPrimeTestMode == 0)))))))) &&
                ((usCodedAccess11TestMode == 0 && (usSBLPowerCycleTestMode == 0)))) &&
               ((((usDieselOSCCustomOverride == 0 &&
                   (((usVCTTestMode == 0 && (usSBLDownloadTestMode == 0)) &&
                     ((usP558ECMSwapPlantMode == 0 &&
                       ((((usTimeStampTestMode == 0 && (usSession40TestMode == 0)) &&
                          (usSecureProgrammingMode == 0)) &&
                         ((usDownloadTestLoop == 0 && (usSocketTestMode == 0)))))))))) &&
                  (usConvertApplinkEnable == 0)) &&
                 (((usConvertSmogCheckData == 0 && (usTransAdaptTestMode == 0)) &&
                   ((((usMode06TestMode == 0 &&
                       (((usProgrammingLoopMode == 0 && (usUreaSensorTestMode == 0)) &&
                         (usSNLockTestMode == 0)))) &&
                      (((usDRDConversionTestMode == 0 && (usAutoOBDIIReadTestMode == 0)) &&
                        (usAutoBootCodeUpdate == 0)))) &&
                     ((usAlternatorRampTestMode == 0 && (usOverrideDefaultSettings == 0)))))))))) &&
              (usShowKeyStrokes == 0)))) {
        if (usThirdInstance == 0) {
            if (usSecondInstance == 0) {
                SetWindowTextA((HWND) hWndMain, "Pass Thru Diagnostics");
            } else {
                SetWindowTextA((HWND) hWndMain, "Pass Thru Diagnostics (#2)");
            }
        } else {
            SetWindowTextA((HWND) hWndMain, "Pass Thru Diagnostics (#3)");
        }
    }
//    else
//    {
//        pUVar10 = (UCHAR *) "Pass Thru Diagnostics";
//        pUVar13 = szPrintString;
//        for (iVar7 = 0x16; iVar7 != 0; iVar7 = iVar7 + -1) {
//            *pUVar13 = *pUVar10;
//            pUVar10 = pUVar10 + 1;
//            pUVar13 = pUVar13 + 1;
//        }
//        uVar8 = 0xffffffff;
//        pUVar10 = szPrintString;
//        do {
//            if (uVar8 == 0) break;
//            uVar8 = uVar8 - 1;
//            UVar1 = *pUVar10;
//            pUVar10 = pUVar10 + 1;
//        } while (UVar1 != '\0');
////            puVar11 = (undefined4 *) " - TESTMODE";
////            puVar14 = (undefined4 * )(~uVar8 + 0xac1c9f);
//        for (iVar7 = 3; iVar7 != 0; iVar7 = iVar7 + -1) {
////                *puVar14 = *puVar11;
////                // puVar11 = puVar11 + 1;
////                // puVar14 = puVar14 + 1;
//        }
//        SetWindowTextA((HWND) hWndMain, (LPCSTR) szPrintString);
//        if ((usOverrideDefaultSettings != 0) && (usGlobalTestMode == 0)) {
//            pUVar10 = (UCHAR *) "Pass Thru Diagnostics";
////                pUVar13 = szPrintString;
//            for (iVar7 = 0x16; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pUVar13 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//                pUVar13 = pUVar13 + 1;
//            }
//            uVar8 = 0xffffffff;
//            pUVar10 = szPrintString;
//            do {
//                if (uVar8 == 0) break;
//                uVar8 = uVar8 - 1;
//                UVar1 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//            } while (UVar1 != '\0');
//            pcVar9 = " - Override Default Settings";
//            pcVar12 = (char *) (~uVar8 + 0xac1c9f);
//            for (iVar7 = 0x1d; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pcVar12 = *pcVar9;
//                pcVar9 = pcVar9 + 1;
//                pcVar12 = pcVar12 + 1;
//            }
//            SetWindowTextA((HWND) hWndMain, (LPCSTR) szPrintString);
//        }
//        if ((usManualEPATSInstall != 0) && (usGlobalTestMode == 0)) {
//            pUVar10 = (UCHAR *) "Pass Thru Diagnostics";
//            pUVar13 = szPrintString;
//            for (iVar7 = 0x16; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pUVar13 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//                pUVar13 = pUVar13 + 1;
//            }
//            uVar8 = 0xffffffff;
//            pUVar10 = szPrintString;
//            do {
//                if (uVar8 == 0) break;
//                uVar8 = uVar8 - 1;
//                UVar1 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//            } while (UVar1 != '\0');
//            puVar11 = " - Manual Epats Install";
//            puVar14 = (void *) (~uVar8 + 0xac1c9f);
//            for (iVar7 = 6; iVar7 != 0; iVar7 = iVar7 + -1) {
////                    *puVar14 = *puVar11;
////                    // puVar11 = puVar11 + 1;
////                    // puVar14 = puVar14 + 1;
//            }
//            SetWindowTextA((HWND) hWndMain, (LPCSTR) szPrintString);
//        }
//        if ((usSpecialTestMode1 != 0) && (usGlobalTestMode == 0)) {
//            pUVar10 = (UCHAR *) "Pass Thru Diagnostics";
//            pUVar13 = szPrintString;
//            for (iVar7 = 0x16; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pUVar13 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//                pUVar13 = pUVar13 + 1;
//            }
//            uVar8 = 0xffffffff;
//            pUVar10 = szPrintString;
//            do {
//                if (uVar8 == 0) break;
//                uVar8 = uVar8 - 1;
//                UVar1 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//            } while (UVar1 != '\0');
//            pcVar9 = " - Special Test Mode 1";
//            pcVar12 = (char *) (~uVar8 + 0xac1c9f);
//            for (iVar7 = 0x17; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pcVar12 = *pcVar9;
//                pcVar9 = pcVar9 + 1;
//                pcVar12 = pcVar12 + 1;
//            }
//            SetWindowTextA((HWND) hWndMain, (LPCSTR) szPrintString);
//        }
//        if ((usFLITestMode != 0) && (usGlobalTestMode == 0)) {
//            pUVar10 = (UCHAR *) "Pass Thru Diagnostics";
//            pUVar13 = szPrintString;
//            for (iVar7 = 0x16; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pUVar13 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//                pUVar13 = pUVar13 + 1;
//            }
//            uVar8 = 0xffffffff;
//            pUVar10 = szPrintString;
//            do {
//                if (uVar8 == 0) break;
//                uVar8 = uVar8 - 1;
//                UVar1 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//            } while (UVar1 != '\0');
//            pcVar9 = " - FLI hard code ****";
//            pcVar12 = (char *) (~uVar8 + 0xac1c9f);
//            for (iVar7 = 0x16; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pcVar12 = *pcVar9;
//                pcVar9 = pcVar9 + 1;
//                pcVar12 = pcVar12 + 1;
//            }
//            SetWindowTextA((HWND) hWndMain, (LPCSTR) szPrintString);
//        }
//        if ((usDataAcqPowerCycle != 0) && (usGlobalTestMode == 0)) {
//            pUVar10 = (UCHAR *) "Pass Thru Diagnostics";
//            pUVar13 = szPrintString;
//            for (iVar7 = 0x16; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pUVar13 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//                pUVar13 = pUVar13 + 1;
//            }
//            uVar8 = 0xffffffff;
//            pUVar10 = szPrintString;
//            do {
//                if (uVar8 == 0) break;
//                uVar8 = uVar8 - 1;
//                UVar1 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//            } while (UVar1 != '\0');
//            pcVar9 = " - Power Cycle Relay in Data Acq ****";
//            pcVar12 = (char *) (~uVar8 + 0xac1c9f);
//            for (iVar7 = 0x26; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pcVar12 = *pcVar9;
//                pcVar9 = pcVar9 + 1;
//                pcVar12 = pcVar12 + 1;
//            }
//            SetWindowTextA((HWND) hWndMain, (LPCSTR) szPrintString);
//        }
//        if ((usForceNoVIDSave != 0) && (usGlobalTestMode == 0)) {
//            pUVar10 = (UCHAR *) "Pass Thru Diagnostics";
//            pUVar13 = szPrintString;
//            for (iVar7 = 0x16; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pUVar13 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//                pUVar13 = pUVar13 + 1;
//            }
//            uVar8 = 0xffffffff;
//            pUVar10 = szPrintString;
//            do {
//                if (uVar8 == 0) break;
//                uVar8 = uVar8 - 1;
//                UVar1 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//            } while (UVar1 != '\0');
//            pcVar9 = " - VID save option disabled ****";
//            pcVar12 = (char *) (~uVar8 + 0xac1c9f);
//            for (iVar7 = 0x21; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pcVar12 = *pcVar9;
//                pcVar9 = pcVar9 + 1;
//                pcVar12 = pcVar12 + 1;
//            }
//            SetWindowTextA((HWND) hWndMain, (LPCSTR) szPrintString);
//        }
//        if ((usDGB1SimulationMode != 0) && (usGlobalTestMode == 0)) {
//            pUVar10 = (UCHAR *) "Pass Thru Diagnostics";
//            pUVar13 = szPrintString;
//            for (iVar7 = 0x16; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pUVar13 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//                pUVar13 = pUVar13 + 1;
//            }
//            uVar8 = 0xffffffff;
//            pUVar10 = szPrintString;
//            do {
//                if (uVar8 == 0) break;
//                uVar8 = uVar8 - 1;
//                UVar1 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//            } while (UVar1 != '\0');
//            pcVar9 = " - DGB1 simulation mode ****";
//            pcVar12 = (char *) (~uVar8 + 0xac1c9f);
//            for (iVar7 = 0x1d; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pcVar12 = *pcVar9;
//                pcVar9 = pcVar9 + 1;
//                pcVar12 = pcVar12 + 1;
//            }
//            SetWindowTextA((HWND) hWndMain, (LPCSTR) szPrintString);
//        }
//        if ((usDGB1SimulationVLS != 0) && (usGlobalTestMode == 0)) {
//            pUVar10 = (UCHAR *) "Pass Thru Diagnostics";
//            pUVar13 = szPrintString;
//            for (iVar7 = 0x16; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pUVar13 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//                pUVar13 = pUVar13 + 1;
//            }
//            uVar8 = 0xffffffff;
//            pUVar10 = szPrintString;
//            do {
//                if (uVar8 == 0) break;
//                uVar8 = uVar8 - 1;
//                UVar1 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//            } while (UVar1 != '\0');
////                puVar11 = (undefined4 *) " - DGB1 VLS simulation ****";
////                puVar14 = (undefined4 * )(~uVar8 + 0xac1c9f);
//            for (iVar7 = 7; iVar7 != 0; iVar7 = iVar7 + -1) {
////                    *puVar14 = *puVar11;
//                // puVar11 = puVar11 + 1;
//                // puVar14 = puVar14 + 1;
//            }
//            SetWindowTextA((HWND) hWndMain, (LPCSTR) szPrintString);
//        }
//        if ((usLogPokeTableTestData != 0) && (usGlobalTestMode == 0)) {
//            pUVar10 = (UCHAR *) "Pass Thru Diagnostics";
//            pUVar13 = szPrintString;
//            for (iVar7 = 0x16; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pUVar13 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//                pUVar13 = pUVar13 + 1;
//            }
//            uVar8 = 0xffffffff;
//            pUVar10 = szPrintString;
//            do {
//                if (uVar8 == 0) break;
//                uVar8 = uVar8 - 1;
//                UVar1 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//            } while (UVar1 != '\0');
//            pcVar9 = " - log poke table test data ****";
//            pcVar12 = (char *) (~uVar8 + 0xac1c9f);
//            for (iVar7 = 0x21; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pcVar12 = *pcVar9;
//                pcVar9 = pcVar9 + 1;
//                pcVar12 = pcVar12 + 1;
//            }
//            SetWindowTextA((HWND) hWndMain, (LPCSTR) szPrintString);
//        }
//        if ((usUploadThreadTestMode != 0) && (usGlobalTestMode == 0)) {
//            pUVar10 = (UCHAR *) "Pass Thru Diagnostics";
//            pUVar13 = szPrintString;
//            for (iVar7 = 0x16; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pUVar13 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//                pUVar13 = pUVar13 + 1;
//            }
//            uVar8 = 0xffffffff;
//            pUVar10 = szPrintString;
//            do {
//                if (uVar8 == 0) break;
//                uVar8 = uVar8 - 1;
//                UVar1 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//            } while (UVar1 != '\0');
//            pcVar9 = " - upload test mode ****";
//            pcVar12 = (char *) (~uVar8 + 0xac1c9f);
//            for (iVar7 = 0x19; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pcVar12 = *pcVar9;
//                pcVar9 = pcVar9 + 1;
//                pcVar12 = pcVar12 + 1;
//            }
//            SetWindowTextA((HWND) hWndMain, (LPCSTR) szPrintString);
//        }
//        if ((usVBFToolTestMode != 0) && (usGlobalTestMode == 0)) {
//            pUVar10 = (UCHAR *) "Pass Thru Diagnostics";
//            pUVar13 = szPrintString;
//            for (iVar7 = 0x16; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pUVar13 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//                pUVar13 = pUVar13 + 1;
//            }
//            uVar8 = 0xffffffff;
//            pUVar10 = szPrintString;
//            do {
//                if (uVar8 == 0) break;
//                uVar8 = uVar8 - 1;
//                UVar1 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//            } while (UVar1 != '\0');
//            pcVar9 = " - VBF tool test mode ****";
//            pcVar12 = (char *) (~uVar8 + 0xac1c9f);
//            for (iVar7 = 0x1b; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pcVar12 = *pcVar9;
//                pcVar9 = pcVar9 + 1;
//                pcVar12 = pcVar12 + 1;
//            }
//            SetWindowTextA((HWND) hWndMain, (LPCSTR) szPrintString);
//        }
//        if ((usEraseRegionOverride != 0) && (usGlobalTestMode == 0)) {
//            pUVar10 = (UCHAR *) "Pass Thru Diagnostics";
//            pUVar13 = szPrintString;
//            for (iVar7 = 0x16; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pUVar13 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//                pUVar13 = pUVar13 + 1;
//            }
//            uVar8 = 0xffffffff;
//            pUVar10 = szPrintString;
//            do {
//                if (uVar8 == 0) break;
//                uVar8 = uVar8 - 1;
//                UVar1 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//            } while (UVar1 != '\0');
//            pcVar9 = " - erase region override ****";
//            pcVar12 = (char *) (~uVar8 + 0xac1c9f);
//            for (iVar7 = 0x1e; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pcVar12 = *pcVar9;
//                pcVar9 = pcVar9 + 1;
//                pcVar12 = pcVar12 + 1;
//            }
//            SetWindowTextA((HWND) hWndMain, (LPCSTR) szPrintString);
//        }
//        if ((usCCPTestMode != 0) && (usGlobalTestMode == 0)) {
//            pUVar10 = (UCHAR *) "Pass Thru Diagnostics";
//            pUVar13 = szPrintString;
//            for (iVar7 = 0x16; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pUVar13 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//                pUVar13 = pUVar13 + 1;
//            }
//            uVar8 = 0xffffffff;
//            pUVar10 = szPrintString;
//            do {
//                if (uVar8 == 0) break;
//                uVar8 = uVar8 - 1;
//                UVar1 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//            } while (UVar1 != '\0');
//            pcVar9 = " - CCP test mode ****";
//            pcVar12 = (char *) (~uVar8 + 0xac1c9f);
//            for (iVar7 = 0x16; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pcVar12 = *pcVar9;
//                pcVar9 = pcVar9 + 1;
//                pcVar12 = pcVar12 + 1;
//            }
//            SetWindowTextA((HWND) hWndMain, (LPCSTR) szPrintString);
//        }
//        if ((usCCPSimulationMode != 0) && (usGlobalTestMode == 0)) {
//            pUVar10 = (UCHAR *) "Pass Thru Diagnostics";
//            pUVar13 = szPrintString;
//            for (iVar7 = 0x16; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pUVar13 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//                pUVar13 = pUVar13 + 1;
//            }
//            uVar8 = 0xffffffff;
//            pUVar10 = szPrintString;
//            do {
//                if (uVar8 == 0) break;
//                uVar8 = uVar8 - 1;
//                UVar1 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//            } while (UVar1 != '\0');
//            pcVar9 = " - CCP simulation mode - use CAN monitor ****";
////                pcVar12 = (char *) (~uVar8 + 0xac1c9f);
//            for (iVar7 = 0x2e; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pcVar12 = *pcVar9;
//                pcVar9 = pcVar9 + 1;
//                pcVar12 = pcVar12 + 1;
//            }
//            SetWindowTextA((HWND) hWndMain, (LPCSTR) szPrintString);
//        }
//        if ((usTridUpdateTestMode != 0) && (usGlobalTestMode == 0)) {
//            pUVar10 = (UCHAR *) "Pass Thru Diagnostics";
//            pUVar13 = szPrintString;
//            for (iVar7 = 0x16; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pUVar13 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//                pUVar13 = pUVar13 + 1;
//            }
//            uVar8 = 0xffffffff;
//            pUVar10 = szPrintString;
//            do {
//                if (uVar8 == 0) break;
//                uVar8 = uVar8 - 1;
//                UVar1 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//            } while (UVar1 != '\0');
////                puVar11 = (undefined4 *) " - TRID block update test mode ****";
////                puVar14 = (undefined4 * )(~uVar8 + 0xac1c9f);
//            for (iVar7 = 9; iVar7 != 0; iVar7 = iVar7 + -1) {
////                    *puVar14 = *puVar11;
//                // puVar11 = puVar11 + 1;
//                // puVar14 = puVar14 + 1;
//            }
//            SetWindowTextA((HWND) hWndMain, (LPCSTR) szPrintString);
//        }
//        if ((usHEVFuelTestMode != 0) && (usGlobalTestMode == 0)) {
//            pUVar10 = (UCHAR *) "Pass Thru Diagnostics";
//            pUVar13 = szPrintString;
//            for (iVar7 = 0x16; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pUVar13 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//                pUVar13 = pUVar13 + 1;
//            }
//            uVar8 = 0xffffffff;
//            pUVar10 = szPrintString;
//            do {
//                if (uVar8 == 0) break;
//                uVar8 = uVar8 - 1;
//                UVar1 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//            } while (UVar1 != '\0');
//            pcVar9 = " - HEV fuel test mode ****";
//            pcVar12 = (char *) (~uVar8 + 0xac1c9f);
//            for (iVar7 = 0x1b; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pcVar12 = *pcVar9;
//                pcVar9 = pcVar9 + 1;
//                pcVar12 = pcVar12 + 1;
//            }
//            SetWindowTextA((HWND) hWndMain, (LPCSTR) szPrintString);
//        }
//        if ((usCD33xClusterEmulation != 0) && (usGlobalTestMode == 0)) {
//            pUVar10 = (UCHAR *) "Pass Thru Diagnostics";
//            pUVar13 = szPrintString;
//            for (iVar7 = 0x16; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pUVar13 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//                pUVar13 = pUVar13 + 1;
//            }
//            uVar8 = 0xffffffff;
//            pUVar10 = szPrintString;
//            do {
//                if (uVar8 == 0) break;
//                uVar8 = uVar8 - 1;
//                UVar1 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//            } while (UVar1 != '\0');
//            pcVar9 = " - CD33x Cluster Emulation Test Build ****";
//            pcVar12 = (char *) (~uVar8 + 0xac1c9f);
//            for (iVar7 = 0x2b; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pcVar12 = *pcVar9;
//                pcVar9 = pcVar9 + 1;
//                pcVar12 = pcVar12 + 1;
//            }
//            SetWindowTextA((HWND) hWndMain, (LPCSTR) szPrintString);
//        }
//        if ((usAdaptiveCruiseEmulation != 0) && (usGlobalTestMode == 0)) {
//            pUVar10 = (UCHAR *) "Pass Thru Diagnostics";
//            pUVar13 = szPrintString;
//            for (iVar7 = 0x16; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pUVar13 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//                pUVar13 = pUVar13 + 1;
//            }
//            uVar8 = 0xffffffff;
//            pUVar10 = szPrintString;
//            do {
//                if (uVar8 == 0) break;
//                uVar8 = uVar8 - 1;
//                UVar1 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//            } while (UVar1 != '\0');
//            pcVar9 = " - Adaptive Cruise Control Emulation ****";
//            pcVar12 = (char *) (~uVar8 + 0xac1c9f);
//            for (iVar7 = 0x2a; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pcVar12 = *pcVar9;
//                pcVar9 = pcVar9 + 1;
//                pcVar12 = pcVar12 + 1;
//            }
//            SetWindowTextA((HWND) hWndMain, (LPCSTR) szPrintString);
//        }
//        if ((usFuelEconEmulation != 0) && (usGlobalTestMode == 0)) {
//            pUVar10 = (UCHAR *) "Pass Thru Diagnostics";
//            pUVar13 = szPrintString;
//            for (iVar7 = 0x16; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pUVar13 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//                pUVar13 = pUVar13 + 1;
//            }
//            uVar8 = 0xffffffff;
//            pUVar10 = szPrintString;
//            do {
//                if (uVar8 == 0) break;
//                uVar8 = uVar8 - 1;
//                UVar1 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//            } while (UVar1 != '\0');
//            pcVar9 = " - Fuel Economy Simulation (CAN Monitor) ****";
//            pcVar12 = (char *) (~uVar8 + 0xac1c9f);
//            for (iVar7 = 0x2e; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pcVar12 = *pcVar9;
//                pcVar9 = pcVar9 + 1;
//                pcVar12 = pcVar12 + 1;
//            }
//            SetWindowTextA((HWND) hWndMain, (LPCSTR) szPrintString);
//        }
//        if ((usFuelEconOBDIITestMode != 0) && (usGlobalTestMode == 0)) {
//            pUVar10 = (UCHAR *) "Pass Thru Diagnostics";
//            pUVar13 = szPrintString;
//            for (iVar7 = 0x16; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pUVar13 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//                pUVar13 = pUVar13 + 1;
//            }
//            uVar8 = 0xffffffff;
//            pUVar10 = szPrintString;
//            do {
//                if (uVar8 == 0) break;
//                uVar8 = uVar8 - 1;
//                UVar1 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//            } while (UVar1 != '\0');
//            pcVar9 = " - Fuel Economy (OBDII) Test Mode ****";
//            pcVar12 = (char *) (~uVar8 + 0xac1c9f);
//            for (iVar7 = 0x27; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pcVar12 = *pcVar9;
//                pcVar9 = pcVar9 + 1;
//                pcVar12 = pcVar12 + 1;
//            }
//            SetWindowTextA((HWND) hWndMain, (LPCSTR) szPrintString);
//        }
//        if ((usFuelEconOBDIISimulation != 0) && (usGlobalTestMode == 0)) {
//            pUVar10 = (UCHAR *) "Pass Thru Diagnostics";
//            pUVar13 = szPrintString;
//            for (iVar7 = 0x16; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pUVar13 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//                pUVar13 = pUVar13 + 1;
//            }
//            uVar8 = 0xffffffff;
//            pUVar10 = szPrintString;
//            do {
//                if (uVar8 == 0) break;
//                uVar8 = uVar8 - 1;
//                UVar1 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//            } while (UVar1 != '\0');
////                puVar11 = (undefined4 *) " - Fuel Economy (OBDII) Simulation ****";
////                puVar14 = (undefined4 * )(~uVar8 + 0xac1c9f);
//            for (iVar7 = 10; iVar7 != 0; iVar7 = iVar7 + -1) {
////                    *puVar14 = *puVar11;
//                // puVar11 = puVar11 + 1;
//                // puVar14 = puVar14 + 1;
//            }
//            SetWindowTextA((HWND) hWndMain, (LPCSTR) szPrintString);
//        }
//        if ((usMyCANICStressTest1 != 0) && (usGlobalTestMode == 0)) {
//            pUVar10 = (UCHAR *) "Pass Thru Diagnostics";
//            pUVar13 = szPrintString;
//            for (iVar7 = 0x16; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pUVar13 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//                pUVar13 = pUVar13 + 1;
//            }
//            uVar8 = 0xffffffff;
//            pUVar10 = szPrintString;
//            do {
//                if (uVar8 == 0) break;
//                uVar8 = uVar8 - 1;
//                UVar1 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//            } while (UVar1 != '\0');
//            pcVar9 = " - MyCANIC Stress Test #1 ****";
//            pcVar12 = (char *) (~uVar8 + 0xac1c9f);
//            for (iVar7 = 0x1f; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pcVar12 = *pcVar9;
//                pcVar9 = pcVar9 + 1;
//                pcVar12 = pcVar12 + 1;
//            }
//            SetWindowTextA((HWND) hWndMain, (LPCSTR) szPrintString);
//        }
//        if ((usBypassCRCMismatchErrors != 0) && (usGlobalTestMode == 0)) {
//            pUVar10 = (UCHAR *) "Pass Thru Diagnostics";
//            pUVar13 = szPrintString;
//            for (iVar7 = 0x16; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pUVar13 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//                pUVar13 = pUVar13 + 1;
//            }
//            uVar8 = 0xffffffff;
//            pUVar10 = szPrintString;
//            do {
//                if (uVar8 == 0) break;
//                uVar8 = uVar8 - 1;
//                UVar1 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//            } while (UVar1 != '\0');
//            pcVar9 = " - Bypass CRC mismatch errors ****";
//            pcVar12 = (char *) (~uVar8 + 0xac1c9f);
//            for (iVar7 = 0x23; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pcVar12 = *pcVar9;
//                pcVar9 = pcVar9 + 1;
//                pcVar12 = pcVar12 + 1;
//            }
//            SetWindowTextA((HWND) hWndMain, (LPCSTR) szPrintString);
//        }
//        if ((usAllowFTCFunctionsWindow != 0) && (usGlobalTestMode == 0)) {
//            pUVar10 = (UCHAR *) "Pass Thru Diagnostics";
//            pUVar13 = szPrintString;
//            for (iVar7 = 0x16; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pUVar13 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//                pUVar13 = pUVar13 + 1;
//            }
//            uVar8 = 0xffffffff;
//            pUVar10 = szPrintString;
//            do {
//                if (uVar8 == 0) break;
//                uVar8 = uVar8 - 1;
//                UVar1 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//            } while (UVar1 != '\0');
//            pcVar9 = " - FTC Functions Window Enabled ****";
//            pcVar12 = (char *) (~uVar8 + 0xac1c9f);
//            for (iVar7 = 0x25; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pcVar12 = *pcVar9;
//                pcVar9 = pcVar9 + 1;
//                pcVar12 = pcVar12 + 1;
//            }
//            SetWindowTextA((HWND) hWndMain, (LPCSTR) szPrintString);
//        }
//        if ((usMEMReadTestMode != 0) && (usGlobalTestMode == 0)) {
//            pUVar10 = (UCHAR *) "Pass Thru Diagnostics";
//            pUVar13 = szPrintString;
//            for (iVar7 = 0x16; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pUVar13 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//                pUVar13 = pUVar13 + 1;
//            }
//            uVar8 = 0xffffffff;
//            pUVar10 = szPrintString;
//            do {
//                if (uVar8 == 0) break;
//                uVar8 = uVar8 - 1;
//                UVar1 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//            } while (UVar1 != '\0');
//            pcVar9 = " - MEM Read Test Mode ****";
//            pcVar12 = (char *) (~uVar8 + 0xac1c9f);
//            for (iVar7 = 0x1b; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pcVar12 = *pcVar9;
//                pcVar9 = pcVar9 + 1;
//                pcVar12 = pcVar12 + 1;
//            }
//            SetWindowTextA((HWND) hWndMain, (LPCSTR) szPrintString);
//        }
//        if ((usEONVTestMode != 0) && (usGlobalTestMode == 0)) {
//            pUVar10 = (UCHAR *) "Pass Thru Diagnostics";
//            pUVar13 = szPrintString;
//            for (iVar7 = 0x16; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pUVar13 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//                pUVar13 = pUVar13 + 1;
//            }
//            uVar8 = 0xffffffff;
//            pUVar10 = szPrintString;
//            do {
//                if (uVar8 == 0) break;
//                uVar8 = uVar8 - 1;
//                UVar1 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//            } while (UVar1 != '\0');
//            pcVar9 = " - EONV Test Mode ****";
//            pcVar12 = (char *) (~uVar8 + 0xac1c9f);
//            for (iVar7 = 0x17; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pcVar12 = *pcVar9;
//                pcVar9 = pcVar9 + 1;
//                pcVar12 = pcVar12 + 1;
//            }
//            SetWindowTextA((HWND) hWndMain, (LPCSTR) szPrintString);
//        }
//        if ((usTimeStampDebugMode != 0) && (usGlobalTestMode == 0)) {
//            pUVar10 = (UCHAR *) "Pass Thru Diagnostics";
//            pUVar13 = szPrintString;
//            for (iVar7 = 0x16; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pUVar13 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//                pUVar13 = pUVar13 + 1;
//            }
//            uVar8 = 0xffffffff;
//            pUVar10 = szPrintString;
//            do {
//                if (uVar8 == 0) break;
//                uVar8 = uVar8 - 1;
//                UVar1 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//            } while (UVar1 != '\0');
//            pcVar9 = " - Time Stamp Debug Mode ****";
//            pcVar12 = (char *) (~uVar8 + 0xac1c9f);
//            for (iVar7 = 0x1e; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pcVar12 = *pcVar9;
//                pcVar9 = pcVar9 + 1;
//                pcVar12 = pcVar12 + 1;
//            }
//            SetWindowTextA((HWND) hWndMain, (LPCSTR) szPrintString);
//        }
//        if ((usShowCGEA_PATS_Msgs_Only != 0) && (usGlobalTestMode == 0)) {
//            pUVar10 = (UCHAR *) "Pass Thru Diagnostics";
//            pUVar13 = szPrintString;
//            for (iVar7 = 0x16; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pUVar13 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//                pUVar13 = pUVar13 + 1;
//            }
//            uVar8 = 0xffffffff;
//            pUVar10 = szPrintString;
//            do {
//                if (uVar8 == 0) break;
//                uVar8 = uVar8 - 1;
//                UVar1 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//            } while (UVar1 != '\0');
//            pcVar9 = " - Show CGEA PATS msgs only ****";
//            pcVar12 = (char *) (~uVar8 + 0xac1c9f);
//            for (iVar7 = 0x21; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pcVar12 = *pcVar9;
//                pcVar9 = pcVar9 + 1;
//                pcVar12 = pcVar12 + 1;
//            }
//            SetWindowTextA((HWND) hWndMain, (LPCSTR) szPrintString);
//        }
//        if ((usKOEO_6R_Plant_Test != 0) && (usGlobalTestMode == 0)) {
//            pUVar10 = (UCHAR *) "Pass Thru Diagnostics";
//            pUVar13 = szPrintString;
//            for (iVar7 = 0x16; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pUVar13 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//                pUVar13 = pUVar13 + 1;
//            }
//            uVar8 = 0xffffffff;
//            pUVar10 = szPrintString;
//            do {
//                if (uVar8 == 0) break;
//                uVar8 = uVar8 - 1;
//                UVar1 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//            } while (UVar1 != '\0');
////                puVar11 = (undefined4 *) " - KOEO Test Firmware - 6R ****";
////                puVar14 = (undefined4 * )(~uVar8 + 0xac1c9f);
//            for (iVar7 = 8; iVar7 != 0; iVar7 = iVar7 + -1) {
////                    *puVar14 = *puVar11;
//                // puVar11 = puVar11 + 1;
//                // puVar14 = puVar14 + 1;
//            }
//            SetWindowTextA((HWND) hWndMain, (LPCSTR) szPrintString);
//        }
//        if ((usShow11and29bitCANData != 0) && (usGlobalTestMode == 0)) {
//            pUVar10 = (UCHAR *) "Pass Thru Diagnostics";
//            pUVar13 = szPrintString;
//            for (iVar7 = 0x16; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pUVar13 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//                pUVar13 = pUVar13 + 1;
//            }
//            uVar8 = 0xffffffff;
//            pUVar10 = szPrintString;
//            do {
//                if (uVar8 == 0) break;
//                uVar8 = uVar8 - 1;
//                UVar1 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//            } while (UVar1 != '\0');
////                puVar11 = (undefined4 *) " - Show 11 and 29 bit CAN data ****";
////                puVar14 = (undefined4 * )(~uVar8 + 0xac1c9f);
//            for (iVar7 = 9; iVar7 != 0; iVar7 = iVar7 + -1) {
////                    *puVar14 = *puVar11;
//                // puVar11 = puVar11 + 1;
//                // puVar14 = puVar14 + 1;
//            }
//            SetWindowTextA((HWND) hWndMain, (LPCSTR) szPrintString);
//        }
//        if ((usProgStressTestMode != 0) && (usGlobalTestMode == 0)) {
//            pUVar10 = (UCHAR *) "Pass Thru Diagnostics";
//            pUVar13 = szPrintString;
//            for (iVar7 = 0x16; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pUVar13 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//                pUVar13 = pUVar13 + 1;
//            }
//            uVar8 = 0xffffffff;
//            pUVar10 = szPrintString;
//            do {
//                if (uVar8 == 0) break;
//                uVar8 = uVar8 - 1;
//                UVar1 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//            } while (UVar1 != '\0');
//            pcVar9 = " - Programming Stress Test Mode ****";
//            pcVar12 = (char *) (~uVar8 + 0xac1c9f);
//            for (iVar7 = 0x25; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pcVar12 = *pcVar9;
//                pcVar9 = pcVar9 + 1;
//                pcVar12 = pcVar12 + 1;
//            }
//            SetWindowTextA((HWND) hWndMain, (LPCSTR) szPrintString);
//        }
//        if ((usSnakeTestMode != 0) && (usGlobalTestMode == 0)) {
//            pUVar10 = (UCHAR *) "Pass Thru Diagnostics";
//            pUVar13 = szPrintString;
//            for (iVar7 = 0x16; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pUVar13 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//                pUVar13 = pUVar13 + 1;
//            }
//            uVar8 = 0xffffffff;
//            pUVar10 = szPrintString;
//            do {
//                if (uVar8 == 0) break;
//                uVar8 = uVar8 - 1;
//                UVar1 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//            } while (UVar1 != '\0');
//            pcVar9 = " - Snake Testing ****";
//            pcVar12 = (char *) (~uVar8 + 0xac1c9f);
//            for (iVar7 = 0x16; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pcVar12 = *pcVar9;
//                pcVar9 = pcVar9 + 1;
//                pcVar12 = pcVar12 + 1;
//            }
//            SetWindowTextA((HWND) hWndMain, (LPCSTR) szPrintString);
//        }
//        if ((usMAFSensorTesting != 0) && (usGlobalTestMode == 0)) {
//            pUVar10 = (UCHAR *) "Pass Thru Diagnostics";
//            pUVar13 = szPrintString;
//            for (iVar7 = 0x16; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pUVar13 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//                pUVar13 = pUVar13 + 1;
//            }
//            uVar8 = 0xffffffff;
//            pUVar10 = szPrintString;
//            do {
//                if (uVar8 == 0) break;
//                uVar8 = uVar8 - 1;
//                UVar1 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//            } while (UVar1 != '\0');
//            pcVar9 = " - MAF Sensor Testing ****";
//            pcVar12 = (char *) (~uVar8 + 0xac1c9f);
//            for (iVar7 = 0x1b; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pcVar12 = *pcVar9;
//                pcVar9 = pcVar9 + 1;
//                pcVar12 = pcVar12 + 1;
//            }
//            SetWindowTextA((HWND) hWndMain, (LPCSTR) szPrintString);
//        }
//        if ((usPCVTestSwitch != 0) && (usGlobalTestMode == 0)) {
//            pUVar10 = (UCHAR *) "Pass Thru Diagnostics";
//            pUVar13 = szPrintString;
//            for (iVar7 = 0x16; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pUVar13 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//                pUVar13 = pUVar13 + 1;
//            }
//            uVar8 = 0xffffffff;
//            pUVar10 = szPrintString;
//            do {
//                if (uVar8 == 0) break;
//                uVar8 = uVar8 - 1;
//                UVar1 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//            } while (UVar1 != '\0');
//            pcVar9 = " - PCV Test Mode ****";
//            pcVar12 = (char *) (~uVar8 + 0xac1c9f);
//            for (iVar7 = 0x16; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pcVar12 = *pcVar9;
//                pcVar9 = pcVar9 + 1;
//                pcVar12 = pcVar12 + 1;
//            }
//            SetWindowTextA((HWND) hWndMain, (LPCSTR) szPrintString);
//        }
//        if ((usCRCTestModeSwitch != 0) && (usGlobalTestMode == 0)) {
//            pUVar10 = (UCHAR *) "Pass Thru Diagnostics";
//            pUVar13 = szPrintString;
//            for (iVar7 = 0x16; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pUVar13 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//                pUVar13 = pUVar13 + 1;
//            }
//            uVar8 = 0xffffffff;
//            pUVar10 = szPrintString;
//            do {
//                if (uVar8 == 0) break;
//                uVar8 = uVar8 - 1;
//                UVar1 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//            } while (UVar1 != '\0');
//            pcVar9 = " - CRC Test Mode ****";
//            pcVar12 = (char *) (~uVar8 + 0xac1c9f);
//            for (iVar7 = 0x16; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pcVar12 = *pcVar9;
//                pcVar9 = pcVar9 + 1;
//                pcVar12 = pcVar12 + 1;
//            }
//            SetWindowTextA((HWND) hWndMain, (LPCSTR) szPrintString);
//        }
//        if ((usAutoOSCEnableTestMode != 0) && (usGlobalTestMode == 0)) {
//            pUVar10 = (UCHAR *) "Pass Thru Diagnostics";
//            pUVar13 = szPrintString;
//            for (iVar7 = 0x16; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pUVar13 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//                pUVar13 = pUVar13 + 1;
//            }
//            uVar8 = 0xffffffff;
//            pUVar10 = szPrintString;
//            do {
//                if (uVar8 == 0) break;
//                uVar8 = uVar8 - 1;
//                UVar1 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//            } while (UVar1 != '\0');
//            pcVar9 = " - Auto OSC Enable Test Mode ****";
//            pcVar12 = (char *) (~uVar8 + 0xac1c9f);
//            for (iVar7 = 0x22; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pcVar12 = *pcVar9;
//                pcVar9 = pcVar9 + 1;
//                pcVar12 = pcVar12 + 1;
//            }
//            SetWindowTextA((HWND) hWndMain, (LPCSTR) szPrintString);
//        }
//        if ((usFileBrowserTestMode != 0) && (usGlobalTestMode == 0)) {
//            pUVar10 = (UCHAR *) "Pass Thru Diagnostics";
//            pUVar13 = szPrintString;
//            for (iVar7 = 0x16; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pUVar13 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//                pUVar13 = pUVar13 + 1;
//            }
//            uVar8 = 0xffffffff;
//            pUVar10 = szPrintString;
//            do {
//                if (uVar8 == 0) break;
//                uVar8 = uVar8 - 1;
//                UVar1 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//            } while (UVar1 != '\0');
//            pcVar9 = " - File Browser Test Mode ****";
//            pcVar12 = (char *) (~uVar8 + 0xac1c9f);
//            for (iVar7 = 0x1f; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pcVar12 = *pcVar9;
//                pcVar9 = pcVar9 + 1;
//                pcVar12 = pcVar12 + 1;
//            }
//            SetWindowTextA((HWND) hWndMain, (LPCSTR) szPrintString);
//        }
//        if ((usCLSDataAcquisitionEnable != 0) && (usGlobalTestMode == 0)) {
//            pUVar10 = (UCHAR *) "Pass Thru Diagnostics";
//            pUVar13 = szPrintString;
//            for (iVar7 = 0x16; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pUVar13 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//                pUVar13 = pUVar13 + 1;
//            }
//            uVar8 = 0xffffffff;
//            pUVar10 = szPrintString;
//            do {
//                if (uVar8 == 0) break;
//                uVar8 = uVar8 - 1;
//                UVar1 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//            } while (UVar1 != '\0');
//            pcVar9 = " - Coolant Level Sensor Data Acq ****";
//            pcVar12 = (char *) (~uVar8 + 0xac1c9f);
//            for (iVar7 = 0x26; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pcVar12 = *pcVar9;
//                pcVar9 = pcVar9 + 1;
//                pcVar12 = pcVar12 + 1;
//            }
//            SetWindowTextA((HWND) hWndMain, (LPCSTR) szPrintString);
//        }
//        if (((usCLSDataAcquisitionEnable2 != 0) && (usCLSDataAcquisitionEnable == 0)) &&
//            (usGlobalTestMode == 0)) {
//            pUVar10 = (UCHAR *) "Pass Thru Diagnostics";
//            pUVar13 = szPrintString;
//            for (iVar7 = 0x16; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pUVar13 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//                pUVar13 = pUVar13 + 1;
//            }
//            uVar8 = 0xffffffff;
//            pUVar10 = szPrintString;
//            do {
//                if (uVar8 == 0) break;
//                uVar8 = uVar8 - 1;
//                UVar1 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//            } while (UVar1 != '\0');
////                puVar11 = (undefined4 *) " - Coolant Level Sensor Data Acq 2 ****";
////                puVar14 = (undefined4 * )(~uVar8 + 0xac1c9f);
//            for (iVar7 = 10; iVar7 != 0; iVar7 = iVar7 + -1) {
////                    *puVar14 = *puVar11;
//                // puVar11 = puVar11 + 1;
//                // puVar14 = puVar14 + 1;
//            }
//            SetWindowTextA((HWND) hWndMain, (LPCSTR) szPrintString);
//        }
//        if ((us10msChronoTestMode != 0) && (usGlobalTestMode == 0)) {
//            pUVar10 = (UCHAR *) "Pass Thru Diagnostics";
//            pUVar13 = szPrintString;
//            for (iVar7 = 0x16; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pUVar13 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//                pUVar13 = pUVar13 + 1;
//            }
//            uVar8 = 0xffffffff;
//            pUVar10 = szPrintString;
//            do {
//                if (uVar8 == 0) break;
//                uVar8 = uVar8 - 1;
//                UVar1 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//            } while (UVar1 != '\0');
//            pcVar9 = " - 10ms Chrono Test Mode ****";
//            pcVar12 = (char *) (~uVar8 + 0xac1c9f);
//            for (iVar7 = 0x1e; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pcVar12 = *pcVar9;
//                pcVar9 = pcVar9 + 1;
//                pcVar12 = pcVar12 + 1;
//            }
//            SetWindowTextA((HWND) hWndMain, (LPCSTR) szPrintString);
//        }
//        if ((usFuelPrimeTestMode != 0) && (usGlobalTestMode == 0)) {
//            pUVar10 = (UCHAR *) "Pass Thru Diagnostics";
//            pUVar13 = szPrintString;
//            for (iVar7 = 0x16; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pUVar13 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//                pUVar13 = pUVar13 + 1;
//            }
//            uVar8 = 0xffffffff;
//            pUVar10 = szPrintString;
//            do {
//                if (uVar8 == 0) break;
//                uVar8 = uVar8 - 1;
//                UVar1 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//            } while (UVar1 != '\0');
//            pcVar9 = " - Fuel Prime Test Mode ****";
//            pcVar12 = (char *) (~uVar8 + 0xac1c9f);
//            for (iVar7 = 0x1d; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pcVar12 = *pcVar9;
//                pcVar9 = pcVar9 + 1;
//                pcVar12 = pcVar12 + 1;
//            }
//            SetWindowTextA((HWND) hWndMain, (LPCSTR) szPrintString);
//        }
//        if ((usCodedAccess11TestMode != 0) && (usGlobalTestMode == 0)) {
//            pUVar10 = (UCHAR *) "Pass Thru Diagnostics";
//            pUVar13 = szPrintString;
//            for (iVar7 = 0x16; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pUVar13 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//                pUVar13 = pUVar13 + 1;
//            }
//            uVar8 = 0xffffffff;
//            pUVar10 = szPrintString;
//            do {
//                if (uVar8 == 0) break;
//                uVar8 = uVar8 - 1;
//                UVar1 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//            } while (UVar1 != '\0');
//            pcVar9 = " - Coded Access Test Mode ****";
//            pcVar12 = (char *) (~uVar8 + 0xac1c9f);
//            for (iVar7 = 0x1f; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pcVar12 = *pcVar9;
//                pcVar9 = pcVar9 + 1;
//                pcVar12 = pcVar12 + 1;
//            }
//            SetWindowTextA((HWND) hWndMain, (LPCSTR) szPrintString);
//        }
//        if ((usSBLPowerCycleTestMode != 0) && (usGlobalTestMode == 0)) {
//            pUVar10 = (UCHAR *) "Pass Thru Diagnostics";
//            pUVar13 = szPrintString;
//            for (iVar7 = 0x16; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pUVar13 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//                pUVar13 = pUVar13 + 1;
//            }
//            uVar8 = 0xffffffff;
//            pUVar10 = szPrintString;
//            do {
//                if (uVar8 == 0) break;
//                uVar8 = uVar8 - 1;
//                UVar1 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//            } while (UVar1 != '\0');
//            pcVar9 = " - SBL Power Cycle Test Mode ****";
//            pcVar12 = (char *) (~uVar8 + 0xac1c9f);
//            for (iVar7 = 0x22; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pcVar12 = *pcVar9;
//                pcVar9 = pcVar9 + 1;
//                pcVar12 = pcVar12 + 1;
//            }
//            SetWindowTextA((HWND) hWndMain, (LPCSTR) szPrintString);
//        }
//        if ((usDieselOSCCustomOverride != 0) && (usGlobalTestMode == 0)) {
//            pUVar10 = (UCHAR *) "Pass Thru Diagnostics";
//            pUVar13 = szPrintString;
//            for (iVar7 = 0x16; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pUVar13 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//                pUVar13 = pUVar13 + 1;
//            }
//            uVar8 = 0xffffffff;
//            pUVar10 = szPrintString;
//            do {
//                if (uVar8 == 0) break;
//                uVar8 = uVar8 - 1;
//                UVar1 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//            } while (UVar1 != '\0');
//            pcVar9 = " - Diesel OSC Custom Override ****";
//            pcVar12 = (char *) (~uVar8 + 0xac1c9f);
//            for (iVar7 = 0x23; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pcVar12 = *pcVar9;
//                pcVar9 = pcVar9 + 1;
//                pcVar12 = pcVar12 + 1;
//            }
//            SetWindowTextA((HWND) hWndMain, (LPCSTR) szPrintString);
//        }
//        if ((usVCTTestMode != 0) && (usGlobalTestMode == 0)) {
//            pUVar10 = (UCHAR *) "Pass Thru Diagnostics";
//            pUVar13 = szPrintString;
//            for (iVar7 = 0x16; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pUVar13 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//                pUVar13 = pUVar13 + 1;
//            }
//            uVar8 = 0xffffffff;
//            pUVar10 = szPrintString;
//            do {
//                if (uVar8 == 0) break;
//                uVar8 = uVar8 - 1;
//                UVar1 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//            } while (UVar1 != '\0');
//            pcVar9 = " - VCT Test Mode ****";
//            pcVar12 = (char *) (~uVar8 + 0xac1c9f);
//            for (iVar7 = 0x16; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pcVar12 = *pcVar9;
//                pcVar9 = pcVar9 + 1;
//                pcVar12 = pcVar12 + 1;
//            }
//            SetWindowTextA((HWND) hWndMain, (LPCSTR) szPrintString);
//        }
//        if ((usSBLDownloadTestMode != 0) && (usGlobalTestMode == 0)) {
//            pUVar10 = (UCHAR *) "Pass Thru Diagnostics";
//            pUVar13 = szPrintString;
//            for (iVar7 = 0x16; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pUVar13 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//                pUVar13 = pUVar13 + 1;
//            }
//            uVar8 = 0xffffffff;
//            pUVar10 = szPrintString;
//            do {
//                if (uVar8 == 0) break;
//                uVar8 = uVar8 - 1;
//                UVar1 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//            } while (UVar1 != '\0');
//            pcVar9 = " - SBL Test Mode ****";
//            pcVar12 = (char *) (~uVar8 + 0xac1c9f);
//            for (iVar7 = 0x16; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pcVar12 = *pcVar9;
//                pcVar9 = pcVar9 + 1;
//                pcVar12 = pcVar12 + 1;
//            }
//            SetWindowTextA((HWND) hWndMain, (LPCSTR) szPrintString);
//        }
//        if ((usP558ECMSwapPlantMode != 0) && (usGlobalTestMode == 0)) {
//            pUVar10 = (UCHAR *) "Pass Thru Diagnostics";
//            pUVar13 = szPrintString;
//            for (iVar7 = 0x16; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pUVar13 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//                pUVar13 = pUVar13 + 1;
//            }
//            uVar8 = 0xffffffff;
//            pUVar10 = szPrintString;
//            do {
//                if (uVar8 == 0) break;
//                uVar8 = uVar8 - 1;
//                UVar1 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//            } while (UVar1 != '\0');
//            pcVar9 = " - P558 ECM Swap Plant Mode ****";
//            pcVar12 = (char *) (~uVar8 + 0xac1c9f);
//            for (iVar7 = 0x21; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pcVar12 = *pcVar9;
//                pcVar9 = pcVar9 + 1;
//                pcVar12 = pcVar12 + 1;
//            }
//            SetWindowTextA((HWND) hWndMain, (LPCSTR) szPrintString);
//        }
//        if ((usTimeStampTestMode != 0) && (usGlobalTestMode == 0)) {
//            pUVar10 = (UCHAR *) "Pass Thru Diagnostics";
//            pUVar13 = szPrintString;
//            for (iVar7 = 0x16; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pUVar13 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//                pUVar13 = pUVar13 + 1;
//            }
//            uVar8 = 0xffffffff;
//            pUVar10 = szPrintString;
//            do {
//                if (uVar8 == 0) break;
//                uVar8 = uVar8 - 1;
//                UVar1 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//            } while (UVar1 != '\0');
////                puVar11 = (undefined4 *) " - Timestamp Test Mode ****";
////                puVar14 = (undefined4 * )(~uVar8 + 0xac1c9f);
//            for (iVar7 = 7; iVar7 != 0; iVar7 = iVar7 + -1) {
////                    *puVar14 = *puVar11;
//                // puVar11 = puVar11 + 1;
//                // puVar14 = puVar14 + 1;
//            }
//            SetWindowTextA((HWND) hWndMain, (LPCSTR) szPrintString);
//        }
//        if ((usSession40TestMode != 0) && (usGlobalTestMode == 0)) {
//            pUVar10 = (UCHAR *) "Pass Thru Diagnostics";
//            pUVar13 = szPrintString;
//            for (iVar7 = 0x16; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pUVar13 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//                pUVar13 = pUVar13 + 1;
//            }
//            uVar8 = 0xffffffff;
//            pUVar10 = szPrintString;
//            do {
//                if (uVar8 == 0) break;
//                uVar8 = uVar8 - 1;
//                UVar1 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//            } while (UVar1 != '\0');
//            pcVar9 = " - Session 40 Test Mode ****";
//            pcVar12 = (char *) (~uVar8 + 0xac1c9f);
//            for (iVar7 = 0x1d; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pcVar12 = *pcVar9;
//                pcVar9 = pcVar9 + 1;
//                pcVar12 = pcVar12 + 1;
//            }
//            SetWindowTextA((HWND) hWndMain, (LPCSTR) szPrintString);
//        }
//        if ((usSecureProgrammingMode != 0) && (usGlobalTestMode == 0)) {
//            pUVar10 = (UCHAR *) "Pass Thru Diagnostics";
//            pUVar13 = szPrintString;
//            for (iVar7 = 0x16; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pUVar13 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//                pUVar13 = pUVar13 + 1;
//            }
//            uVar8 = 0xffffffff;
//            pUVar10 = szPrintString;
//            do {
//                if (uVar8 == 0) break;
//                uVar8 = uVar8 - 1;
//                UVar1 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//            } while (UVar1 != '\0');
////                puVar11 = (undefined4 *) " - Secure Programming Mode ****";
////                puVar14 = (undefined4 * )(~uVar8 + 0xac1c9f);
//            for (iVar7 = 8; iVar7 != 0; iVar7 = iVar7 + -1) {
////                    *puVar14 = *puVar11;
//                // puVar11 = puVar11 + 1;
//                // puVar14 = puVar14 + 1;
//            }
//            SetWindowTextA((HWND) hWndMain, (LPCSTR) szPrintString);
//        }
//        if ((usDownloadTestLoop != 0) && (usGlobalTestMode == 0)) {
//            pUVar10 = (UCHAR *) "Pass Thru Diagnostics";
//            pUVar13 = szPrintString;
//            for (iVar7 = 0x16; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pUVar13 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//                pUVar13 = pUVar13 + 1;
//            }
//            uVar8 = 0xffffffff;
//            pUVar10 = szPrintString;
//            do {
//                if (uVar8 == 0) break;
//                uVar8 = uVar8 - 1;
//                UVar1 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//            } while (UVar1 != '\0');
//            pcVar9 = " - Download Test Loop ****";
//            pcVar12 = (char *) (~uVar8 + 0xac1c9f);
//            for (iVar7 = 0x1b; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pcVar12 = *pcVar9;
//                pcVar9 = pcVar9 + 1;
//                pcVar12 = pcVar12 + 1;
//            }
//            SetWindowTextA((HWND) hWndMain, (LPCSTR) szPrintString);
//        }
//        if ((usSocketTestMode != 0) && (usGlobalTestMode == 0)) {
//            pUVar10 = (UCHAR *) "Pass Thru Diagnostics";
//            pUVar13 = szPrintString;
//            for (iVar7 = 0x16; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pUVar13 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//                pUVar13 = pUVar13 + 1;
//            }
//            uVar8 = 0xffffffff;
//            pUVar10 = szPrintString;
//            do {
//                if (uVar8 == 0) break;
//                uVar8 = uVar8 - 1;
//                UVar1 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//            } while (UVar1 != '\0');
//            pcVar9 = " - Socket Test Mode ****";
//            pcVar12 = (char *) (~uVar8 + 0xac1c9f);
//            for (iVar7 = 0x19; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pcVar12 = *pcVar9;
//                pcVar9 = pcVar9 + 1;
//                pcVar12 = pcVar12 + 1;
//            }
//            SetWindowTextA((HWND) hWndMain, (LPCSTR) szPrintString);
//        }
//        if ((usConvertApplinkEnable != 0) && (usGlobalTestMode == 0)) {
//            pUVar10 = (UCHAR *) "Pass Thru Diagnostics";
//            pUVar13 = szPrintString;
//            for (iVar7 = 0x16; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pUVar13 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//                pUVar13 = pUVar13 + 1;
//            }
//            uVar8 = 0xffffffff;
//            pUVar10 = szPrintString;
//            do {
//                if (uVar8 == 0) break;
//                uVar8 = uVar8 - 1;
//                UVar1 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//            } while (UVar1 != '\0');
////                puVar11 = (undefined4 *) " - Convert Applink File Enabled";
////                puVar14 = (undefined4 * )(~uVar8 + 0xac1c9f);
//            for (iVar7 = 8; iVar7 != 0; iVar7 = iVar7 + -1) {
////                    *puVar14 = *puVar11;
//                // puVar11 = puVar11 + 1;
//                // puVar14 = puVar14 + 1;
//            }
//            SetWindowTextA((HWND) hWndMain, (LPCSTR) szPrintString);
//        }
//        if ((usConvertSmogCheckData != 0) && (usGlobalTestMode == 0)) {
//            pUVar10 = (UCHAR *) "Pass Thru Diagnostics";
//            pUVar13 = szPrintString;
//            for (iVar7 = 0x16; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pUVar13 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//                pUVar13 = pUVar13 + 1;
//            }
//            uVar8 = 0xffffffff;
//            pUVar10 = szPrintString;
//            do {
//                if (uVar8 == 0) break;
//                uVar8 = uVar8 - 1;
//                UVar1 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//            } while (UVar1 != '\0');
//            pcVar9 = " - Convert Smog Check Data";
//            pcVar12 = (char *) (~uVar8 + 0xac1c9f);
//            for (iVar7 = 0x1b; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pcVar12 = *pcVar9;
//                pcVar9 = pcVar9 + 1;
//                pcVar12 = pcVar12 + 1;
//            }
//            SetWindowTextA((HWND) hWndMain, (LPCSTR) szPrintString);
//        }
//        if ((usTransAdaptTestMode != 0) && (usGlobalTestMode == 0)) {
//            pUVar10 = (UCHAR *) "Pass Thru Diagnostics";
//            pUVar13 = szPrintString;
//            for (iVar7 = 0x16; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pUVar13 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//                pUVar13 = pUVar13 + 1;
//            }
//            uVar8 = 0xffffffff;
//            pUVar10 = szPrintString;
//            do {
//                if (uVar8 == 0) break;
//                uVar8 = uVar8 - 1;
//                UVar1 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//            } while (UVar1 != '\0');
//            pcVar9 = " - Trans Adapt Test Mode ****";
//            pcVar12 = (char *) (~uVar8 + 0xac1c9f);
//            for (iVar7 = 0x1e; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pcVar12 = *pcVar9;
//                pcVar9 = pcVar9 + 1;
//                pcVar12 = pcVar12 + 1;
//            }
//            SetWindowTextA((HWND) hWndMain, (LPCSTR) szPrintString);
//        }
//        if ((usMode06TestMode != 0) && (usGlobalTestMode == 0)) {
//            pUVar10 = (UCHAR *) "Pass Thru Diagnostics";
//            pUVar13 = szPrintString;
//            for (iVar7 = 0x16; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pUVar13 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//                pUVar13 = pUVar13 + 1;
//            }
//            uVar8 = 0xffffffff;
//            pUVar10 = szPrintString;
//            do {
//                if (uVar8 == 0) break;
//                uVar8 = uVar8 - 1;
//                UVar1 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//            } while (UVar1 != '\0');
//            pcVar9 = " - Mode 06 Test Mode ****";
//            pcVar12 = (char *) (~uVar8 + 0xac1c9f);
//            for (iVar7 = 0x1a; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pcVar12 = *pcVar9;
//                pcVar9 = pcVar9 + 1;
//                pcVar12 = pcVar12 + 1;
//            }
//            SetWindowTextA((HWND) hWndMain, (LPCSTR) szPrintString);
//        }
//        if ((usProgrammingLoopMode != 0) && (usGlobalTestMode == 0)) {
//            pUVar10 = (UCHAR *) "Pass Thru Diagnostics";
//            pUVar13 = szPrintString;
//            for (iVar7 = 0x16; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pUVar13 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//                pUVar13 = pUVar13 + 1;
//            }
//            uVar8 = 0xffffffff;
//            pUVar10 = szPrintString;
//            do {
//                if (uVar8 == 0) break;
//                uVar8 = uVar8 - 1;
//                UVar1 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//            } while (UVar1 != '\0');
////                puVar11 = (undefined4 *) " - Flash Loop Mode ****";
////                puVar14 = (undefined4 * )(~uVar8 + 0xac1c9f);
//            for (iVar7 = 6; iVar7 != 0; iVar7 = iVar7 + -1) {
////                    *puVar14 = *puVar11;
//                // puVar11 = puVar11 + 1;
//                // puVar14 = puVar14 + 1;
//            }
//            SetWindowTextA((HWND) hWndMain, (LPCSTR) szPrintString);
//        }
//        if ((usUreaSensorTestMode != 0) && (usGlobalTestMode == 0)) {
//            pUVar10 = (UCHAR *) "Pass Thru Diagnostics";
//            pUVar13 = szPrintString;
//            for (iVar7 = 0x16; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pUVar13 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//                pUVar13 = pUVar13 + 1;
//            }
//            uVar8 = 0xffffffff;
//            pUVar10 = szPrintString;
//            do {
//                if (uVar8 == 0) break;
//                uVar8 = uVar8 - 1;
//                UVar1 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//            } while (UVar1 != '\0');
//            pcVar9 = " - Urea Sensor Test Mode ****";
//            pcVar12 = (char *) (~uVar8 + 0xac1c9f);
//            for (iVar7 = 0x1e; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pcVar12 = *pcVar9;
//                pcVar9 = pcVar9 + 1;
//                pcVar12 = pcVar12 + 1;
//            }
//            SetWindowTextA((HWND) hWndMain, (LPCSTR) szPrintString);
//        }
//        if ((usSNLockTestMode != 0) && (usGlobalTestMode == 0)) {
//            pUVar10 = (UCHAR *) "Pass Thru Diagnostics";
//            pUVar13 = szPrintString;
//            for (iVar7 = 0x16; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pUVar13 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//                pUVar13 = pUVar13 + 1;
//            }
//            uVar8 = 0xffffffff;
//            pUVar10 = szPrintString;
//            do {
//                if (uVar8 == 0) break;
//                uVar8 = uVar8 - 1;
//                UVar1 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//            } while (UVar1 != '\0');
//            pcVar9 = " - SN Lock Test Mode ****";
//            pcVar12 = (char *) (~uVar8 + 0xac1c9f);
//            for (iVar7 = 0x1a; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pcVar12 = *pcVar9;
//                pcVar9 = pcVar9 + 1;
//                pcVar12 = pcVar12 + 1;
//            }
//            SetWindowTextA((HWND) hWndMain, (LPCSTR) szPrintString);
//            usBypassCRCMismatchErrors = 1;
//        }
//        if ((usDRDConversionTestMode != 0) && (usGlobalTestMode == 0)) {
//            pUVar10 = (UCHAR *) "Pass Thru Diagnostics";
//            pUVar13 = szPrintString;
//            for (iVar7 = 0x16; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pUVar13 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//                pUVar13 = pUVar13 + 1;
//            }
//            uVar8 = 0xffffffff;
//            pUVar10 = szPrintString;
//            do {
//                if (uVar8 == 0) break;
//                uVar8 = uVar8 - 1;
//                UVar1 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//            } while (UVar1 != '\0');
//            pcVar9 = " - DRD Conversion Test Mode ****";
//            pcVar12 = (char *) (~uVar8 + 0xac1c9f);
//            for (iVar7 = 0x21; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pcVar12 = *pcVar9;
//                pcVar9 = pcVar9 + 1;
//                pcVar12 = pcVar12 + 1;
//            }
//            SetWindowTextA((HWND) hWndMain, (LPCSTR) szPrintString);
//        }
//        if ((usAutoOBDIIReadTestMode != 0) && (usGlobalTestMode == 0)) {
//            pUVar10 = (UCHAR *) "Pass Thru Diagnostics";
//            pUVar13 = szPrintString;
//            for (iVar7 = 0x16; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pUVar13 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//                pUVar13 = pUVar13 + 1;
//            }
//            uVar8 = 0xffffffff;
//            pUVar10 = szPrintString;
//            do {
//                if (uVar8 == 0) break;
//                uVar8 = uVar8 - 1;
//                UVar1 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//            } while (UVar1 != '\0');
//            pcVar9 = " - Auto OBDII Read Test Mode ****";
//            pcVar12 = (char *) (~uVar8 + 0xac1c9f);
//            for (iVar7 = 0x22; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pcVar12 = *pcVar9;
//                pcVar9 = pcVar9 + 1;
//                pcVar12 = pcVar12 + 1;
//            }
//            SetWindowTextA((HWND) hWndMain, (LPCSTR) szPrintString);
//        }
//        if ((usAutoBootCodeUpdate != 0) && (usGlobalTestMode == 0)) {
//            pUVar10 = (UCHAR *) "Pass Thru Diagnostics";
//            pUVar13 = szPrintString;
//            for (iVar7 = 0x16; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pUVar13 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//                pUVar13 = pUVar13 + 1;
//            }
//            uVar8 = 0xffffffff;
//            pUVar10 = szPrintString;
//            do {
//                if (uVar8 == 0) break;
//                uVar8 = uVar8 - 1;
//                UVar1 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//            } while (UVar1 != '\0');
////                puVar11 = (undefined4 *) " - Auto Boot Code Update (700) ****";
////                puVar14 = (undefined4 * )(~uVar8 + 0xac1c9f);
//            for (iVar7 = 9; iVar7 != 0; iVar7 = iVar7 + -1) {
////                    *puVar14 = *puVar11;
//                // puVar11 = puVar11 + 1;
//                // puVar14 = puVar14 + 1;
//            }
//            SetWindowTextA((HWND) hWndMain, (LPCSTR) szPrintString);
//        }
//        if ((usAlternatorRampTestMode != 0) && (usGlobalTestMode == 0)) {
//            pUVar10 = (UCHAR *) "Pass Thru Diagnostics";
//            pUVar13 = szPrintString;
//            for (iVar7 = 0x16; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pUVar13 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//                pUVar13 = pUVar13 + 1;
//            }
//            uVar8 = 0xffffffff;
//            pUVar10 = szPrintString;
//            do {
//                if (uVar8 == 0) break;
//                uVar8 = uVar8 - 1;
//                UVar1 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//            } while (UVar1 != '\0');
//            pcVar9 = " - Alternator Ramp Test Mode ****";
//            pcVar12 = (char *) (~uVar8 + 0xac1c9f);
//            for (iVar7 = 0x22; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pcVar12 = *pcVar9;
//                pcVar9 = pcVar9 + 1;
//                pcVar12 = pcVar12 + 1;
//            }
//            SetWindowTextA((HWND) hWndMain, (LPCSTR) szPrintString);
//        }
//        if ((usProCalUploadTestMode != 0) && (usGlobalTestMode == 0)) {
//            pUVar10 = (UCHAR *) "Pass Thru Diagnostics";
//            pUVar13 = szPrintString;
//            for (iVar7 = 0x16; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pUVar13 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//                pUVar13 = pUVar13 + 1;
//            }
//            uVar8 = 0xffffffff;
//            pUVar10 = szPrintString;
//            do {
//                if (uVar8 == 0) break;
//                uVar8 = uVar8 - 1;
//                UVar1 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//            } while (UVar1 != '\0');
////                puVar11 = (undefined4 *) " - ProCal Upload Test Mode ****";
////                puVar14 = (undefined4 * )(~uVar8 + 0xac1c9f);
//            for (iVar7 = 8; iVar7 != 0; iVar7 = iVar7 + -1) {
////                    *puVar14 = *puVar11;
//                // puVar11 = puVar11 + 1;
//                // puVar14 = puVar14 + 1;
//            }
//            SetWindowTextA((HWND) hWndMain, (LPCSTR) szPrintString);
//        }
//        if ((usShowKeyStrokes != 0) && (usGlobalTestMode == 0)) {
//            pUVar10 = (UCHAR *) "Pass Thru Diagnostics";
//            pUVar13 = szPrintString;
//            for (iVar7 = 0x16; iVar7 != 0; iVar7 = iVar7 + -1) {
//                *pUVar13 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//                pUVar13 = pUVar13 + 1;
//            }
//            uVar8 = 0xffffffff;
//            pUVar10 = szPrintString;
//            do {
//                if (uVar8 == 0) break;
//                uVar8 = uVar8 - 1;
//                UVar1 = *pUVar10;
//                pUVar10 = pUVar10 + 1;
//            } while (UVar1 != '\0');
////                puVar11 = (undefined4 *) " - Show Keystrokes ****";
////                puVar14 = (undefined4 * )(~uVar8 + 0xac1c9f);
//            for (iVar7 = 6; iVar7 != 0; iVar7 = iVar7 + -1) {
////                    *puVar14 = *puVar11;
//                // puVar11 = puVar11 + 1;
//                // puVar14 = puVar14 + 1;
//            }
//            SetWindowTextA((HWND) hWndMain, (LPCSTR) szPrintString);
//        }
//    }
    szInputFileHex[0] = '\0';
    szInputFileVBF[0] = '\0';
    szInputFileVBF2[0] = '\0';
    szGenericInputFile[0] = '\0';
    szGenericSecondaryFile[0] = '\0';
    szGenericSecondaryFileVBF[0] = '\0';
    szLookupTableFile[0] = '\0';
    szConfigPath[0] = '\0';
    szPIDListPath[0] = '\0';
    szDMRListPath[0] = '\0';
    szConfigPathOSC[0] = '\0';
    szListPathOSC[0] = '\0';
    szGenericSavePath[0] = '\0';
    szLogFilePath[0] = '\0';
    szLoadScanFile[0] = '\0';
    szLoadScanFileMyCANIC[0] = '\0';
    szDTCFileMode6Data[0] = '\0';
    bGlobalSecurityBytesInitFile1[0] = 0xc1;
    bGlobalSecurityBytesInitFile1[1] = 0xc1;
    bGlobalSecurityBytesInitFile1[2] = 0xc1;
    bGlobalSecurityBytesInitFile1[3] = 0xc1;
    bGlobalSecurityBytesInitFile1[4] = 0xc1;
    bGlobalSecurityBytesInitFile1[5] = 0xc1;
    bGlobalSecurityBytesInitFile1[6] = 0xc1;
    bGlobalSecurityBytesInitFile1[7] = 0xc1;
    bGlobalSecurityBytesInitFile1[8] = 0xc1;
    bGlobalSecurityBytesInitFile1[9] = 0xc1;
    bGlobalSecurityBytesInitFile1[10] = 0xc1;
    bGlobalSecurityBytesInitFile1[11] = 0xc1;
    bGlobalSecurityBytesInitFile2[0] = 0xc2;
    bGlobalSecurityBytesInitFile2[1] = 0xc2;
    bGlobalSecurityBytesInitFile2[2] = 0xc2;
    bGlobalSecurityBytesInitFile2[3] = 0xc2;
    bGlobalSecurityBytesInitFile2[4] = 0xc2;
    bGlobalSecurityBytesInitFile2[5] = 0xc2;
    bGlobalSecurityBytesInitFile2[6] = 0xc2;
    bGlobalSecurityBytesInitFile2[7] = 0xc2;
    bGlobalSecurityBytesInitFile2[8] = 0xc2;
    bGlobalSecurityBytesInitFile2[9] = 0xc2;
    bGlobalSecurityBytesInitFile2[10] = 0xc2;
    bGlobalSecurityBytesInitFile2[11] = 0xc2;
    usSampleRate = 0;
//        szSampleRate._0_2_ = 0x30;
    szTranslatedTableNumRows[0] = '\0';
    szTranslatedTableNumCols[0] = '\0';
    szTranslatedTableStartAddress[0] = '\0';
    usNumInitParms = 0x54;
    InitLogFile();
    SwapDataByte('\0', 0xf000);
    if (usP558ECMSwapPlantMode == 0) {
        ReadInitFile();
    }
    ConfigureOffsetValuesVID(0);
    if (usOverrideDefaultSettings != 0) {
        usDataRate = 1;
        usCaptureDataWarning = 1;
        usContinuousCapture = 0;
        usLogMilliseconds = 0;
        usDataDisplaySwap = 1;
        usDataDisplayWide = 0;
        usAutoStartMode = 1;
        usDataCapWindowVisible = 1;
        usExtraDataWindowVisible = 1;
        usMinMaxDataWindowVisible = 0;
        usMinMaxDataWindowState = 0;
        usPIDSelectionWindowVisible = 0;
        usDMRSelectionWindowVisible = 0;
        usOutputSelection = 1;
        usDMRByteOrderMotorola = 0;
        usDisableLowVoltageWarning = 1;
    }
    DisplayConfigItems();
    if ((usProtocol == 0) && (usComlevel == 2)) {
        hMenuItem = GetMenu((HWND) hWndMain);
        EnableMenuItem(hMenuItem, 0x9c98, 1);
    } else {
        hMenuItem = GetMenu((HWND) hWndMain);
        EnableMenuItem(hMenuItem, 0x9c98, 0);
    }
    if ((usBuildVersion < 2) && (usShowBlackIceStatus - 1 < 2)) {
//            pBVar6 = GetBlackIceStatus();
//            sprintf((char *) szPrintString, "%s(cardaq)(bi:%s)", (char *) szPtdiagVersionShort,
//                    (char *) pBVar6->pszStatus);
        UpdateMainStatusLine(szPrintString, "", hWndDialog1);
        if ((usShowBlackIceStatus == 2) != 0) {
            MessageBoxA((HWND) hWndDialog1, (LPCSTR) "InitInstance::szBlackIceRunning", (LPCSTR) szTitle,
                        0x30);
        }
    }
    if (usRunBikeAtStartup != 0) {
//            RunBike();
    }
    if (usEnableExtraSecurityFunctions != 0) {
        MessageBoxA((HWND) hWndDialog1,
                    "One or more mandated default = FALSE conditions does not exist!",
                    (LPCSTR) szTitle, 0x30);
    }
    //// TODO Bad decompilation of string concatination

//    if (usProgModeLockoutMode1 != 0) {
//        pUVar10 = (UCHAR *) "Pass Thru Diagnostics";
//        pUVar13 = szPrintString;
//        for (iVar7 = 0x16; iVar7 != 0; iVar7 = iVar7 + -1) {
//            *pUVar13 = *pUVar10;
//            pUVar10 = pUVar10 + 1;
//            pUVar13 = pUVar13 + 1;
//        }
//        uVar8 = 0xffffffff;
//        pUVar10 = szPrintString;
//        do {
//            if (uVar8 == 0) break;
//            uVar8 = uVar8 - 1;
//            UVar1 = *pUVar10;
//            pUVar10 = pUVar10 + 1;
//        } while (UVar1 != '\0');
//        pcVar9 = " - SPM #1";
//        pcVar12 = (char *) (~uVar8 + 0xac1c9f);
//        for (iVar7 = 10; iVar7 != 0; iVar7 = iVar7 + -1) {
//            *pcVar12 = *pcVar9;
//            pcVar9 = pcVar9 + 1;
//            pcVar12 = pcVar12 + 1;
//        }
//        SetWindowTextA((HWND) hWndMain, (LPCSTR) szPrintString);
//    }

    //// TODO Bad decompilation of string concatination
//
//    if (usEOLVIDProgramStressTestMode != 0) {
//        pUVar10 = (UCHAR *) "Pass Thru Diagnostics";
//        pUVar13 = szPrintString;
//        for (iVar7 = 0x16; iVar7 != 0; iVar7 = iVar7 + -1) {
//            *pUVar13 = *pUVar10;
//            pUVar10 = pUVar10 + 1;
//            pUVar13 = pUVar13 + 1;
//        }
//        uVar8 = 0xffffffff;
//        pUVar10 = szPrintString;
//        do {
//            if (uVar8 == 0) break;
//            uVar8 = uVar8 - 1;
//            UVar1 = *pUVar10;
//            pUVar10 = pUVar10 + 1;
//        } while (UVar1 != '\0');
//        pcVar9 = " - EOL VID Program Loop - Custom Trigger";
////            pcVar12 = (char *) (~uVar8 + 0xac1c9f);
//        for (iVar7 = 0x29; iVar7 != 0; iVar7 = iVar7 + -1) {
//            *pcVar12 = *pcVar9;
//            pcVar9 = pcVar9 + 1;
//            pcVar12 = pcVar12 + 1;
//        }
//        SetWindowTextA((HWND) hWndMain, (LPCSTR) szPrintString);
//    }
//

    if (usP558ECMSwapPlantMode != 0) {
        DialogBoxParamA((HINSTANCE) hInst, (LPCSTR) 0x8e, (HWND) hWndMain, ProcessPatsFunctions, 0);
//            ExitProgram(0); //TODO
    }

    return TRUE;
}

ATOM RegisterMainWindow(HINSTANCE hInstance) {
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, IDI_APPLICATION);
    wcex.hCursor = LoadCursor(NULL, IDC_CROSS);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_PTDIAG);
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW( &wcex);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch(message) {
        case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            switch(wmId) {
                //// FILE MENU
                case IDM_FILE_VIEW_LOG:
                    ViewLogFile(hInst, hWnd); //COMPLETE
                    break;
                case IDM_FILE_EXIT:
                    DestroyWindow(hWnd); //COMPLETE
                    break;

                    //// VIEW MENU
                case IDM_VIEW_INIT:
                    break;
                case IDM_VIEW_CAN_IDS:
                    break;
                case IDM_VIEW_CODE_DEFS:
                    break;

                    //// SETUP MENU
                case IDM_SETUP_CONFIG:
                    break;
                case IDM_SETUP_SPECIAL_CONFIG:
                    break;
                case IDM_SETUP_IP_CONFIG:
                    break;

                    //// DIAGNOSTIC MENU
                case IDM_DIAG_DATA_ACQ:
                    break;
                case IDM_DIAG_GET_DTC:
                    break;
                case IDM_DIAG_OBD_FUNCS:
                    break;
                case IDM_DIAG_PATS:
                    break;
                case IDM_DIAG_OUTPUT_TEST:
                    break;
                case IDM_DIAG_MISC_COMM:
                    break;

                    //// PROGRAMMING MENU
                case IDM_PROG_PCM:
                    break;
                case IDM_PROG_GENERIC:
                    break;
                case IDM_PROG_BLOCK_READ_WRITE:
                    break;

                    //// TOOLS MENU
                case IDM_TOOLS_BRAKE:
                    break;
                case IDM_TOOLS_READ_MEMORY_BLOCK:
                    break;
                case IDM_TOOLS_READ_MEMORY_SEGMENT:
                    break;
                case IDM_TOOLS_READ_TRANSLATED_TABLE:
                    break;
                case IDM_TOOLS_RAM_DOWNLOAD:
                    break;
                case IDM_TOOLS_KAM_SAVE_RESTORE:
                    break;
                case IDM_TOOLS_LOGGING_TOOLS:
                    break;
                case IDM_TOOLS_DIESEL_TOOLS:
                    break;
                case IDM_TOOLS_FUEL_MULT:
                    break;
                case IDM_TOOLS_FAULT_INJECTION:
                    break;
                case IDM_TOOLS_MONITOR_CAN:
                    break;

                    //// UTILITY MENU
                case IDM_UTILITY_VEHICLE_LINK_SIM:
                    break;
                case IDM_UTILITY_TRANSMIT_PERIODIC_MSG:
                    break;
                case IDM_UTILITY_TEST_PANEL:
                    break;
                case IDM_UTILITY_VIEW_HEX:
                    break;
                case IDM_UTILITY_VIEW_VBF:
                    break;
                case IDM_UTILITY_VIEW_BIN:
                    break;
                case IDM_UTILITY_SPECIAL_TESTS:
                    break;
                case IDM_UTILITY_STRESS_TESTS:
                    break;

                    //// HELP MENU
                case IDM_HELP_USAGE:
                    break;
                case IDM_HELP_REL_NOTES:
                    break;
                case IDM_HELP_ABOUT:
                    DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                    break;
                default:
                    return DefWindowProcW(hWnd, message, wParam, lParam);
            }
        }
            break;
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            RECT rect;
            HDC hdc = BeginPaint(hWnd, &ps);
            GetClientRect(hWnd, &rect);

//            LPCSTR text = (LPCSTR)malloc(sizeof(LPCSTR) * 256);
//            memset(text, 'A', 10);
//            memset(text + 10, NULL, 256 - 10);
//
//            _snprintf(text, 256, "w: %ld, h: %ld", width, height);
////            _snwprintf_s(text, 256 - 1, 256 - 1, "W: %ld H: %ld", width, height);
//
//            DrawTextA(hdc, text, -1, &rect, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
            EndPaint(hWnd, &ps);
        }
            break;

        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProcW(hWnd, message, wParam, lParam);
    }

    return 0;
}

LRESULT ProcessMainDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    USHORT UVar1;
    unsigned int uVar2;
    HWND hWnd;
    HWND hWnd_00;
    HWND hWnd_01;
    HWND hWnd_02;
    HWND hWnd_03;
    HWND hWnd_04;
    HWND hWnd_05;
    HWND hWnd_06;
    HFONT pHVar3;
    HMENU pHVar4;
    PUCHAR pszString;
    int iVar5;
    UCHAR *pUVar6;
    UCHAR *pUVar7;
    USHORT usReturn;
    HMENU hMenu;
    HFONT hFontTextBox3;
    HFONT hFontTextBox2;
    HFONT hFontTextBox9;
    HFONT hFontTextBox8;
    HFONT hFontTextBox7;
    HFONT hFontTextBox6;
    HFONT hFontTextBox5;
    HFONT hFontTextBox1;
    HWND hTextBox9;
    HWND hTextBox8;
    HWND hTextBox7;
    HWND hTextBox6;
    HWND hTextBox5;
    HWND hTextBox3;
    HWND hTextBox2;
    HWND hTextBox1;
    int wmEvent;
    int wmId;
    int wFontSize1;

    uVar2 = wParam & 0xffff;
    if (message == 0x110)
    {
        hWnd = GetDlgItem((HWND)hDlg, 0x3ed);
        hWnd_00 = GetDlgItem((HWND)hDlg, 0x3ee);
        hWnd_01 = GetDlgItem((HWND)hDlg, 0x3ef);
        hWnd_02 = GetDlgItem((HWND)hDlg, 0x3f4);
        hWnd_03 = GetDlgItem((HWND)hDlg, 0x3f5);
        hWnd_04 = GetDlgItem((HWND)hDlg, 0x3f6);
        hWnd_05 = GetDlgItem((HWND)hDlg, 0x402);
        hWnd_06 = GetDlgItem((HWND)hDlg, 0x408);
        pHVar3 = CreateFontA(0xc, 0, 0, 0, 400, 0, 0, 0, 0, 4, 0, 2, 1, "Courier");
        if (pHVar3 != (HFONT)0x0)
        {
            SendMessageA(hWnd, 0x30, (WPARAM)pHVar3, 1);
        }
        pHVar3 = CreateFontA(0xc, 0, 0, 0, 400, 0, 0, 0, 0, 4, 0, 2, 1, "Courier");
        if (pHVar3 != (HFONT)0x0)
        {
            SendMessageA(hWnd_02, 0x30, (WPARAM)pHVar3, 1);
        }
        pHVar3 = CreateFontA(0xc, 0, 0, 0, 400, 0, 0, 0, 0, 4, 0, 2, 1, "Courier");
        if (pHVar3 != (HFONT)0x0)
        {
            SendMessageA(hWnd_03, 0x30, (WPARAM)pHVar3, 1);
        }
        pHVar3 = CreateFontA(0xc, 0, 0, 0, 400, 0, 0, 0, 0, 4, 0, 2, 1, "Courier");
        if (pHVar3 != (HFONT)0x0)
        {
            SendMessageA(hWnd_04, 0x30, (WPARAM)pHVar3, 1);
        }
        pHVar3 = CreateFontA(0xc, 0, 0, 0, 400, 0, 0, 0, 0, 4, 0, 2, 1, "Courier");
        if (pHVar3 != (HFONT)0x0)
        {
            SendMessageA(hWnd_05, 0x30, (WPARAM)pHVar3, 1);
        }
        pHVar3 = CreateFontA(0xc, 0, 0, 0, 400, 0, 0, 0, 0, 4, 0, 2, 1, "Courier");
        if (pHVar3 != (HFONT)0x0)
        {
            SendMessageA(hWnd_06, 0x30, (WPARAM)pHVar3, 1);
        }
        pHVar3 = CreateFontA(0x10, 0, 0, 0, 400, 0, 0, 0, 0, 4, 0, 2, 2, "MS Sans Serif");
        if (pHVar3 != (HFONT)0x0)
        {
            SendMessageA(hWnd_00, 0x30, (WPARAM)pHVar3, 1);
        }
        pHVar3 = CreateFontA(0x18, 0, 0, 0, 700, 0, 0, 0, 0, 4, 0, 2, 1, "Courier New");
        if (pHVar3 != (HFONT)0x0)
        {
            SendMessageA(hWnd_01, 0x30, (WPARAM)pHVar3, 1);
        }

        USHORT usInitOnce = 0;

        if (usInitOnce == 0)
        {
            ulActiveChannelId = 0;
//            szFirmwareRev = 0x612f6e;
            usInitOnce = 1;
        }
        if (usBuildVersion < 2)
        {
            
            sprintf((char *)szPrintString, "%s(%s)", (char *)szPtdiagVersionShort,
                     (char *)szPassThruToolType);
            UpdateMainStatusLine(szPrintString, "", hDlg);
        }
        else if (usBuildVersion == 2)
        {
            VerifyMcS1Build();
            sprintf((char *)szPrintString, "%s(%s)", (char *)szPtdiagVersionShort,
                     (char *)szPassThruToolType);
            UpdateMainStatusLine(szPrintString, "", hDlg);
            pHVar4 = GetMenu((HWND)hWndMain);
            EnableMenuItem(pHVar4, 0x9c76, 1);
            EnableMenuItem(pHVar4, 0x9c77, 1);
        }
        else
        {
            UpdateMainStatusLine(szPtdiagVersion, "", hDlg);
            pHVar4 = GetMenu((HWND)hWndMain);
            EnableMenuItem(pHVar4, 0x9c76, 1);
            EnableMenuItem(pHVar4, 0x9c77, 1);
        }
    }
    else if (message == 0x111)
    {
        if (uVar2 == 0x3f1)
        {
            UpdateMainStatusLine("", "", hDlg);
            UVar1 = BoxConnect(0, hDlg);
            if (UVar1 != 0x12)
            {
                if (UVar1 == 0)
                {
                    if (usBuildVersion < 2)
                    {
                        pUVar6 = (UCHAR *)"Connect to CarDAQ";
                        pUVar7 = szPrintString;
                        for (iVar5 = 0x12; iVar5 != 0; iVar5 = iVar5 + -1)
                        {
                            *pUVar7 = *pUVar6;
                            pUVar6 = pUVar6 + 1;
                            pUVar7 = pUVar7 + 1;
                        }
                        UpdateMainStatusLine((PUCHAR) "successful", szPrintString, hDlg);
                    }
                    else
                    {
//                        szPrintString._0_4_ = 0x6e6e6f43;
//                        szPrintString._4_4_ = "easonable sample rate. (%d) ";
                        UpdateMainStatusLine((PUCHAR) "successful", szPrintString, hDlg);
                    }
                }
                else if ((UVar1 == 99) && (usMyCANICFlag != 0))
                {
                    UpdateMainStatusLine((PUCHAR) "invalid hardware version", (PUCHAR) "Connect Failure", hDlg);
                }
                else
                {
                    UpdateMainStatusLine((PUCHAR) "Connect failure", "", hDlg);
                }
            }
        }
        else if (uVar2 == 0x3f2)
        {
            UpdateMainStatusLine("", "", hDlg);
            Sleep(100);
            if (ulActiveChannelId == 0)
            {
                UpdateMainStatusLine((PUCHAR) "connect to the comms box first", "", hDlg);
            }
            else
            {
                UpdateMainStatusLine((PUCHAR) "pinging module...", "", hDlg);
                pszString = PingModule(ulActiveChannelId);
                UpdateMainStatusLine(pszString, (PUCHAR) "Ping Module", hDlg);
            }
        }
        else if (uVar2 == 2)
        {
            ExitProgram(0);
        }
    }
    return 0;
}

void ExitProgram(int status) {
    BOOL BVar1;

    ///TODO Fix this, so no memory leaks

//    if (usBoxConnect != 0)
//    {
//        if (usBuildVersion == 2)
//        {
//            if (usBypassDisconnectMcS1 == 0)
//            {
//                BoxDisconnect(hWndDialog1);
//            }
//        }
//        else
//        {
//            ControlPeriodicMessages(0x80);
//            BoxDisconnect(hWndDialog1);
//        }
//    }
//    if (ulThreadHandle != 0)
//    {
//        CloseHandle((HANDLE)ulThreadHandle);
//    }
//    if (ulVLSThreadHandle != 0)
//    {
//        CloseHandle((HANDLE)ulVLSThreadHandle);
//    }
//    if (ulEnterFlashThreadHandle != 0)
//    {
//        CloseHandle((HANDLE)ulEnterFlashThreadHandle);
//    }
//    if (ulProgThreadHandle != 0)
//    {
//        CloseHandle((HANDLE)ulProgThreadHandle);
//    }
//    if (ulEraseThreadHandle != 0)
//    {
//        CloseHandle((HANDLE)ulEraseThreadHandle);
//    }
//    if (ulObjectThreadHandle != 0)
//    {
//        CloseHandle((HANDLE)ulObjectThreadHandle);
//    }
//    if (ulStressThreadHandle != 0)
//    {
//        CloseHandle((HANDLE)ulStressThreadHandle);
//    }
//    if (ulOnDemandThreadHandle != 0)
//    {
//        CloseHandle((HANDLE)ulOnDemandThreadHandle);
//    }
//    if (ulUploadThreadHandle != 0)
//    {
//        CloseHandle((HANDLE)ulUploadThreadHandle);
//    }
//    if (ulSecAccessThreadHandle != 0)
//    {
//        CloseHandle((HANDLE)ulSecAccessThreadHandle);
//    }
//    if (ulFuelMultThreadHandle != 0)
//    {
//        CloseHandle((HANDLE)ulFuelMultThreadHandle);
//    }
//    if (ulEvacFillThreadHandle != 0)
//    {
//        CloseHandle((HANDLE)ulEvacFillThreadHandle);
//    }
//    if (ulBleedThreadHandle != 0)
//    {
//        CloseHandle((HANDLE)ulBleedThreadHandle);
//    }
//    if (ulEraseThreadHandleGeneric != 0)
//    {
//        CloseHandle((HANDLE)ulEraseThreadHandleGeneric);
//    }
//    if (ulProgThreadHandleGeneric != 0)
//    {
//        CloseHandle((HANDLE)ulProgThreadHandleGeneric);
//    }
//    if (ulFaultInjThreadHandle != 0)
//    {
//        CloseHandle((HANDLE)ulFaultInjThreadHandle);
//    }
//    if (ulPeriodiocThreadHandle != 0)
//    {
//        CloseHandle((HANDLE)ulPeriodiocThreadHandle);
//    }
//    if (ulDieselThreadHandle != 0)
//    {
//        CloseHandle((HANDLE)ulDieselThreadHandle);
//    }
//    if (ulMMNPCThreadHandle != 0)
//    {
//        CloseHandle((HANDLE)ulMMNPCThreadHandle);
//    }
//    if (ulSocketThreadHandle != 0)
//    {
//        CloseHandle((HANDLE)ulSocketThreadHandle);
//    }
//    if (ulGPFThreadHandle != 0)
//    {
//        CloseHandle((HANDLE)ulGPFThreadHandle);
//    }
//    if (ulOBDIIDecodeThreadHandle != 0)
//    {
//        CloseHandle((HANDLE)ulOBDIIDecodeThreadHandle);
//    }
//    if (ulOBDIITimerThreadHandle != 0)
//    {
//        CloseHandle((HANDLE)ulOBDIITimerThreadHandle);
//    }
    BVar1 = IsWindow((HWND)hWndDialog1);
    if (BVar1 != 0)
    {
        DestroyWindow((HWND)hWndDialog1);
    }
    BVar1 = IsWindow((HWND)hWndTextPop1);
    if (BVar1 != 0)
    {
        DestroyWindow((HWND)hWndTextPop1);
    }
    BVar1 = IsWindow((HWND)hWndTextPop2);
    if (BVar1 != 0)
    {
        DestroyWindow((HWND)hWndTextPop2);
    }
    BVar1 = IsWindow((HWND)hWndTextPop3);
    if (BVar1 != 0)
    {
        DestroyWindow((HWND)hWndTextPop3);
    }
    BVar1 = IsWindow((HWND)hWndGenLog);
    if (BVar1 != 0)
    {
        DestroyWindow((HWND)hWndGenLog);
    }
    BVar1 = IsWindow((HWND)hWndUsageWin);
    if (BVar1 != 0)
    {
        DestroyWindow((HWND)hWndUsageWin);
    }
    BVar1 = IsWindow((HWND)hWndCANDebug);
    if (BVar1 != 0)
    {
        DestroyWindow((HWND)hWndCANDebug);
    }
//    if (usMemoryAllocationDataCapture != 0)
//    {
//        free(pDataCapture);
//    }
//    if (usMemoryAllocationCANMonitor != 0)
//    {
//        free(pMsgSave);
//    }
//    if (usAction == 0)
//    {
//        PostMessageA((HWND)hWndMain, 0x10, 0, 0);
//    }
}

void DisplayConfigItems() {

}

void InitialTextPopAdjust1() {

}

void InitialTextPopAdjust2() {

}

void InitialTextPopAdjust3(){}
void GenericLogAdjust(int somethiing){}
void AdjustUsageNotesWindow(int somethiing){}

void UpdateMainStatusLine(PUCHAR pszString, PUCHAR pszStatus, HWND hOwnerWindow)
{
    unsigned char uVar1;
    HWND hWnd;
    HWND hWnd_00;
    int iVar2;
    PUCHAR puVar3;
    USHORT usStringLength;
    HWND hGroupBox;
    HWND hDisplayBox;

    hWnd = GetDlgItem((HWND)hOwnerWindow, 0x3ef);
    hWnd_00 = GetDlgItem((HWND)hOwnerWindow, 0x401);
    iVar2 = -1;
    puVar3 = pszStatus;
    do
    {
        if (iVar2 == 0)
            break;
        iVar2 = iVar2 + -1;
        uVar1 = *puVar3;
        puVar3 = puVar3 + 1;
    } while (uVar1 != '\0');
    if ((short)iVar2 == -2)
    {
        SetWindowTextA(hWnd_00, "Status");
    }
    else
    {
        SetWindowTextA(hWnd_00, (LPCSTR)pszStatus);
    }
    SetWindowTextA(hWnd, (LPCSTR)pszString);
    UpdateWindow(hWnd);
    UpdateWindow((HWND)hOwnerWindow);
}

void VerifyMcS1Build() {

}

USHORT BoxConnect() {
    return 0;
}

PUCHAR PingModule(ULONG id) {
    return NULL;
}

void InitialWindowAdjust(HWND hWnd) {

}
