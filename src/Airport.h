#ifndef AIRPORT_H
#define AIRPORT_H

#include <string>

// One airport record from airports.dat (OpenFlights). Holds only the fields the
// project stores: name, city, country, IATA, ICAO, latitude and longitude.
// The IATA code is the BST key; Airport itself knows nothing about the tree.
class Airport {
public:
    Airport() = default;

    // Parameters follow the column order of airports.dat (fields 2-8).
    Airport(std::string name, std::string city, std::string country,
            std::string iata, std::string icao, double latitude, double longitude);

    const std::string& getName() const { return name; }
    const std::string& getCity() const { return city; }
    const std::string& getCountry() const { return country; }
    const std::string& getIata() const { return iata; }
    const std::string& getIcao() const { return icao; }
    double getLatitude() const { return latitude; }
    double getLongitude() const { return longitude; }

    // "BOS  General Edward Lawrence Logan International Airport, Boston, United States"
    std::string summary() const;

    // summary() plus ICAO code and location.
    std::string details() const;

private:
    std::string name;
    std::string city;
    std::string country;
    std::string iata;   // 3-character BST key, e.g. "BOS"
    std::string icao;   // 4-character code; empty when the data file has \N
    double latitude = 0.0;
    double longitude = 0.0;
};

#endif
