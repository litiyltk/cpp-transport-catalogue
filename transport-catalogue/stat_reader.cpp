#include "stat_reader.h"


namespace transport_catalogue::stat_reader {
    using namespace std::literals;

void OutputRequests(const TransportCatalogue& transport_catalogue, std::istream& input, std::ostream& output) {
    int stat_request_count;
    input >> stat_request_count >> std::ws;
    for (int i = 0; i < stat_request_count; ++i) {
        std::string line;
        getline(input, line);
        transport_catalogue::stat_reader::ParseAndPrintStat(transport_catalogue, line, output);
    }
}

void ParseAndPrintStat(const TransportCatalogue& transport_catalogue, std::string_view request, std::ostream& output) {
    using namespace std::string_view_literals;
    CommandDescription line = ParseCommandDescription(request);
    if (line.command == "Bus"sv) {
        PrintBusInfo(transport_catalogue, std::move(line.description), output);
    } else if (std::move(line.command) == "Stop"sv) {
        PrintStopInfo(transport_catalogue, std::move(line.description), output);
    }
}

CommandDescription ParseCommandDescription(const std::string_view line) {
    using namespace std::string_literals;
    auto space_pos = line.find(' ');
    auto data_begin = line.find_first_not_of(' ', space_pos);

    std::string command = std::string(line.substr(0, space_pos));
    std::string description = std::string(line.substr(data_begin));

    return { command, description };
}

void PrintBusInfo(const TransportCatalogue& transport_catalogue, std::string_view name, std::ostream& output) {
    using namespace std::string_view_literals;
    auto bus_info = transport_catalogue.GetBusInfo(name);
    output << "Bus "sv << name << ": "sv;

    if (bus_info == std::nullopt) {
        output << "not found"sv << "\n"sv;
    } else {
        output  << bus_info.value().stops_on_route << " stops on route, "sv
                << bus_info.value().unique_stops << " unique stops, "sv
                << std::setprecision(6)
                << bus_info.value().route_length << " route length"sv
                << "\n"sv;
    }
}

void PrintStopInfo(const TransportCatalogue& transport_catalogue, std::string_view name, std::ostream& output) {
    using namespace std::string_view_literals;
    auto stop_info = transport_catalogue.GetStopInfo(name);
    output << "Stop "sv << name << ": "sv;
    
    if (stop_info == std::nullopt) {
        output << "not found"sv << "\n"sv;
    } else if (stop_info.value().bus_names.size()) {
        output << "buses "sv;
        for (const auto& name : stop_info.value().bus_names) {
            output << name << " "sv;
        }
        output << "\n"sv;
    } else {
        output << "no buses"sv << "\n"sv;
    }
}

} // namespace stat_reader