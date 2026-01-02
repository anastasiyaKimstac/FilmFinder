#include "SearchWindow.h"
#include "MainWindow.h"
#include <commctrl.h>
#include <sstream>
#include <iomanip>
#include <algorithm>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "odbc32.lib")

// Глобальные переменные для этого модуля
static HWND g_hSearchWindow = NULL;
static Database g_database; // Делаем static
static HFONT hSearchFont = NULL;
static std::vector<FilmData> g_currentResults;

Database& GetDatabase() {
    return g_database;
}

//SQLHDBC GetDatabaseConnection() {
//    return g_database.hdbc;
//}

// Класс Database implementation
Database::Database() : henv(NULL), hdbc(NULL) {}

//Database::~Database() {
//    Disconnect();
//}

bool Database::Connect() {
    SQLRETURN ret;

    // Инициализация окружения ODBC
    ret = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);
    if (!SQL_SUCCEEDED(ret)) {
        return false;
    }

    ret = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);
    if (!SQL_SUCCEEDED(ret)) {
        SQLFreeHandle(SQL_HANDLE_ENV, henv);
        return false;
    }

    // Выделение дескриптора соединения
    ret = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);
    if (!SQL_SUCCEEDED(ret)) {
        SQLFreeHandle(SQL_HANDLE_ENV, henv);
        return false;
    }

    // Подключение к базе данных
    SQLWCHAR connectionString[] = L"DRIVER={SQL Server};SERVER=.\\SQLEXPRESS;DATABASE=FilmDatabase;Trusted_Connection=yes;";
    SQLWCHAR outConnStr[1024];
    SQLSMALLINT outConnStrLen;

    ret = SQLDriverConnectW(hdbc, NULL, connectionString, SQL_NTS,
        outConnStr, sizeof(outConnStr) / sizeof(SQLWCHAR), &outConnStrLen,
        SQL_DRIVER_COMPLETE);

    if (!SQL_SUCCEEDED(ret)) {
        SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
        SQLFreeHandle(SQL_HANDLE_ENV, henv);
        hdbc = NULL;
        henv = NULL;
        return false;
    }

    return true;
}

void Database::Disconnect() {
    if (hdbc) {
        SQLDisconnect(hdbc);
        SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
        hdbc = NULL;
    }
    if (henv) {
        SQLFreeHandle(SQL_HANDLE_ENV, henv);
        henv = NULL;
    }
}

std::vector<FilmData> Database::SearchFilms(const std::wstring& query,
    const std::wstring& yearFrom,
    const std::wstring& yearTo,
    const std::wstring& genre,
    const std::wstring& country,
    const std::wstring& director,
    const std::wstring& actor,
    const std::wstring& minRating) {

    std::vector<FilmData> results;

    if (!hdbc) {
        return results;
    }

    SQLHSTMT hstmt = NULL;
    SQLRETURN ret = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
    if (!SQL_SUCCEEDED(ret)) {
        return results;
    }

    // Строим динамический запрос
    std::wstring sql = L"SELECT DISTINCT f.filmID, f.filmName, f.yearOfRelease, f.director, f.reting, f.duration FROM Film f ";

    // Добавляем JOIN для фильтров
    if (!genre.empty()) {
        sql += L"INNER JOIN FilmGenre fg ON f.filmID = fg.filmID ";
        sql += L"INNER JOIN Genre g ON fg.genreID = g.genreId ";
    }

    if (!actor.empty()) {
        sql += L"INNER JOIN FilmActor fa ON f.filmID = fa.filmID ";
        sql += L"INNER JOIN Actor a ON fa.actorID = a.actorId ";
    }

    sql += L"WHERE 1=1 ";

    // Добавляем условия поиска
    if (!query.empty()) {
        sql += L"AND (f.filmName LIKE N'%" + query + L"%' ";
        sql += L"OR f.discription LIKE N'%" + query + L"%') ";
    }

    if (!yearFrom.empty()) {
        sql += L"AND f.yearOfRelease >= " + yearFrom + L" ";
    }

    if (!yearTo.empty()) {
        sql += L"AND f.yearOfRelease <= " + yearTo + L" ";
    }

    if (!genre.empty()) {
        sql += L"AND g.genreName = N'" + genre + L"' ";
    }

    if (!director.empty()) {
        sql += L"AND f.director LIKE N'%" + director + L"%' ";
    }

    if (!actor.empty()) {
        sql += L"AND a.actorName LIKE N'%" + actor + L"%' ";
    }

    if (!minRating.empty()) {
        sql += L"AND f.reting >= " + minRating + L" ";
    }

    sql += L"ORDER BY f.filmName";

    // Выполняем запрос
    ret = SQLExecDirectW(hstmt, (SQLWCHAR*)sql.c_str(), SQL_NTS);

    if (SQL_SUCCEEDED(ret)) {
        // Временные переменные для хранения данных
        SQLINTEGER filmId, year, duration;
        SQLDOUBLE rating;
        SQLWCHAR title[256];
        SQLWCHAR directorName[256];
        SQLINTEGER titleLen = 0, directorLen = 0;
        SQLLEN idInd = 0, yearInd = 0, ratingInd = 0, durationInd = 0, titleInd = 0, directorInd = 0;

        // Привязываем столбцы
        SQLBindCol(hstmt, 1, SQL_C_SLONG, &filmId, 0, &idInd);
        SQLBindCol(hstmt, 2, SQL_C_WCHAR, title, sizeof(title) / sizeof(SQLWCHAR), &titleInd);
        SQLBindCol(hstmt, 3, SQL_C_SLONG, &year, 0, &yearInd);
        SQLBindCol(hstmt, 4, SQL_C_WCHAR, directorName, sizeof(directorName) / sizeof(SQLWCHAR), &directorInd);
        SQLBindCol(hstmt, 5, SQL_C_DOUBLE, &rating, 0, &ratingInd);
        SQLBindCol(hstmt, 6, SQL_C_SLONG, &duration, 0, &durationInd);

        while (SQLFetch(hstmt) == SQL_SUCCESS) {
            // Проверяем, что данные не NULL
            if (idInd == SQL_NULL_DATA) continue;

            FilmData film;
            film.id = filmId;

            if (titleInd != SQL_NULL_DATA) {
                film.title = title;
            }

            if (yearInd != SQL_NULL_DATA) {
                film.year = year;
            }

            if (directorInd != SQL_NULL_DATA) {
                film.director = directorName;
            }

            if (ratingInd != SQL_NULL_DATA) {
                film.rating = rating;
            }

            if (durationInd != SQL_NULL_DATA) {
                film.duration = duration;
            }

            // Получаем жанры для этого фильма
            SQLHSTMT hstmtGenre = NULL;
            if (SQL_SUCCEEDED(SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmtGenre))) {
                std::wstring genreSql = L"SELECT g.genreName FROM Genre g INNER JOIN FilmGenre fg ON g.genreId = fg.genreID WHERE fg.filmID = " + std::to_wstring(film.id);

                if (SQLExecDirectW(hstmtGenre, (SQLWCHAR*)genreSql.c_str(), SQL_NTS) == SQL_SUCCESS) {
                    SQLWCHAR genreName[100];
                    SQLLEN genreNameInd;
                    SQLBindCol(hstmtGenre, 1, SQL_C_WCHAR, genreName, sizeof(genreName) / sizeof(SQLWCHAR), &genreNameInd);

                    while (SQLFetch(hstmtGenre) == SQL_SUCCESS) {
                        if (genreNameInd != SQL_NULL_DATA) {
                            if (!film.genres.empty()) film.genres += L", ";
                            film.genres += genreName;
                        }
                    }
                }
                SQLFreeHandle(SQL_HANDLE_STMT, hstmtGenre);
            }

            results.push_back(film);
        }
    }

    SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
    return results;
}

// Регистрация класса окна поиска
ATOM RegisterSearchWindowClass(HINSTANCE hInstance) {
    WNDCLASSEXW wcex = { 0 };

    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = SearchWndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = L"SearchWindowClass";
    wcex.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

    return RegisterClassExW(&wcex);
}

// Создание окна поиска
HWND CreateSearchWindow(HINSTANCE hInstance, HWND hParent) {
    if (!g_hSearchWindow) {
        RegisterSearchWindowClass(hInstance);

        g_hSearchWindow = CreateWindowW(
            L"SearchWindowClass",
            L"Поиск фильмов",
            WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            CW_USEDEFAULT, CW_USEDEFAULT,
            1000, 700,
            hParent,
            NULL,
            hInstance,
            NULL
        );

        // Подключаемся к базе данных
        if (!g_database.Connect()) {
            MessageBox(g_hSearchWindow,
                L"Не удалось подключиться к базе данных!\n"
                L"Проверьте:\n"
                L"1. Запущен ли SQL Server Express\n"
                L"2. Существует ли база данных FilmDatabase\n"
                L"3. Имя сервера: .\\SQLEXPRESS\n"
                L"4. Trusted_Connection=yes включен",
                L"Ошибка подключения",
                MB_OK | MB_ICONWARNING);
        }
    }

    return g_hSearchWindow;
}

// Вспомогательная функция для получения дескриптора соединения
SQLHDBC GetDatabaseConnection() {
    // В реальном проекте лучше использовать более сложную логику
    return g_database.hdbc;
}

// Заполнение фильтров
void PopulateSearchFilters(HWND hWnd) {
    // Заполняем годы
    HWND hYearFrom = GetDlgItem(hWnd, ID_YEAR_FROM_COMBO);
    HWND hYearTo = GetDlgItem(hWnd, ID_YEAR_TO_COMBO);
    HWND hRating = GetDlgItem(hWnd, ID_RATING_COMBO);

    // Годы с 1900 по текущий
    for (int year = 1900; year <= 2024; year++) {
        std::wstring yearStr = std::to_wstring(year);
        SendMessage(hYearFrom, CB_ADDSTRING, 0, (LPARAM)yearStr.c_str());
        SendMessage(hYearTo, CB_ADDSTRING, 0, (LPARAM)yearStr.c_str());
    }

    SendMessage(hYearFrom, CB_SETCURSEL, 124, 0); // 2024
    SendMessage(hYearTo, CB_SETCURSEL, 124, 0);   // 2024

    // Рейтинги
    const wchar_t* ratings[] = { L"Любой", L"1+", L"2+", L"3+", L"4+", L"5+", L"6+", L"7+", L"8+", L"9+", L"10" };
    for (int i = 0; i < 11; i++) {
        SendMessage(hRating, CB_ADDSTRING, 0, (LPARAM)ratings[i]);
    }
    SendMessage(hRating, CB_SETCURSEL, 0, 0);

    // Загружаем жанры из базы данных
    SQLHDBC hdbc = GetDatabaseConnection();
    if (hdbc) {
        SQLHSTMT hstmt = NULL;
        if (SQL_SUCCEEDED(SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt))) {
            if (SQLExecDirectW(hstmt, (SQLWCHAR*)L"SELECT genreName FROM Genre ORDER BY genreName", SQL_NTS) == SQL_SUCCESS) {
                SQLWCHAR genreName[100];
                SQLLEN nameInd;
                SQLBindCol(hstmt, 1, SQL_C_WCHAR, genreName, sizeof(genreName) / sizeof(SQLWCHAR), &nameInd);

                HWND hGenreList = GetDlgItem(hWnd, ID_GENRE_LIST);
                SendMessage(hGenreList, LB_ADDSTRING, 0, (LPARAM)L"Все жанры");

                while (SQLFetch(hstmt) == SQL_SUCCESS) {
                    if (nameInd != SQL_NULL_DATA) {
                        SendMessage(hGenreList, LB_ADDSTRING, 0, (LPARAM)genreName);
                    }
                }
                SendMessage(hGenreList, LB_SETCURSEL, 0, 0);
            }
            SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
        }
    }
}

// Выполнение поиска
void PerformSearch(HWND hWnd) {
    // Получаем значения фильтров
    wchar_t searchText[256];
    GetDlgItemText(hWnd, ID_SEARCH_EDIT, searchText, 256);

    wchar_t yearFrom[10], yearTo[10], rating[10];
    GetDlgItemText(hWnd, ID_YEAR_FROM_COMBO, yearFrom, 10);
    GetDlgItemText(hWnd, ID_YEAR_TO_COMBO, yearTo, 10);
    GetDlgItemText(hWnd, ID_RATING_COMBO, rating, 10);

    wchar_t director[100], actor[100];
    GetDlgItemText(hWnd, ID_DIRECTOR_EDIT, director, 100);
    GetDlgItemText(hWnd, ID_ACTOR_EDIT, actor, 100);

    // Получаем выбранный жанр
    HWND hGenreList = GetDlgItem(hWnd, ID_GENRE_LIST);
    int selIndex = SendMessage(hGenreList, LB_GETCURSEL, 0, 0);
    wchar_t genre[100] = L"";
    if (selIndex > 0) {
        SendMessage(hGenreList, LB_GETTEXT, selIndex, (LPARAM)genre);
    }

    // Выполняем поиск
    g_currentResults = g_database.SearchFilms(
        searchText,
        yearFrom,
        yearTo,
        genre,
        L"", // country - в вашей схеме нет стран
        director,
        actor,
        (std::wstring(rating) == L"Любой" || rating[0] == L'\0') ? L"" : std::wstring(rating).substr(0, 1)
    );

    // Обновляем список результатов
    HWND hResultsList = GetDlgItem(hWnd, ID_RESULTS_LIST);
    SendMessage(hResultsList, LB_RESETCONTENT, 0, 0);

    for (const auto& film : g_currentResults) {
        std::wstring listItem = film.title + L" (" + std::to_wstring(film.year) + L") - ?";

        // Форматируем рейтинг с одной цифрой после запятой
        std::wstringstream ratingStream;
        ratingStream << std::fixed << std::setprecision(1) << film.rating;
        listItem += ratingStream.str();

        if (!film.director.empty()) {
            listItem += L" | Реж. " + film.director;
        }
        if (!film.genres.empty()) {
            listItem += L" | " + film.genres;
        }
        SendMessage(hResultsList, LB_ADDSTRING, 0, (LPARAM)listItem.c_str());
    }

    // Обновляем статус
    std::wstring status = L"Найдено фильмов: " + std::to_wstring(g_currentResults.size());
    SetWindowText(GetDlgItem(hWnd, 120), status.c_str());
}

// Отображение деталей фильма
void DisplayFilmDetails(int filmId) {
    // Находим фильм в результатах
    auto it = std::find_if(g_currentResults.begin(), g_currentResults.end(),
        [filmId](const FilmData& film) { return film.id == filmId; });

    if (it != g_currentResults.end()) {
        std::wstring details = L"?? " + it->title + L"\n\n";
        details += L"?? Год: " + std::to_wstring(it->year) + L"\n";
        details += L"?? Режиссер: " + it->director + L"\n";

        std::wstringstream ratingStream;
        ratingStream << std::fixed << std::setprecision(1) << it->rating;
        details += L"? Рейтинг: " + ratingStream.str() + L"/10\n";

        details += L"?? Длительность: " + std::to_wstring(it->duration / 60) + L" ч " + std::to_wstring(it->duration % 60) + L" мин\n";
        details += L"??? Жанры: " + it->genres + L"\n";

        if (!it->actors.empty()) {
            details += L"?? Актёры: " + it->actors + L"\n";
        }

        MessageBox(NULL, details.c_str(), L"Информация о фильме", MB_OK | MB_ICONINFORMATION);
    }
}

// Обработчик сообщений окна поиска
LRESULT CALLBACK SearchWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE: {
        // Создаем шрифт
        hSearchFont = CreateFontW(
            16, 0, 0, 0, FW_NORMAL,
            FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE,
            L"Segoe UI"
        );

        // === СОЗДАЕМ ИНТЕРФЕЙС ПОИСКА ===

        // 1. Поле поиска
        CreateWindowW(L"STATIC", L"?? Поиск:",
            WS_CHILD | WS_VISIBLE,
            20, 20, 100, 30, hWnd, NULL, NULL, NULL);

        CreateWindowW(L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
            120, 20, 400, 30, hWnd, (HMENU)ID_SEARCH_EDIT, NULL, NULL);

        CreateWindowW(L"BUTTON", L"Найти",
            WS_CHILD | WS_VISIBLE,
            530, 20, 100, 30, hWnd, (HMENU)ID_SEARCH_BTN, NULL, NULL);

        CreateWindowW(L"BUTTON", L"Очистить",
            WS_CHILD | WS_VISIBLE,
            640, 20, 100, 30, hWnd, (HMENU)ID_CLEAR_BTN, NULL, NULL);

        // 2. Панель фильтров
        CreateWindowW(L"STATIC", L"Фильтры:",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            20, 70, 100, 30, hWnd, NULL, NULL, NULL);

        // Год выпуска
        CreateWindowW(L"STATIC", L"Год от:",
            WS_CHILD | WS_VISIBLE,
            20, 110, 60, 30, hWnd, NULL, NULL, NULL);
        CreateWindowW(L"COMBOBOX", L"",
            WS_CHILD | WS_VISIBLE | CBS_DROPDOWN | WS_VSCROLL,
            90, 110, 80, 200, hWnd, (HMENU)ID_YEAR_FROM_COMBO, NULL, NULL);

        CreateWindowW(L"STATIC", L"до:",
            WS_CHILD | WS_VISIBLE,
            180, 110, 30, 30, hWnd, NULL, NULL, NULL);
        CreateWindowW(L"COMBOBOX", L"",
            WS_CHILD | WS_VISIBLE | CBS_DROPDOWN | WS_VSCROLL,
            210, 110, 80, 200, hWnd, (HMENU)ID_YEAR_TO_COMBO, NULL, NULL);

        // Жанр
        CreateWindowW(L"STATIC", L"Жанр:",
            WS_CHILD | WS_VISIBLE,
            310, 110, 50, 30, hWnd, NULL, NULL, NULL);
        CreateWindowW(L"LISTBOX", L"",
            WS_CHILD | WS_VISIBLE | LBS_NOINTEGRALHEIGHT | WS_VSCROLL,
            370, 110, 150, 100, hWnd, (HMENU)ID_GENRE_LIST, NULL, NULL);

        // Режиссер
        CreateWindowW(L"STATIC", L"Режиссер:",
            WS_CHILD | WS_VISIBLE,
            20, 160, 80, 30, hWnd, NULL, NULL, NULL);
        CreateWindowW(L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | WS_BORDER,
            110, 160, 200, 30, hWnd, (HMENU)ID_DIRECTOR_EDIT, NULL, NULL);

        // Актёр
        CreateWindowW(L"STATIC", L"Актёр:",
            WS_CHILD | WS_VISIBLE,
            320, 160, 50, 30, hWnd, NULL, NULL, NULL);
        CreateWindowW(L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | WS_BORDER,
            380, 160, 200, 30, hWnd, (HMENU)ID_ACTOR_EDIT, NULL, NULL);

        // Рейтинг
        CreateWindowW(L"STATIC", L"Рейтинг от:",
            WS_CHILD | WS_VISIBLE,
            20, 210, 80, 30, hWnd, NULL, NULL, NULL);
        CreateWindowW(L"COMBOBOX", L"",
            WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,
            110, 210, 100, 200, hWnd, (HMENU)ID_RATING_COMBO, NULL, NULL);

        // Кнопки фильтров
        CreateWindowW(L"BUTTON", L"Применить фильтры",
            WS_CHILD | WS_VISIBLE,
            590, 110, 150, 35, hWnd, (HMENU)ID_APPLY_FILTERS_BTN, NULL, NULL);

        CreateWindowW(L"BUTTON", L"Сбросить фильтры",
            WS_CHILD | WS_VISIBLE,
            590, 155, 150, 35, hWnd, (HMENU)ID_RESET_FILTERS_BTN, NULL, NULL);

        // 3. Результаты поиска
        CreateWindowW(L"LISTBOX", L"",
            WS_CHILD | WS_VISIBLE | WS_BORDER | LBS_NOINTEGRALHEIGHT | WS_VSCROLL | LBS_NOTIFY,
            20, 260, 750, 300, hWnd, (HMENU)ID_RESULTS_LIST, NULL, NULL);

        // 4. Кнопки действий
        CreateWindowW(L"BUTTON", L"Подробнее",
            WS_CHILD | WS_VISIBLE,
            20, 580, 120, 40, hWnd, (HMENU)ID_DETAILS_BTN, NULL, NULL);

        CreateWindowW(L"BUTTON", L"В избранное",
            WS_CHILD | WS_VISIBLE,
            150, 580, 120, 40, hWnd, (HMENU)ID_ADD_FAVORITE_BTN, NULL, NULL);

        CreateWindowW(L"BUTTON", L"Оценить",
            WS_CHILD | WS_VISIBLE,
            280, 580, 120, 40, hWnd, (HMENU)ID_RATE_BTN, NULL, NULL);

        // 5. Статус
        CreateWindowW(L"STATIC", L"Найдено фильмов: 0",
            WS_CHILD | WS_VISIBLE,
            650, 580, 200, 40, hWnd, (HMENU)120, NULL, NULL);

        // Применяем шрифт
        EnumChildWindows(hWnd, [](HWND hwnd, LPARAM lParam) -> BOOL {
            SendMessage(hwnd, WM_SETFONT, (WPARAM)lParam, TRUE);
            return TRUE;
            }, (LPARAM)hSearchFont);

        // Заполняем фильтры
        PopulateSearchFilters(hWnd);

        break;
    }

    case WM_COMMAND: {
        int wmId = LOWORD(wParam);

        switch (wmId) {
        case ID_SEARCH_BTN:
        case ID_APPLY_FILTERS_BTN:
            PerformSearch(hWnd);
            break;

        case ID_CLEAR_BTN:
        case ID_RESET_FILTERS_BTN:
            // Очищаем все поля
            SetDlgItemText(hWnd, ID_SEARCH_EDIT, L"");
            SetDlgItemText(hWnd, ID_DIRECTOR_EDIT, L"");
            SetDlgItemText(hWnd, ID_ACTOR_EDIT, L"");
            SendDlgItemMessage(hWnd, ID_YEAR_FROM_COMBO, CB_SETCURSEL, 124, 0);
            SendDlgItemMessage(hWnd, ID_YEAR_TO_COMBO, CB_SETCURSEL, 124, 0);
            SendDlgItemMessage(hWnd, ID_RATING_COMBO, CB_SETCURSEL, 0, 0);
            SendDlgItemMessage(hWnd, ID_GENRE_LIST, LB_SETCURSEL, 0, 0);
            break;

        case ID_DETAILS_BTN: {
            HWND hList = GetDlgItem(hWnd, ID_RESULTS_LIST);
            int selIndex = SendMessage(hList, LB_GETCURSEL, 0, 0);
            if (selIndex != LB_ERR && selIndex < (int)g_currentResults.size()) {
                DisplayFilmDetails(g_currentResults[selIndex].id);
            }
            else {
                MessageBox(hWnd, L"Выберите фильм из списка", L"Ошибка", MB_OK | MB_ICONWARNING);
            }
            break;
        }

        case ID_ADD_FAVORITE_BTN:
            MessageBox(hWnd, L"Фильм добавлен в избранное", L"Успешно", MB_OK | MB_ICONINFORMATION);
            break;

        case ID_RATE_BTN:
            MessageBox(hWnd, L"Функция оценки фильма\n(будет реализовано позже)", L"Оценка", MB_OK | MB_ICONINFORMATION);
            break;

        case ID_RESULTS_LIST:
            if (HIWORD(wParam) == LBN_DBLCLK) {
                HWND hList = GetDlgItem(hWnd, ID_RESULTS_LIST);
                int selIndex = SendMessage(hList, LB_GETCURSEL, 0, 0);
                if (selIndex != LB_ERR && selIndex < (int)g_currentResults.size()) {
                    DisplayFilmDetails(g_currentResults[selIndex].id);
                }
            }
            break;
        }
        break;
    }

    case WM_SIZE: {
        // Изменяем размеры элементов при изменении размера окна
        RECT rc;
        GetClientRect(hWnd, &rc);

        // Масштабируем список результатов
        HWND hList = GetDlgItem(hWnd, ID_RESULTS_LIST);
        SetWindowPos(hList, NULL, 20, 260, rc.right - 40, rc.bottom - 350, SWP_NOZORDER);

        // Перемещаем кнопки действий
        int buttonY = rc.bottom - 70;
        SetWindowPos(GetDlgItem(hWnd, ID_DETAILS_BTN), NULL, 20, buttonY, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
        SetWindowPos(GetDlgItem(hWnd, ID_ADD_FAVORITE_BTN), NULL, 150, buttonY, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
        SetWindowPos(GetDlgItem(hWnd, ID_RATE_BTN), NULL, 280, buttonY, 0, 0, SWP_NOZORDER | SWP_NOSIZE);

        // Перемещаем статус
        SetWindowPos(GetDlgItem(hWnd, 120), NULL, rc.right - 200, buttonY, 0, 0, SWP_NOZORDER | SWP_NOSIZE);

        break;
    }

    case WM_DESTROY:
        if (hSearchFont) {
            DeleteObject(hSearchFont);
            hSearchFont = NULL;
        }
        g_database.Disconnect(); // Отключаемся от БД

        // Возвращаемся к главному окну
        HWND hMainWnd = GetParent(hWnd);
        if (hMainWnd && IsWindow(hMainWnd)) {
            ShowWindow(hMainWnd, SW_SHOW);
            BringWindowToTop(hMainWnd);
        }

        g_hSearchWindow = NULL;
        break;

        /*default:
            return DefWindowProc(hWnd, message, wParam, lParam);*/
    }
    return 0;
}