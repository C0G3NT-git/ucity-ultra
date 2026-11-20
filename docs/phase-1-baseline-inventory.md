# Phase 1 — Baseline and tile dependency inventory

## Scope and evidence

This inventory covers the current 64 × 64 gameplay map, the direct tile read/write paths that couple rules to GBA art, available map fixtures, and the current save layout. It is based on the checked-in sources and generated assets. A standalone baseline utility and its CMake target were added; game runtime behavior and save data were not changed.

## City map representation

- `CITY_MAP_WIDTH` and `CITY_MAP_HEIGHT` are both 64, in `source/room_game/room_game.h`.
- The active city map is the GBA regular background map at `CITY_MAP_BASE`, in `room_game.c` loaded with `Load_City_Data()`.
- The cell tile index is used both as a drawing selection and as gameplay state. `CityMapGetType*()` in `draw_common.c` decodes type/flags through the indexed metadata in `tileset_info.c`; the metadata also carries multi-tile building origin offsets.
- `Simulation_TypeMatrixRefresh()` in `simulation/common.c` creates a derived 8-bit type/flag matrix for some passes. It does not remove the coupling: multiple simulation modules query tile IDs or write tile IDs directly.
- Placement and network editing are also drawing operations: `MapDrawBuilding()`, `MapDeleteBuilding()`, road/train/power-line routines, and port routines update the map tile as the operation takes effect.

## Direct tile-ID dependencies

`CityMapGetType*()` calls are semantically named but still depend on the tile metadata table. Direct tile ID comparisons are the strongest visual-data coupling and are listed below. `CityMapDrawTile*()` calls show rule changes that are currently committed by replacing the displayed tile.

| Area | Files | Tile-derived behavior to preserve in semantic data |
|---|---|---|
| Terrain generation and cleanup | `room_gen_map/generate_map.c`, `room_gen_map/room_gen_map.c` | Terrain classes, neighbor matching, coastline/forest autotiling, cleared tiles, extra animation variants. Generation includes deterministic seeds and water/land offset settings. |
| Building placement and demolition | `room_game/draw_building.c`, `room_game/draw_port.c`, `room_game/draw_common.c` | Valid occupancy, price/size, multi-cell footprint, origin lookup via base offsets, zone vs developed building, port water/dock cells, demolition state. |
| Road network | `room_game/draw_road.c`, `room_game/draw_common.c` | Directional connectivity, road/rail/power overlays, crossings, bridges, terrain restrictions, tile orientation. Road connections are currently materialized as composite art variants. |
| Rail network | `room_game/draw_train.c` | Directional connectivity, crossings with roads and power lines, bridges, and route-compatible structure. |
| Power network | `room_game/draw_power_lines.c` | Directional connectivity, overlap with road/rail, bridges, electricity transmission. |
| City type and shared map queries | `room_game/tileset_info.c`, `room_game/draw_common.c`, `simulation/common.c` | Tile-to-type/flag mapping, occupancy categories, neighbor queries, building origin metadata, derived type matrix. |
| Growth and abandonment | `simulation/create_buildings.c`, `simulation/building_count.c`, `simulation/calculate_stats.c`, `simulation/budget.c` | RCI zoning vs developed stages, occupied areas, building counts/types, tile footprint iteration and building state transitions. Many calls are through `CityMapGetTypeAndTile*()` and metadata rather than literal tile comparisons. |
| Power distribution | `simulation/power.c` | Energy cost and transmission properties, directional bridge blocking, powered building traversal, power plant classification. `CityTileDensityInfo(tile)` makes energy demand depend on tile ID. |
| Traffic and route calculation | `simulation/traffic.c` | Roads/rail and directional connections, building density/destination detection, multi-cell building origin, road traffic-level display variants and traffic animation. Traffic colors/levels are currently drawn by replacing road art tiles and flipping them. |
| Service coverage | `simulation/services.c` | Coverage sources are found by exact center-tile IDs; standard and large service masks depend on the building center and power status. |
| Water animation | `simulation/water.c` | Alternates `T_WATER` and `T_WATER_EXTRA`; this is visual animation only. |
| Fire and disasters | `simulation/fire.c`, `simulation/meltdown.c` | Fire eligibility/spread, multi-cell building origin, demolished residue, radiation on land/water. Fire animation is tile ID replacement. Disaster effects alter gameplay and must not be relegated to rendering. |
| Transport | `simulation/anim_planes.c`, `simulation/anim_trains.c`, `simulation/anim_boats.c`, `simulation/transport_anims.c` | Airport runway detection, rail and water route eligibility/orientation, spawn locations, bridge and port handling, moving vehicle state. Sprite display is separate but map routing is tile-dependent. |
| Player inspection / interaction | `room_game/room_game.c`, `room_game/building_info.c`, `room_game/build_menu.c` | Current cursor tile selection, information panel/building identity, allowed build action and visual preview. |
| Map overlays and city stats | `room_minimap/room_minimap.c`, `room_city_stats/room_city_stats.c` | Overview, zones, transport, service, power, density, traffic, pollution, happiness views, and city-area summaries. These are presentation consumers of simulation maps and semantic cell classifications. |

### Migration order implied by the inventory

1. Define independent cell queries and commands for terrain, zone/building occupancy, and network layers.
2. Move build/demolish and road/rail/power commands off of “write resulting artwork tile” operations.
3. Move traffic intensity, animation phase, fire, and radiation into explicit state.
4. Convert exact building-center scans and transport route checks to semantic entity/network queries.
5. Retain the old tile renderer as a derived view until parity is established; then retire its storage role.

## Available map fixtures

`maps/city/` contains source Tiled maps and generated map binaries:

| Fixture | Source | Compiled data | Size | Potential baseline coverage |
|---|---|---|---:|---|
| Rock River | `scenario_0_rock_river.tmx` | `scenario_0_rock_river.bin` | 8,192 bytes | River/terrain and early city state |
| Boringtown | `scenario_1_boringtown.tmx` | `scenario_1_boringtown.bin` | 8,192 bytes | Developed town and basic networks |
| Portville | `scenario_2_portville.tmx` | `scenario_2_portville.bin` | 8,192 bytes | Confirmed river/shore, port/docks, road and rail connections, dense mixed buildings |
| Newdale | `scenario_3_newdale.tmx` | `scenario_3_newdale.bin` | 8,192 bytes | Established city and service systems |
| Central | `scenario_4_central.tmx` | `scenario_4_central.bin` | 8,192 bytes | Confirmed dense city, water, rail links, large facilities and varied services |
| Futura | `scenario_5_futura.tmx` | `scenario_5_futura.bin` | 8,192 bytes | Confirmed advanced power/technology facilities, airport, rail and dense urban blocks |
| Test map | `test_map.tmx` | `test_map.bin` | 8,192 bytes | Confirmed focused developer layout with network junctions, zones, water, rail, facilities and unusual terrain; inspect at cell level for exact edge cases |

Each is 4,096 16-bit entries by size, matching 64 × 64 map cells. Scenario start funds, date, tax, city class, technology, and messages are defined separately in `source/room_scenarios/room_scenarios.c`; the binary map alone is not a complete game-state fixture. `generate_map.c` provides another deterministic fixture source when both seed bytes and land/water offset are recorded.

The current save is SRAM-backed through `source/save.c` and `MEM_SRAM`, with the SDL2 host implementation supplied by `libugba/source/sdl2/save_file.c`. User save files should remain outside source control. Use synthetic fixtures or an explicitly supplied sanitized fixture for shared import and regression work.

## Existing save format and migration implications

`city_save_data` in `source/save.h` stores city name, month/year, random seed, funds, class, last scroll cell, tax, technology, budget/loan information, persistent message flags, and graphs. The map is 4,096 low bytes plus a packed high-bit array of 512 bytes: every cell stores a 9-bit tile index. Four city slots fit in the overall SRAM save structure, which is limited to 32 KiB and guarded by a compile-time size assertion. The global settings save includes disasters, animations, music, and legacy graphics mode.

`Room_Game_City_Save()` reads each live map tile and packs its low 8 bits plus bit 8. `Room_Game_City_Load()` reconstructs the tile indices and writes them into the active GBA map. Consequently:

- Old saves are compact tile-map snapshots, not renderer-independent city documents.
- The save doesn't directly persist the derived traffic/service/power maps; those are recomputed, while persistent city details and graphs are stored.
- A desktop converter must decode each tile ID with the legacy tileset metadata, group multi-cell buildings, extract network layers and bridges, preserve zone/developed state and disasters, and import city metadata.
- Focused synthetic fixtures are still needed before claiming broad save migration coverage.

## Baseline fixture set to capture

Visual inspection of the checked-in scenario renderings confirms the broad feature coverage below. Cell-level decoding is still needed to make exact assertions about orientations and individual tile edge cases:

1. **Rock River:** terrain, coastline, roads, basic construction and map edge handling.
2. **Portville:** port/docks, water, boats, bridge/network combinations.
3. **Central:** dense buildings, service buildings, traffic and developed RCI areas.
4. **Futura:** advanced power/building types and technology progression.
5. **Generated map pair:** record identical seeds/offsets for land-heavy and water-heavy generation.
6. **Focused synthetic maps:** one fixture each for road/rail/power crossings, bridges in both orientations, each network over water, every large building footprint, runway, fire, meltdown, radiation, and map boundaries if the scenario assets do not cover them.

### Cell-level tile-family coverage

The compiled maps were read as 4,096 little-endian 16-bit tile entries each. This is static inspection, not a simulation regression run. Unique tile-ID counts by ranges in `tileset_info.h`:

| Map | Unique tile IDs | Zones 30–33 | Road 34–54 | Rail 55–71 | Power 72–84 | Disaster 480–483 |
|---|---:|---:|---:|---:|---:|---:|
| Rock River | 294 | 0 | 13 | 0 | 0 | 0 |
| Boringtown | 179 | 0 | 13 | 0 | 3 | 0 |
| Portville | 400 | 0 | 14 | 11 | 0 | 0 |
| Newdale | 389 | 4 | 15 | 0 | 6 | 0 |
| Central | 377 | 0 | 15 | 14 | 6 | 0 |
| Futura | 396 | 0 | 13 | 13 | 1 | 0 |
| Test map | 479 | 4 | 21 | 17 | 13 | 0 |

The test map has 479 distinct values across tile IDs 0–479, making it a strong fixture for the current terrain/network/building tile families. None of the seven scenario maps contains IDs 480–483 (fire and radiation), so those disaster states need synthetic fixtures or a controlled runtime capture. Visual tile-family presence does not by itself confirm the validity of all connections or building footprints.

The static semantic summary counts cells by base type and overlapping network flags. The `field` count includes cells that also carry road/rail/power, as the current metadata represents those as `TYPE_FIELD | network flag`.

| Map | Distinct tiles | Field | Forest | Water | Residential | Commercial | Industrial | Road cells | Rail cells | Power cells |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| Rock River | 294 | 840 | 1,952 | 1,017 | 54 | 44 | 26 | 193 | 0 | 34 |
| Boringtown | 179 | 831 | 2,508 | 616 | 33 | 20 | 24 | 105 | 0 | 25 |
| Portville | 400 | 1,210 | 1,072 | 1,308 | 99 | 83 | 71 | 321 | 97 | 66 |
| Newdale | 389 | 1,529 | 1,492 | 615 | 108 | 74 | 65 | 530 | 0 | 96 |
| Central | 377 | 1,727 | 638 | 763 | 216 | 149 | 67 | 756 | 221 | 166 |
| Futura | 396 | 1,846 | 57 | 212 | 582 | 350 | 147 | 1,572 | 271 | 286 |
| Test map | 479 | 1,750 | 1,121 | 612 | 146 | 74 | 74 | 719 | 199 | 174 |

The audit also prints an FNV-1a hash of the raw 16-bit map entries and a semantic hash over each cell's type/flags and building-origin offsets. Those hashes make accidental fixture changes easy to detect. They do not encode scenario metadata or simulation output.

### Directional network contact survey

The inspector decodes the tile's N/E/S/W visual connector mask, including GBA horizontal/vertical flip flags and the combined road/rail/power tiles. It reports `reciprocal / open / unpaired-contact` connector counts by network:

| Map | Road | Rail | Power |
|---|---:|---:|---:|
| Rock River | 422 / 0 / 1 | 0 / 0 / 0 | 0 / 68 / 0 |
| Boringtown | 222 / 3 / 0 | 0 / 0 / 0 | 20 / 29 / 2 |
| Portville | 694 / 7 / 0 | 194 / 1 / 0 | 0 / 126 / 6 |
| Newdale | 1,152 / 4 / 0 | 0 / 0 / 0 | 32 / 158 / 2 |
| Central | 1,650 / 5 / 0 | 444 / 2 / 0 | 46 / 270 / 16 |
| Futura | 3,476 / 3 / 4 | 544 / 3 / 0 | 2 / 550 / 20 |
| Test map | 1,552 / 13 / 0 | 422 / 6 / 0 | 142 / 201 / 13 |

These are visual adjacency counts, not connectivity pass/fail results. An unpaired contact can be an intentional perpendicular adjacency, and the power simulation's flood fill traverses ordinary power cells orthogonally regardless of the line graphic; only bridge orientation constrains it. The map renderer's connectivity tables also intentionally avoid updating bridge graphics. Before using these counts to validate gameplay connectivity, compare them against `draw_road.c`, `draw_train.c`, `draw_power_lines.c`, and the corresponding traffic/power route logic. The static checker is useful for locating unusual contacts, not proving service or route reachability.

For each fixture, capture canonical semantic summaries before migration: per-cell terrain, zone/building type and ID/footprint, network layer connectivity, bridge orientation, disaster state, city metadata, and selected simulation results after a fixed number of simulation steps. Rendering pixels should be captured separately and should not define gameplay parity.

### Footprint audit findings

Added `tools/map_audit.c`, a static inspector that uses the game's actual tile metadata and building dimensions. Build it from the repository root with:

```sh
cc -std=c11 -I source -I libugba/include \
  tools/map_audit.c source/room_game/tileset_info.c \
  source/room_game/building_info.c source/map_utils.c \
  -o /tmp/ucity-map-audit
```

Then pass compiled map binaries to `/tmp/ucity-map-audit`. This checks tile IDs, cell semantics, and whether multi-cell building parts point to complete footprints. The static scan found footprint mismatches in the checked-in map data:

| Map | Mismatched building parts reported |
|---|---:|
| Rock River | 1 |
| Boringtown | 0 |
| Portville | 2 |
| Newdale | 3 |
| Central | 2 |
| Futura | 4 |
| Test map | 1 |

Several mismatches come from high-school parts (tiles 148–156) overlapping cells assigned to a school (tiles 142–147); other cases involve dense industrial building parts. The audit uses the explicit 3×3, 3×2, and other footprints in `building_info.c`, and the coordinate-origin offsets in `tileset_info.c`. The original TMX cells confirm at least the high-school/school overlap, so this is present in source data rather than introduced by the binary conversion. These should not be silently repaired during import: first decide whether the old game intentionally tolerated these layouts, whether the maps have authoring errors, or whether the tile metadata interpretation needs adjustment.

The audit does not yet check network connection reciprocity or simulation consequences. Road, rail, and power tile family counts are summarized above, but those counts alone are not topology validation.

## Headless simulation baseline

Added `tools/sim_baseline/` and the `ucity-sim-baseline` CMake target. The runner initializes libugba in headless mode with no argv and passes null save paths, so it uses fresh memory rather than opening or writing the interactive game's `.sav`. It loads a compiled 64 × 64 map, applies the same controlled setup to every fixture (fixed fast/slow random seeds, January 1950, fixed funds/tax/class/technology, disasters and animations disabled), and advances `Simulation_SimulateAll()` for the requested number of steps. The CSV records population/RCI, demand, funds, pollution, traffic, power and service coverage plus FNV hashes of the map and several simulation grids.

Baseline output is checked in under `docs/baselines/`. Each scenario map and the test map has a 12-step CSV. Running each fixture twice produced byte-identical output across all seven maps. This is a controlled simulation fixture, not a capture of each scenario's authored starting date, budget, technology, or settings; those need separate scenario-specific baselines if exact scenario behavior is required. The hashes are regression fingerprints, not a substitute for inspecting semantic differences.

Build and run from the repository root:

```sh
cmake -S . -B /tmp/ucity-phase1-build -DBUILD_GBA=OFF -DENABLE_OPENGL=ON -DLUA_INTERPRETER_ENABLED=OFF
cmake --build /tmp/ucity-phase1-build --target ucity-sim-baseline -j4
/tmp/ucity-phase1-build/sdl2/ucity-sim-baseline built_assets/maps/city/test_map.bin 12
```

The executable accepts 1–240 steps. It only reads the supplied compiled map and prints CSV to stdout.

### Import policy proposal

Treat each legacy tile ID as source data during conversion and retain its original cell location and flags for diagnostics. Convert only complete, metadata-consistent building footprints into canonical building entities. Preserve incomplete or overlapping parts as unresolved legacy cell records and report their coordinates; do not drop, relocate, or silently repair them. The target renderer can show a fallback marker until a compatibility rule is agreed. This keeps import lossless while the old game's tolerance and the checked-in map authoring overlaps are still being investigated.

## Phase 1 status

- [x] Find existing map and scenario sources.
- [x] Trace save map encoding and load/save flow.
- [x] Inventory major direct tile-ID dependencies by subsystem.
- [x] Inspect scenario renderings and verify broad feature coverage.
- [x] Decode compiled maps and add a tile-family feature matrix.
- [x] Check multi-cell building footprints with the game's declared metadata; record mismatches for review.
- [x] Add and run a reusable static map audit over the seven compiled map fixtures.
- [x] Survey directional network connector reciprocity and open contacts from tile metadata.
- [ ] Validate unusual contacts against road/rail/power gameplay rules, especially bridges and compound tiles.
- [ ] Decide how desktop import should preserve or handle pre-existing incomplete footprints.
- [ ] Define deterministic semantic summaries and add broader synthetic save-slot fixtures.
- [x] Capture deterministic controlled simulation baselines for all seven checked-in map fixtures; run each twice and compare output.
- [ ] Add semantic save-slot summaries and synthetic fixtures for save decoding and import round trips.
