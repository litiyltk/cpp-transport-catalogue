#include "transport_catalogue.h"


namespace transport_catalogue {

TransportCatalogue::TransportCatalogue() {}

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
    bus_info = { std::move(bus)->name, route.size(), 
                 ComputeCountUniqueStops(route),
                 ComputeRouteLength(route), 
                 ComputeRouteDistance(route) }; 
    return bus_info;
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

size_t TransportCatalogue::ComputeCountUniqueStops(const std::vector<const Stop*>& route) const {
    std::unordered_set<const Stop*> unique_stops{ route.begin(), route.end() };
    return unique_stops.size();
}

double TransportCatalogue::ComputeRouteLength(const std::vector<const Stop*>& route) const {
    double route_length = 0;
    for (auto i = 0; i < route.size() - 1; ++i) {
        geo::Coordinates start = route[i]->coordinates;
        geo::Coordinates finish = route[i+1]->coordinates;
        route_length += ComputeDistance(std::move(start), std::move(finish)); // расчёт по координатам остановок
    }
    return route_length;
}

int TransportCatalogue::ComputeRouteDistance(const std::vector<const Stop*>& route) const {
    int distance = 0;
    for (auto i = 0; i < route.size() - 1; ++i) {
        std::string_view start = route[i]->name;
        std::string_view finish = route[i+1]->name;
        distance += GetDistance(start, finish); // значение из distances_ для пар остановок
    }
    return distance;
}

} // namespace transport_catalogue