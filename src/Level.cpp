#include "Level.h"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <unordered_map>
#include <utility>

namespace tri {
using namespace std;

namespace {
using Json = nlohmann::json;

// ==================== TILED HELPERS ====================
// Find a custom property by its name
Json FindProperty(const Json& owner, const string& name) {
    if (!owner.contains("properties") || !owner["properties"].is_array()) {
        return nullptr;
    }

    for (const Json& property : owner["properties"]) {
        if (property.value("name", string{}) == name) {
            return property.value("value", Json{});
        }
    }

    return nullptr;
}

// Turn a Tiled tile name into a game tile
Tile TileFromName(const string& name) {
    if (name == "wall") return Tile::Wall;
    if (name == "water") return Tile::Water;
    if (name == "cargo") return Tile::CargoStation;
    if (name == "hazard") return Tile::Hazard;
    return Tile::Floor;
}

// Read an integer setting or keep its default
int ReadIntProperty(const Json& map, const string& name, int defaultValue) {
    Json value = FindProperty(map, name);
    if (!value.is_number_integer()) {
        return defaultValue;
    }
    return value.get<int>();
}

// The start tile has to be inside the map and safe
bool IsSafeCell(const LevelDefinition& level, CellPos cell) {
    bool insideMap = cell.x >= 0 && cell.y >= 0;
    insideMap = insideMap && cell.x < GridWidth && cell.y < GridHeight;
    if (!insideMap) {
        return false;
    }

    Tile tile = level.terrain[cell.y][cell.x];
    return tile != Tile::Wall && tile != Tile::Hazard;
}

// ==================== LOAD ONE MAP ====================
// Read the map, tileset and start point from a Tiled JSON file
bool LoadDefinition(const filesystem::path& file, LevelDefinition& level) {
    try {
        ifstream mapFile(file);
        if (!mapFile) {
            return false;
        }

        Json map;
        mapFile >> map;

        // This game uses a fixed size grid for every map
        bool correctFormat = map.value("orientation", string{}) == "orthogonal";
        correctFormat = correctFormat && !map.value("infinite", true);
        correctFormat = correctFormat && map.value("width", 0) == GridWidth;
        correctFormat = correctFormat && map.value("height", 0) == GridHeight;
        correctFormat = correctFormat && map.value("tilewidth", 0) == CellSize;
        correctFormat = correctFormat && map.value("tileheight", 0) == CellSize;

        if (!correctFormat) {
            return false;
        }

        Json name = FindProperty(map, "levelName");
        if (name.is_string()) {
            level.name = name.get<string>();
        } else {
            level.name = file.stem().string();
        }

        level.greenBeaconLimit = ReadIntProperty(map, "greenBeaconLimit", level.greenBeaconLimit);
        level.blueBeaconLimit = ReadIntProperty(map, "blueBeaconLimit", level.blueBeaconLimit);
        level.redBeaconLimit = ReadIntProperty(map, "redBeaconLimit", level.redBeaconLimit);

        if (!map.contains("tilesets") || map["tilesets"].empty()) {
            return false;
        }

        // Match Tiled tile IDs with the game tile types
        unordered_map<unsigned int, Tile> terrainByGid;
        for (const Json& tilesetReference : map["tilesets"]) {
            unsigned int firstGid = tilesetReference.at("firstgid").get<unsigned int>();
            string source = tilesetReference.at("source").get<string>();
            filesystem::path tilesetFile = file.parent_path() / source;

            ifstream tilesetStream(tilesetFile);
            if (!tilesetStream) {
                return false;
            }

            Json tileset;
            tilesetStream >> tileset;
            if (!tileset.contains("tiles")) {
                continue;
            }

            for (const Json& tile : tileset["tiles"]) {
                int localId = tile.at("id").get<int>();
                Json terrain = FindProperty(tile, "terrain");

                if (terrain.is_string()) {
                    unsigned int gid = firstGid + localId;
                    terrainByGid[gid] = TileFromName(terrain.get<string>());
                }
            }
        }

        // Read the terrain grid and the start marker
        bool foundTerrain = false;
        bool foundStart = false;
        if (!map.contains("layers") || !map["layers"].is_array()) {
            return false;
        }

        for (const Json& layer : map["layers"]) {
            string type = layer.value("type", string{});
            string layerName = layer.value("name", string{});

            if (type == "tilelayer" && layerName == "Terrain") {
                const Json& tileIds = layer.at("data");
                if (!tileIds.is_array() || tileIds.size() != GridWidth * GridHeight) {
                    return false;
                }

                for (int y = 0; y < GridHeight; y++) {
                    for (int x = 0; x < GridWidth; x++) {
                        int index = y * GridWidth + x;
                        unsigned int gid = tileIds[index].get<unsigned int>();
                        gid = gid & 0x1fffffffu;

                        auto terrain = terrainByGid.find(gid);
                        if (gid == 0 || terrain == terrainByGid.end()) {
                            return false;
                        }

                        level.terrain[y][x] = terrain->second;
                    }
                }

                foundTerrain = true;
            }

            if (type != "objectgroup") {
                continue;
            }

            Json objects = layer.value("objects", Json::array());
            for (const Json& object : objects) {
                if (object.value("name", string{}) != "start") {
                    continue;
                }

                int objectX = static_cast<int>(object.value("x", 0.0) / CellSize);
                int objectY = static_cast<int>(object.value("y", 0.0) / CellSize);

                if (object.contains("gid")) {
                    objectY--;
                }

                level.start = {objectX, objectY};
                foundStart = true;
            }
        }

        if (!foundTerrain || !foundStart || !IsSafeCell(level, level.start)) {
            return false;
        }

        // A playable map needs cargo and valid beacon limits
        int stationCount = 0;
        for (const auto& row : level.terrain) {
            for (Tile tile : row) {
                if (tile == Tile::CargoStation) {
                    stationCount++;
                }
            }
        }

        bool validBeaconLimits = level.greenBeaconLimit >= 1;
        validBeaconLimits = validBeaconLimits && level.blueBeaconLimit >= 0;
        validBeaconLimits = validBeaconLimits && level.redBeaconLimit >= 0;

        return stationCount > 0 && validBeaconLimits;
    } catch (const exception&) {
        return false;
    }
}
}

// ==================== LOAD LEVEL CATALOG ====================
// Find every valid Tiled map in the levels folder
vector<LevelDefinition> LoadLevelCatalog(const filesystem::path& directory) {
    vector<filesystem::path> mapFiles;
    error_code error;

    if (!filesystem::exists(directory, error)) {
        return {};
    }

    for (const auto& item : filesystem::directory_iterator(directory, error)) {
        if (item.is_regular_file() && item.path().extension() == ".tmj") {
            mapFiles.push_back(item.path());
        }
    }

    sort(mapFiles.begin(), mapFiles.end());

    vector<LevelDefinition> levels;
    for (const filesystem::path& file : mapFiles) {
        LevelDefinition level;
        if (LoadDefinition(file, level)) {
            levels.push_back(move(level));
        }
    }

    return levels;
}

// ==================== LEVEL DATA ====================
Level::Level(LevelDefinition definition)
    : definition_(move(definition)) {
    Reset();
}

// Restore the original tiles and rebuild the station list
void Level::Reset() {
    tiles_ = definition_.terrain;
    cargoStations_.clear();

    for (int y = 0; y < GridHeight; y++) {
        for (int x = 0; x < GridWidth; x++) {
            if (tiles_[y][x] == Tile::CargoStation) {
                cargoStations_.push_back({x, y});
            }
        }
    }

}

// Check whether a grid position is inside the map
bool Level::IsInside(int x, int y) const {
    return x >= 0 && y >= 0 && x < GridWidth && y < GridHeight;
}

// Walls block movement, while hazards can still be entered
bool Level::IsPathable(int x, int y) const {
    return IsInside(x, y) && tiles_[y][x] != Tile::Wall;
}

// A simple grid distance used to pick nearby goals
int Level::Distance(CellPos a, CellPos b) const {
    return abs(a.x - b.x) + abs(a.y - b.y);
}

// Return a wall when the position is outside the map
Tile Level::At(int x, int y) const {
    if (!IsInside(x, y)) {
        return Tile::Wall;
    }
    return tiles_[y][x];
}

}
