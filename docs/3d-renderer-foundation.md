# 3D renderer foundation

## Current architecture

The game has a 64 × 64 city grid. The current map is stored in GBA background map memory (`CITY_MAP_BASE` in `source/room_game/room_game.h`). Each cell is a tile index. `source/room_game/tileset_info.c` maps that index to a semantic type and, for multi-cell buildings, an offset back to the building's origin. The tile index therefore carries both gameplay state and presentation state.

`source/simulation/common.c` builds a byte `type_matrix` from the map for simulation passes, but that is only a derived cache: simulation modules also query tile indices directly. Examples include:

- `simulation/water.c`: animation toggles between two water tile IDs.
- `simulation/traffic.c`: traffic level is encoded by choosing among road tile IDs.
- `simulation/fire.c` and `simulation/meltdown.c`: disasters replace map tiles with fire, demolished, or radiation tiles.
- `simulation/services.c`: service coverage locates building centers by tile ID.
- `simulation/anim_planes.c`: airport runway detection uses a specific tile ID.
- `simulation/anim_trains.c` and `simulation/anim_boats.c`: transport routing inspects tile IDs.

Most simulation behavior uses `CityMapGetType*()` and `TYPE_*` flags, but these functions derive type from the graphics-oriented tile table. Building placement, growth, road connectivity, train connectivity, and power line connectivity live in `source/room_game/draw_*.c` and also mutate the rendered map directly. The current renderer and gameplay data are not independent.

## Recommended target model

Introduce a platform-neutral city model as the authoritative state, with rendering treated as a consumer. Keep grid coordinates and the existing simulation concepts initially; avoid redesigning the simulation and renderer at once.

At minimum, represent per-cell terrain and occupancy separately from overlays:

- **Terrain:** land, forest, water, demolished/contaminated state.
- **Networks:** road, rail, power, with connectivity/direction and bridge/elevation attributes as needed.
- **Zone or building:** semantic building type, stable building ID, and origin/footprint. Large buildings should be one building record with a footprint, not multiple unrelated visual tiles.
- **Simulation state:** traffic, service/power coverage, fire, pollution, and growth stage as explicit values instead of encoded artwork variants.
- **Presentation:** visual variant, mesh/model, material, animation phase, and LOD belong to renderer data or are derived from semantic state.

The SDL2 desktop target is the target product. The GBA renderer can remain temporarily available as a migration reference, but GBA support is not a release requirement. The 3D renderer can initially render the same fixed grid from an orbit camera; free placement, curved roads, terrain shaping, and other larger design changes can follow after the data boundary is proven.

## End-to-end implementation roadmap

The order below keeps the current game playable while reducing the risk of mixing a gameplay rewrite with a renderer rewrite. Each phase should land as a reviewable change. The legacy GBA build is useful during the semantic-model transition, then can be retired once the desktop game loop has replaced it.

### Phase 0 — Confirm product and technical constraints

**Repository findings so far**

- The available target today is the SDL2 desktop port plus optional GBA build; the configured local build has `BUILD_GBA=OFF` and `ENABLE_OPENGL=ON`.
- SDL2/OpenGL support is present in the bundled `libugba` layer, but it currently presents a GBA-sized 240 × 160 framebuffer. The game itself still renders through emulated GBA background and sprite APIs.
- Therefore, OpenGL being enabled is not itself a 3D renderer. A desktop city view needs its own render loop and renderer boundary; it should not be bolted into the existing GBA framebuffer path.
- The project has a working local desktop build and installed executable, so the SDL2 target can serve as the starting host while preserving the GBA build as an optional compatibility check.

**Decisions recorded**

- **Platform:** desktop only for the target game. Keep GBA compiling temporarily as a reference during data migration, then retire it when the desktop game loop replaces it.
- **Camera:** free orbit camera with pan and zoom.
- **Visual direction:** stylized low-poly city, aiming for a modern 3D city-builder presentation.
- **Compatibility:** import existing saves and scenarios.
- **Gameplay:** preserve current simulation rules for the first 3D version; treat larger simulation changes as separate follow-up work.
- **Renderer:** use SDL2 for window, input, and host integration, with a direct OpenGL renderer for the city scene. The current SDL/OpenGL-enabled dependency setup minimizes new platform dependencies. The 240 × 160 GBA emulation output stays separate from the new scene renderer.
- **Initial world scope:** retain the 64 × 64 grid and current placement rules. Terrain sculpting, freeform roads, citizen agents, and major simulation changes are outside the first playable 3D milestone.

**Still to measure before implementation**

- Establish a target desktop resolution and frame rate on the laptop, then set initial minimums after measuring the current machine.
- Confirm which operating systems need release packages; use the laptop OS as the first supported target.
- Select a 3D asset format and authoring workflow when the first placeholder building is introduced.

**Deliverable:** scope is sufficiently fixed to begin the dependency inventory and renderer spike. Performance and asset-format choices remain measurable implementation details rather than blockers for the semantic-map API.

### Phase 1 — Make the current behavior measurable

**Tasks**

1. Inventory each direct tile-ID read/write in simulation and gameplay; label the semantic role (road connectivity, traffic visual level, building origin, disaster state, animation frame, route lookup, etc.).
2. Record representative maps and save files covering water, forests, roads, rail, power lines, bridges, RCI zones and grown buildings, large buildings, disasters, and transport.
3. Document current map load/save flow, map generation, scenario initialization, simulation cadence, and GBA memory constraints.
4. Add deterministic regression checks for semantic map behavior before changing representation: load fixtures, run selected simulation steps, compare semantic outputs and save round trips.

**Deliverable:** dependency inventory and a small fixture-based baseline. The goal is to distinguish actual gameplay changes from rendering or data migration bugs.

### Phase 2 — Introduce a semantic city model behind the old renderer

**Progress:** the adapter is in `source/city_model/city_view.h` and `city_view.c`. It owns an 8 KiB row-major legacy tile array, bounds-checks semantic cell reads, can copy a complete row-major city into caller-owned storage, imports a legacy 64 × 64 map, and exposes a named cell kind, independent road/rail/power flags, cardinal connection masks for each network, building footprint origin and dimensions, water animation phase, and a temporary legacy tile ID for diagnostics. Connection masks use the `CITY_VIEW_DIR_NORTH/EAST/SOUTH/WEST` bits and account for legacy map flips. `CityMapGet*()` reads now use that array; `CityMapDrawTile*()` writes update it and the GBA display map. Save capture uses the model data, and scenario/save map loading crosses the adapter boundary. `city_commands.h/.c` defines typed standard-building, port, demolition, RCI-growth, and network-placement commands, plus a cell-level network removal command that handles bridge deletion through the legacy adapter. Build-menu placement, player demolition, forced simulation demolition, simulation-grown RCI buildings, ports, and road/rail/power placement and removal now route through these commands. The water animation phase is model-owned and the drawing adapter maps it to the old art variants; simulation no longer checks water tile IDs. The model still stores legacy tile IDs and derives semantics through the tile metadata, so gameplay rules remain coupled to tile families. Bridge topology and composite-tile interpretation still live in legacy routines. Canonical building records and full semantic network state remain future work. The 64 × 64 size is a deliberate transitional constraint.

**Storage boundary finding:** normal cell edits funnel through `CityMapDrawTile()` or `CityMapDrawTilePreserveFlip()`; scenario/save loads import through the adapter, and generated maps use the draw helper. Those paths now update model-owned legacy tile data and synchronize the GBA display map. Flip flags remain renderer details, mirrored in the adapter only so it can translate old art orientation into world-space network directions. Save capture reads model data. The store is now separate from VRAM, but tile IDs still encode gameplay state; the next representation change should replace them with explicit terrain, zoning/building, and network fields while keeping legacy conversion at import/export boundaries.

**Tasks**

1. Define platform-neutral types for terrain, zones, buildings, network layers, bridges, and per-cell simulation state. Keep the 64 × 64 grid initially.
2. Define explicit commands for construction, demolition, zone placement, network edits, building growth, disasters, and map loading. Define queries for cell type, neighbor connectivity, building footprint/origin, and route suitability.
3. Implement an adapter that exposes the current GBA tile map through those queries and commands. Keep the old tile map as the initial storage backend.
4. Route code through the API incrementally, beginning with city map access and building placement, then roads, trains, power, simulation, disasters, and transport.
5. Keep renderer-only choices—tile indices, flip bits, sprite coordinates, visual animation frames—inside the adapter/drawing layer.

**Deliverable:** gameplay callers use semantic operations while the game still renders and saves as before. No new renderer should depend on GBA VRAM layout.

### Phase 3 — Remove gameplay dependence on artwork variants

**Tasks**

1. Move traffic intensity out of road tile variants into traffic state; make road connectivity independent of the selected road graphic.
2. Move water, fire, and other animation phase out of map tile IDs into animation state.
3. Represent buildings as semantic records with stable IDs, type, origin, footprint, and growth stage; make cells refer to building occupancy rather than inferring origin from art tile offsets.
4. Represent roads, rail, and power as independent network layers so crossings and combinations are explicit, not a single composite tile ID.
5. Replace service-center and airport-runway tile scans with queries on building/network records.
6. Migrate simulation modules one subsystem at a time. Compare outputs against Phase 1 fixtures after each subsystem.

**Deliverable:** the simulation can update the city without drawing or choosing a visual tile. The old renderer derives its tiles from semantic state.

### Phase 4 — Establish desktop rendering and input boundaries

**Progress:** `ucity-3d-prototype` is a separate SDL2/OpenGL desktop executable. It reads a `CityView_CopySnapshot()` and draws a procedural demonstration city without reading GBA VRAM. The first camera supports drag-to-orbit, wheel zoom, and `R` reset. This is a renderer prototype only: it does not yet load saves or run the interactive game loop.

**Tasks**

1. Separate desktop startup, frame update, input, and rendering from the GBA display path; retain shared simulation code.
2. Add a renderer interface that consumes a read-only city view and receives viewport dimensions, camera state, and visual settings.
3. Add desktop input mapping for mouse, keyboard, and optionally controller. Keep simulation commands separate from camera controls.
4. Add viewport resizing, frame timing, logging, and a developer overlay for coordinates, selected cell, entity IDs, and frame time.
5. Preserve menus and game state where economical; identify UI pieces that need desktop-specific layout rather than carrying GBA screen assumptions into the new view.

**Deliverable:** desktop app opens a city through the new renderer boundary and can inspect the map, even if it is still rendered in a simple style.

### Phase 5 — Build the 3D city renderer in visual layers

**Tasks**

1. Render terrain as a grid mesh with terrain materials and water; add terrain height only if selected in Phase 0.
2. Render roads, rail, and power as separate overlays generated from semantic connectivity. Include bridges and intersections.
3. Render one placeholder 3D building per semantic building record, using footprint and growth stage for size and appearance.
4. Add a camera with pan, zoom, and orbit (or the chosen fixed camera), plus picking from screen position to grid cell.
5. Add selection highlighting, placement previews, invalid placement feedback, and build/demolish interaction.
6. Add lighting, shadows, fog/distance handling, asset variation, and LOD only after the basic renderer is stable and profiled.
7. Add animated traffic, trains, boats, and planes as renderer entities driven by simulation positions or route state.

**Deliverable:** a city can be viewed and edited in 3D with terrain, networks, buildings, and core selection/build flows.

### Phase 6 — Reconnect the full game loop and desktop UI

**Tasks**

1. Wire build tools and costs to semantic city commands.
2. Display budget, population, RCI demand, city class, service coverage, and notifications in desktop UI.
3. Add overlay modes for traffic, power, pollution, services, and education based on simulation fields.
4. Connect simulation time controls, pause, speed, disasters, scenarios, and map generation.
5. Add selection details for a building, road segment, network, or terrain cell.

**Deliverable:** the 3D build is a playable city builder, not only a scene viewer.

### Phase 7 — Persistence, compatibility, and content migration

**Tasks**

1. Define a versioned semantic save format independent of rendering assets.
2. Write a converter from existing saves and scenarios, including composite tiles, building origins, bridges, traffic levels, and disaster states.
3. Keep old-save import separate from ongoing new-format saves; do not silently reinterpret unknown versions.
4. Round-trip representative fixtures and compare city semantics, money/date, simulation state, and scenario settings.
5. Implement old city import for desktop. GBA save export/interoperability is out of scope because the target is desktop only.

**Deliverable:** deterministic save/load for the new game and a documented migration path for existing cities.

### Phase 8 — Content, performance, and release hardening

**Tasks**

1. Replace placeholder models/materials with a consistent asset set; establish scale, pivots, palettes/material rules, and naming conventions.
2. Profile at target city sizes. Optimize draw calls, geometry reuse, visibility culling, updates, and simulation/render synchronization based on measurements.
3. Check UI readability, camera usability, color/accessibility options, and keyboard/controller navigation.
4. Exercise long-running simulation, dense cities, map edges, unusual bridges/intersections, disasters, save migration, and window resize/minimize/restore.
5. Package assets and runtime dependencies for the target desktop OSes; document build, controls, known limitations, and save backups/migration.

**Deliverable:** release candidate with tested performance targets, migrated content, and reproducible builds.

## Milestone gates

| Gate | Evidence required to proceed |
|---|---|
| M1: Scope set | Platform, renderer, camera, visual target, compatibility, and performance expectations are written down. |
| M2: Baseline captured | Tile dependency inventory and representative deterministic fixtures exist. |
| M3: Semantic boundary works | Existing game runs through the map API with unchanged fixture results. |
| M4: Simulation is renderer independent | No simulation result depends on art tile IDs or renderer animation choices. |
| M5: 3D vertical slice | One loaded city supports camera movement, picking, terrain, roads, buildings, and a build action. |
| M6: Playable loop | Simulation, construction, overlays, UI, save/load, and scenarios work in desktop build. |
| M7: Release ready | Performance, compatibility, content, packaging, and regression checks meet agreed targets. |

## Main risks and mitigations

- **Tile IDs encode hidden rules.** A broad storage rewrite could subtly change connectivity, building scans, or disaster behavior. Mitigate with the inventory and fixture comparisons before migrating each subsystem.
- **Large buildings and composite networks are awkward in a per-cell model.** Use entity records and independent network layers with cell indexes, while keeping the grid as the initial placement space.
- **The current SDL2 path shares GBA-style assumptions.** Isolate a desktop renderer behind a clear boundary; avoid threading desktop graphics types through simulation code. Retire the GBA path only after the desktop loop and save import work.
- **Save conversion loses detail.** Preserve old saves as import fixtures and check semantic round trips before changing the default save path.
- **A visual prototype can hide a weak gameplay foundation.** Keep the first 3D slice limited to existing rules and verify build, simulation, and save behavior before expanding the game design.
- **Scope can expand into a full Cities: Skylines simulation.** Treat traffic simulation, curved/freeform roads, terrain tools, citizen agents, and large-scale economy changes as separate product decisions after the first playable 3D city.

## Immediate next work

Phase 0 is complete with the scope decisions above. Phase 1's static dependency and fixture inventory, seven-map controlled simulation baselines, and unresolved footprint findings are recorded in [phase-1-baseline-inventory.md](phase-1-baseline-inventory.md). The baseline comparison is now reproducible; validating selected bridge/network contacts and adding synthetic save import fixtures remain open tasks. Phase 2 has a tile-backed city view, explicit edit commands, directional network masks, and building footprint dimensions, and migrated map access, build, demolition, growth, and network editing through the adapter. An initial Phase 3 slice now stores water animation phase independently and keeps legacy visual tile selection in the drawing adapter. A separate OpenGL city-view prototype now reads the snapshot API and supports orbit and zoom. Remaining work includes replacing legacy tile-family interpretation with canonical cell and building state, connecting the prototype to actual city loading and simulation, adding picking and edit tools, moving bridge/composite interpretation behind semantic operations, and auditing residual direct tile-ID dependencies. The renderer must continue to consume the view without reading GBA VRAM directly.

The roadmap is being implemented incrementally; the existing save format remains unchanged.
