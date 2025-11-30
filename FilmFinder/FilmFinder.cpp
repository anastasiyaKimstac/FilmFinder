// FilmFinder.cpp : Определяет точку входа для приложения.
//

#include "framework.h"
#include "FilmFinder.h"
#include <vector>
#include <string>
#include <fstream>
#include <commctrl.h>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "msimg32.lib")

#define MAX_LOADSTRING 100

// Глобальные переменные:
HINSTANCE hInst;                                // текущий экземпляр
WCHAR szTitle[MAX_LOADSTRING];                  // Текст строки заголовка
WCHAR szWindowClass[MAX_LOADSTRING];            // имя класса главного окна

// Идентификаторы для элементов управления
#define IDC_EDIT_EMAIL        1001
#define IDC_EDIT_PASSWORD     1002
#define IDC_BUTTON_LOGIN      1003
#define IDC_BUTTON_REGISTER   1004
#define IDC_CHECK_REMEMBER    1005
#define IDM_LOGOUT            1006
#define IDC_EDIT_CONFIRM_PASSWORD 1007
#define IDC_BUTTON_CONFIRM_REGISTER 1008
#define IDC_BUTTON_BACK_TO_LOGIN 1009

// Структура для хранения данных пользователя
struct User {
    std::wstring email;
    std::wstring password;
};

// Глобальные переменные для авторизации
std::vector<User> users;
bool isLoggedIn = false;
std::wstring currentUser;
bool isRegistrationScreen = false; // Флаг для определения экрана регистрации

// Цвета для оформления
const COLORREF BG_COLOR = RGB(240, 245, 255); // Светло-голубой фон
const COLORREF HEADER_COLOR = RGB(70, 130, 180); // Steel blue для заголовка
const COLORREF BUTTON_COLOR = RGB(100, 149, 237); // Cornflower blue

// Файл для сохранения пользователей
const wchar_t* USER_DATA_FILE = L"users.dat";

// Глобальные шрифты для всего приложения
static HFONT hTitleFont = NULL;
static HFONT hLargeFont = NULL;

// Отправить объявления функций, включенных в этот модуль кода:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

// Функции для работы с авторизацией
void LoadUsers();
void SaveUsers();
void ShowAuthScreen(HWND hWnd);
void ShowRegistrationScreen(HWND hWnd);
void ShowMainScreen(HWND hWnd);
bool LoginUser(const std::wstring& email, const std::wstring& password);
bool RegisterUser(const std::wstring& email, const std::wstring& password);

// Функции для красивого оформления
void DrawGradientBackground(HDC hdc, RECT rect, COLORREF topColor, COLORREF bottomColor);
HFONT CreateCustomFont(int height, int weight = FW_NORMAL, const wchar_t* faceName = L"Arial");
void ApplyLargeFontToChildren(HWND hWnd);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // Инициализация common controls для современных элементов
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_STANDARD_CLASSES | ICC_BAR_CLASSES;
    InitCommonControlsEx(&icex);

    // Загружаем пользователей из файла
    LoadUsers();

    // Инициализация глобальных строк
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_FILMFINDER, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Выполнить инициализацию приложения:
    if (!InitInstance(hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_FILMFINDER));

    MSG msg;

    // Цикл основного сообщения:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    // Освобождаем шрифты при выходе
    if (hTitleFont) DeleteObject(hTitleFont);
    if (hLargeFont) DeleteObject(hLargeFont);

    return (int)msg.wParam;
}

//
//  ФУНКЦИЯ: MyRegisterClass()
//
//  ЦЕЛЬ: Регистрирует класс окна.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_FILMFINDER));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_FILMFINDER);
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   ФУНКЦИЯ: InitInstance(HINSTANCE, int)
//
//   ЦЕЛЬ: Сохраняет маркер экземпляра и создает главное окно
//
//   КОММЕНТАРИИ:
//
//        В этой функции маркер экземпляра сохраняется в глобальной переменной, а также
//        создается и выводится главное окно программы.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance; // Сохранить маркер экземпляра в глобальной переменной

    // Создаем кастомные шрифты с ОЧЕНЬ крупными размерами
    hTitleFont = CreateCustomFont(36, FW_BOLD, L"Segoe UI");      // Очень крупный заголовок
    hLargeFont = CreateCustomFont(20, FW_NORMAL, L"Segoe UI");    // Очень крупный обычный текст

    // Создаем окно с фиксированным размером и без изменения размера
    HWND hWnd = CreateWindowW(szWindowClass, L"FilmFinder - Авторизация",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, 0, 600, 700, nullptr, nullptr, hInstance, nullptr);

    if (!hWnd)
    {
        return FALSE;
    }

    // Центрируем окно на экране
    RECT rc;
    GetWindowRect(hWnd, &rc);
    int x = (GetSystemMetrics(SM_CXSCREEN) - (rc.right - rc.left)) / 2;
    int y = (GetSystemMetrics(SM_CYSCREEN) - (rc.bottom - rc.top)) / 2;
    SetWindowPos(hWnd, NULL, x, y, 0, 0, SWP_NOZORDER | SWP_NOSIZE);

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    return TRUE;
}

//
//  ФУНКЦИЯ: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  ЦЕЛЬ: Обрабатывает сообщения в главном окне.
//
//  WM_COMMAND  - обработать меню приложения
//  WM_PAINT    - Отрисовка главного окна
//  WM_DESTROY  - отправить сообщение о выходе и вернуться
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static std::wstring tempEmail, tempPassword; // Временные переменные для данных регистрации

    switch (message)
    {
    case WM_CREATE:
    {
        // Показываем экран авторизации при создании окна
        ShowAuthScreen(hWnd);
    }
    break;

    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORSTATIC:
    {
        HDC hdc = (HDC)wParam;
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(50, 50, 50)); // Темно-серый текст

        // Возвращаем прозрачную кисть
        static HBRUSH hBrush = NULL;
        if (!hBrush) {
            hBrush = CreateSolidBrush(BG_COLOR);
        }
        return (LRESULT)hBrush;
    }
    break;

    case WM_CTLCOLORBTN:
    {
        HDC hdc = (HDC)wParam;
        SetBkColor(hdc, BUTTON_COLOR);
        SetTextColor(hdc, RGB(255, 255, 255)); // Белый текст на кнопках

        static HBRUSH hButtonBrush = NULL;
        if (!hButtonBrush) {
            hButtonBrush = CreateSolidBrush(BUTTON_COLOR);
        }
        return (LRESULT)hButtonBrush;
    }
    break;

    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);
        // Разобрать выбор в меню:
        switch (wmId)
        {
        case IDC_BUTTON_LOGIN:
        {
            // Получаем данные из полей ввода
            wchar_t email[100], password[100];
            GetDlgItemText(hWnd, IDC_EDIT_EMAIL, email, 100);
            GetDlgItemText(hWnd, IDC_EDIT_PASSWORD, password, 100);

            if (LoginUser(email, password)) {
                currentUser = email;
                isLoggedIn = true;
                ShowMainScreen(hWnd);
                SetWindowText(hWnd, (L"FilmFinder - " + currentUser).c_str());
            }
            else {
                MessageBox(hWnd, L"Неверный email или пароль!", L"Ошибка", MB_OK | MB_ICONERROR);
            }
        }
        break;

        case IDC_BUTTON_REGISTER:
        {
            // Переходим на экран регистрации
            ShowRegistrationScreen(hWnd);
        }
        break;

        case IDC_BUTTON_CONFIRM_REGISTER:
        {
            // Получаем данные из полей регистрации
            wchar_t email[100], password[100], confirmPassword[100];
            GetDlgItemText(hWnd, IDC_EDIT_EMAIL, email, 100);
            GetDlgItemText(hWnd, IDC_EDIT_PASSWORD, password, 100);
            GetDlgItemText(hWnd, IDC_EDIT_CONFIRM_PASSWORD, confirmPassword, 100);

            // Проверяем совпадение паролей
            if (wcscmp(password, confirmPassword) != 0) {
                MessageBox(hWnd, L"Пароли не совпадают!", L"Ошибка", MB_OK | MB_ICONERROR);
                break;
            }

            if (RegisterUser(email, password)) {
                // Сохраняем данные для автозаполнения
                tempEmail = email;
                tempPassword = password;

                MessageBox(hWnd, L"Регистрация успешна! Теперь вы можете войти.", L"Успех", MB_OK | MB_ICONINFORMATION);

                // Возвращаемся на экран авторизации с заполненными данными
                ShowAuthScreen(hWnd);

                // Автоматически заполняем поля
                if (!tempEmail.empty()) {
                    SetDlgItemText(hWnd, IDC_EDIT_EMAIL, tempEmail.c_str());
                }
                if (!tempPassword.empty()) {
                    SetDlgItemText(hWnd, IDC_EDIT_PASSWORD, tempPassword.c_str());
                }
            }
            else {
                MessageBox(hWnd, L"Пользователь с таким email уже существует!", L"Ошибка", MB_OK | MB_ICONERROR);
            }
        }
        break;

        case IDC_BUTTON_BACK_TO_LOGIN:
        {
            // Возвращаемся на экран авторизации
            ShowAuthScreen(hWnd);
        }
        break;

        case IDM_LOGOUT:
        {
            isLoggedIn = false;
            currentUser = L"";
            ShowAuthScreen(hWnd);
            SetWindowText(hWnd, L"FilmFinder - Авторизация");
        }
        break;

        case IDM_ABOUT:
            DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
            break;

        case IDM_EXIT:
            DestroyWindow(hWnd);
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

        // Рисуем градиентный фон
        DrawGradientBackground(hdc, clientRect, BG_COLOR, RGB(255, 255, 255));

        // Создаем буфер для двойной буферизации (убирает мерцание)
        HDC hdcMem = CreateCompatibleDC(hdc);
        HBITMAP hBitmap = CreateCompatibleBitmap(hdc, clientRect.right, clientRect.bottom);
        SelectObject(hdcMem, hBitmap);

        // Копируем фон
        BitBlt(hdcMem, 0, 0, clientRect.right, clientRect.bottom, hdc, 0, 0, SRCCOPY);

        if (!isLoggedIn) {
            if (!isRegistrationScreen) {
                // Рисуем красивый заголовок "Авторизация"
                RECT titleRect = { 0, 50, clientRect.right, 150 };
                HFONT hOldFont = (HFONT)SelectObject(hdcMem, hTitleFont);
                SetTextColor(hdcMem, HEADER_COLOR);
                SetBkMode(hdcMem, TRANSPARENT);

                DrawText(hdcMem, L"АВТОРИЗАЦИЯ", -1, &titleRect, DT_CENTER | DT_SINGLELINE);

                // Рисуем декоративную линию под заголовком
                HPEN hLinePen = CreatePen(PS_SOLID, 3, HEADER_COLOR);
                HPEN hOldPen = (HPEN)SelectObject(hdcMem, hLinePen);
                MoveToEx(hdcMem, clientRect.right / 4, 130, NULL);
                LineTo(hdcMem, 3 * clientRect.right / 4, 130);
                SelectObject(hdcMem, hOldPen);
                DeleteObject(hLinePen);

                SelectObject(hdcMem, hOldFont);
            }
            else {
                // Рисуем красивый заголовок "Регистрация"
                RECT titleRect = { 0, 50, clientRect.right, 150 };
                HFONT hOldFont = (HFONT)SelectObject(hdcMem, hTitleFont);
                SetTextColor(hdcMem, HEADER_COLOR);
                SetBkMode(hdcMem, TRANSPARENT);

                DrawText(hdcMem, L"РЕГИСТРАЦИЯ", -1, &titleRect, DT_CENTER | DT_SINGLELINE);

                // Рисуем декоративную линию под заголовком
                HPEN hLinePen = CreatePen(PS_SOLID, 3, HEADER_COLOR);
                HPEN hOldPen = (HPEN)SelectObject(hdcMem, hLinePen);
                MoveToEx(hdcMem, clientRect.right / 4, 130, NULL);
                LineTo(hdcMem, 3 * clientRect.right / 4, 130);
                SelectObject(hdcMem, hOldPen);
                DeleteObject(hLinePen);

                SelectObject(hdcMem, hOldFont);
            }
        }
        else {
            // Рисуем приветствие для авторизованного пользователя
            std::wstring welcome = L"Добро пожаловать, " + currentUser + L"!";
            RECT welcomeRect = { 0, 80, clientRect.right, 180 };
            HFONT hOldFont = (HFONT)SelectObject(hdcMem, hTitleFont);
            SetTextColor(hdcMem, HEADER_COLOR);
            SetBkMode(hdcMem, TRANSPARENT);

            DrawText(hdcMem, welcome.c_str(), -1, &welcomeRect, DT_CENTER | DT_SINGLELINE);
            SelectObject(hdcMem, hOldFont);
        }

        // Копируем буфер на экран
        BitBlt(hdc, 0, 0, clientRect.right, clientRect.bottom, hdcMem, 0, 0, SRCCOPY);

        // Очистка
        DeleteObject(hBitmap);
        DeleteDC(hdcMem);

        EndPaint(hWnd, &ps);
    }
    break;

    case WM_DESTROY:
        SaveUsers(); // Сохраняем пользователей при выходе
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Функция для применения крупного шрифта ко всем дочерним элементам
void ApplyLargeFontToChildren(HWND hWnd)
{
    EnumChildWindows(hWnd, [](HWND hwnd, LPARAM lParam) -> BOOL {
        SendMessage(hwnd, WM_SETFONT, (WPARAM)hLargeFont, TRUE);
        return TRUE;
        }, 0);
}

// Функция для отображения экрана авторизации
void ShowAuthScreen(HWND hWnd)
{
    isRegistrationScreen = false;

    // Удаляем все существующие элементы
    HWND child = GetWindow(hWnd, GW_CHILD);
    while (child) {
        DestroyWindow(child);
        child = GetWindow(hWnd, GW_CHILD);
    }


    // Статический текст для email
    CreateWindowW(L"STATIC", L"Email:",
        WS_CHILD | WS_VISIBLE,
        80, 160, 150, 40, hWnd, NULL, hInst, NULL);

    // Поле ввода email
    CreateWindowW(L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT,
        80, 200, 400, 30, hWnd, (HMENU)IDC_EDIT_EMAIL, hInst, NULL);

    // Статический текст для пароля
    CreateWindowW(L"STATIC", L"Пароль:",
        WS_CHILD | WS_VISIBLE,
        80, 270, 150, 40, hWnd, NULL, hInst, NULL);

    // Поле ввода пароля (с звездочками)
    CreateWindowW(L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT | ES_PASSWORD,
        80, 310, 400, 30, hWnd, (HMENU)IDC_EDIT_PASSWORD, hInst, NULL); 

    // Чекбокс "Запомнить меня"
    CreateWindowW(L"BUTTON", L"Запомнить данные для входа",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        80, 380, 300, 35, hWnd, (HMENU)IDC_CHECK_REMEMBER, hInst, NULL); 

    // Кнопка "Войти"
    CreateWindowW(L"BUTTON", L"Войти",
        WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
        80, 440, 180, 55, hWnd, (HMENU)IDC_BUTTON_LOGIN, hInst, NULL);

    // Кнопка "Зарегистрироваться"
    CreateWindowW(L"BUTTON", L"Зарегистрироваться",
        WS_CHILD | WS_VISIBLE,
        280, 440, 200, 55, hWnd, (HMENU)IDC_BUTTON_REGISTER, hInst, NULL); 

    // Обновляем меню (убираем пункт "Выйти")
    HMENU hMenu = GetMenu(hWnd);
    if (hMenu) {
        RemoveMenu(hMenu, IDM_LOGOUT, MF_BYCOMMAND);
        DrawMenuBar(hWnd);
    }

    // Применяем ОЧЕНЬ крупные шрифты к элементам
    ApplyLargeFontToChildren(hWnd);
}

// Функция для отображения экрана регистрации
void ShowRegistrationScreen(HWND hWnd)
{
    isRegistrationScreen = true;

    // Удаляем все существующие элементы
    HWND child = GetWindow(hWnd, GW_CHILD);
    while (child) {
        DestroyWindow(child);
        child = GetWindow(hWnd, GW_CHILD);
    }

    // Создаем элементы регистрации с ОЧЕНЬ крупными размерами

    // Статический текст для email
    CreateWindowW(L"STATIC", L"Email:",
        WS_CHILD | WS_VISIBLE,
        80, 150, 150, 40, hWnd, NULL, hInst, NULL); // Увеличили высоту до 40

    // Поле ввода email
    CreateWindowW(L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT,
        80, 190, 400, 30, hWnd, (HMENU)IDC_EDIT_EMAIL, hInst, NULL); // Увеличили высоту до 50

    // Статический текст для пароля
    CreateWindowW(L"STATIC", L"Пароль:",
        WS_CHILD | WS_VISIBLE,
        80, 260, 150, 40, hWnd, NULL, hInst, NULL); // Увеличили высоту до 40

    // Поле ввода пароля (с звездочками)
    CreateWindowW(L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT | ES_PASSWORD,
        80, 300, 400, 30, hWnd, (HMENU)IDC_EDIT_PASSWORD, hInst, NULL); // Увеличили высоту до 50

    // Статический текст для подтверждения пароля
    CreateWindowW(L"STATIC", L"Повторите пароль:",
        WS_CHILD | WS_VISIBLE,
        80, 370, 200, 40, hWnd, NULL, hInst, NULL); // Увеличили высоту до 40

    // Поле ввода подтверждения пароля
    CreateWindowW(L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT | ES_PASSWORD,
        80, 410, 400, 30, hWnd, (HMENU)IDC_EDIT_CONFIRM_PASSWORD, hInst, NULL); // Увеличили высоту до 50

    // Кнопка "Зарегистрироваться"
    CreateWindowW(L"BUTTON", L"Зарегистрироваться",
        WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
        80, 490, 200, 55, hWnd, (HMENU)IDC_BUTTON_CONFIRM_REGISTER, hInst, NULL); // Увеличили высоту до 55

    // Кнопка "Назад к входу"
    CreateWindowW(L"BUTTON", L"Назад к входу",
        WS_CHILD | WS_VISIBLE,
        300, 490, 180, 55, hWnd, (HMENU)IDC_BUTTON_BACK_TO_LOGIN, hInst, NULL); // Увеличили высоту до 55

    // Применяем ОЧЕНЬ крупные шрифты к элементам
    ApplyLargeFontToChildren(hWnd);
}

// Функция для отображения главного экрана (после авторизации)
void ShowMainScreen(HWND hWnd)
{
    // Удаляем все существующие элементы
    HWND child = GetWindow(hWnd, GW_CHILD);
    while (child) {
        DestroyWindow(child);
        child = GetWindow(hWnd, GW_CHILD);
    }

    // Создаем элементы главного экрана с ОЧЕНЬ крупными шрифтами
    CreateWindowW(L"STATIC", L"Добро пожаловать в FilmFinder!",
        WS_CHILD | WS_VISIBLE,
        80, 200, 450, 60, hWnd, NULL, hInst, NULL); // Увеличили размеры

    CreateWindowW(L"STATIC", L"Здесь будет ваш поисковик фильмов...",
        WS_CHILD | WS_VISIBLE,
        80, 280, 450, 60, hWnd, NULL, hInst, NULL); // Увеличили размеры

    // Применяем ОЧЕНЬ крупные шрифты
    ApplyLargeFontToChildren(hWnd);

    // Добавляем пункт "Выйти" в меню
    HMENU hMenu = CreateMenu();
    HMENU hFileMenu = CreateMenu();

    AppendMenu(hFileMenu, MF_STRING, IDM_LOGOUT, L"Выйти");
    AppendMenu(hFileMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(hFileMenu, MF_STRING, IDM_EXIT, L"Выход");

    HMENU hHelpMenu = CreateMenu();
    AppendMenu(hHelpMenu, MF_STRING, IDM_ABOUT, L"О программе");

    AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hFileMenu, L"Файл");
    AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hHelpMenu, L"Помощь");

    SetMenu(hWnd, hMenu);
}


void DrawGradientBackground(HDC hdc, RECT rect, COLORREF topColor, COLORREF bottomColor)
{
    TRIVERTEX vertex[2];
    vertex[0].x = rect.left;
    vertex[0].y = rect.top;
    vertex[0].Red = GetRValue(topColor) << 8;
    vertex[0].Green = GetGValue(topColor) << 8;
    vertex[0].Blue = GetBValue(topColor) << 8;
    vertex[0].Alpha = 0x0000;

    vertex[1].x = rect.right;
    vertex[1].y = rect.bottom;
    vertex[1].Red = GetRValue(bottomColor) << 8;
    vertex[1].Green = GetGValue(bottomColor) << 8;
    vertex[1].Blue = GetBValue(bottomColor) << 8;
    vertex[1].Alpha = 0x0000;

    GRADIENT_RECT gRect;
    gRect.UpperLeft = 0;
    gRect.LowerRight = 1;

    GradientFill(hdc, vertex, 2, &gRect, 1, GRADIENT_FILL_RECT_V);
}

HFONT CreateCustomFont(int height, int weight, const wchar_t* faceName)
{
    return CreateFontW(
        height, 0, 0, 0, weight,
        FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE,
        faceName
    );
}

// Функция для входа пользователя
bool LoginUser(const std::wstring& email, const std::wstring& password)
{
    if (email.empty() || password.empty()) {
        return false;
    }

    for (const auto& user : users) {
        if (user.email == email && user.password == password) {
            return true;
        }
    }

    return false;
}

// Функция для регистрации пользователя
bool RegisterUser(const std::wstring& email, const std::wstring& password)
{
    if (email.empty() || password.empty()) {
        return false;
    }

    // Проверяем, нет ли уже пользователя с таким email
    for (const auto& user : users) {
        if (user.email == email) {
            return false;
        }
    }

    // Добавляем нового пользователя
    users.push_back({ email, password });
    SaveUsers();
    return true;
}

// Загрузка пользователей из файла
void LoadUsers()
{
    std::ifstream file(USER_DATA_FILE, std::ios::binary);
    if (!file) return;

    size_t count;
    file.read(reinterpret_cast<char*>(&count), sizeof(count));

    for (size_t i = 0; i < count; ++i) {
        User user;
        size_t emailLen, passwordLen;

        file.read(reinterpret_cast<char*>(&emailLen), sizeof(emailLen));
        user.email.resize(emailLen);
        file.read(reinterpret_cast<char*>(&user.email[0]), emailLen * sizeof(wchar_t));

        file.read(reinterpret_cast<char*>(&passwordLen), sizeof(passwordLen));
        user.password.resize(passwordLen);
        file.read(reinterpret_cast<char*>(&user.password[0]), passwordLen * sizeof(wchar_t));

        users.push_back(user);
    }
}

// Сохранение пользователей в файл
void SaveUsers()
{
    std::ofstream file(USER_DATA_FILE, std::ios::binary);
    if (!file) return;

    size_t count = users.size();
    file.write(reinterpret_cast<const char*>(&count), sizeof(count));

    for (const auto& user : users) {
        size_t emailLen = user.email.length();
        size_t passwordLen = user.password.length();

        file.write(reinterpret_cast<const char*>(&emailLen), sizeof(emailLen));
        file.write(reinterpret_cast<const char*>(&user.email[0]), emailLen * sizeof(wchar_t));

        file.write(reinterpret_cast<const char*>(&passwordLen), sizeof(passwordLen));
        file.write(reinterpret_cast<const char*>(&user.password[0]), passwordLen * sizeof(wchar_t));
    }
}

// Обработчик сообщений для окна "О программе".
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}