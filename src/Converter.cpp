#include "Converter.hpp"

#include <uuid.h>

#include <filesystem>
#include <fstream>
#include <iostream>

#include "../include/zipper.h"
#include "BookmarksReader.hpp"

namespace fs = std::filesystem;

#define COLLECTION_DIRNAME "collection"
#define COLLECTION_FILENAME "collection"
#define COLLECTION_JSONPATH COLLECTION_DIRNAME "/" COLLECTION_FILENAME ".json"
#define COLLECTION_M3UPATH COLLECTION_DIRNAME "/" COLLECTION_FILENAME ".m3u"

#define BACKUP_DIRNAME "transistor_backup"
#define COLLECTION_ZIPNAME BACKUP_DIRNAME ".zip"

#define NOMEDIA_FILENAME ".nomedia"

static std::string getCurrentDate() {
    constexpr size_t strSize = std::size("dd/mm/yy hh:mm am");
    char timeStr[strSize];

    time_t time = std::time(nullptr);
    strftime(std::data(timeStr), std::size(timeStr), "%D %I:%M %p", localtime(&time));

    return std::string(timeStr);
}

static std::string generateUUID() {
    constexpr size_t strSize = std::size("00011122-2333-4445-5566-777888999000");
    char uuidStr[strSize];

    uuid_t uuid;
    uuid_generate(uuid);
    uuid_unparse_lower(uuid, uuidStr);

    return std::string(uuidStr);
}

void Converter::buildBookmarksJSON() {
    // Modification date must be in the format "12/24/22 12:55 AM"
    bookmarksJSON["modificationDate"] = getCurrentDate();
    bookmarksJSON["stations"];
    bookmarksJSON["version"] = 0;

    size_t i = 0;
    for (const auto& station : bookmarksReader.getStationsList()) {
        nlohmann::json stationJSON(nullptr);

        // Default values
        stationJSON["homepage"] = "";
        stationJSON["image"] = "";
        stationJSON["imageColor"] = -1;
        stationJSON["imageManuallySet"] = false;
        stationJSON["nameManuallySet"] = true;
        stationJSON["playbackState"] = 1;
        stationJSON["radioBrowserChangeUuid"] = "";
        stationJSON["radioBrowserStationUuid"] = "";
        stationJSON["remoteImageLocation"] = "";
        stationJSON["remoteStationLocation"] = "";
        stationJSON["smallImage"] = "";
        stationJSON["starred"] = false;
        stationJSON["stream"] = 0;
        stationJSON["streamContent"] = "audio/mpeg";

        // Station-specific values
        stationJSON["modificationDate"] = bookmarksJSON["modificationDate"];
        stationJSON["name"] = station.first;
        stationJSON["streamUris"][0] = station.second;
        stationJSON["uuid"] = generateUUID();

        bookmarksJSON["stations"][i] = stationJSON;
        i++;
    }
}

void Converter::buildBookmarksM3U() {
    bookmarksM3U = "#EXTM3U\n\n";

    size_t i = 0;
    for (const auto& station : bookmarksReader.getStationsList()) {
        if (i) bookmarksM3U += "\n\n";

        bookmarksM3U += "#EXTINF:-1," + station.first + "\n";
        bookmarksM3U += station.second;

        i++;
    }
}

Converter::Converter(const BookmarksReader& _bookmarksReader)
    : bookmarksReader(_bookmarksReader), bookmarksJSON(nullptr) {
    buildBookmarksJSON();
    buildBookmarksM3U();
}

static void createCollectionDir() {
    std::string sep(1, fs::path::preferred_separator);
    std::string path = fs::current_path().generic_string() + sep + BACKUP_DIRNAME;
    fs::create_directory(path);

    path += sep + COLLECTION_DIRNAME;
    fs::create_directory(path);
}

void Converter::dumpJSON(bool verbose) const {
    if (verbose) std::cout << std::endl << "JSON:" << std::endl << bookmarksJSON.dump(2) << std::endl;

    std::string sep(1, fs::path::preferred_separator);
    std::string path = fs::current_path().generic_string() + sep + BACKUP_DIRNAME + sep +
                       COLLECTION_DIRNAME + sep + COLLECTION_FILENAME + ".json";

    std::ofstream file(path);

    if (!file) {
        std::cout << "Can't create json dump file: " << path << std::endl;
        return;
    }

    file << bookmarksJSON.dump();

    std::cout << "json is dumped to " << path << std::endl;
}

void Converter::dumpM3U(bool verbose) const {
    if (verbose) std::cout << std::endl << "M3U:" << std::endl << bookmarksM3U << std::endl;

    std::string sep(1, fs::path::preferred_separator);
    std::string path = fs::current_path().generic_string() + sep + BACKUP_DIRNAME + sep +
                       COLLECTION_DIRNAME + sep + COLLECTION_FILENAME + ".m3u";

    std::ofstream file(path);

    if (!file) {
        std::cout << "Can't create m3u dump file: " << path << std::endl;
        return;
    }

    file << bookmarksM3U << std::endl;

    std::cout << "m3u is dumped to " << path << std::endl;
}

static void dumpNomedia() {
    std::string sep(1, fs::path::preferred_separator);
    std::string path = fs::current_path().generic_string() + sep + BACKUP_DIRNAME + sep + ".nomedia";

    std::ofstream file(path);

    if (!file) {
        std::cout << "Can't create nomedia file: " << path << std::endl;
        return;
    }

    file << std::string(1, '\x00');
}

void Converter::dumpCollection(bool archive, bool verbose) const {
    if (archive) {
        zipper::Zip zip(COLLECTION_ZIPNAME);

        if (verbose) {
            std::cout << std::endl << "JSON:" << std::endl << bookmarksJSON.dump(2) << std::endl;
            std::cout << std::endl << "M3U:" << std::endl << bookmarksM3U << std::endl << std::endl;
        }

        if (!zip.is_open()) {
            std::cout << "Can't create backup archive: " << COLLECTION_ZIPNAME << std::endl;
            return;
        }

        zip.add_dir(COLLECTION_DIRNAME);
        zip.add_file(COLLECTION_JSONPATH, bookmarksJSON.dump());
        zip.add_file(COLLECTION_M3UPATH, bookmarksM3U + "\n");
        zip.add_file(NOMEDIA_FILENAME, std::string(1, '\x00'));
        zip.close();

        std::cout << "Collection is archived to " << COLLECTION_ZIPNAME << std::endl;
    } else {
        createCollectionDir();

        dumpJSON(verbose);
        dumpM3U(verbose);
        dumpNomedia();
    }
}
