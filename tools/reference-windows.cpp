// Offline acceptance probe for public PTDIAG configuration and MX32 conversion dialogs.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <fstream>
#include <iomanip>

namespace {
DWORD pid;
BOOL CALLBACK collect(HWND h, LPARAM p) {
    DWORD owner;
    GetWindowThreadProcessId(h, &owner);
    if (owner == pid && IsWindowVisible(h))
        reinterpret_cast<std::vector<HWND> *>(p)->push_back(h);
    return TRUE;
}
std::vector<HWND> windows() {
    std::vector<HWND> v;
    EnumWindows(collect, reinterpret_cast<LPARAM>(&v));
    return v;
}
std::wstring caption(HWND h) {
    wchar_t name[512]{};
    DWORD_PTR ignored;
    SendMessageTimeoutW(h, WM_GETTEXT, 512, reinterpret_cast<LPARAM>(name), SMTO_ABORTIFHUNG, 300, &ignored);
    return name;
}
bool menuHas(HMENU menu, UINT id) {
    if (!menu)
        return false;
    for (int i = 0; i < GetMenuItemCount(menu); ++i)
        if (GetMenuItemID(menu, i) == id || menuHas(GetSubMenu(menu, i), id))
            return true;
    return false;
}
void snapshot() {
    for (auto h : windows()) {
        wchar_t cls[128]{};
        GetClassNameW(h, cls, 128);
        auto menu = GetMenu(h);
        std::wcerr << L"Window: " << caption(h) << L" [" << cls << L"] enabled=" << IsWindowEnabled(h)
                   << L" menu entries=" << GetMenuItemCount(menu) << L'\n';
        for (int i = 0; i < GetMenuItemCount(menu); ++i) {
            wchar_t text[128]{};
            GetMenuStringW(menu, i, text, 128, MF_BYPOSITION);
            std::wcerr << L" menu: " << text << L'\n';
        }
        for (HWND c = GetWindow(h, GW_CHILD); c; c = GetWindow(c, GW_HWNDNEXT)) {
            auto s = caption(c);
            if (!s.empty())
                std::wcerr << L"  " << GetDlgCtrlID(c) << L": " << s << L'\n';
        }
    }
}
template <class F> HWND waitFor(F accepts, const char *step) {
    ULONGLONG deadline = GetTickCount64() + 30000;
    do {
        for (auto h : windows())
            if (accepts(h))
                return h;
        Sleep(80);
    } while (GetTickCount64() < deadline);
    snapshot();
    throw std::runtime_error(step);
}
HWND byMenu(UINT id) {
    return waitFor(
        [=](HWND h) {
            if (!IsWindowEnabled(h))
                return false;
            if (menuHas(GetMenu(h), id))
                return true;
            // Some compatibility hosts do not expose another process's menu handle.
            wchar_t cls[64]{};
            GetClassNameW(h, cls, 64);
            if (id == 40031)
                return std::wstring(cls).find(L"PTDIAG") == 0 && caption(h) == L"Pass Thru Diagnostics";
            return GetDlgItem(h, 1019) && caption(GetDlgItem(h, 1019)) == L"Output State Control" &&
                   caption(GetDlgItem(h, 1009)) == L"Run = OFF";
        },
        "Menu window did not become available");
}
void click(HWND dlg, int id) {
    auto b = GetDlgItem(dlg, id);
    if (!b || !IsWindowEnabled(b))
        throw std::runtime_error("Missing expected button");
    PostMessageW(dlg, WM_COMMAND, MAKEWPARAM(id, BN_CLICKED), reinterpret_cast<LPARAM>(b));
}
void enterFile(const wchar_t *name) {
    auto picker = waitFor(
        [](HWND h) {
            return IsWindowEnabled(h) && GetDlgItem(h, 1008) && GetDlgItem(h, 1047) && GetDlgItem(h, 1);
        },
        "Expected PTDIAG file browser (dialog 136)");
    DWORD_PTR result;
    if (!SendMessageTimeoutW(GetDlgItem(picker, 1008), WM_SETTEXT, 0, reinterpret_cast<LPARAM>(name),
                             SMTO_ABORTIFHUNG, 1000, &result))
        throw std::runtime_error("Cannot enter filename");
    click(picker, 1);
    waitFor([=](HWND) { return !IsWindowVisible(picker); }, "File browser did not accept fixture");
}
void mxConfig(const std::wstring &profile) {
    struct Shape {
        const wchar_t *name;
        unsigned menu, address, size;
    };
    const Shape shapes[] = {{L"vid128", 40054, 0x100c0, 128},
                            {L"trid256", 40055, 0x10140, 256},
                            {L"vidtrid384", 40059, 0x100c0, 384},
                            {L"trid128", 40064, 0x10140, 128}};
    auto shape = std::find_if(std::begin(shapes), std::end(shapes), [&](auto &s) { return profile == s.name; });
    if (shape == std::end(shapes))
        throw std::runtime_error("Unknown configuration-block profile");
    const auto output = L"Sample.VBF";
    if (std::filesystem::exists(L"Sample.h32") || std::filesystem::exists(output))
        throw std::runtime_error("Use a new reference directory");
    std::ofstream input("Sample.h32", std::ios::binary);
    input << ":020000040001F9\r\n" << std::hex << std::uppercase << std::setfill('0');
    for (unsigned i = 0; i < shape->size; i += 16) {
        unsigned addr = (shape->address + i) & 65535, sum = 16 + (addr >> 8) + (addr & 255);
        input << ":10" << std::setw(4) << addr << "00";
        for (unsigned j = 0; j < 16; ++j) {
            unsigned b = (i + j) & 255;
            sum += b;
            input << std::setw(2) << b;
        }
        input << std::setw(2) << ((-sum) & 255) << "\r\n";
    }
    input << ":00000001FF\r\n";
    input.close();
    if (!input)
        throw std::runtime_error("Write fixture failed");
    auto main = waitFor(
        [=](HWND h) {
            return IsWindowEnabled(h) && (menuHas(GetMenu(h), shape->menu) || caption(h) == L"MX32");
        },
        "MX32 window unavailable");
    PostMessageW(main, WM_COMMAND, shape->menu, 0);
    PostMessageW(main, WM_COMMAND, 40051, 0);
    auto picker = waitFor(
        [](HWND h) {
            return IsWindowEnabled(h) && GetDlgItem(h, 1000) && GetDlgItem(h, 1015) &&
                   caption(GetDlgItem(h, 1)) == L"Open";
        },
        "MX32 input browser unavailable");
    DWORD_PTR result;
    if (!SendMessageTimeoutW(GetDlgItem(picker, 1000), WM_SETTEXT, 0, reinterpret_cast<LPARAM>(L"Sample.h32"),
                             SMTO_ABORTIFHUNG, 1000, &result))
        throw std::runtime_error("Cannot enter input file");
    click(picker, 1);
    auto save = waitFor(
        [=](HWND h) {
            return IsWindowEnabled(h) && caption(GetDlgItem(h, 1)) == L"Save" &&
                   _wcsicmp(caption(GetDlgItem(h, 1000)).c_str(), output) == 0;
        },
        "MX32 output browser unavailable");
    click(save, 1);
    auto metadata = waitFor([](HWND h) { return caption(h) == L"Input Hex File Data" && GetDlgItem(h, 1024); },
                            "MX32 metadata unavailable");
    click(metadata, 1);
    waitFor(
        [](HWND h) {
            return caption(h) == L"MX32" && caption(GetDlgItem(h, 65535)).find(L"Process complete.") == 0;
        },
        "MX32 conversion did not complete");
    if (!std::filesystem::exists(output) || !std::filesystem::file_size(output))
        throw std::runtime_error("MX32 output missing");
}
} // namespace
int wmain(int argc, wchar_t **argv) {
    bool mx = argc == 4 && std::wstring(argv[2]) == L"--mx-config";
    bool kam = argc == 3 && std::wstring(argv[2]) == L"--kam";
    const auto input = kam ? L"KAM.cfg" : L"RPM-VBAT.cfg";
    if (!mx &&
        ((argc != 2 && !kam) || !std::filesystem::exists(input) || std::filesystem::exists(L"RoundTrip.cfg"))) {
        std::cerr << "Run in a fresh scratch directory with the input configuration; pass the reference EXE "
                     "and optionally --kam\n";
        return 2;
    }
    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION proc{};
    if (!CreateProcessW(argv[1], nullptr, nullptr, nullptr, FALSE, 0, nullptr, nullptr, &startup, &proc)) {
        std::cerr << "Cannot start reference\n";
        return 1;
    }
    pid = proc.dwProcessId;
    CloseHandle(proc.hThread);
    int status = 0;
    try {
        WaitForInputIdle(proc.hProcess, 20000);
        if (mx)
            mxConfig(argv[3]);
        else {
            auto main = byMenu(40031);
            if (kam) {
                PostMessageW(main, WM_COMMAND, 40365, 0);
                auto panel = waitFor(
                    [](HWND h) {
                        return IsWindowEnabled(h) && caption(GetDlgItem(h, 1018)) == L"Load Config File" &&
                               caption(GetDlgItem(h, 1020)) == L"Save Config File";
                    },
                    "Expected KAM config window");
                click(panel, 1018);
                enterFile(input);
                click(panel, 1020);
                auto confirm = waitFor(
                    [](HWND h) {
                        return IsWindowEnabled(h) && caption(h) == L"Write these KAM regions to file?" &&
                               caption(GetDlgItem(h, 1)) == L"OK";
                    },
                    "Expected save-config confirmation");
                click(confirm, 1);
            } else {
                std::wcout << L"Found main Data Acquisition menu\n" << std::flush;
                PostMessageW(main, WM_COMMAND, 40031, 0);
                auto acq = byMenu(40037);
                std::wcout << L"Acquisition window: " << caption(acq) << std::endl;
                PostMessageW(acq, WM_COMMAND, 40037, 0);
                enterFile(L"RPM-VBAT.cfg");
                acq = byMenu(40036);
                PostMessageW(acq, WM_COMMAND, 40036, 0);
            }
            enterFile(L"RoundTrip.cfg");
            waitFor(
                [](HWND) {
                    return std::filesystem::exists(L"RoundTrip.cfg") &&
                           std::filesystem::file_size(L"RoundTrip.cfg") > 0;
                },
                "Original did not write the configuration");
        }
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        status = 1;
    }
    // Only the reference process started here, running in its disposable working directory.
    TerminateProcess(proc.hProcess, 0);
    WaitForSingleObject(proc.hProcess, 3000);
    CloseHandle(proc.hProcess);
    return status;
}
