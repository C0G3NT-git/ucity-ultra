// SPDX-License-Identifier: GPL-3.0-only
//
// Read-only semantic view of the active city. This is a transition boundary:
// the current adapter still reads the legacy tile-backed map, but consumers
// can depend on this interface instead of VRAM addresses and tile metadata.

#ifndef CITY_MODEL_CITY_VIEW_H__
#define CITY_MODEL_CITY_VIEW_H__

#include <stddef.h>
#include <stdint.h>

#define CITY_VIEW_WIDTH  64
#define CITY_VIEW_HEIGHT 64
#define CITY_VIEW_CELL_COUNT (CITY_VIEW_WIDTH * CITY_VIEW_HEIGHT)

typedef enum {
    CITY_VIEW_FIELD = 0,
    CITY_VIEW_FOREST = 1,
    CITY_VIEW_WATER = 2,
    CITY_VIEW_RESIDENTIAL = 3,
    CITY_VIEW_INDUSTRIAL = 4,
    CITY_VIEW_COMMERCIAL = 5,
    CITY_VIEW_POLICE = 6,
    CITY_VIEW_FIRE_STATION = 7,
    CITY_VIEW_HOSPITAL = 8,
    CITY_VIEW_PARK = 9,
    CITY_VIEW_STADIUM = 10,
    CITY_VIEW_SCHOOL = 11,
    CITY_VIEW_HIGH_SCHOOL = 12,
    CITY_VIEW_UNIVERSITY = 13,
    CITY_VIEW_MUSEUM = 14,
    CITY_VIEW_LIBRARY = 15,
    CITY_VIEW_AIRPORT = 16,
    CITY_VIEW_PORT = 17,
    CITY_VIEW_DOCK = 18,
    CITY_VIEW_POWER_PLANT = 19,
    CITY_VIEW_DISASTER_FIRE = 20,
    CITY_VIEW_RADIATION = 21,
} city_view_kind;

enum {
    CITY_VIEW_NETWORK_ROAD = 1 << 0,
    CITY_VIEW_NETWORK_RAIL = 1 << 1,
    CITY_VIEW_NETWORK_POWER = 1 << 2,
};

// Direction bits for the three network masks returned on each cell.
enum {
    CITY_VIEW_DIR_NORTH = 1 << 0,
    CITY_VIEW_DIR_EAST  = 1 << 1,
    CITY_VIEW_DIR_SOUTH = 1 << 2,
    CITY_VIEW_DIR_WEST  = 1 << 3,
};

typedef struct {
    city_view_kind kind;
    uint8_t networks;
    uint8_t road_connections;
    uint8_t rail_connections;
    uint8_t power_connections;
    // Simulation animation phase, independent of the legacy water art tile.
    uint8_t water_animation_phase;

    // Temporary migration/debug field. New rendering code should use kind and
    // networks; this ID exists to diagnose legacy conversion discrepancies.
    uint16_t legacy_tile_id;

    // Coordinate of the declared building footprint origin for this tile.
    // For cells whose type is not a building, this is the cell coordinate.
    int16_t footprint_origin_x;
    int16_t footprint_origin_y;
    // Legacy-authored footprint size. Non-building cells are 1 × 1.
    uint8_t footprint_width;
    uint8_t footprint_height;
} city_view_cell;

// Returns 1 and fills out_cell for an in-bounds cell; returns 0 otherwise.
// The result is a snapshot of the current active city at call time.
int CityView_ReadCell(int x, int y, city_view_cell *out_cell);

// Advances water's simulation-owned animation phase for an in-bounds water
// cell. Returns 0 for invalid coordinates or non-water cells.
int CityView_AdvanceWaterAnimation(int x, int y, uint8_t *out_phase);

// Copies the complete city in row-major order into caller-owned storage.
// Returns 1 on success and 0 for a null or undersized output buffer.
int CityView_CopySnapshot(city_view_cell *out_cells, size_t cell_capacity);

// Imports a legacy 64 × 64 tile map into the current backing store.
int CityView_ImportLegacyMap(const uint16_t *legacy_tiles);

// Legacy adapter hooks. Coordinates must be in bounds. The setter changes
// model data only; renderer synchronization remains the draw layer's job.
uint16_t CityView_GetLegacyTile(int x, int y);
void CityView_SetLegacyTile(int x, int y, uint16_t tile_id);
void CityView_SetLegacyFlipFlags(int x, int y, uint8_t flags);
uint8_t CityView_GetLegacyFlipFlags(int x, int y);

#endif // CITY_MODEL_CITY_VIEW_H__
