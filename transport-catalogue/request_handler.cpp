#include "request_handler.h"


namespace request_handler {
    using namespace std::literals;
    using namespace geo;
    using namespace domain;

std::optional<BusInfo> RequestHandler::GetBusStat(const std::string_view& bus_name) const {
    return db_.GetBusInfo(bus_name);
}

std::optional<StopInfo> RequestHandler::GetStopStat(const std::string_view& stop_name) const {
    return db_.GetStopInfo(stop_name);
}

const std::unordered_set<const Bus*>* RequestHandler::GetBusesByStop(const std::string_view& stop_name) const {
    return db_.GetBusesByStop(stop_name);
}

void RequestHandler::AddBusBaseRequest(const BusBaseRequest& request) {
    bus_base_requests_.emplace_back(request);
}

void RequestHandler::AddStopBaseRequest(const StopBaseRequest& request) {
    stop_base_requests_.emplace_back(request);
}

void RequestHandler::AddStatResult(const StatRequest& request) { //переработать через variant, чтобы не было обращения к полю type
    if (request.type == "Bus"s) {
        stat_results_.emplace_back(std::make_pair(request.id, db_.GetBusInfo(request.name)));
    } else if (request.type == "Stop"s) {
        stat_results_.emplace_back(std::make_pair(request.id, db_.GetStopInfo(request.name)));
    } else if (request.type == "Map"s) {
        stat_results_.emplace_back(std::make_pair(request.id, GetStringSVG()));
    } else if (request.type == "Route"s) {
        stat_results_.emplace_back(std::make_pair(request.id, ro_.GetOptimalRoute(request.from, request.to)));
    }
}

void RequestHandler::ApplyAllRequests() const { 
    
    // добавляем все остановки
    for (const auto& request : stop_base_requests_) {
        db_.AddStop(Stop({ request.name, Coordinates({ request.lat, request.lng }) }));
    }

    // добавляем все автобусы
    for (const auto& request : bus_base_requests_) {    
        std::vector<const Stop*> route; 
        for (const auto& stop: request.stops) {
            route.push_back(db_.FindStop(stop));
        } 
        db_.AddBus({ request.is_roundtrip, request.name, std::move(route) });
    }

    // устанавливаем расстояния между остановками
    for (const auto& request : stop_base_requests_) {
        for (const auto& line : request.road_distances) {
            db_.SetDistance(request.name, line.first, line.second); //start, finish, distance
        }
    }

}

void RequestHandler::AddRenderSettings(const map_renderer::RenderSettings& settings) {
    mr_.SetRenderSettings(settings);
}

void RequestHandler::AddRouterSettings(const RouterSettings& settings) {
    ro_.SetRouterSettings(settings);
}

void RequestHandler::SetTransportRouter() {
    ro_.SetVertexCount(db_.GetStops().size());
    ro_.BuildTransportRouter();
}

const std::vector<domain::StatResult>& RequestHandler::GetStatResults() const {
    return stat_results_; 
}

void RequestHandler::AddAllBuses() {
    for (const auto& bus : db_.GetBuses()) {
        if (bus.route.size()) {
            buses_.emplace_back(bus);
        }
    }
    std::sort(buses_.begin(), buses_.end(), //остановки в алфавитном порядке
        [](const Bus& lhs, const Bus& rhs) {
            return lhs.name < rhs.name;
        });
}

void RequestHandler::AddAllStops() {
    for (const auto& stop : db_.GetStops()) {
        if (GetBusesByStop(stop.name) != nullptr) {
            stops_.emplace_back(stop);
        }
    }
    std::sort(stops_.begin(), stops_.end(), //остановки в алфавитном порядке
        [](const Stop& lhs, const Stop& rhs) {
            return lhs.name < rhs.name;
        });
}

const std::vector<Bus>& RequestHandler::GetAllBuses() const {
    return buses_;
}

const std::vector<Stop>& RequestHandler::GetAllStops() const {
    return stops_;
}

const std::string RequestHandler::GetStringSVG() const {
    std::ostringstream strm;
    mr_.RenderMap(strm, buses_, stops_);
    return strm.str();
}
      
} //namespace request_handler