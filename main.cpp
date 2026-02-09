#include <windows.h>
#include <string>
#include <random>
#include <ctime>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "kernel32.lib")

#define ID_BTN_GENERATE     101
#define ID_BTN_COPY         102
#define ID_CHECK_UPPER      103
#define ID_CHECK_LOWER      104
#define ID_CHECK_DIGITS     105
#define ID_CHECK_SYMBOLS    106
#define ID_EDIT_LENGTH      107

HWND hEditLength = NULL;
HWND hCheckUpper = NULL;
HWND hCheckLower = NULL;
HWND hCheckDigits = NULL;
HWND hCheckSymbols = NULL;

std::string currentPassword;
std::string currentStrength;

std::string GeneratePassword() {
    const char* upper = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    const char* lower = "abcdefghijklmnopqrstuvwxyz";
    const char* digits = "0123456789";
    const char* symbols = "!@#$%^&*()_+-=[]{}|;:,.<>?";

    std::string chars;
    if (IsDlgButtonChecked(GetParent(hCheckUpper), ID_CHECK_UPPER) == BST_CHECKED) chars += upper;
    if (IsDlgButtonChecked(GetParent(hCheckLower), ID_CHECK_LOWER) == BST_CHECKED) chars += lower;
    if (IsDlgButtonChecked(GetParent(hCheckDigits), ID_CHECK_DIGITS) == BST_CHECKED) chars += digits;
    if (IsDlgButtonChecked(GetParent(hCheckSymbols), ID_CHECK_SYMBOLS) == BST_CHECKED) chars += symbols;

    if (chars.empty()) chars = lower;

    char lenStr[10] = { 0 };
    GetWindowTextA(hEditLength, lenStr, 9);
    int length = atoi(lenStr);
    if (length < 4) length = 4;
    if (length > 128) length = 128;

    static std::mt19937 gen(static_cast<unsigned>(time(nullptr)));
    std::string password;
    std::uniform_int_distribution<> dis(0, (int)chars.size() - 1);

    for (int i = 0; i < length; i++) {
        password += chars[dis(gen)];
    }

    return password;
}

std::string CheckStrength(const std::string& pwd) {
    int score = 0;
    if (pwd.length() >= 8) score++;
    if (pwd.length() >= 12) score++;
    if (pwd.length() >= 16) score++;

    bool hasUpper = false, hasLower = false, hasDigit = false, hasSymbol = false;
    for (char c : pwd) {
        if (isupper((unsigned char)c)) hasUpper = true;
        else if (islower((unsigned char)c)) hasLower = true;
        else if (isdigit((unsigned char)c)) hasDigit = true;
        else hasSymbol = true;
    }

    if (hasUpper) score++;
    if (hasLower) score++;
    if (hasDigit) score++;
    if (hasSymbol) score++;

    if (score <= 2) return "Slabyy";
    if (score <= 4) return "Sredniy";
    if (score <= 6) return "Horoshiy";
    return "Nadezhnyy";
}

void UpdatePassword(HWND hwnd) {
    currentPassword = GeneratePassword();
    currentStrength = CheckStrength(currentPassword);
    InvalidateRect(hwnd, NULL, TRUE);
    UpdateWindow(hwnd);
}

void CopyToClipboard(HWND hwnd) {
    if (currentPassword.empty()) return;

    if (OpenClipboard(hwnd)) {
        EmptyClipboard();
        size_t len = currentPassword.length() + 1;
        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, len);
        if (hMem) {
            void* ptr = GlobalLock(hMem);
            if (ptr) {
                memcpy(ptr, currentPassword.c_str(), len);
                GlobalUnlock(hMem);
                SetClipboardData(CF_TEXT, hMem);
            }
        }
        CloseClipboard();

        SetWindowTextA(GetDlgItem(hwnd, ID_BTN_COPY), "Skopirovano!");
        SetTimer(hwnd, 1, 1000, NULL);
    }
}

// Кастомная отрисовка
void DrawCustom(HWND hwnd, HDC hdc) {
    RECT rect;
    GetClientRect(hwnd, &rect);

    // Фон
    HBRUSH hbrBg = CreateSolidBrush(RGB(30, 30, 46));
    FillRect(hdc, &rect, hbrBg);
    DeleteObject(hbrBg);

    HFONT hFontTitle = CreateFontA(20, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, "Segoe UI");
    HFONT hFontPassword = CreateFontA(18, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, "Consolas");
    HFONT hFontNormal = CreateFontA(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, "Segoe UI");

    // Заголовок
    SetTextColor(hdc, RGB(137, 180, 250));
    SetBkMode(hdc, TRANSPARENT);
    SelectObject(hdc, hFontTitle);

    RECT titleRect;
    titleRect.left = 0;
    titleRect.top = 15;
    titleRect.right = 400;
    titleRect.bottom = 45;
    DrawTextA(hdc, "Generator paroley", -1, &titleRect, DT_CENTER | DT_SINGLELINE);

    // Рамка пароля
    RECT pwdRect;
    pwdRect.left = 20;
    pwdRect.top = 55;
    pwdRect.right = 380;
    pwdRect.bottom = 95;

    HBRUSH hbrPwdBg = CreateSolidBrush(RGB(40, 40, 60));
    FillRect(hdc, &pwdRect, hbrPwdBg);
    DeleteObject(hbrPwdBg);

    HBRUSH hbrFrame = CreateSolidBrush(RGB(137, 180, 250));
    FrameRect(hdc, &pwdRect, hbrFrame);
    DeleteObject(hbrFrame);

    // Текст пароля
    SelectObject(hdc, hFontPassword);
    SetTextColor(hdc, RGB(0, 217, 255));
    DrawTextA(hdc, currentPassword.c_str(), -1, &pwdRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    // Сила пароля
    SelectObject(hdc, hFontNormal);
    COLORREF strengthColor;
    if (currentStrength == "Slabyy") strengthColor = RGB(243, 139, 168);
    else if (currentStrength == "Sredniy") strengthColor = RGB(250, 179, 135);
    else if (currentStrength == "Horoshiy") strengthColor = RGB(249, 226, 175);
    else strengthColor = RGB(166, 227, 161);

    SetTextColor(hdc, strengthColor);

    RECT strengthRect;
    strengthRect.left = 20;
    strengthRect.top = 105;
    strengthRect.right = 380;
    strengthRect.bottom = 130;

    std::string strengthText = "Sila: " + currentStrength;
    DrawTextA(hdc, strengthText.c_str(), -1, &strengthRect, DT_CENTER | DT_SINGLELINE);

    DeleteObject(hFontTitle);
    DeleteObject(hFontPassword);
    DeleteObject(hFontNormal);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        // Длина
        CreateWindowA("static", "Dlina:",
            WS_VISIBLE | WS_CHILD,
            20, 145, 40, 20, hwnd, NULL, NULL, NULL);

        hEditLength = CreateWindowA("edit", "16",
            WS_VISIBLE | WS_CHILD | WS_BORDER | ES_CENTER | ES_NUMBER,
            65, 142, 50, 25, hwnd, (HMENU)ID_EDIT_LENGTH, NULL, NULL);

        // Чекбоксы
        hCheckUpper = CreateWindowA("button", "ABC",
            WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
            130, 145, 60, 20, hwnd, (HMENU)ID_CHECK_UPPER, NULL, NULL);
        SendMessage(hCheckUpper, BM_SETCHECK, BST_CHECKED, 0);

        hCheckLower = CreateWindowA("button", "abc",
            WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
            200, 145, 60, 20, hwnd, (HMENU)ID_CHECK_LOWER, NULL, NULL);
        SendMessage(hCheckLower, BM_SETCHECK, BST_CHECKED, 0);

        hCheckDigits = CreateWindowA("button", "123",
            WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
            270, 145, 60, 20, hwnd, (HMENU)ID_CHECK_DIGITS, NULL, NULL);
        SendMessage(hCheckDigits, BM_SETCHECK, BST_CHECKED, 0);

        hCheckSymbols = CreateWindowA("button", "!@#",
            WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
            340, 145, 60, 20, hwnd, (HMENU)ID_CHECK_SYMBOLS, NULL, NULL);
        SendMessage(hCheckSymbols, BM_SETCHECK, BST_CHECKED, 0);

        // Кнопки
        CreateWindowA("button", "Generirovat",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            20, 190, 170, 40, hwnd, (HMENU)ID_BTN_GENERATE, NULL, NULL);

        CreateWindowA("button", "Kopirovat",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            210, 190, 170, 40, hwnd, (HMENU)ID_BTN_COPY, NULL, NULL);

        UpdatePassword(hwnd);
        return 0;
    }

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        DrawCustom(hwnd, hdc);
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case ID_BTN_GENERATE:
            UpdatePassword(hwnd);
            return 0;
        case ID_BTN_COPY:
            CopyToClipboard(hwnd);
            return 0;
        case ID_CHECK_UPPER:
        case ID_CHECK_LOWER:
        case ID_CHECK_DIGITS:
        case ID_CHECK_SYMBOLS:
            UpdatePassword(hwnd);
            return 0;
        }
        break;

    case WM_CTLCOLOREDIT: {
        HDC hdc = (HDC)wParam;
        SetBkColor(hdc, RGB(50, 50, 70));
        SetTextColor(hdc, RGB(255, 255, 255));
        static HBRUSH hbr = CreateSolidBrush(RGB(50, 50, 70));
        return (LRESULT)hbr;
    }

    case WM_CTLCOLORBTN:
        return (LRESULT)GetStockObject(NULL_BRUSH);

    case WM_CTLCOLORSTATIC: {
        HDC hdc = (HDC)wParam;
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(220, 220, 220));
        return (LRESULT)GetStockObject(NULL_BRUSH);
    }

    case WM_TIMER:
        SetWindowTextA(GetDlgItem(hwnd, ID_BTN_COPY), "Kopirovat");
        KillTimer(hwnd, 1);
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

int main() {
    HINSTANCE hInstance = GetModuleHandle(NULL);

    WNDCLASSEXA wc = { 0 };
    wc.cbSize = sizeof(WNDCLASSEXA);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)CreateSolidBrush(RGB(30, 30, 46));
    wc.lpszClassName = "PasswordGenerator";

    RegisterClassExA(&wc);

    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    HWND hwnd = CreateWindowExA(0, "PasswordGenerator", "Generator paroley",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        (screenWidth - 420) / 2, (screenHeight - 280) / 2, 420, 280,
        NULL, NULL, hInstance, NULL);

    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}