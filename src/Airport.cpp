#include "Airport.h"

#include <iomanip>
#include <sstream>
#include <utility>

Airport::Airport(std::string name, std::string city, std::string country,
                 std::string iata, std::string icao, double latitude, double longitude)
    : name(std::move(name)),
      city(std::move(city)),
      country(std::move(country)),
      iata(std::move(iata)),
      icao(std::move(icao)),
      latitude(latitude),
      longitude(longitude) {}

std::string Airport::summary() const {
    return iata + "  " + name + ", " + city + ", " + country;
}

std::string Airport::details() const {
    std::ostringstream out;
    out << summary() << "  [ICAO " << (icao.empty() ? "-" : icao)
        << ", lat " << std::fixed << std::setprecision(4) << latitude
        << ", lon " << longitude << "]";
    return out.str();
}
