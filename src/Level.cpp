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

// Look up one of the extra properties we added in Tiled
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

// Turn a terrain name from Tiled into something the game understands
Tile TileFromName(const string& name) {
    if (name == "wall") return Tile::Wall;
    if (name == "water") return Tile::Water;
    if (name == "cargo") return Tile::Crystal;
    if (name == "hazard") return Tile::Hazard;
    return Tile::Floor;
}

// Start and goal need to be on the map and somewhere safe
bool IsSafeCell(const LevelDefinition& level, CellPos cell) {
    bool insideMap = cell.x >= 0 && cell.y >= 0;
    insideMap = insideMap && cell.x < GridWidth && cell.y < GridHeight;
    if (!insideMap) {
        return false;
    }

    Tile tile = level.terrain[cell.y][cell.x];
    return tile != Tile::Wall && tile != Tile::Hazard;
}

// Grab a number from the map or stick with the default
float ReadFloatProperty(const Json& map, const string& name, float defaultValue) {
    Json value = FindProperty(map, name);
    if (!value.is_number()) {
        return defaultValue;
    }
    return value.get<float>();
}

int ReadIntProperty(const Json& map, const string& name, int defaultValue) {
    Json value = FindProperty(map, name);
    if (!value.is_number_integer()) {
        return defaultValue;
    }
    return value.get<int>();
}

// Open a Tiled map and make sure it has what the game needs
bool LoadDefinition(const filesystem::path& file, LevelDefinition& level) {
    try {
        ifstream mapFile(file);
        if (!mapFile) {
            return false;
        }

        Json map;
        mapFile >> map;

        // For now every level uses the same square grid
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

        level.pawnSpawnSeconds = ReadFloatProperty(map, "pawnSpawnSeconds", level.pawnSpawnSeconds);
        level.pawnLimit = ReadIntProperty(map, "pawnLimit", level.pawnLimit);
        level.cargoGoal = ReadIntProperty(map, "cargoGoal", level.cargoGoal);
        level.rescueGoal = ReadIntProperty(map, "rescueGoal", level.rescueGoal);

        // Tiled writes a number for each tile
        // Match those numbers with the ground types in the tileset
        if (!map.contains("tilesets") || map["tilesets"].empty()) {
            return false;
        }

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

        // First read the ground, then look for start and goal
        bool foundTerrain = false;
        bool foundStart = false;
        bool foundGoal = false;

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
                string role = object.value("name", string{});
                int objectX = static_cast<int>(object.value("x", 0.0) / CellSize);
                int objectY = static_cast<int>(object.value("y", 0.0) / CellSize);

                // Tiled measures tile objects from the bottom edge
                bool isTileObject = object.contains("gid");
                if (isTileObject) {
                    objectY--;
                }

                CellPos cell = {objectX, objectY};
                if (role == "start") {
                    level.start = cell;
                    foundStart = true;
                }
                if (role == "goal") {
                    level.goal = cell;
                    foundGoal = true;
                }
            }
        }

        if (!foundTerrain) {
            return false;
        }

        // Make sure this map has a start, a goal and enough cargo
        int crystalCount = 0;
        for (const auto& row : level.terrain) {
            for (Tile tile : row) {
                if (tile == Tile::Crystal) {
                    crystalCount++;
                }
            }
        }

        bool hasValidObjects = foundStart && foundGoal;
        hasValidObjects = hasValidObjects && IsSafeCell(level, level.start);
        hasValidObjects = hasValidObjects && IsSafeCell(level, level.goal);

        bool hasEnoughCargo = crystalCount >= level.cargoGoal && level.cargoGoal > 0;
        bool hasValidPawnRules = level.pawnLimit > 0 && level.rescueGoal > 0;
        hasValidPawnRules = hasValidPawnRules && level.rescueGoal <= level.pawnLimit;
        hasValidPawnRules = hasValidPawnRules && level.pawnSpawnSeconds > 0.0f;

        return hasValidObjects && hasEnoughCargo && hasValidPawnRules;
    } catch (const exception&) {
        // A half-finished map should not stop the whole game loading
        return false;
    }
}
}

// Pick up the maps that are ready and sort them by name
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

// Keep the original around so Reset can put the map back
Level::Level(LevelDefinition definition)
    : definition_(move(definition)) {
    Reset();
}

void Level::Reset() {
    tiles_ = definition_.terrain;
}

// See if this cell is actually on the map
bool Level::IsInside(int x, int y) const {
    return x >= 0 && y >= 0 && x < GridWidth && y < GridHeight;
}

// Minions can try hazards, but walls stop them
bool Level::IsPathable(int x, int y) const {
    return IsInside(x, y) && tiles_[y][x] != Tile::Wall;
}

// Count how many grid steps apart these cells are
int Level::Distance(CellPos a, CellPos b) const {
    return abs(a.x - b.x) + abs(a.y - b.y);
}

// Treat anything outside the map like a wall
Tile Level::At(int x, int y) const {
    if (!IsInside(x, y)) {
        return Tile::Wall;
    }
    return tiles_[y][x];
}

// Pick up a crystal and clear its tile
bool Level::CollectCrystal(CellPos cell) {
    if (At(cell.x, cell.y) != Tile::Crystal) {
        return false;
    }

    tiles_[cell.y][cell.x] = Tile::Floor;
    return true;
}

// Find all the crystals still waiting on the map
vector<CellPos> Level::AvailableCrystals() const {
    vector<CellPos> crystals;

    for (int y = 0; y < GridHeight; y++) {
        for (int x = 0; x < GridWidth; x++) {
            if (tiles_[y][x] == Tile::Crystal) {
                crystals.push_back({x, y});
            }
        }
    }

    return crystals;
}
}
