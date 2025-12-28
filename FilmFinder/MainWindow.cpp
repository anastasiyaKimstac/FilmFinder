#include "MainWindow.h"
#include <commctrl.h>
#include <string>
#include "resource.h"  // Добавляем заголовок ресурсов

#pragma comment(lib, "comctl32.lib")

// Идентификаторы для элементов главного окна
#define ID_BTN_SEARCH        201
#define ID_BTN_PROFILE       202
#define ID_BTN_FAVORITES     203
#define ID_BTN_LOGOUT_MAIN   204  
#define ID_BTN_SETTINGS      205
#define ID_BTN_RECOMMEND     206
#define ID_BTN_HISTORY       207
#define ID_BTN_RANDOM        208
#define ID_STATIC_WELCOME    209
#define ID_STATIC_FILM_INFO  210

// Объявления внешних переменных из FilmFinder.cpp
extern HINSTANCE hInst;
extern std::wstring currentUser;
extern bool isLoggedIn;

// Глобальные переменные для MainWindow.cpp
static HFONT hMainTitleFont = NULL;
static HFONT hButtonFont = NULL;
static HWND g_hMainWindow = NULL;

// Объявление функции About
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

// Регистрация класса для главного окна
ATOM RegisterMainWindowClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = MainWndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(101));  // Используем числовой идентификатор
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = NULL; // Без меню для главного окна
    wcex.lpszClassName = L"FilmFinderMainWindow";
    wcex.hIconSm = NULL;  // Можно оставить NULL или использовать LoadIcon

    return RegisterClassExW(&wcex);
}

// Создание главного окна
void CreateMainWindow(HINSTANCE hInstance, const std::wstring& username)
{
    // Регистрируем класс главного окна, если еще не зарегистрирован
    static bool classRegistered = false;
    if (!classRegistered) {
        RegisterMainWindowClass(hInstance);
        classRegistered = true;
    }

    // Создаем главное окно с большими размерами
    g_hMainWindow = CreateWindowW(
        L"FilmFinderMainWindow",
        (L"FilmFinder - " + username).c_str(),
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, 0,
        900, 750, // Увеличил размер окна
        nullptr,
        nullptr,
        hInstance,
        nullptr
    );

    if (!g_hMainWindow)
    {
        MessageBox(NULL, L"Не удалось создать главное окно!", L"Ошибка", MB_OK | MB_ICONERROR);
        return;
    }

    // Создаем кастомные шрифты для главного окна
    hMainTitleFont = CreateFontW(
        32, 0, 0, 0, FW_BOLD,
        FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE,
        L"Segoe UI"
    );

    hButtonFont = CreateFontW(
        20, 0, 0, 0, FW_NORMAL,
        FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE,
        L"Segoe UI"
    );

    // Центрируем окно на экране
    RECT rc;
    GetWindowRect(g_hMainWindow, &rc);
    int x = (GetSystemMetrics(SM_CXSCREEN) - (rc.right - rc.left)) / 2;
    int y = (GetSystemMetrics(SM_CYSCREEN) - (rc.bottom - rc.top)) / 2;
    SetWindowPos(g_hMainWindow, NULL, x, y, 0, 0, SWP_NOZORDER | SWP_NOSIZE);

    ShowWindow(g_hMainWindow, SW_SHOW);
    UpdateWindow(g_hMainWindow);
}

// Функция для красивой отрисовки фона
void DrawMainBackground(HDC hdc, RECT rect)
{
    // Градиентный фон от синего к голубому
    TRIVERTEX vertex[2];
    vertex[0].x = rect.left;
    vertex[0].y = rect.top;
    vertex[0].Red = 30 << 8;     // Темно-синий
    vertex[0].Green = 60 << 8;
    vertex[0].Blue = 120 << 8;
    vertex[0].Alpha = 0x0000;

    vertex[1].x = rect.right;
    vertex[1].y = rect.bottom;
    vertex[1].Red = 200 << 8;    // Светло-голубой
    vertex[1].Green = 220 << 8;
    vertex[1].Blue = 255 << 8;
    vertex[1].Alpha = 0x0000;

    GRADIENT_RECT gRect;
    gRect.UpperLeft = 0;
    gRect.LowerRight = 1;

    GradientFill(hdc, vertex, 2, &gRect, 1, GRADIENT_FILL_RECT_V);
}

// Простая реализация ApplyLargeFontToChildren
void ApplyLargeFontToChildren(HWND hWnd)
{
    // Используем существующий шрифт или создаем новый
    HFONT hFont = CreateFontW(
        18, 0, 0, 0, FW_NORMAL,
        FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE,
        L"Segoe UI"
    );

    if (hFont) {
        EnumChildWindows(hWnd, [](HWND hwnd, LPARAM lParam) -> BOOL {
            SendMessage(hwnd, WM_SETFONT, (WPARAM)lParam, TRUE);
            return TRUE;
            }, (LPARAM)hFont);
    }
}

// Функция для поиска окна авторизации
HWND FindAuthWindow()
{
    // Ищем по имени класса окна (используем фактическое имя класса)
    return FindWindow(L"FilmFinder", NULL);  // Ищем по классу, название можно оставить NULL
}

// Обработчик сообщений для главного окна
LRESULT CALLBACK MainWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
    {
        // === СОЗДАЕМ ЭЛЕМЕНТЫ ГЛАВНОГО МЕНЮ ===

        // 1. Заголовок приветствия
        CreateWindowW(L"STATIC", (L"Добро пожаловать, " + currentUser + L"!").c_str(),
            WS_CHILD | WS_VISIBLE | SS_CENTER,
            50, 30, 800, 60, hWnd, (HMENU)ID_STATIC_WELCOME, hInst, NULL);

        // 2. Панель с основными функциями (левая колонка)
        int yPos = 120;
        int buttonWidth = 250;
        int buttonHeight = 60;
        int spacing = 20;

        // Поиск фильмов
        CreateWindowW(L"BUTTON", L"🔍 Поиск фильмов",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            100, yPos, buttonWidth, buttonHeight, hWnd, (HMENU)ID_BTN_SEARCH, hInst, NULL);
        yPos += buttonHeight + spacing;

        // Рекомендации
        CreateWindowW(L"BUTTON", L"⭐ Рекомендации",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            100, yPos, buttonWidth, buttonHeight, hWnd, (HMENU)ID_BTN_RECOMMEND, hInst, NULL);
        yPos += buttonHeight + spacing;

        // Случайный фильм
        CreateWindowW(L"BUTTON", L"🎲 Случайный фильм",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            100, yPos, buttonWidth, buttonHeight, hWnd, (HMENU)ID_BTN_RANDOM, hInst, NULL);
        yPos += buttonHeight + spacing;

        // История просмотров
        CreateWindowW(L"BUTTON", L"📜 История просмотров",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            100, yPos, buttonWidth, buttonHeight, hWnd, (HMENU)ID_BTN_HISTORY, hInst, NULL);

        // 3. Панель с пользовательскими функциями (правая колонка)
        yPos = 120;

        // Мой профиль
        CreateWindowW(L"BUTTON", L"👤 Мой профиль",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            550, yPos, buttonWidth, buttonHeight, hWnd, (HMENU)ID_BTN_PROFILE, hInst, NULL);
        yPos += buttonHeight + spacing;

        // Избранное
        CreateWindowW(L"BUTTON", L"❤️ Избранное",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            550, yPos, buttonWidth, buttonHeight, hWnd, (HMENU)ID_BTN_FAVORITES, hInst, NULL);
        yPos += buttonHeight + spacing;

        // Настройки
        CreateWindowW(L"BUTTON", L"⚙️ Настройки",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            550, yPos, buttonWidth, buttonHeight, hWnd, (HMENU)ID_BTN_SETTINGS, hInst, NULL);
        yPos += buttonHeight + spacing;

        // Выйти
        CreateWindowW(L"BUTTON", L"🚪 Выйти",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            550, yPos, buttonWidth, buttonHeight, hWnd, (HMENU)ID_BTN_LOGOUT_MAIN, hInst, NULL);

        // 4. Панель с информацией о последнем просмотренном (внизу)
        CreateWindowW(L"STATIC", L"🎬 Последний просмотренный фильм: \nИнтерстеллар (2014) - ★★★★★",
            WS_CHILD | WS_VISIBLE | SS_CENTER | SS_NOPREFIX,
            50, 550, 800, 80, hWnd, (HMENU)ID_STATIC_FILM_INFO, hInst, NULL);

        // 5. Статистика (в правом верхнем углу)
        CreateWindowW(L"STATIC", L"📊 Статистика:\nПросмотрено: 47 фильмов\nВ избранном: 12\nОценок: 28",
            WS_CHILD | WS_VISIBLE,
            650, 400, 220, 120, hWnd, NULL, hInst, NULL);

        // Применяем шрифты
        ApplyLargeFontToChildren(hWnd);

        // Устанавливаем специальные шрифты для заголовка
        HWND hWelcome = GetDlgItem(hWnd, ID_STATIC_WELCOME);
        if (hWelcome && hMainTitleFont) {
            SendMessage(hWelcome, WM_SETFONT, (WPARAM)hMainTitleFont, TRUE);
        }

        // Устанавливаем шрифт для кнопок
        HWND hChild = GetWindow(hWnd, GW_CHILD);
        while (hChild) {
            wchar_t className[256];
            GetClassName(hChild, className, 256);
            if (wcscmp(className, L"Button") == 0) {
                SendMessage(hChild, WM_SETFONT, (WPARAM)hButtonFont, TRUE);
            }
            hChild = GetWindow(hChild, GW_HWNDNEXT);
        }
    }
    break;

    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);
        HWND hAuthWnd = NULL;  // Объявляем переменную в начале блока

        switch (wmId)
        {
        case ID_BTN_SEARCH:
        {
            // Скрываем главное окно
            ShowWindow(hWnd, SW_HIDE);

            // Создаем окно поиска
            HWND hSearchWnd = CreateSearchWindow(hInst, hWnd);

            if (hSearchWnd) {
                // Центрируем окно поиска
                RECT rc;
                GetWindowRect(hSearchWnd, &rc);
                int x = (GetSystemMetrics(SM_CXSCREEN) - (rc.right - rc.left)) / 2;
                int y = (GetSystemMetrics(SM_CYSCREEN) - (rc.bottom - rc.top)) / 2;
                SetWindowPos(hSearchWnd, NULL, x, y, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
                ShowWindow(hSearchWnd, SW_SHOW);
                SetForegroundWindow(hSearchWnd);
            }
            else {
                MessageBox(hWnd, L"Не удалось создать окно поиска", L"Ошибка", MB_OK | MB_ICONERROR);
                ShowWindow(hWnd, SW_SHOW);
            }
            break;
        }

        case ID_BTN_LOGOUT_MAIN:
            DestroyWindow(hWnd); // Закрываем главное окно

            // Находим окно авторизации и показываем его
            hAuthWnd = FindAuthWindow();
            if (hAuthWnd) {
                ShowWindow(hAuthWnd, SW_SHOW);
                // Сбрасываем состояние авторизации
                isLoggedIn = false;
                currentUser = L"";
            }
            break;

        case ID_BTN_SETTINGS:
            MessageBox(hWnd, L"Настройки приложения\n(будет реализовано позже)", L"Настройки", MB_OK | MB_ICONINFORMATION);
            break;

        case ID_BTN_RECOMMEND:
            MessageBox(hWnd, L"Персональные рекомендации\n(будет реализовано позже)", L"Рекомендации", MB_OK | MB_ICONINFORMATION);
            break;

        case ID_BTN_HISTORY:
            MessageBox(hWnd, L"История ваших просмотров\n(будет реализовано позже)", L"История", MB_OK | MB_ICONINFORMATION);
            break;

        case ID_BTN_RANDOM:
            MessageBox(hWnd, L"Случайный фильм для вас:\n'Начало' (2010)\nРежиссер: Кристофер Нолан", L"Случайный фильм", MB_OK | MB_ICONINFORMATION);
            break;

        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
    }
    break;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        // Получаем размеры клиентской области
        RECT clientRect;
        GetClientRect(hWnd, &clientRect);

        // Создаем буфер для двойной буферизации
        HDC hdcMem = CreateCompatibleDC(hdc);
        HBITMAP hBitmap = CreateCompatibleBitmap(hdc, clientRect.right, clientRect.bottom);
        SelectObject(hdcMem, hBitmap);

        // Рисуем фон
        DrawMainBackground(hdcMem, clientRect);

        // Рисуем декоративные элементы

        // 1. Разделительная линия между колонками
        HPEN hLinePen = CreatePen(PS_SOLID, 2, RGB(100, 149, 237));
        HPEN hOldPen = (HPEN)SelectObject(hdcMem, hLinePen);
        MoveToEx(hdcMem, clientRect.right / 2 - 5, 120, NULL);
        LineTo(hdcMem, clientRect.right / 2 - 5, 500);
        SelectObject(hdcMem, hOldPen);
        DeleteObject(hLinePen);

        // 2. Заголовок над левой колонкой
        RECT leftTitleRect = { 100, 90, 350, 120 };
        SetTextColor(hdcMem, RGB(255, 255, 255));
        SetBkMode(hdcMem, TRANSPARENT);
        HFONT hOldFont = (HFONT)SelectObject(hdcMem, hButtonFont);
        DrawText(hdcMem, L"Поиск фильмов", -1, &leftTitleRect, DT_LEFT | DT_SINGLELINE);

        // 3. Заголовок над правой колонкой
        RECT rightTitleRect = { 550, 90, 800, 120 };
        DrawText(hdcMem, L"Мой аккаунт", -1, &rightTitleRect, DT_LEFT | DT_SINGLELINE);

        SelectObject(hdcMem, hOldFont);

        // Копируем буфер на экран
        BitBlt(hdc, 0, 0, clientRect.right, clientRect.bottom, hdcMem, 0, 0, SRCCOPY);

        // Очистка
        DeleteObject(hBitmap);
        DeleteDC(hdcMem);

        EndPaint(hWnd, &ps);
    }
    break;

    case WM_CTLCOLORSTATIC:
    {
        HDC hdc = (HDC)wParam;
        SetBkMode(hdc, TRANSPARENT);

        // Для заголовка - белый текст
        HWND hStatic = (HWND)lParam;
        if (hStatic == GetDlgItem(hWnd, ID_STATIC_WELCOME)) {
            SetTextColor(hdc, RGB(255, 255, 255));
        }
        else {
            SetTextColor(hdc, RGB(240, 240, 240));
        }

        return (LRESULT)GetStockObject(NULL_BRUSH);
    }
    break;

    case WM_CTLCOLORBTN:
    {
        HDC hdc = (HDC)wParam;
        SetBkColor(hdc, RGB(70, 130, 180)); // Цвет кнопок
        SetTextColor(hdc, RGB(255, 255, 255)); // Белый текст

        static HBRUSH hButtonBrush = NULL;
        if (!hButtonBrush) {
            hButtonBrush = CreateSolidBrush(RGB(70, 130, 180));
        }
        return (LRESULT)hButtonBrush;
    }
    break;

    case WM_DESTROY:
        // Освобождаем шрифты
        if (hMainTitleFont) DeleteObject(hMainTitleFont);
        if (hButtonFont) DeleteObject(hButtonFont);

        g_hMainWindow = NULL;
        // Не закрываем приложение полностью, только это окно
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}