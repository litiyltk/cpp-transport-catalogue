#pragma once

#include <cmath>
#include <iostream>


// функции для работы с географическими координатами
namespace geo {

struct Coordinates {

    bool operator==(const Coordinates& other) const {
        return lat == other.lat && lng == other.lng;
    }
    bool operator!=(const Coordinates& other) const {
        return !(*this == other);
    }

    double lat;
    double lng;
};

std::ostream& operator<<(std::ostream& out, const Coordinates& coord);

inline double ComputeDistance(Coordinates from, Coordinates to) {
    static const double EarthRadius = 6371000.0;
    using namespace std;
    if (from == to) {
        return 0;
    }
    static const double dr = 3.1415926535 / 180.;
    return acos(sin(from.lat * dr) * sin(to.lat * dr)
                + cos(from.lat * dr) * cos(to.lat * dr) * cos(abs(from.lng - to.lng) * dr))
        * EarthRadius;
}

} //namespace geo