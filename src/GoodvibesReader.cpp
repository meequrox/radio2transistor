#include "GoodvibesReader.hpp"

#include <exception>

GoodvibesReader::GoodvibesReader(const std::string& path) noexcept(false) {
    if (path.substr(path.find_last_of(".")) != ".xml")
        throw std::invalid_argument(path + " is not an XML file.");

    doc.LoadFile(path.c_str());

    if (doc.Error()) throw std::invalid_argument("Error parsing " + path + " file. Does it exist?");

    stations = doc.FirstChildElement("Stations");
}

GoodvibesReader::stationsList GoodvibesReader::getStationsList() const {
    stationsList v;
    const tinyxml2::XMLElement* station = stations->FirstChildElement("Station");

    while (station) {
        const tinyxml2::XMLText* name = station->FirstChildElement("name")->FirstChild()->ToText();
        const tinyxml2::XMLText* uri = station->FirstChildElement("uri")->FirstChild()->ToText();

        v.push_back(std::make_pair(name->Value(), uri->Value()));

        station = station->NextSiblingElement("Station");
    }

    return v;
}
