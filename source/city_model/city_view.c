// SPDX-License-Identifier: GPL-3.0-only

#include <stdint.h>

#include "city_model/city_view.h"

#include "map_utils.h"
#include "room_game/building_info.h"
#include "room_game/room_game.h"
#include "room_game/tileset_info.h"

static uint16_t legacy_city_tiles[CITY_VIEW_CELL_COUNT];
static uint8_t legacy_city_flips[CITY_VIEW_CELL_COUNT];
static uint8_t water_animation_phases[CITY_VIEW_CELL_COUNT];
static uint8_t water_animation_enabled[CITY_VIEW_CELL_COUNT];

static uint8_t flip_connections(uint8_t mask, uint8_t flips)
{
    if (flips & 1)
    {
        uint8_t east = mask & CITY_VIEW_DIR_EAST;
        uint8_t west = mask & CITY_VIEW_DIR_WEST;
        mask &= (uint8_t)~(CITY_VIEW_DIR_EAST | CITY_VIEW_DIR_WEST);
        if (east) mask |= CITY_VIEW_DIR_WEST;
        if (west) mask |= CITY_VIEW_DIR_EAST;
    }
    if (flips & 2)
    {
        uint8_t north = mask & CITY_VIEW_DIR_NORTH;
        uint8_t south = mask & CITY_VIEW_DIR_SOUTH;
        mask &= (uint8_t)~(CITY_VIEW_DIR_NORTH | CITY_VIEW_DIR_SOUTH);
        if (north) mask |= CITY_VIEW_DIR_SOUTH;
        if (south) mask |= CITY_VIEW_DIR_NORTH;
    }
    return mask;
}

static uint8_t tile_connections(uint16_t tile, uint8_t network)
{
    uint8_t ns = CITY_VIEW_DIR_NORTH | CITY_VIEW_DIR_SOUTH;
    uint8_t ew = CITY_VIEW_DIR_EAST | CITY_VIEW_DIR_WEST;
    uint8_t ne = CITY_VIEW_DIR_NORTH | CITY_VIEW_DIR_EAST;
    uint8_t nw = CITY_VIEW_DIR_NORTH | CITY_VIEW_DIR_WEST;
    uint8_t es = CITY_VIEW_DIR_EAST | CITY_VIEW_DIR_SOUTH;
    uint8_t sw = CITY_VIEW_DIR_SOUTH | CITY_VIEW_DIR_WEST;
    uint8_t nes = ne | CITY_VIEW_DIR_SOUTH;
    uint8_t esw = es | CITY_VIEW_DIR_WEST;
    uint8_t nsw = ns | CITY_VIEW_DIR_WEST;
    uint8_t new_ = ne | CITY_VIEW_DIR_WEST;
    uint8_t all = ns | CITY_VIEW_DIR_EAST | CITY_VIEW_DIR_WEST;

    if (network == CITY_VIEW_NETWORK_ROAD)
    {
        if ((tile >= T_ROAD_TB) && (tile <= T_ROAD_TB_3)) return ns;
        if ((tile >= T_ROAD_LR) && (tile <= T_ROAD_LR_3)) return ew;
        if (tile == T_ROAD_RB) return es;
        if (tile == T_ROAD_LB) return sw;
        if (tile == T_ROAD_TR) return ne;
        if (tile == T_ROAD_TL) return nw;
        if (tile == T_ROAD_TRB) return nes;
        if (tile == T_ROAD_LRB) return esw;
        if (tile == T_ROAD_TLB) return nsw;
        if (tile == T_ROAD_TLR) return new_;
        if (tile == T_ROAD_TLRB) return all;
        if (tile == T_ROAD_TB_POWER_LINES || tile == T_ROAD_TB_BRIDGE) return ns;
        if (tile == T_ROAD_LR_POWER_LINES || tile == T_ROAD_LR_BRIDGE) return ew;
        if (tile == T_TRAIN_LR_ROAD) return ns;
        if (tile == T_TRAIN_TB_ROAD) return ew;
    }
    else if (network == CITY_VIEW_NETWORK_RAIL)
    {
        if (tile == T_TRAIN_TB || tile == T_TRAIN_TB_POWER_LINES || tile == T_TRAIN_TB_BRIDGE) return ns;
        if (tile == T_TRAIN_LR || tile == T_TRAIN_LR_POWER_LINES || tile == T_TRAIN_LR_BRIDGE) return ew;
        if (tile == T_TRAIN_RB) return es;
        if (tile == T_TRAIN_LB) return sw;
        if (tile == T_TRAIN_TR) return ne;
        if (tile == T_TRAIN_TL) return nw;
        if (tile == T_TRAIN_TRB) return nes;
        if (tile == T_TRAIN_LRB) return esw;
        if (tile == T_TRAIN_TLB) return nsw;
        if (tile == T_TRAIN_TLR) return new_;
        if (tile == T_TRAIN_TLRB) return all;
        if (tile == T_TRAIN_LR_ROAD) return ew;
        if (tile == T_TRAIN_TB_ROAD) return ns;
    }
    else if (network == CITY_VIEW_NETWORK_POWER)
    {
        if (tile == T_ROAD_TB_POWER_LINES || tile == T_POWER_LINES_TB || tile == T_POWER_LINES_TB_BRIDGE) return ns;
        if (tile == T_ROAD_LR_POWER_LINES || tile == T_POWER_LINES_LR || tile == T_POWER_LINES_LR_BRIDGE) return ew;
        if (tile == T_TRAIN_TB_POWER_LINES) return ew;
        if (tile == T_TRAIN_LR_POWER_LINES) return ns;
        if (tile == T_POWER_LINES_RB) return es;
        if (tile == T_POWER_LINES_LB) return sw;
        if (tile == T_POWER_LINES_TR) return ne;
        if (tile == T_POWER_LINES_TL) return nw;
        if (tile == T_POWER_LINES_TRB) return nes;
        if (tile == T_POWER_LINES_LRB) return esw;
        if (tile == T_POWER_LINES_TLB) return nsw;
        if (tile == T_POWER_LINES_TLR) return new_;
        if (tile == T_POWER_LINES_TLRB) return all;
    }
    return 0;
}

uint16_t CityView_GetLegacyTile(int x, int y)
{
    if ((x < 0) || (x >= CITY_VIEW_WIDTH) ||
        (y < 0) || (y >= CITY_VIEW_HEIGHT))
        return 0;

    return legacy_city_tiles[y * CITY_VIEW_WIDTH + x];
}

void CityView_SetLegacyFlipFlags(int x, int y, uint8_t flags)
{
    if ((x < 0) || (x >= CITY_VIEW_WIDTH) ||
        (y < 0) || (y >= CITY_VIEW_HEIGHT))
        return;
    legacy_city_flips[y * CITY_VIEW_WIDTH + x] = flags & 3;
}

uint8_t CityView_GetLegacyFlipFlags(int x, int y)
{
    if ((x < 0) || (x >= CITY_VIEW_WIDTH) ||
        (y < 0) || (y >= CITY_VIEW_HEIGHT))
        return 0;
    return legacy_city_flips[y * CITY_VIEW_WIDTH + x];
}

void CityView_SetLegacyTile(int x, int y, uint16_t tile_id)
{
    if ((x < 0) || (x >= CITY_VIEW_WIDTH) ||
        (y < 0) || (y >= CITY_VIEW_HEIGHT))
        return;

    int index = y * CITY_VIEW_WIDTH + x;
    uint16_t old_tile = legacy_city_tiles[index];
    uint16_t new_tile = tile_id & 0x3FF;
    uint16_t old_type = City_Tileset_Entry_Info(old_tile)->element_type & TYPE_MASK;
    legacy_city_tiles[index] = new_tile;

    water_animation_enabled[index] =
        (new_tile == T_WATER) || (new_tile == T_WATER_EXTRA);
    if (!water_animation_enabled[index])
        water_animation_phases[index] = 0;
    else if (old_type != TYPE_WATER)
        water_animation_phases[index] = new_tile == T_WATER_EXTRA;
}

int CityView_ReadCell(int x, int y, city_view_cell *out_cell)
{
    if ((out_cell == 0) || (x < 0) || (x >= CITY_VIEW_WIDTH) ||
        (y < 0) || (y >= CITY_VIEW_HEIGHT))
        return 0;

    uint16_t tile = CityView_GetLegacyTile(x, y);
    const city_tile_info *info = City_Tileset_Entry_Info(tile);
    uint16_t type = info->element_type;

    out_cell->kind = (city_view_kind)(type & TYPE_MASK);
    out_cell->networks = 0;
    if (type & TYPE_HAS_ROAD)
        out_cell->networks |= CITY_VIEW_NETWORK_ROAD;
    if (type & TYPE_HAS_TRAIN)
        out_cell->networks |= CITY_VIEW_NETWORK_RAIL;
    if (type & TYPE_HAS_POWER)
        out_cell->networks |= CITY_VIEW_NETWORK_POWER;

    uint8_t flips = legacy_city_flips[y * CITY_VIEW_WIDTH + x];
    out_cell->road_connections = flip_connections(tile_connections(tile, CITY_VIEW_NETWORK_ROAD), flips);
    out_cell->rail_connections = flip_connections(tile_connections(tile, CITY_VIEW_NETWORK_RAIL), flips);
    out_cell->power_connections = flip_connections(tile_connections(tile, CITY_VIEW_NETWORK_POWER), flips);
    out_cell->water_animation_phase = water_animation_phases[y * CITY_VIEW_WIDTH + x];

    out_cell->legacy_tile_id = tile;

    out_cell->footprint_origin_x = x + info->base_x_delta;
    out_cell->footprint_origin_y = y + info->base_y_delta;
    out_cell->footprint_width = 1;
    out_cell->footprint_height = 1;

    if ((type & TYPE_MASK) >= TYPE_RESIDENTIAL &&
        (type & TYPE_MASK) <= TYPE_RADIATION)
    {
        int ox = out_cell->footprint_origin_x;
        int oy = out_cell->footprint_origin_y;
        if ((ox >= 0) && (ox < CITY_VIEW_WIDTH) &&
            (oy >= 0) && (oy < CITY_VIEW_HEIGHT))
        {
            uint16_t base_tile = CityView_GetLegacyTile(ox, oy);
            const building_info *building = Get_BuildingFromBaseTile(base_tile);
            if ((building->width > 0) && (building->height > 0))
            {
                out_cell->footprint_width = (uint8_t)building->width;
                out_cell->footprint_height = (uint8_t)building->height;
            }
        }
    }
    return 1;
}

int CityView_AdvanceWaterAnimation(int x, int y, uint8_t *out_phase)
{
    if ((out_phase == 0) || (x < 0) || (x >= CITY_VIEW_WIDTH) ||
        (y < 0) || (y >= CITY_VIEW_HEIGHT))
        return 0;

    city_view_cell cell;
    int index = y * CITY_VIEW_WIDTH + x;
    if (!CityView_ReadCell(x, y, &cell) ||
        (cell.kind != CITY_VIEW_WATER) || !water_animation_enabled[index])
        return 0;

    water_animation_phases[index] ^= 1;
    *out_phase = water_animation_phases[index];
    return 1;
}

int CityView_CopySnapshot(city_view_cell *out_cells, size_t cell_capacity)
{
    if ((out_cells == 0) || (cell_capacity < CITY_VIEW_CELL_COUNT))
        return 0;

    for (int y = 0; y < CITY_VIEW_HEIGHT; y++)
    {
        for (int x = 0; x < CITY_VIEW_WIDTH; x++)
        {
            size_t index = (size_t)y * CITY_VIEW_WIDTH + x;
            if (!CityView_ReadCell(x, y, &out_cells[index]))
                return 0;
        }
    }

    return 1;
}

int CityView_ImportLegacyMap(const uint16_t *legacy_tiles)
{
    if (legacy_tiles == 0)
        return 0;

    for (int i = 0; i < CITY_VIEW_CELL_COUNT; i++)
    {
        legacy_city_tiles[i] = legacy_tiles[i] & 0x3FF;
        legacy_city_flips[i] = (uint8_t)(((legacy_tiles[i] >> 10) & 3));
        water_animation_enabled[i] = (legacy_city_tiles[i] == T_WATER) ||
                                     (legacy_city_tiles[i] == T_WATER_EXTRA);
        water_animation_phases[i] = legacy_city_tiles[i] == T_WATER_EXTRA;
    }

    copy_map_to_sbb(legacy_tiles, (void *)CITY_MAP_BASE,
                    CITY_VIEW_HEIGHT, CITY_VIEW_WIDTH);
    return 1;
}
