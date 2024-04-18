#include "transport_catalogue.h"


namespace transport_catalogue {
    using namespace domain;

void TransportCatalogue::AddStop(const Stop& stop) {
    const Stop* temp = &stops_.emplace_back(stop);
    stopname_to_stop_[temp->name] = temp;
    //std::cerr << "new Stop #"<< stops_.size() <<" added in deque" << std::endl;
}
    
void TransportCatalogue::AddBus(const Bus& bus) {
    const auto* temp = &buses_.emplace_back(bus);
    busname_to_bus_[temp->name] = temp;

    for (size_t i = 0; i < temp->route.size(); ++i) {
        stopname_to_buses_[std::move(temp->route[i])].insert(temp);
    }
    //std::cerr << "new Bus #" << buses_.size() <<" added in deque" << std::endl;
}

const Stop* TransportCatalogue::FindStop(const std::string_view name) const {
    return stopname_to_stop_.count(name) ? stopname_to_stop_.at(name) : nullptr;
}

const Bus* TransportCatalogue::FindBus(const std::string_view name) const {
    return busname_to_bus_.count(name) ? busname_to_bus_.at(name) : nullptr;
}
 
std::optional<BusInfo> TransportCatalogue::GetBusInfo(const std::string_view name) const {
    if (!busname_to_bus_.count(name)) {
        return std::nullopt;
    }
    BusInfo bus_info; 
    auto& bus = busname_to_bus_.at(name);
    auto route = bus->route;
    bool is_roundtrip = FindBus(name)->is_roundtrip;
    bus_info = { std::move(bus)->name,
                 ComputeCountStops(route.size(), is_roundtrip),
                 ComputeCountUniqueStops(route),
                 ComputeRouteLength(route, is_roundtrip), // географическая длина
                 ComputeRouteDistance(route, is_roundtrip) }; // фактическая длина
    return bus_info;
}

const std::unordered_set<const Bus*>* TransportCatalogue::GetBusesByStop(const std::string_view name) const {
    if (stopname_to_buses_.count(FindStop(name))) {
        auto* temp = &stopname_to_buses_.at(FindStop(name));
        return temp;
    } else {
        return nullptr;
    }
} 

std::optional<StopInfo> TransportCatalogue::GetStopInfo(const std::string_view name) const {
    const auto* stop = FindStop(name);
    if (stop == nullptr) {
        return std::nullopt;  
    }
    StopInfo stop_info;
    stop_info.name = name;
    if (stopname_to_buses_.count(stop)) {
        auto buses = stopname_to_buses_.at(stop);
        stop_info.bus_names.reserve(buses.size());
        for (const auto* bus : std::move(buses)) {
            stop_info.bus_names.emplace_back(std::move(bus->name));
        }
        std::sort(stop_info.bus_names.begin(), stop_info.bus_names.end());
    }
    return stop_info;
}

void TransportCatalogue::SetDistance(const std::string_view start, const std::string_view finish, const int distance) {
    const Stop* start_stop = FindStop(start);
    const Stop* finish_stop   = FindStop(finish);

    distances_[{ start_stop, finish_stop }] = distance;
    //std::cerr << "Distance between " << start << " and " << finish << ": " << distance << " m" << std::endl;
}
    
int TransportCatalogue::GetDistance(const std::string_view start, const std::string_view finish) const {
    const Stop* start_stop = FindStop(start);
    const Stop* finish_stop = FindStop(finish);

    if (distances_.count({ start_stop, finish_stop })) {
        return distances_.at({ start_stop, finish_stop });
    }

    return distances_.at({ finish_stop, start_stop });
}

const std::deque<Bus>& TransportCatalogue::GetBuses() const {
    return buses_;
}

const std::deque<Stop>& TransportCatalogue::GetStops() const {
    return stops_;
}

size_t TransportCatalogue::ComputeCountUniqueStops(const std::vector<const Stop*>& route) const {
    std::unordered_set<const Stop*> unique_stops{ route.begin(), route.end() };
    return unique_stops.size();
}

size_t TransportCatalogue::ComputeCountStops(const size_t stops_count, const bool is_roundtrip) const {
    return is_roundtrip ? stops_count : 2*stops_count - 1;
}

double TransportCatalogue::ComputeRouteLength(const std::vector<const Stop*>& route, const bool is_roundtrip) const {
    double route_length = 0;
    for (size_t i = 0; i < route.size() - 1; ++i) { // кольцевой маршрут по умолчанию A,B,C,A
        route_length += ComputeDistance(route[i]->coordinates, route[i+1]->coordinates); // расчёт по координатам остановок
    }

    if (!is_roundtrip) { // для прямого маршрута A,B,C,B,A путь туда-обратно A,B,C + C,B,A
        route_length *= 2;
    }

    return route_length;
}

int TransportCatalogue::ComputeRouteDistance(const std::vector<const Stop*>& route, const bool is_roundtrip) const {
    int distance = 0;
    for (size_t i = 0; i < route.size() - 1; ++i) {
        distance += GetDistance(route[i]->name, route[i+1]->name); // значение из distances_ для пар остановок
    }

    if (!is_roundtrip) { // для прямого маршрута A,B,C,B,A путь туда-обратно A,B,C + C,B,A
        for (size_t i = route.size() - 1; i > 0; --i) {
            distance += GetDistance(route[i]->name, route[i-1]->name); // значение из distances_ для пар остановок
        }
    }

    return distance;
}

} // namespace transport_catalogue