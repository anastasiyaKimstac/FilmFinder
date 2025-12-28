// FilmFinder.cpp : Определяет точку входа для приложения.
//

#include "framework.h"
#include "FilmFinder.h"
#include "MainWindow.h"
#include <vector>
#include <string>
#include "SearchWindow.h"
#include <fstream>
#include <commctrl.h>


#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "msimg32.lib")

#define MAX_LOADSTRING 100
Database g_database;
bool g_dbConnected = false;
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

// Глобальные шрифты для всего приложения
static HFONT hTitleFont = NULL;
static HFONT hLargeFont = NULL;

// Отправить объявления функций, включенных в этот модуль кода:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

// Функции для работы с авторизацией
void ShowAuthScreen(HWND hWnd);
void ShowRegistrationScreen(HWND hWnd);
void ShowMainScreen(HWND hWnd);
bool LoginUser(const std::wstring& email, const std::wstring& password);
bool RegisterUser(const std::wstring& email, const std::wstring& password);

// Функции для красивого оформления
void DrawGradientBackground(HDC hdc, RECT rect, COLORREF topColor, COLORREF bottomColor);
HFONT CreateCustomFont(int height, int weight = FW_NORMAL, const wchar_t* faceName = L"Arial");

bool CheckDatabaseConnection() {
    g_dbConnected = g_database.Connect();
    if (!g_dbConnected) {
        MessageBox(NULL,
            L"ВНИМАНИЕ: Не удалось подключиться к базе данных FilmDatabase\n\n"
            L"Приложение будет работать в демо-режиме.\n"
            L"Для полноценной работы:\n"
            L"1. Убедитесь, что SQL Server Express установлен и запущен\n"
            L"2. База данных FilmDatabase создана\n"
            L"3. Разрешить доступ для Windows-аутентификации\n\n"
            L"Для входа используйте: Email: 1, Пароль: 1",
            L"База данных недоступна",
            MB_OK | MB_ICONWARNING);
    }
    return g_dbConnected;
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // Инициализация common controls
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_STANDARD_CLASSES | ICC_BAR_CLASSES;
    InitCommonControlsEx(&icex);

    // Проверка подключения к БД
    CheckDatabaseConnection();

    // Инициализация глобальных строк
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_FILMFINDER, szWindowClass, MAX_LOADSTRING);

    // Регистрируем класс окна авторизации
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

    // Освобождаем ресурсы при выходе
    if (hTitleFont) DeleteObject(hTitleFont);
    if (hLargeFont) DeleteObject(hLargeFont);

    // Отключаемся от базы данных
    g_database.Disconnect();

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
//  ЦЕЛЬ: Обрабатывает сообщения в главном окна.
//
//  WM_COMMAND  - обработать меню приложения
//  WM_PAINT    - Отрисовка главного окна
//  WM_DESTROY  - отправить сообщение о выходе и вернуться
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
    {
        // ПОДКЛЮЧЕНИЕ К БАЗЕ ДАННЫХ
        Database db;
        if (!db.Connect()) {
            MessageBox(hWnd,
                L"Не удалось подключиться к базе данных FilmDatabase\n"
                L"Проверьте:\n"
                L"1. Запущен ли SQL Server Express\n"
                L"2. Существует ли база данных FilmDatabase\n"
                L"3. Включена ли аутентификация Windows\n"
                L"Для теста используйте '1' в оба поля",
                L"Ошибка подключения",
                MB_OK | MB_ICONWARNING);
        }
        else {
            MessageBox(hWnd,
                L"Подключение к базе данных успешно!",
                L"Успех",
                MB_OK | MB_ICONINFORMATION);
            db.Disconnect(); // Отключаемся, так как соединение будет создаваться в SearchWindow
        }
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

            // УПРОЩЕННАЯ АВТОРИЗАЦИЯ - достаточно ввести "1"
            if (wcscmp(email, L"1") == 0 && wcscmp(password, L"1") == 0) {
                currentUser = L"Администратор";
                isLoggedIn = true;
                ShowWindow(hWnd, SW_HIDE); // Скрываем окно авторизации
                CreateMainWindow(hInst, currentUser); // Создаем главное окно (из MainWindow.cpp)
            }
            else {
                MessageBox(hWnd, L"Неверный email или пароль! Введите '1' в оба поля для входа.", L"Ошибка", MB_OK | MB_ICONERROR);
            }
        }
        break;

        case IDC_BUTTON_REGISTER:
        {
            // Просто сообщаем, что регистрация не требуется
            MessageBox(hWnd, L"Регистрация не требуется. Для входа введите '1' в оба поля.", L"Информация", MB_OK | MB_ICONINFORMATION);
        }
        break;

        case IDC_BUTTON_CONFIRM_REGISTER:
        {
            // Ничего не делаем - регистрация отключена
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

            // Рисуем подсказку
            RECT hintRect = { 50, 500, clientRect.right - 50, 600 };
            HFONT hOldFont2 = (HFONT)SelectObject(hdcMem, hLargeFont);
            SetTextColor(hdcMem, RGB(100, 100, 100));
            DrawText(hdcMem, L"Для входа введите '1' в оба поля", -1, &hintRect, DT_CENTER | DT_SINGLELINE);
            SelectObject(hdcMem, hOldFont2);
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
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
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

    // Убираем кнопку регистрации - она не нужна
    // CreateWindowW(L"BUTTON", L"Зарегистрироваться",
    //     WS_CHILD | WS_VISIBLE,
    //     280, 440, 200, 55, hWnd, (HMENU)IDC_BUTTON_REGISTER, hInst, NULL);

    // Обновляем меню (убираем пункт "Выйти")
    HMENU hMenu = GetMenu(hWnd);
    if (hMenu) {
        RemoveMenu(hMenu, IDM_LOGOUT, MF_BYCOMMAND);
        DrawMenuBar(hWnd);
    }
}

// Функция для отображения экрана регистрации
void ShowRegistrationScreen(HWND hWnd)
{
    // Эта функция больше не используется
    ShowAuthScreen(hWnd);
}

// Функция для отображения главного экрана (после авторизации)
void ShowMainScreen(HWND hWnd)
{
    // Эта функция не используется, так как главное окно создается отдельно
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
    // УПРОЩЕННАЯ ВЕРСИЯ - только логин "1"
    if (email == L"1" && password == L"1") {
        return true;
    }

    return false;
}

// Функция для регистрации пользователя
bool RegisterUser(const std::wstring& email, const std::wstring& password)
{
    // Регистрация отключена
    return false;
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