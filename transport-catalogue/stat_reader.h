#pragma once

#include <iomanip> //для setprecision
#include <iosfwd>
#include <string>
#include <string_view>

#include "transport_catalogue.h"

// обработка запросов к базе и вывод данных
namespace transport_catalogue::stat_reader {

struct CommandDescription {
    // Определяет, задана ли команда (поле command непустое)
    explicit operator bool() const {
        return !command.empty();
    }

    bool operator!() const {
        return !operator bool();
    }

    std::string command;      // Название команды
    std::string description;  // Параметры команды
};

// вызывает PrintBusInfo или PrintStopInfo в зависимости от запроса в CommandDescription
void ParseAndPrintStat(const TransportCatalogue& transport_catalogue, std::string_view request, std::ostream& output);

// парсит строку запроса и возвращает CommandDescription
CommandDescription ParseCommandDescription(const std::string_view request);

// выводит информацию о маршруте автобуса Bus X: R stops on route, U unique stops, L route length
void PrintBusInfo(const TransportCatalogue& transport_catalogue, std::string_view name, std::ostream& output);

// выводит информацию об остановке выводит Stop X: buses bus1 bus2 ... busN
void PrintStopInfo(const TransportCatalogue& transport_catalogue, std::string_view name, std::ostream& output);

} // namespace stat_reader