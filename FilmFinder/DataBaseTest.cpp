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

// Быстрая функция проверки подключения с разными строками подключения
bool TestDatabaseConnections() {
    // Список возможных строк подключения для тестирования
    std::vector<std::wstring> connectionStrings = {
        L"DRIVER={SQL Server};SERVER=.\\SQLEXPRESS;DATABASE=FilmDatabase;Trusted_Connection=yes;",
        L"DRIVER={SQL Server};SERVER=(local)\\SQLEXPRESS;DATABASE=FilmDatabase;Trusted_Connection=yes;",
        L"DRIVER={SQL Server};SERVER=localhost\\SQLEXPRESS;DATABASE=FilmDatabase;Trusted_Connection=yes;",
        L"DRIVER={SQL Server};SERVER=.;DATABASE=FilmDatabase;Trusted_Connection=yes;",
        L"DRIVER={SQL Server};SERVER=(local);DATABASE=FilmDatabase;Trusted_Connection=yes;",
        L"DRIVER={SQL Server};SERVER=localhost;DATABASE=FilmDatabase;Trusted_Connection=yes;",
    };

    for (const auto& connStr : connectionStrings) {
        MessageBox(NULL,
            (L"Попытка подключения:\n" + connStr).c_str(),
            L"Тест подключения",
            MB_OK | MB_ICONINFORMATION);

        // Здесь нужно как-то передать строку подключения в Database
        // В текущей реализации это не предусмотрено
    }

    // Используем стандартную проверку
    return CheckDatabaseConnection(connectionStrings[0]);
}