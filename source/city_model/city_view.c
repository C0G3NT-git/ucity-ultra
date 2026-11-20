// SPDX-License-Identifier: GPL-3.0-only

#include <stdint.h>

#include "city_model/city_view.h"

#include "map_utils.h"
#include "room_game/room_game.h"
#include "room_game/tileset_info.h"

static uint16_t legacy_city_tiles[CITY_VIEW_CELL_COUNT];

uint16_t CityView_GetLegacyTile(int x, int y)
{
    if ((x < 0) || (x >= CITY_VIEW_WIDTH) ||
        (y < 0) || (y >= CITY_VIEW_HEIGHT))
        return 0;

    return legacy_city_tiles[y * CITY_VIEW_WIDTH + x];
}

void CityView_SetLegacyTile(int x, int y, uint16_t tile_id)
{
    if ((x < 0) || (x >= CITY_VIEW_WIDTH) ||
        (y < 0) || (y >= CITY_VIEW_HEIGHT))
        return;

    legacy_city_tiles[y * CITY_VIEW_WIDTH + x] = tile_id & 0x3FF;
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

    out_cell->legacy_tile_id = tile;

    out_cell->footprint_origin_x = x + info->base_x_delta;
    out_cell->footprint_origin_y = y + info->base_y_delta;
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
        legacy_city_tiles[i] = legacy_tiles[i] & 0x3FF;

    copy_map_to_sbb(legacy_tiles, (void *)CITY_MAP_BASE,
                    CITY_VIEW_HEIGHT, CITY_VIEW_WIDTH);
    return 1;
}
