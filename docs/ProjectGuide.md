# Game Overview

Tri-Beacon is a route-planning puzzle game built with C++ and raylib. The player studies a grid map, places colored beacons, and starts a run. Minions then move on their own, collect every cargo station, and return the cargo to the start.

The challenge is to find a safe, efficient route using the available map and beacons. A completed level is scored by delivery time, cargo delivered, beacon use, and minion survival.

## How a level works

1. Choose a level from the menu.
2. Study walls, hazards, cargo stations, portals, and any timed bridge.
3. Place one or more green beacons. A run cannot start without a green beacon.
4. Add blue and red beacons to influence movement costs around routes.
5. Start the run. Minions spawn once per second and follow the same route rules.
6. Each minion visits cargo stations one at a time. It must reach a station tile to collect it.
7. After visiting every station, a minion returns to the start with its cargo.
8. The first complete delivery stops spawning and starts a five-second scoring period. The run then ends and the score is saved if it beats the local best.

Every station remains available to every minion. A minion can collect each station once during its own trip. The level requires all stations to be visited and cargo to be returned home for a complete delivery.

## Beacon rules

Green beacons set route waypoints and cost 2 points each. Each minion follows green beacons it has not visited on its trip.

Blue beacons lower nearby movement costs and cost 1 point each.

Red beacons raise nearby movement costs and cost 1 point each.

Beacon effects use a square 5 by 5 area centered on the beacon. The A star pathfinder uses blue and red beacons when comparing routes. Red beacons only make nearby cells less attractive; they do not make hazards impassable. Minions can still enter hazards if the player leaves a low-cost route through them.

Cargo stations attract a minion only when it enters the station's 5 by 5 detection area. Once detected, the minion routes to the exact station tile to pick up the cargo. Cargo takes priority over a green waypoint while the minion is traveling to a detected station. After a pickup, it resumes following the next unvisited green waypoint. If there are no unvisited green beacons left, it heads home. Portal pads are one-way map transitions, but the pathfinder does not know their linked destinations. The player must guide minions to an entrance with beacons.

## Movement and pathfinding

The pathfinder uses A star search on the grid. It checks the four orthogonal neighbors of each cell, so minions cannot cut diagonally through corners. Walls block movement. Water is passable but slower and has a higher path cost.

The estimated remaining distance is Manhattan distance scaled by 0.25. Each normal step starts at cost 1. Water adds 1.5. A blue beacon subtracts 0.5 for each covered step and a red beacon adds 2.5. The final step cost cannot go below 0.25. The route with the lowest accumulated cost plus estimated remaining cost is selected.

The pathfinder intentionally does not treat hazards as blocked cells. Hazards are a risk the player must route around with beacons. If a minion enters one, it is lost.

## Cargo, run ending, and score

A minion must visit every cargo station before its trip is considered complete. Visiting a station means reaching its exact grid cell. A station visited by one minion remains available to other minions.

When the first minion carrying cargo from every station returns to the start, spawning stops immediately. The game allows five more seconds for already spawned minions to return. At the end of that period, remaining minions are counted as lost.

The score is based on cargo loads delivered during the scoring period, total run time, beacon cost counted for the run, and survival rate during the scoring period.

The implementation uses this formula:

    base = 1000 * loadsReturned / elapsedTime
    beaconMultiplier = 25 / (25 + runBeaconCost)
    score = base * beaconMultiplier

If at least 75 percent of the minions counted during the scoring period return, the score receives a 1.75 multiplier. Scores are saved per level in tri_beacon_scores.txt in the game's working directory. Beacons present when the run starts count toward beacon cost. Beacons placed during the run add cost, and removing them does not refund it.

## Project structure

- main.cpp creates the raylib window and runs input, update, and drawing each frame.
- Game.h and Game.cpp handle the menu, player input, beacons, minion behavior, run state, and scoring.
- GameTypes.h holds shared grid settings and the small data types used by the game.
- Level.h and Level.cpp read Tiled maps, store terrain, pair one-way portals, and update bridge events.
- Pathfinder.h and Pathfinder.cpp find minimum-cost routes using A star.
- Renderer.h and Renderer.cpp draw the menu, map, minions, beacons, and interface.

The code separates map loading, game rules, pathfinding, and drawing so each part has one main job. Game coordinates these parts.

## Main data structures

- CellPos stores a grid coordinate.
- Tile names the terrain type in each cell, such as floor, wall, water, cargo, hazard, bridge, or portal.
- LevelDefinition stores one loaded map, its start cell, and event timing.
- Beacon stores a beacon's position, type, and display color.
- Minion stores its current position and the stations and waypoints visited during one trip.
- GameStats stores totals used by the HUD and final score.
- Level owns a fixed-size 32 by 22 terrain grid and lists of cargo, bridge, and portal cells.
- std::vector stores variable-length lists such as minions, beacons, and map files. std::array stores the fixed map grid. std::priority_queue keeps the next A star cell with the lowest estimated route cost. std::unordered_map maps Tiled global tile IDs to game terrain types while loading a map.

## Level files

Levels are made in Tiled and saved as JSON map files with the .tmj extension. The game scans the levels folder and loads valid maps in filename order.

Each map must be orthogonal, finite, and 32 by 22 cells, with 32 by 32 pixel tiles. It needs a Terrain tile layer, a start object, at least one cargo station, and a tileset that defines the terrain names used by the game. Portal entrances and exits must have matching counts. A timed bridge level needs at least one bridge tile.

The map can define these custom properties:

- levelName sets the name shown in the level menu.
- bridgeCollapseSeconds sets when bridge cells become hazards. Set it to 0 to disable the event.
- bridgeWarningSeconds sets when the collapse warning appears.

The loader reads tile names from tileset terrain properties and converts Tiled tile IDs into Tile values. It also validates the grid, start location, cargo, and paired portal markers. Invalid maps are skipped.

## Controls

- Left and Right arrows change the selected level in the menu.
- Enter opens the selected level or starts a run.
- 1, 2, and 3 select green, blue, and red beacons.
- Left mouse button places the selected beacon on the map.
- Right mouse button removes a beacon from the selected cell.
- Space pauses or resumes a run.
- R resets the current level.
- M or Escape returns to the level menu.
- F11 toggles fullscreen.

## Main classes

- Game coordinates level selection, input, minions, beacons, run phases, and score saving.
- Level owns the terrain and changes bridge tiles into hazards when their event timer ends.
- Pathfinder returns a list of cells from a start cell to a target using A star.
- DrawGame renders the active screen from the current game state.

## Libraries

The game uses raylib for the window, input, and drawing. It uses the nlohmann JSON single-header library to read Tiled map files. The game logic and level loader are written in C++.

## Building the documentation

The Doxyfile is the Doxygen configuration. It reads the overview in this page and the documented public declarations in src. Doxygen creates LaTeX files, which are then compiled into the PDF with a LaTeX tool such as MiKTeX or TeX Live. The generated PDF is the submission copy; the temporary LaTeX files are build output.


