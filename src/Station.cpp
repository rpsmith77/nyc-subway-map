#include "Station.h"

Station::Station() : id(""), name("") {}

Station::Station(const std::string& id, const std::string& name)
    : id(id), name(name) {}