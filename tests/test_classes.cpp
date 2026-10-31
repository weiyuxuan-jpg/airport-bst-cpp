// Task 2 checks: Airport stores and prints its fields; an empty AirportBST
// behaves correctly; AirportBST cannot be copied.

#include <iostream>
#include <type_traits>

#include "Airport.h"
#include "AirportBST.h"
#include "check.h"

static_assert(!std::is_copy_constructible_v<AirportBST>, "AirportBST must not be copyable");
static_assert(!std::is_copy_assignable_v<AirportBST>, "AirportBST must not be copy-assignable");

int main() {
    // Boston, values copied from line 3,250 of airports.dat.
    Airport bos("General Edward Lawrence Logan International Airport", "Boston", "United States",
                "BOS", "KBOS", 42.36429977, -71.00520325);
    CHECK(bos.getIata() == "BOS");
    CHECK(bos.getIcao() == "KBOS");
    CHECK(bos.getName() == "General Edward Lawrence Logan International Airport");
    CHECK(bos.getCity() == "Boston");
    CHECK(bos.getCountry() == "United States");
    CHECK(bos.getLatitude() == 42.36429977);
    CHECK(bos.getLongitude() == -71.00520325);
    CHECK(bos.summary() ==
          "BOS  General Edward Lawrence Logan International Airport, Boston, United States");
    std::cout << "summary: " << bos.summary() << "\n";
    std::cout << "details: " << bos.details() << "\n";

    // A record with no ICAO code prints "-" in its place.
    Airport noIcao("Test Field", "Nowhere", "Testland", "QQQ", "", 0.5, -0.25);
    CHECK(noIcao.details().find("[ICAO -,") != std::string::npos);
    std::cout << "details: " << noIcao.details() << "\n";

    Airport empty;
    CHECK(empty.getIata().empty());
    CHECK(empty.getLatitude() == 0.0);

    AirportBST tree;
    CHECK(tree.isEmpty());
    CHECK(tree.size() == 0);
    tree.clear();   // clearing an empty tree is safe
    CHECK(tree.isEmpty());
    CHECK(tree.size() == 0);

    return finishTests();
}
