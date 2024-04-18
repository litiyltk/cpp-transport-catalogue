#include "map_renderer.h"


namespace sphere_projector {
    using namespace geo;
    using namespace domain;

bool IsZero(double value) {
    return std::abs(value) < EPSILON;
}

svg::Point SphereProjector::operator()(Coordinates coords) const {
    return {
        (coords.lng - min_lon_) * zoom_coeff_ + padding_,
        (max_lat_ - coords.lat) * zoom_coeff_ + padding_
    };
}

} // namespace sphere_projector


namespace map_renderer {
	using namespace std::literals;
    using namespace svg;
	using namespace geo;
    using namespace domain;

    void MapRendererSVG::SetRenderSettings(const RenderSettings& settings) {
        settings_ = settings;
    }	

    void MapRendererSVG::SetSphereProjector(const std::vector<Stop>& stops) {
		//добавляем координаты для остановок маршрута
		std::vector<Coordinates> coords;
		for (const auto& stop : stops) {
			coords.emplace_back(stop.coordinates); 
		}

		// проецируем координаты на плоскость
		sp_ = sphere_projector::SphereProjector(coords.begin(),
                                                coords.end(),
                                                settings_.width,
                                                settings_.height,
                                                settings_.padding);
	}

    Color MapRendererSVG::SetColor(uint32_t color_count) {
        uint32_t i = color_count % static_cast<uint32_t>(settings_.color_palette.size()); //
		return Color{ settings_.color_palette[i] };
	}

    Polyline MapRendererSVG::RouteToPolyline(const std::vector<const Stop*>& route, const Color& color) {
		// Задаём параметры линии маршрута для отрисовки
		Polyline polyline;
		polyline.SetStrokeColor(color)
				.SetFillColor("none"s)
				.SetStrokeLineCap(StrokeLineCap::ROUND)
				.SetStrokeLineJoin(StrokeLineJoin::ROUND)
				.SetStrokeWidth(settings_.line_width);

		// Проецируем координаты на плоскость
		for (auto stop : route) { 
			Point point = sp_({ stop->coordinates.lat, stop->coordinates.lng });
			polyline.AddPoint(point);
		}
		return polyline;
	}

	NameSVG MapRendererSVG::RouteToText(const std::string& bus_name, const Stop* stop, const Color& color) {
		// текст - название маршрута bus_name в точке, соответствующей остановке stop
		Text text;
		Point point = sp_({ stop->coordinates.lat, stop->coordinates.lng });
		text
			.SetPosition(point)
			.SetOffset(settings_.bus_label_offset)
			.SetFontSize(static_cast<uint32_t>(settings_.bus_label_font_size))
			.SetFontFamily("Verdana"s)
			.SetFontWeight("bold"s)
			.SetData(bus_name)
			.SetFillColor(color);
		// подложка
		Text background = text;
		background
			.SetStrokeColor(settings_.underlayer_color)
			.SetFillColor(settings_.underlayer_color)
			.SetStrokeWidth(settings_.underlayer_width)
			.SetStrokeLineCap(StrokeLineCap::ROUND)
			.SetStrokeLineJoin(StrokeLineJoin::ROUND);
		return { text, background };
	}

	Circle MapRendererSVG::StopToSymb(Coordinates coords) {
		Circle symbol;
		Point point = sp_({ coords.lat, coords.lng });
		symbol
			.SetCenter(point)
            .SetRadius(settings_.stop_radius)
            .SetFillColor("white"s);
		return symbol;
	}

	NameSVG MapRendererSVG::StopToText(const Stop& stop) {
		// текст - название остановки
		Text text;
		Point point = sp_({ stop.coordinates.lat, stop.coordinates.lng });
		text
			.SetPosition(point)
			.SetOffset(settings_.stop_label_offset)
			.SetFontSize(static_cast<uint32_t>(settings_.stop_label_font_size))
			.SetFontFamily("Verdana"s)
			.SetData(stop.name)
			.SetFillColor("black"s);
		// подложка
		Text background = text;
		background
			.SetStrokeColor(settings_.underlayer_color)
			.SetFillColor(settings_.underlayer_color)
			.SetStrokeWidth(settings_.underlayer_width)
			.SetStrokeLineCap(StrokeLineCap::ROUND)
			.SetStrokeLineJoin(StrokeLineJoin::ROUND);
		return { text, background };
	}

    std::vector<Polyline> MapRendererSVG::AllRoutesToPolylines(const std::vector<Bus>& buses) {
		std::vector<Polyline> polylines;
		uint32_t color_count = 0;
		for (const auto& bus : buses) {
			if (bus.route.size()) {
				// добавляем кольцевой маршрут ABCA
				auto route = bus.route;
				if (bus.is_roundtrip) {
					polylines.emplace_back(RouteToPolyline(route, SetColor(color_count)));
				// добавляем прямой маршрут ABC -> ABCBA
				} else {
					auto line_route(route);
					std::copy(route.rbegin() + 1, route.rend(), std::back_inserter(line_route));
					polylines.emplace_back(RouteToPolyline(line_route, SetColor(color_count)));
				}
				++color_count;
			}
		}
		return polylines;
	}

	std::vector<NameSVG> MapRendererSVG::AllRoutesToText(const std::vector<Bus>& buses) {
		std::vector<NameSVG> routes_names;
		uint32_t color_count = 0;
		for (const auto& bus : buses) {
			if (bus.route.size()) {
				// добавляем название маршрута у первой остановки (кольцевой маршрут ABCA)
				routes_names.emplace_back(RouteToText(bus.name, bus.route.front(), SetColor(color_count)));
				// добавляем название маршрута у последней остановки (только для прямого маршрута ABC, если остановки разные)
				if (!bus.is_roundtrip && bus.route.front() != bus.route.back()) { // добавить проверку, что остановки разные
					routes_names.emplace_back(RouteToText(bus.name, bus.route.back(), SetColor(color_count)));
				}
				++color_count;
			}
		}
		return routes_names;
	}

	std::vector<Circle> MapRendererSVG::AllStopsToSymbs(const std::vector<Stop>& stops) {
		std::vector<Circle> symbols;
		for (const auto& stop : stops) {
			symbols.emplace_back(StopToSymb(stop.coordinates));
		}
		return symbols;
	}

	std::vector<NameSVG> MapRendererSVG::AllStopsToText(const std::vector<Stop>& stops) {
		std::vector<NameSVG> stops_names;
		for (const auto& stop : stops) {
			stops_names.emplace_back(StopToText(stop));
		}
		return stops_names;
	}


	void MapRendererSVG::RenderMap(std::ostream& out, const std::vector<Bus>& buses, const std::vector<Stop>& stops) {
		SetSphereProjector(stops); // один раз настроить проекцию здесь
		svg::Document doc;
		// Слой 1. Добавляем все линии маршрутов в svg-документ
    	for (const auto& polyline : AllRoutesToPolylines(buses)) {
        	doc.Add(polyline);
    	}
		// Слой 2. Добавляем названия маршрутов автобусов в svg-документ
		for (const auto& route_name : AllRoutesToText(buses)) {
        	doc.Add(route_name.background);
			doc.Add(route_name.text);
    	}
		// Слой 3. Добавляем символы кругов для остановок в svg-документ
		for (const auto& stop : AllStopsToSymbs(stops)) {
			doc.Add(stop); 
		}
		// Слой 4. Добавляем названия остановок в svg-документ
		for (const auto& stop : AllStopsToText(stops)) {
        	doc.Add(stop.background);
			doc.Add(stop.text);
    	}
		doc.Render(out);
		//std::cerr << buses.size() << " " << stops.size() << std::endl;
	}
    
} // namespace map_renderer 