#pragma once

#include "geo.h"
#include "svg.h"
#include "domain.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <vector>
#include <string>


/*
код, отвечающий за визуализацию карты маршрутов в формате SVG.
*/
namespace sphere_projector {

inline const double EPSILON = 1e-6;
bool IsZero(double value);

// Класс SphereProjector проецирует координаты (долготу, широту) в координаты (x,y) на плоскости
class SphereProjector {
public:

    SphereProjector() = default;
    /*
    points_begin и points_end задают начало и конец интервала элементов geo::Coordinates
    width и height задают ширину и высоту карты в пикселях,
    padding задаёт отступ краёв карты от границ SVG документа и также измеряется в пикселях
    */
    template <typename PointInputIt>
    SphereProjector(PointInputIt points_begin, PointInputIt points_end,
                    double max_width, double max_height, double padding)
        : padding_(padding) //
    {
        // Если точки поверхности сферы не заданы, вычислять нечего
        if (points_begin == points_end) {
            return;
        }

        // Находим точки с минимальной и максимальной долготой
        const auto [left_it, right_it] = std::minmax_element(
            points_begin, points_end,
            [](auto lhs, auto rhs) { return lhs.lng < rhs.lng; });
        min_lon_ = left_it->lng;
        const double max_lon = right_it->lng;

        // Находим точки с минимальной и максимальной широтой
        const auto [bottom_it, top_it] = std::minmax_element(
            points_begin, points_end,
            [](auto lhs, auto rhs) { return lhs.lat < rhs.lat; });
        const double min_lat = bottom_it->lat;
        max_lat_ = top_it->lat;

        // Вычисляем коэффициент масштабирования вдоль координаты x
        std::optional<double> width_zoom;
        if (!IsZero(max_lon - min_lon_)) {
            width_zoom = (max_width - 2 * padding) / (max_lon - min_lon_);
        }

        // Вычисляем коэффициент масштабирования вдоль координаты y
        std::optional<double> height_zoom;
        if (!IsZero(max_lat_ - min_lat)) {
            height_zoom = (max_height - 2 * padding) / (max_lat_ - min_lat);
        }

        if (width_zoom && height_zoom) {
            // Коэффициенты масштабирования по ширине и высоте ненулевые,
            // берём минимальный из них
            zoom_coeff_ = std::min(*width_zoom, *height_zoom);
        } else if (width_zoom) {
            // Коэффициент масштабирования по ширине ненулевой, используем его
            zoom_coeff_ = *width_zoom;
        } else if (height_zoom) {
            // Коэффициент масштабирования по высоте ненулевой, используем его
            zoom_coeff_ = *height_zoom;
        }
    }

    // Проецирует широту и долготу в координаты внутри SVG-изображения
    svg::Point operator()(geo::Coordinates coords) const;

private:
    double padding_;
    double min_lon_ = 0;
    double max_lat_ = 0;
    double zoom_coeff_ = 0;
};

} // namespace sphere_projector


namespace map_renderer { //

// структура svg-текста и подложки
struct NameSVG {
	svg::Text text;
	svg::Text background;
};

struct RenderSettings {
    /*
    ширина и высота изображения в пикселях. Вещественное число в диапазоне от 0 до 100000.
    отступ краёв карты от границ SVG-документа. Вещественное число не меньше 0 и меньше min(width, height)/2.
    */
    double width; 
    double height;
    double padding;

    //  толщина линий, которыми рисуются автобусные маршруты. Вещественное число в диапазоне от 0 до 100000
    double line_width;

    // радиус окружностей, которыми обозначаются остановки. Вещественное число в диапазоне от 0 до 100000
    double stop_radius;

    // размер текста, которым написаны названия автобусных маршрутов. Целое число в диапазоне от 0 до 100000
    int bus_label_font_size;

    /*
    смещение надписи с названием маршрута относительно координат конечной остановки на карте.
    Массив из двух элементов типа double. Задаёт значения свойств dx и dy SVG-элемента <text>.
    Элементы массива — числа в диапазоне от –100000 до 100000
    */
    svg::Point bus_label_offset;

    // размер текста, которым отображаются названия остановок. Целое число в диапазоне от 0 до 100000
    int stop_label_font_size;

    /*
    смещение названия остановки относительно её координат на карте. Массив из двух элементов типа double.
    Задаёт значения свойств dx и dy SVG-элемента <text>. Числа в диапазоне от –100000 до 100000
    */
    svg::Point stop_label_offset;

    //  цвет подложки под названиями остановок и маршрутов. Формат хранения цвета будет ниже.
    svg::Color underlayer_color;

    // толщина подложки под названиями остановок и маршрутов. Задаёт значение атрибута stroke-width элемента <text>. Вещественное число в диапазоне от 0 до 100000.
    double underlayer_width;

    // цветовая палитра. Непустой массив.
    std::vector<svg::Color> color_palette;
};    
    
class MapRendererSVG {
public:

    explicit MapRendererSVG() = default;

    // добавляет настройки визуализации карты маршрутов
    void AddRenderSettings(const RenderSettings& settings);

    // создаём проектор сферических координат на плоскость
    void SetSphereProjector(const std::vector<domain::Stop>& stops);

    // Устанавливает цвет для маршрута по его номеру
    svg::Color SetColor(uint32_t color_count);

    // возвращает svg-линию для заданного маршрута автобуса (для случаев кольцевого и прямого маршрутов)
    svg::Polyline RouteToPolyline(const std::vector<const domain::Stop*>& route, const svg::Color& color);

    // возвращает svg-название и подложку для заданного маршрута автобуса в точке остановки
    map_renderer::NameSVG RouteToText(const std::string& bus_name, const domain::Stop* stop, const svg::Color& color);

    // возвращает svg-круг для заданной остановки
    svg::Circle StopToSymb(geo::Coordinates coords);

    // возвращает svg-название и подложку для заданной остановки
    map_renderer::NameSVG StopToText(const domain::Stop& stop);

    // возвращает svg-линии для всех маршрутов
    std::vector<svg::Polyline> AllRoutesToPolylines(const std::vector<domain::Bus>& buses);

    // возвращает svg-текст с названиями маршрутов для всех маршрутов
    std::vector<map_renderer::NameSVG> AllRoutesToText(const std::vector<domain::Bus>& buses);

    // возвращает svg-круги для всех остановок 
    std::vector<svg::Circle> AllStopsToSymbs(const std::vector<domain::Stop>& stops);

    // возвращает svg-название и подложку для заданной остановки
    std::vector<map_renderer::NameSVG> AllStopsToText(const std::vector<domain::Stop>& stops);

    // рендерит карту маршрутов
    void RenderMap(std::ostream& out, const std::vector<domain::Bus>& buses, const std::vector<domain::Stop>& stop);

    

private:
    RenderSettings settings_;
    sphere_projector::SphereProjector sp_;
};   
    
} // namespace map_renderer 