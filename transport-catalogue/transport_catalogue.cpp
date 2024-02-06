#include "transport_catalogue.h"


namespace transport_catalogue {

TransportCatalogue::TransportCatalogue() {}

void TransportCatalogue::AddStop(const Stop& stop) {
    const Stop* temp = &stops_.emplace_back(stop);
    stopname_to_stop_[std::move(temp->name)] = temp;
    //std::cerr << "new Stop #"<< stops_.size() <<" added in deque" << std::endl;
}
    
void TransportCatalogue::AddBus(const Bus& bus) {
    const auto* temp = &buses_.emplace_back(bus);
    busname_to_bus_[std::move(temp->name)] = temp;

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
    bus_info = { std::move(bus)->name, route.size(), ComputeCountUniqueStops(route), ComputeRouteLength(std::move(route)) };
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

size_t TransportCatalogue::ComputeCountUniqueStops(std::vector<const Stop*> route) const {
    std::unordered_set<const Stop*> unique_stops{ route.begin(), route.end() };
    return unique_stops.size();
}

double TransportCatalogue::ComputeRouteLength(std::vector<const Stop*> route) const {
    double route_length = 0;
    for (auto i = 0; i < route.size() - 1; ++i) {
        geo::Coordinates start = route[i]->coordinates;
        geo::Coordinates finish = route[i+1]->coordinates;
        route_length += ComputeDistance(std::move(start), std::move(finish));
    }
    return route_length;
}

} // namespace transport_catalogue