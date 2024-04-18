#define _USE_MATH_DEFINES
#include "geo.h"


namespace geo {
    using namespace std::literals;

    std::ostream& operator<<(std::ostream& out, const Coordinates& coord) {
            out << "("s << coord.lat << ","s << coord.lng << ")"s;
            return out;
        }

}  // namespace geo