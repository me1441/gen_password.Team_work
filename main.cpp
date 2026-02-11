#include <windows.h>
#include <commctrl.h>
#include <gdiplus.h>
#include <string>
#include <vector>
#include <random>
#include <ctime>
#include <cstring>

#pragma GCC diagnostic ignored "-Wwrite-strings"
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(linker, "/SUBSYSTEM:WINDOWS")
#pragma comment(linker, "/ENTRY:WinMainCRTStartup")

using namespace Gdiplus;

#define ID_BTN_GENERATE   101
#define ID_BTN_COPY       102
#define ID_BTN_SAVE       103
#define ID_CHECK_UPPER    104
#define ID_CHECK_LOWER    105
#define ID_CHECK_DIGITS   106
#define ID_CHECK_SYMBOLS  107
#define ID_EDIT_LENGTH    108
#define ID_EDIT_MANUAL    109
#define ID_BTN_CHECK      110
#define ID_TAB_CONTROL    111
#define ID_LIST_SAVED     112
#define ID_BTN_DELETE     113
#define ID_BTN_COPY_SAVED 114
#define ID_SPIN_LENGTH    115

#define WIN_WIDTH         500
#define WIN_HEIGHT        450

#define GEN_BTN_X         30
#define GEN_BTN_Y         320
#define GEN_BTN_WIDTH     120
#define GEN_BTN_HEIGHT    40

#define COPY_BTN_X        195
#define COPY_BTN_Y        320
#define COPY_BTN_WIDTH    120
#define COPY_BTN_HEIGHT   40

#define SAVE_BTN_X        350
#define SAVE_BTN_Y        320
#define SAVE_BTN_WIDTH    120
#define SAVE_BTN_HEIGHT   40

#define CHECK_BTN_X       350
#define CHECK_BTN_Y       245
#define CHECK_BTN_WIDTH   120
#define CHECK_BTN_HEIGHT  40

// Цвета
#define EDIT_BG_COLOR     RGB(38, 38, 38)
#define EDIT_TEXT_COLOR   RGB(230, 230, 230)
#define PANEL_BG_COLOR    RGB(38, 38, 38)
#define PANEL_BORDER_COLOR RGB(180, 180, 180)

struct SavedPassword {
    std::string password;
    std::string strength;
    SYSTEMTIME time;
};

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
void UpdateSavedList();
void SwitchTab(HWND hwnd, int tabIndex);

HWND g_hEditLength = NULL;
HWND g_hSpinLength = NULL;
HWND g_hCheckUpper = NULL;
HWND g_hCheckLower = NULL;
HWND g_hCheckDigits = NULL;
HWND g_hCheckSymbols = NULL;
HWND g_hEditManual = NULL;
HWND g_hTabControl = NULL;
HWND g_hListSaved = NULL;
HWND g_hBtnDelete = NULL;
HWND g_hBtnCopySaved = NULL;
HWND g_hStaticLength = NULL;

std::string g_currentPassword;
std::string g_currentStrength;
std::vector<SavedPassword> g_savedPasswords;

Image* g_pBackground = NULL;
Image* g_pBtnGenerate = NULL;
Image* g_pBtnCopy = NULL;
Image* g_pBtnSave = NULL;
Image* g_pBtnCheck = NULL;

ULONG_PTR g_gdiplusToken = 0;

bool g_genHovered = false;
bool g_copyHovered = false;
bool g_saveHovered = false;
bool g_checkHovered = false;

int g_currentTab = 0;

// Шрифты
HFONT g_hFontTitle = NULL;
HFONT g_hFontLabel = NULL;      // Увеличенный для меток
HFONT g_hFontNormal = NULL;
HFONT g_hFontBold = NULL;
HFONT g_hFontPassword = NULL;

void InitGdiplus()
{
    GdiplusStartupInput input;
    ZeroMemory(&input, sizeof(input));
    input.GdiplusVersion = 1;
    GdiplusStartup(&g_gdiplusToken, &input, NULL);
}

void ShutdownGdiplus()
{
    if (g_pBackground) { delete g_pBackground; g_pBackground = NULL; }
    if (g_pBtnGenerate) { delete g_pBtnGenerate; g_pBtnGenerate = NULL; }
    if (g_pBtnCopy) { delete g_pBtnCopy; g_pBtnCopy = NULL; }
    if (g_pBtnSave) { delete g_pBtnSave; g_pBtnSave = NULL; }
    if (g_pBtnCheck) { delete g_pBtnCheck; g_pBtnCheck = NULL; }
    GdiplusShutdown(g_gdiplusToken);
}

void CreateFonts()
{
    // Заголовок - поднят выше, не должен лезть на панель
    g_hFontTitle = CreateFontA(24, 0, 0, 0, FW_SEMIBOLD, 0, 0, 0, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_SWISS, "Segoe UI");

    // Метки (Длина, Ручной ввод, Сила) - увеличены
    g_hFontLabel = CreateFontA(16, 0, 0, 0, FW_SEMIBOLD, 0, 0, 0, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_SWISS, "Segoe UI");

    // Обычный текст
    g_hFontNormal = CreateFontA(14, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_SWISS, "Segoe UI");

    // Жирный текст
    g_hFontBold = CreateFontA(14, 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_SWISS, "Segoe UI");

    // Моноширинный для паролей
    g_hFontPassword = CreateFontA(20, 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_MODERN, "Consolas");
}

void DeleteFonts()
{
    if (g_hFontTitle) DeleteObject(g_hFontTitle);
    if (g_hFontLabel) DeleteObject(g_hFontLabel);
    if (g_hFontNormal) DeleteObject(g_hFontNormal);
    if (g_hFontBold) DeleteObject(g_hFontBold);
    if (g_hFontPassword) DeleteObject(g_hFontPassword);
}

Image* LoadPng(const wchar_t* filename)
{
    Image* img = Image::FromFile(filename);
    if (img && img->GetLastStatus() == Ok) return img;
    if (img) delete img;
    return NULL;
}

void LoadImages()
{
    g_pBackground = LoadPng(L"fon2.png");
    g_pBtnGenerate = LoadPng(L"generate2.png");
    g_pBtnCopy = LoadPng(L"copy.png");
    g_pBtnSave = LoadPng(L"save.png");
    g_pBtnCheck = LoadPng(L"check.png");
}

void DrawPngButton(HDC hdc, int x, int y, int w, int h, Image* img, bool hovered)
{
    if (!img) {
        RECT rc = { x, y, x + w, y + h };
        FillRect(hdc, &rc, (HBRUSH)GetStockObject(GRAY_BRUSH));
        FrameRect(hdc, &rc, (HBRUSH)GetStockObject(BLACK_BRUSH));
        return;
    }

    Graphics g(hdc);
    g.SetInterpolationMode(InterpolationModeHighQualityBicubic);
    g.DrawImage(img, x, y, w, h);

    if (hovered) {
        SolidBrush brush(Color(40, 38, 74, 255));
        Rect rect(x, y, w, h);
        g.FillRectangle(&brush, rect);
    }
}

std::string GeneratePassword()
{
    const char* upper = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    const char* lower = "abcdefghijklmnopqrstuvwxyz";
    const char* digits = "0123456789";
    const char* symbols = "!@#$%^&*()_+-=[]{}|;:,.<>?";

    std::string chars;
    HWND parent = GetParent(g_hCheckUpper);

    if (IsDlgButtonChecked(parent, ID_CHECK_UPPER) == BST_CHECKED) chars += upper;
    if (IsDlgButtonChecked(parent, ID_CHECK_LOWER) == BST_CHECKED) chars += lower;
    if (IsDlgButtonChecked(parent, ID_CHECK_DIGITS) == BST_CHECKED) chars += digits;
    if (IsDlgButtonChecked(parent, ID_CHECK_SYMBOLS) == BST_CHECKED) chars += symbols;
    if (chars.empty()) chars = lower;

    int length = (int)SendMessage(g_hSpinLength, UDM_GETPOS, 0, 0);
    if (length < 4) length = 4;
    if (length > 128) length = 128;

    static std::mt19937 gen(static_cast<unsigned>(time(NULL)));
    std::uniform_int_distribution<> dis(0, (int)chars.size() - 1);

    std::string pwd;
    pwd.reserve(length);
    for (int i = 0; i < length; i++) pwd += chars[dis(gen)];
    return pwd;
}

std::string CheckStrength(const std::string& pwd)
{
    if (pwd.empty()) return "";

    int score = (pwd.length() >= 8) + (pwd.length() >= 12) + (pwd.length() >= 16);

    bool hu = false, hl = false, hd = false, hs = false;
    for (size_t i = 0; i < pwd.length(); i++) {
        unsigned char c = pwd[i];
        if (isupper(c)) hu = true;
        else if (islower(c)) hl = true;
        else if (isdigit(c)) hd = true;
        else hs = true;
    }
    score += hu + hl + hd + hs;

    if (score <= 2) return "Слабый";
    if (score <= 4) return "Средний";
    if (score <= 6) return "Хороший";
    return "Надежный";
}

COLORREF GetStrengthColor(const std::string& strength)
{
    if (strength == "Слабый") return RGB(243, 139, 168);
    if (strength == "Средний") return RGB(250, 179, 135);
    if (strength == "Хороший") return RGB(249, 226, 175);
    return RGB(166, 227, 161);
}

void UpdatePassword(HWND hwnd)
{
    g_currentPassword = GeneratePassword();
    g_currentStrength = CheckStrength(g_currentPassword);
    SetWindowTextA(g_hEditManual, g_currentPassword.c_str());
    InvalidateRect(hwnd, NULL, TRUE);
}

void CopyToClipboard(HWND hwnd, const std::string& text)
{
    if (text.empty() || !OpenClipboard(hwnd)) return;

    EmptyClipboard();
    size_t len = text.length() + 1;
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, len);
    if (hMem) {
        char* ptr = (char*)GlobalLock(hMem);
        if (ptr) {
            memcpy(ptr, text.c_str(), len);
            GlobalUnlock(hMem);
            SetClipboardData(CF_TEXT, hMem);
        }
    }
    CloseClipboard();
    SetTimer(hwnd, 1, 500, NULL);
}

void UpdateSavedList()
{
    if (!g_hListSaved) return;

    SendMessageA(g_hListSaved, LB_RESETCONTENT, 0, 0);

    for (size_t i = 0; i < g_savedPasswords.size(); i++) {
        char buf[256];
        sprintf_s(buf, sizeof(buf), "[%02d:%02d] %s (%s)",
            g_savedPasswords[i].time.wHour,
            g_savedPasswords[i].time.wMinute,
            g_savedPasswords[i].password.c_str(),
            g_savedPasswords[i].strength.c_str());
        SendMessageA(g_hListSaved, LB_ADDSTRING, 0, (LPARAM)buf);
    }
}

void SaveCurrentPassword(HWND hwnd)
{
    if (g_currentPassword.empty()) return;

    SavedPassword sp;
    sp.password = g_currentPassword;
    sp.strength = g_currentStrength;
    GetLocalTime(&sp.time);

    g_savedPasswords.push_back(sp);
    UpdateSavedList();
    MessageBoxA(hwnd, "Пароль сохранен!", "Успех", MB_OK | MB_ICONINFORMATION);
}

void DeleteSelectedPassword()
{
    if (!g_hListSaved) return;

    int sel = (int)SendMessageA(g_hListSaved, LB_GETCURSEL, 0, 0);
    if (sel >= 0 && sel < (int)g_savedPasswords.size()) {
        g_savedPasswords.erase(g_savedPasswords.begin() + sel);
        UpdateSavedList();
    }
}

void CopySelectedPassword(HWND hwnd)
{
    if (!g_hListSaved) return;

    int sel = (int)SendMessageA(g_hListSaved, LB_GETCURSEL, 0, 0);
    if (sel >= 0 && sel < (int)g_savedPasswords.size()) {
        CopyToClipboard(hwnd, g_savedPasswords[sel].password);
    }
}

void CheckManualPassword(HWND hwnd)
{
    char buf[256] = { 0 };
    GetWindowTextA(g_hEditManual, buf, 255);

    std::string pwd(buf);
    std::string strength = CheckStrength(pwd);

    g_currentPassword = pwd;
    g_currentStrength = strength;

    InvalidateRect(hwnd, NULL, TRUE);
}

void DrawMain(HWND hwnd, HDC hdc)
{
    RECT rc;
    GetClientRect(hwnd, &rc);
    int w = rc.right;
    int h = rc.bottom;

    if (g_pBackground) {
        Graphics g(hdc);
        g.DrawImage(g_pBackground, 0, 0, w, h);
    }
    else {
        HBRUSH br = CreateSolidBrush(RGB(30, 30, 46));
        FillRect(hdc, &rc, br);
        DeleteObject(br);
    }

    if (g_currentTab == 0) {
        // Серая панель настроек - поднята ниже, чтобы заголовок не лез
        RECT settingsPanel = { 20, 155, w - 20, 195 };
        {
            Graphics g(hdc);
            SolidBrush panelBrush(Color(38, 38, 38));
            Rect panelRect(settingsPanel.left, settingsPanel.top,
                settingsPanel.right - settingsPanel.left,
                settingsPanel.bottom - settingsPanel.top);
            g.FillRectangle(&panelBrush, panelRect);

            Pen panelPen(Color(38, 74, 255), 1);
            g.DrawRectangle(&panelPen, panelRect);
        }

        // Поле сгенерированного пароля
        RECT pwdRc = { 30, 80, w - 30, 120 };
        {
            Graphics g(hdc);
            SolidBrush brush(Color(38, 38, 38));
            Rect r(pwdRc.left, pwdRc.top, pwdRc.right - pwdRc.left, pwdRc.bottom - pwdRc.top);
            g.FillRectangle(&brush, r);
        }

        HBRUSH frame = CreateSolidBrush(RGB(38, 74, 255));
        FrameRect(hdc, &pwdRc, frame);
        DeleteObject(frame);

        SetBkMode(hdc, TRANSPARENT);

        // Заголовок - поднят выше (y=45), чтобы не лез на панель
        SetTextColor(hdc, RGB(0, 0, 0));
        SelectObject(hdc, g_hFontTitle);
        RECT tr = { 2, 47, w + 2, 72 };
        DrawTextA(hdc, "Генератор паролей", -1, &tr, DT_CENTER | DT_SINGLELINE);

        SetTextColor(hdc, RGB(38, 74, 255));
        tr = { 0, 45, w, 70 };
        DrawTextA(hdc, "Генератор паролей", -1, &tr, DT_CENTER | DT_SINGLELINE);

        // Пароль
        SetTextColor(hdc, RGB(230, 230, 230));
        SelectObject(hdc, g_hFontPassword);
        DrawTextA(hdc, g_currentPassword.c_str(), -1, &pwdRc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        // Сила пароля - увеличенный шрифт
        if (!g_currentStrength.empty()) {
            SetTextColor(hdc, GetStrengthColor(g_currentStrength));
            SelectObject(hdc, g_hFontLabel);  // Увеличенный шрифт
            RECT sr = { 30, 125, w - 30, 150 };
            std::string txt = "Сила: " + g_currentStrength;
            DrawTextA(hdc, txt.c_str(), -1, &sr, DT_CENTER | DT_SINGLELINE);
        }

        // Метка ручного ввода - увеличенный шрифт
        SetTextColor(hdc, RGB(220, 220, 220));
        SelectObject(hdc, g_hFontLabel);  // Увеличенный шрифт
        RECT manualLabel = { 30, 220, 200, 245 };
        DrawTextA(hdc, "Ручной ввод:", -1, &manualLabel, DT_LEFT | DT_SINGLELINE);

        // Кнопки PNG
        DrawPngButton(hdc, GEN_BTN_X, GEN_BTN_Y, GEN_BTN_WIDTH, GEN_BTN_HEIGHT,
            g_pBtnGenerate, g_genHovered);
        DrawPngButton(hdc, COPY_BTN_X, COPY_BTN_Y, COPY_BTN_WIDTH, COPY_BTN_HEIGHT,
            g_pBtnCopy, g_copyHovered);
        DrawPngButton(hdc, SAVE_BTN_X, SAVE_BTN_Y, SAVE_BTN_WIDTH, SAVE_BTN_HEIGHT,
            g_pBtnSave, g_saveHovered);
        DrawPngButton(hdc, CHECK_BTN_X, CHECK_BTN_Y, CHECK_BTN_WIDTH, CHECK_BTN_HEIGHT,
            g_pBtnCheck, g_checkHovered);
    }


}

void SwitchTab(HWND hwnd, int tabIndex)
{
    g_currentTab = tabIndex;

    ShowWindow(g_hStaticLength, tabIndex == 0 ? SW_SHOW : SW_HIDE);
    ShowWindow(g_hEditLength, tabIndex == 0 ? SW_SHOW : SW_HIDE);
    ShowWindow(g_hSpinLength, tabIndex == 0 ? SW_SHOW : SW_HIDE);
    ShowWindow(g_hCheckUpper, tabIndex == 0 ? SW_SHOW : SW_HIDE);
    ShowWindow(g_hCheckLower, tabIndex == 0 ? SW_SHOW : SW_HIDE);
    ShowWindow(g_hCheckDigits, tabIndex == 0 ? SW_SHOW : SW_HIDE);
    ShowWindow(g_hCheckSymbols, tabIndex == 0 ? SW_SHOW : SW_HIDE);
    ShowWindow(g_hEditManual, tabIndex == 0 ? SW_SHOW : SW_HIDE);

    ShowWindow(g_hListSaved, tabIndex == 1 ? SW_SHOW : SW_HIDE);
    ShowWindow(g_hBtnDelete, tabIndex == 1 ? SW_SHOW : SW_HIDE);
    ShowWindow(g_hBtnCopySaved, tabIndex == 1 ? SW_SHOW : SW_HIDE);

    InvalidateRect(hwnd, NULL, TRUE);
}

void InitTabControl(HWND hwnd)
{
    g_hTabControl = CreateWindowW(L"SysTabControl32", L"",
        WS_CHILD | WS_VISIBLE | TCS_TABS,
        0, 0, WIN_WIDTH, 30, hwnd, (HMENU)ID_TAB_CONTROL,
        GetModuleHandle(NULL), NULL);

    TCITEMW tie;
    tie.mask = TCIF_TEXT;

    tie.pszText = (LPWSTR)L"Generate";
    TabCtrl_InsertItem(g_hTabControl, 0, &tie);

    tie.pszText = (LPWSTR)L"Saved";
    TabCtrl_InsertItem(g_hTabControl, 1, &tie);
}

bool IsPointInRect(int x, int y, int rx, int ry, int rw, int rh)
{
    return (x >= rx && x <= rx + rw && y >= ry && y <= ry + rh);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg) {
    case WM_CREATE: {
        HINSTANCE hi = GetModuleHandle(NULL);

        InitGdiplus();
        CreateFonts();
        LoadImages();
        InitTabControl(hwnd);

        // Статик "Длина:" - увеличенный шрифт
        g_hStaticLength = CreateWindowA("static", "Длина:",
            WS_VISIBLE | WS_CHILD | SS_CENTERIMAGE,
            30, 162, 55, 24, hwnd, NULL, hi, NULL);
        SendMessage(g_hStaticLength, WM_SETFONT, (WPARAM)g_hFontLabel, TRUE);

        // Поле длины - по центру панели
        g_hEditLength = CreateWindowA("edit", "16",
            WS_VISIBLE | WS_CHILD | ES_CENTER | ES_READONLY,
            90, 162, 45, 24, hwnd, (HMENU)ID_EDIT_LENGTH, hi, NULL);
        SendMessage(g_hEditLength, WM_SETFONT, (WPARAM)g_hFontBold, TRUE);

        // Up-Down Control - стрелочки по центру поля длины
        // UDS_ALIGNRIGHT размещает стрелочки справа от buddy-окна
        g_hSpinLength = CreateWindowA(UPDOWN_CLASSA, "",
            WS_CHILD | WS_VISIBLE | UDS_SETBUDDYINT | UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_NOTHOUSANDS,
            0, 0, 0, 0, hwnd, (HMENU)ID_SPIN_LENGTH, hi, NULL);
        SendMessage(g_hSpinLength, UDM_SETBUDDY, (WPARAM)g_hEditLength, 0);
        SendMessage(g_hSpinLength, UDM_SETRANGE, 0, MAKELPARAM(128, 4));
        SendMessage(g_hSpinLength, UDM_SETPOS, 0, 16);

        // Чекбоксы - подвинуты ниже на новую панель
        g_hCheckUpper = CreateWindowA("button", "ABC",
            WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX | BS_VCENTER,
            150, 164, 60, 22, hwnd, (HMENU)ID_CHECK_UPPER, hi, NULL);
        SendMessage(g_hCheckUpper, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);
        SendMessage(g_hCheckUpper, BM_SETCHECK, BST_CHECKED, 0);

        g_hCheckLower = CreateWindowA("button", "abc",
            WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX | BS_VCENTER,
            220, 164, 60, 22, hwnd, (HMENU)ID_CHECK_LOWER, hi, NULL);
        SendMessage(g_hCheckLower, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);
        SendMessage(g_hCheckLower, BM_SETCHECK, BST_CHECKED, 0);

        g_hCheckDigits = CreateWindowA("button", "123",
            WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX | BS_VCENTER,
            290, 164, 60, 22, hwnd, (HMENU)ID_CHECK_DIGITS, hi, NULL);
        SendMessage(g_hCheckDigits, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);
        SendMessage(g_hCheckDigits, BM_SETCHECK, BST_CHECKED, 0);

        g_hCheckSymbols = CreateWindowA("button", "!@#",
            WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX | BS_VCENTER,
            360, 164, 60, 22, hwnd, (HMENU)ID_CHECK_SYMBOLS, hi, NULL);
        SendMessage(g_hCheckSymbols, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);
        SendMessage(g_hCheckSymbols, BM_SETCHECK, BST_CHECKED, 0);

        // Поле ручного ввода - подвинуто ниже
        g_hEditManual = CreateWindowA("edit", "",
            WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
            30, 250, 310, 30, hwnd, (HMENU)ID_EDIT_MANUAL, hi, NULL);
        SendMessage(g_hEditManual, WM_SETFONT, (WPARAM)g_hFontPassword, TRUE);

        // Кнопка проверки (невидимая)
        CreateWindowA("button", "Проверить", WS_CHILD,
            CHECK_BTN_X, CHECK_BTN_Y + 10, CHECK_BTN_WIDTH, CHECK_BTN_HEIGHT,
            hwnd, (HMENU)ID_BTN_CHECK, hi, NULL);

        // Список сохраненных
        g_hListSaved = CreateWindowA("listbox", "",
            WS_CHILD | WS_VISIBLE | LBS_NOTIFY | WS_VSCROLL | LBS_HASSTRINGS,
            20, 50, WIN_WIDTH - 40, 280, hwnd, (HMENU)ID_LIST_SAVED, hi, NULL);
        SendMessage(g_hListSaved, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);
        ShowWindow(g_hListSaved, SW_HIDE);

        g_hBtnDelete = CreateWindowA("button", "Удалить",
            WS_CHILD | BS_PUSHBUTTON,
            50, 340, 100, 35, hwnd, (HMENU)ID_BTN_DELETE, hi, NULL);
        SendMessage(g_hBtnDelete, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);
        ShowWindow(g_hBtnDelete, SW_HIDE);

        g_hBtnCopySaved = CreateWindowA("button", "Копировать",
            WS_CHILD | BS_PUSHBUTTON,
            200, 340, 100, 35, hwnd, (HMENU)ID_BTN_COPY_SAVED, hi, NULL);
        SendMessage(g_hBtnCopySaved, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);
        ShowWindow(g_hBtnCopySaved, SW_HIDE);

        UpdatePassword(hwnd);
        return 0;
    }

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        DrawMain(hwnd, hdc);
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_NOTIFY: {
        if (((LPNMHDR)lp)->idFrom == ID_TAB_CONTROL) {
            if (((LPNMHDR)lp)->code == TCN_SELCHANGE) {
                int sel = TabCtrl_GetCurSel(g_hTabControl);
                SwitchTab(hwnd, sel);
            }
        }
        return 0;
    }

    case WM_VSCROLL:
        if ((HWND)lp == g_hSpinLength) {
            UpdatePassword(hwnd);
        }
        return 0;

    case WM_MOUSEMOVE: {
        if (g_currentTab != 0) break;

        int x = LOWORD(lp);
        int y = HIWORD(lp);

        bool oldGen = g_genHovered;
        bool oldCopy = g_copyHovered;
        bool oldSave = g_saveHovered;
        bool oldCheck = g_checkHovered;

        g_genHovered = IsPointInRect(x, y, GEN_BTN_X, GEN_BTN_Y, GEN_BTN_WIDTH, GEN_BTN_HEIGHT);
        g_copyHovered = IsPointInRect(x, y, COPY_BTN_X, COPY_BTN_Y, COPY_BTN_WIDTH, COPY_BTN_HEIGHT);
        g_saveHovered = IsPointInRect(x, y, SAVE_BTN_X, SAVE_BTN_Y, SAVE_BTN_WIDTH, SAVE_BTN_HEIGHT);
        g_checkHovered = IsPointInRect(x, y, CHECK_BTN_X, CHECK_BTN_Y + 10, CHECK_BTN_WIDTH, CHECK_BTN_HEIGHT);

        if (oldGen != g_genHovered || oldCopy != g_copyHovered ||
            oldSave != g_saveHovered || oldCheck != g_checkHovered) {
            InvalidateRect(hwnd, NULL, FALSE);
        }

        if (g_genHovered || g_copyHovered || g_saveHovered || g_checkHovered) {
            TRACKMOUSEEVENT tme;
            ZeroMemory(&tme, sizeof(tme));
            tme.cbSize = sizeof(TRACKMOUSEEVENT);
            tme.dwFlags = TME_LEAVE;
            tme.hwndTrack = hwnd;
            TrackMouseEvent(&tme);
        }
        break;
    }

    case WM_MOUSELEAVE: {
        if (g_currentTab == 0 && (g_genHovered || g_copyHovered || g_saveHovered || g_checkHovered)) {
            g_genHovered = false;
            g_copyHovered = false;
            g_saveHovered = false;
            g_checkHovered = false;
            InvalidateRect(hwnd, NULL, FALSE);
        }
        break;
    }

    case WM_LBUTTONDOWN: {
        if (g_currentTab != 0) break;

        int x = LOWORD(lp);
        int y = HIWORD(lp);

        if (IsPointInRect(x, y, GEN_BTN_X, GEN_BTN_Y, GEN_BTN_WIDTH, GEN_BTN_HEIGHT)) {
            UpdatePassword(hwnd);
            return 0;
        }
        if (IsPointInRect(x, y, COPY_BTN_X, COPY_BTN_Y, COPY_BTN_WIDTH, COPY_BTN_HEIGHT)) {
            CopyToClipboard(hwnd, g_currentPassword);
            return 0;
        }
        if (IsPointInRect(x, y, SAVE_BTN_X, SAVE_BTN_Y, SAVE_BTN_WIDTH, SAVE_BTN_HEIGHT)) {
            SaveCurrentPassword(hwnd);
            return 0;
        }
        if (IsPointInRect(x, y, CHECK_BTN_X, CHECK_BTN_Y + 10, CHECK_BTN_WIDTH, CHECK_BTN_HEIGHT)) {
            CheckManualPassword(hwnd);
            return 0;
        }
        break;
    }

    case WM_COMMAND:
        switch (LOWORD(wp)) {
        case ID_BTN_CHECK:
            CheckManualPassword(hwnd);
            break;
        case ID_BTN_DELETE:
            DeleteSelectedPassword();
            break;
        case ID_BTN_COPY_SAVED:
            CopySelectedPassword(hwnd);
            break;
        case ID_CHECK_UPPER:
        case ID_CHECK_LOWER:
        case ID_CHECK_DIGITS:
        case ID_CHECK_SYMBOLS:
            UpdatePassword(hwnd);
            break;
        case ID_LIST_SAVED:
            if (HIWORD(wp) == LBN_DBLCLK) {
                CopySelectedPassword(hwnd);
            }
            break;
        }
        return 0;

    case WM_CTLCOLOREDIT: {
        HDC hdc = (HDC)wp;
        SetBkColor(hdc, EDIT_BG_COLOR);
        SetTextColor(hdc, EDIT_TEXT_COLOR);
        static HBRUSH br = CreateSolidBrush(EDIT_BG_COLOR);
        return (LRESULT)br;
    }

    case WM_CTLCOLORSTATIC: {
        HDC hdc = (HDC)wp;
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(230, 230, 230));
        SelectObject(hdc, g_hFontLabel);
        return (LRESULT)GetStockObject(NULL_BRUSH);
    }

    case WM_CTLCOLORBTN: {
        HDC hdc = (HDC)wp;
        SetBkMode(hdc, TRANSPARENT);
        return (LRESULT)GetStockObject(NULL_BRUSH);
    }

    case WM_TIMER:
        InvalidateRect(hwnd, NULL, TRUE);
        KillTimer(hwnd, 1);
        return 0;

    case WM_DESTROY:
        DeleteFonts();
        ShutdownGdiplus();
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcA(hwnd, msg, wp, lp);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPSTR lpCmdLine, int nCmdShow)
{
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);

    HANDLE hMutex = CreateMutexA(NULL, TRUE, "GeneratorPasswordMutex_v2");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        MessageBoxA(NULL, "Программа уже запущена!", "Ошибка", MB_OK);
        return 1;
    }

    INITCOMMONCONTROLSEX iccex;
    iccex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    iccex.dwICC = ICC_TAB_CLASSES | ICC_UPDOWN_CLASS;
    InitCommonControlsEx(&iccex);

    WNDCLASSEXA wc;
    ZeroMemory(&wc, sizeof(wc));
    wc.cbSize = sizeof(WNDCLASSEXA);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
    wc.lpszClassName = "PwdGen";

    if (!RegisterClassExA(&wc)) {
        MessageBoxA(NULL, "Ошибка регистрации класса!", "Ошибка", MB_OK);
        return 1;
    }

    int sx = GetSystemMetrics(SM_CXSCREEN);
    int sy = GetSystemMetrics(SM_CYSCREEN);
    HWND hwnd = CreateWindowExA(WS_EX_COMPOSITED, "PwdGen", "Генератор паролей",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        (sx - WIN_WIDTH) / 2, (sy - WIN_HEIGHT) / 2, WIN_WIDTH, WIN_HEIGHT, NULL, NULL, hInstance, NULL);

    if (!hwnd) {
        MessageBoxA(NULL, "Ошибка создания окна!", "Ошибка", MB_OK);
        return 1;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessageA(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    ReleaseMutex(hMutex);
    CloseHandle(hMutex);
    return (int)msg.wParam;
}
