// SearchWindow.h
#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include <sqlext.h>  // Добавляем этот заголовок

// Идентификаторы элементов окна поиска
#define ID_SEARCH_EDIT            101
#define ID_SEARCH_BTN             102
#define ID_CLEAR_BTN              103
#define ID_YEAR_FROM_COMBO        104
#define ID_YEAR_TO_COMBO          105
#define ID_GENRE_LIST             106
#define ID_COUNTRY_EDIT           107
#define ID_DIRECTOR_EDIT          108
#define ID_ACTOR_EDIT             109
#define ID_RATING_COMBO           110
#define ID_RESULTS_LIST           111
#define ID_DETAILS_BTN            112
#define ID_ADD_FAVORITE_BTN       113
#define ID_RATE_BTN               114
#define ID_APPLY_FILTERS_BTN      115
#define ID_RESET_FILTERS_BTN      116

// Структура для хранения данных о фильме
struct FilmData {
    int id;
    std::wstring title;
    int year;
    std::wstring director;
    double rating;
    std::wstring genres;
    std::wstring actors;
    int duration;
};

// Класс для работы с базой данных
class Database {
private:
    SQLHENV henv;

public:
    SQLHDBC hdbc;  // Делаем публичным для доступа из SearchWindow.cpp

    Database();
    ~Database();
    bool Connect();
    void Disconnect();
    std::vector<FilmData> SearchFilms(const std::wstring& query,
        const std::wstring& yearFrom,
        const std::wstring& yearTo,
        const std::wstring& genre,
        const std::wstring& country,
        const std::wstring& director,
        const std::wstring& actor,
        const std::wstring& minRating);
};

// Функции окна поиска
ATOM RegisterSearchWindowClass(HINSTANCE hInstance);
HWND CreateSearchWindow(HINSTANCE hInstance, HWND hParent);
LRESULT CALLBACK SearchWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
void PopulateSearchFilters(HWND hWnd);
void PerformSearch(HWND hWnd);
void DisplayFilmDetails(int filmId);