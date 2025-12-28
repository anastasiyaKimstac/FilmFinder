// DatabaseTest.cpp - для тестирования подключения
#include "SearchWindow.h"
#include <windows.h>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    Database db;

    if (db.Connect()) {
        MessageBox(NULL, L"Подключение к SQL Server успешно!", L"Успех", MB_OK | MB_ICONINFORMATION);

        // Тестовый поиск
        std::vector<FilmData> results = db.SearchFilms(
            L"Начало", L"", L"", L"", L"", L"", L"", L""
        );

        std::wstring msg = L"Найдено фильмов: " + std::to_wstring(results.size());
        MessageBox(NULL, msg.c_str(), L"Результат", MB_OK);

        db.Disconnect();
    }
    else {
        MessageBox(NULL,
            L"Не удалось подключиться к SQL Server!\n"
            L"Проверьте:\n"
            L"1. Запущен ли SQL Server (служба MSSQLSERVER)\n"
            L"2. Правильность строки подключения\n"
            L"3. Существует ли база данных FilmDatabase",
            L"Ошибка",
            MB_OK | MB_ICONERROR);
    }

    return 0;
}