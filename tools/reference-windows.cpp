// Offline acceptance probe: drive the two public file menus of our private PTDIAG copy.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

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
} // namespace
int wmain(int argc, wchar_t **argv) {
    if (argc != 2 || !std::filesystem::exists(L"RPM-VBAT.cfg") || std::filesystem::exists(L"RoundTrip.cfg")) {
        std::cerr << "Run in a fresh scratch directory with RPM-VBAT.cfg; pass the path to the reference EXE\n";
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
        auto main = byMenu(40031);
        std::wcout << L"Found main Data Acquisition menu\n" << std::flush;
        PostMessageW(main, WM_COMMAND, 40031, 0);
        auto acq = byMenu(40037);
        std::wcout << L"Acquisition window: " << caption(acq) << std::endl;
        PostMessageW(acq, WM_COMMAND, 40037, 0);
        enterFile(L"RPM-VBAT.cfg");
        acq = byMenu(40036);
        PostMessageW(acq, WM_COMMAND, 40036, 0);
        enterFile(L"RoundTrip.cfg");
        waitFor(
            [](HWND) {
                return std::filesystem::exists(L"RoundTrip.cfg") &&
                       std::filesystem::file_size(L"RoundTrip.cfg") > 0;
            },
            "Original did not write the configuration");
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
