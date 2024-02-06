#pragma once

#include <iostream>
#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

#include "transport_catalogue.h"

// обработка запросов на заполнение справочника
namespace transport_catalogue::input_reader {
    using namespace std::literals;

struct CommandDescription {
    // Определяет, задана ли команда (поле command непустое)
    explicit operator bool() const {
        return !command.empty();
    }

    bool operator!() const {
        return !operator bool();
    }

    std::string command;      // Название команды
    std::string id;           // id маршрута или остановки
    std::string description;  // Параметры команды
};

class InputReader {
public:
    // 
    InputReader InputRequests(std::istream& input, TransportCatalogue& catalogue);

    /**
     * Парсит строку в структуру CommandDescription и сохраняет результат в commands_
     */
    void ParseLine(std::string_view line);

    /**
     * Наполняет данными транспортный справочник, используя команды из commands_
     */
    void ApplyCommands(TransportCatalogue& catalogue) const;

private:
    std::vector<CommandDescription> commands_;
    
};

} // namespace input_reader;