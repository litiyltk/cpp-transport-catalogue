#include "domain.h"


namespace domain {

    size_t Hasher::operator() (const Stop* pointer) const {
        return p_hasher_(pointer) * simple_number;
    }

    size_t Hasher::operator() (const std::pair<const Stop*, const Stop*>& pointer) const {
        return p_hasher_(pointer.first) * simple_number + p_hasher_(pointer.second) * simple_number * simple_number;
    }

} // namespace domain