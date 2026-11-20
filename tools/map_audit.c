// SPDX-License-Identifier: GPL-3.0-only
//
// Static inspector for compiled 64x64 city maps. This is a development tool,
// not part of the game executable.

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "room_game/building_info.h"
#include "room_game/draw_common.h"
#include "room_game/room_game.h"
#include "room_game/tileset_info.h"

static uint16_t map[CITY_MAP_WIDTH * CITY_MAP_HEIGHT];

#define DIR_NORTH   (1 << 0)
#define DIR_EAST    (1 << 1)
#define DIR_SOUTH   (1 << 2)
#define DIR_WEST    (1 << 3)

static const int dir_dx[] = { 0, 1, 0, -1 };
static const int dir_dy[] = { -1, 0, 1, 0 };
static const unsigned char dir_bit[] = {
    DIR_NORTH, DIR_EAST, DIR_SOUTH, DIR_WEST
};
static const unsigned char opposite_bit[] = {
    DIR_SOUTH, DIR_WEST, DIR_NORTH, DIR_EAST
};

enum {
    NETWORK_ROAD,
    NETWORK_RAIL,
    NETWORK_POWER,
    NETWORK_COUNT,
};

static unsigned char flip_mask(unsigned char mask, int hflip, int vflip)
{
    if (hflip)
    {
        unsigned char east = mask & DIR_EAST;
        unsigned char west = mask & DIR_WEST;
        mask &= (unsigned char)~(DIR_EAST | DIR_WEST);
        if (east) mask |= DIR_WEST;
        if (west) mask |= DIR_EAST;
    }

    if (vflip)
    {
        unsigned char north = mask & DIR_NORTH;
        unsigned char south = mask & DIR_SOUTH;
        mask &= (unsigned char)~(DIR_NORTH | DIR_SOUTH);
        if (north) mask |= DIR_SOUTH;
        if (south) mask |= DIR_NORTH;
    }

    return mask;
}

static unsigned char network_directions(uint16_t tile, int network,
                                        int hflip, int vflip)
{
    unsigned char mask = 0;

    if (network == NETWORK_ROAD)
    {
        if ((tile >= T_ROAD_TB) && (tile <= T_ROAD_TB_3))
            mask = DIR_NORTH | DIR_SOUTH;
        else if ((tile >= T_ROAD_LR) && (tile <= T_ROAD_LR_3))
            mask = DIR_EAST | DIR_WEST;
        else if (tile == T_ROAD_RB) mask = DIR_EAST | DIR_SOUTH;
        else if (tile == T_ROAD_LB) mask = DIR_WEST | DIR_SOUTH;
        else if (tile == T_ROAD_TR) mask = DIR_NORTH | DIR_EAST;
        else if (tile == T_ROAD_TL) mask = DIR_NORTH | DIR_WEST;
        else if (tile == T_ROAD_TRB) mask = DIR_NORTH | DIR_EAST | DIR_SOUTH;
        else if (tile == T_ROAD_LRB) mask = DIR_EAST | DIR_SOUTH | DIR_WEST;
        else if (tile == T_ROAD_TLB) mask = DIR_NORTH | DIR_SOUTH | DIR_WEST;
        else if (tile == T_ROAD_TLR) mask = DIR_NORTH | DIR_EAST | DIR_WEST;
        else if (tile == T_ROAD_TLRB) mask = DIR_NORTH | DIR_EAST | DIR_SOUTH | DIR_WEST;
        else if (tile == T_ROAD_TB_POWER_LINES) mask = DIR_NORTH | DIR_SOUTH;
        else if (tile == T_ROAD_LR_POWER_LINES) mask = DIR_EAST | DIR_WEST;
        else if (tile == T_ROAD_TB_BRIDGE) mask = DIR_NORTH | DIR_SOUTH;
        else if (tile == T_ROAD_LR_BRIDGE) mask = DIR_EAST | DIR_WEST;
        // These combined tiles cross the two networks: the road keeps its
        // original orientation while the train runs perpendicular to it.
        else if (tile == T_TRAIN_LR_ROAD) mask = DIR_NORTH | DIR_SOUTH;
        else if (tile == T_TRAIN_TB_ROAD) mask = DIR_EAST | DIR_WEST;
    }
    else if (network == NETWORK_RAIL)
    {
        if (tile == T_TRAIN_TB) mask = DIR_NORTH | DIR_SOUTH;
        else if (tile == T_TRAIN_LR) mask = DIR_EAST | DIR_WEST;
        else if (tile == T_TRAIN_RB) mask = DIR_EAST | DIR_SOUTH;
        else if (tile == T_TRAIN_LB) mask = DIR_WEST | DIR_SOUTH;
        else if (tile == T_TRAIN_TR) mask = DIR_NORTH | DIR_EAST;
        else if (tile == T_TRAIN_TL) mask = DIR_NORTH | DIR_WEST;
        else if (tile == T_TRAIN_TRB) mask = DIR_NORTH | DIR_EAST | DIR_SOUTH;
        else if (tile == T_TRAIN_LRB) mask = DIR_EAST | DIR_SOUTH | DIR_WEST;
        else if (tile == T_TRAIN_TLB) mask = DIR_NORTH | DIR_SOUTH | DIR_WEST;
        else if (tile == T_TRAIN_TLR) mask = DIR_NORTH | DIR_EAST | DIR_WEST;
        else if (tile == T_TRAIN_TLRB) mask = DIR_NORTH | DIR_EAST | DIR_SOUTH | DIR_WEST;
        else if (tile == T_TRAIN_LR_ROAD) mask = DIR_EAST | DIR_WEST;
        else if (tile == T_TRAIN_TB_ROAD) mask = DIR_NORTH | DIR_SOUTH;
        else if (tile == T_TRAIN_TB_POWER_LINES) mask = DIR_NORTH | DIR_SOUTH;
        else if (tile == T_TRAIN_LR_POWER_LINES) mask = DIR_EAST | DIR_WEST;
        else if (tile == T_TRAIN_TB_BRIDGE) mask = DIR_NORTH | DIR_SOUTH;
        else if (tile == T_TRAIN_LR_BRIDGE) mask = DIR_EAST | DIR_WEST;
    }
    else if (network == NETWORK_POWER)
    {
        if (tile == T_ROAD_TB_POWER_LINES ||
            tile == T_POWER_LINES_TB || tile == T_POWER_LINES_TB_BRIDGE)
            mask = DIR_NORTH | DIR_SOUTH;
        else if (tile == T_ROAD_LR_POWER_LINES ||
                 tile == T_POWER_LINES_LR || tile == T_POWER_LINES_LR_BRIDGE)
            mask = DIR_EAST | DIR_WEST;
        else if (tile == T_TRAIN_TB_POWER_LINES)
            mask = DIR_EAST | DIR_WEST;
        else if (tile == T_TRAIN_LR_POWER_LINES)
            mask = DIR_NORTH | DIR_SOUTH;
        else if (tile == T_POWER_LINES_RB) mask = DIR_EAST | DIR_SOUTH;
        else if (tile == T_POWER_LINES_LB) mask = DIR_WEST | DIR_SOUTH;
        else if (tile == T_POWER_LINES_TR) mask = DIR_NORTH | DIR_EAST;
        else if (tile == T_POWER_LINES_TL) mask = DIR_NORTH | DIR_WEST;
        else if (tile == T_POWER_LINES_TRB) mask = DIR_NORTH | DIR_EAST | DIR_SOUTH;
        else if (tile == T_POWER_LINES_LRB) mask = DIR_EAST | DIR_SOUTH | DIR_WEST;
        else if (tile == T_POWER_LINES_TLB) mask = DIR_NORTH | DIR_SOUTH | DIR_WEST;
        else if (tile == T_POWER_LINES_TLR) mask = DIR_NORTH | DIR_EAST | DIR_WEST;
        else if (tile == T_POWER_LINES_TLRB) mask = DIR_NORTH | DIR_EAST | DIR_SOUTH | DIR_WEST;
    }

    return flip_mask(mask, hflip, vflip);
}

static int is_building_type(uint16_t type)
{
    type &= TYPE_MASK;
    return (type >= TYPE_RESIDENTIAL) && (type <= TYPE_RADIATION);
}

static int read_map(const char *path)
{
    unsigned char raw[sizeof(map)];
    FILE *f = fopen(path, "rb");
    if (f == NULL)
    {
        perror(path);
        return 0;
    }

    size_t n = fread(raw, 1, sizeof(raw), f);
    int extra = fgetc(f);
    fclose(f);

    if (n != sizeof(raw) || extra != EOF)
    {
        fprintf(stderr, "%s: expected exactly %zu bytes, read %zu bytes\n",
                path, sizeof(raw), n);
        return 0;
    }

    for (int i = 0; i < CITY_MAP_WIDTH * CITY_MAP_HEIGHT; i++)
        map[i] = raw[i * 2] | ((uint16_t)raw[i * 2 + 1] << 8);

    return 1;
}

static int audit_map(const char *path)
{
    if (!read_map(path))
        return 0;

    unsigned int tile_counts[T_LAST + 1] = { 0 };
    unsigned int type_counts[TYPE_NUMBER] = { 0 };
    unsigned int network_counts[NETWORK_COUNT] = { 0 };
    unsigned int network_reciprocal[NETWORK_COUNT] = { 0 };
    unsigned int network_open[NETWORK_COUNT] = { 0 };
    unsigned int network_unpaired[NETWORK_COUNT] = { 0 };
    unsigned int network_mismatch_examples = 0;
    unsigned char tile_seen[T_LAST + 1] = { 0 };
    unsigned int unique_tiles = 0;
    unsigned int footprint_errors = 0;
    unsigned int missing_footprint_cells = 0;
    uint64_t raw_hash = 14695981039346656037ULL;
    uint64_t semantic_hash = 14695981039346656037ULL;

    for (int y = 0; y < CITY_MAP_HEIGHT; y++)
    {
        for (int x = 0; x < CITY_MAP_WIDTH; x++)
        {
            uint16_t entry = map[y * CITY_MAP_WIDTH + x];
            uint16_t tile = entry & 0x03FF;
            if (tile > T_LAST)
            {
                fprintf(stderr, "%s: tile %u outside tileset at (%d,%d)\n",
                        path, tile, x, y);
                return 0;
            }

            const city_tile_info *info = City_Tileset_Entry_Info(tile);
            uint16_t type = info->element_type;
            uint16_t base_type = type & TYPE_MASK;

            raw_hash = (raw_hash ^ (entry & 0xFF)) * 1099511628211ULL;
            raw_hash = (raw_hash ^ (entry >> 8)) * 1099511628211ULL;
            semantic_hash = (semantic_hash ^ (type & 0xFF)) * 1099511628211ULL;
            semantic_hash = (semantic_hash ^ (type >> 8)) * 1099511628211ULL;
            semantic_hash = (semantic_hash ^ (uint8_t)info->base_x_delta) *
                            1099511628211ULL;
            semantic_hash = (semantic_hash ^ (uint8_t)info->base_y_delta) *
                            1099511628211ULL;

            tile_counts[tile]++;
            if (!tile_seen[tile])
            {
                tile_seen[tile] = 1;
                unique_tiles++;
            }

            if (base_type < TYPE_NUMBER)
                type_counts[base_type]++;
            if (type & TYPE_HAS_ROAD)
                network_counts[NETWORK_ROAD]++;
            if (type & TYPE_HAS_TRAIN)
                network_counts[NETWORK_RAIL]++;
            if (type & TYPE_HAS_POWER)
                network_counts[NETWORK_POWER]++;

            for (int network = 0; network < NETWORK_COUNT; network++)
            {
                unsigned short network_flag =
                    network == NETWORK_ROAD ? TYPE_HAS_ROAD :
                    network == NETWORK_RAIL ? TYPE_HAS_TRAIN : TYPE_HAS_POWER;
                if ((type & network_flag) == 0)
                    continue;

                unsigned char directions = network_directions(
                    tile, network,
                    (entry & MAP_REGULAR_HFLIP) != 0,
                    (entry & MAP_REGULAR_VFLIP) != 0);

                for (int direction = 0; direction < 4; direction++)
                {
                    if ((directions & dir_bit[direction]) == 0)
                        continue;

                    int nx = x + dir_dx[direction];
                    int ny = y + dir_dy[direction];
                    if ((nx < 0) || (nx >= CITY_MAP_WIDTH) ||
                        (ny < 0) || (ny >= CITY_MAP_HEIGHT))
                    {
                        network_open[network]++;
                        continue;
                    }

                    uint16_t neighbor_entry = map[ny * CITY_MAP_WIDTH + nx];
                    uint16_t neighbor_tile = neighbor_entry & 0x03FF;
                    const city_tile_info *neighbor_info =
                        City_Tileset_Entry_Info(neighbor_tile);
                    if ((neighbor_info->element_type & network_flag) == 0)
                    {
                        network_open[network]++;
                        continue;
                    }

                    unsigned char neighbor_directions = network_directions(
                        neighbor_tile, network,
                        (neighbor_entry & MAP_REGULAR_HFLIP) != 0,
                        (neighbor_entry & MAP_REGULAR_VFLIP) != 0);
                    if (neighbor_directions & opposite_bit[direction])
                        network_reciprocal[network]++;
                    else
                    {
                        network_unpaired[network]++;
                        if (network_mismatch_examples < 2)
                        {
                            static const char *network_names[] = {
                                "road", "rail", "power"
                            };
                            static const char *direction_names[] = {
                                "north", "east", "south", "west"
                            };
                            fprintf(stderr,
                                    "%s: %s unpaired visual contact at (%d,%d) tile %u points %s to (%d,%d) tile %u\n",
                                    path, network_names[network], x, y, tile,
                                    direction_names[direction], nx, ny,
                                    neighbor_tile);
                            network_mismatch_examples++;
                        }
                    }
                }
            }

            if (!is_building_type(base_type))
                continue;

            int ox = x + info->base_x_delta;
            int oy = y + info->base_y_delta;
            if ((ox < 0) || (ox >= CITY_MAP_WIDTH) ||
                (oy < 0) || (oy >= CITY_MAP_HEIGHT))
            {
                fprintf(stderr, "%s: building part at (%d,%d) points outside map\n",
                        path, x, y);
                footprint_errors++;
                continue;
            }

            uint16_t origin_tile = map[oy * CITY_MAP_WIDTH + ox] & 0x03FF;
            const city_tile_info *origin_info =
                City_Tileset_Entry_Info(origin_tile);
            if (((origin_info->element_type & TYPE_MASK) != base_type) ||
                origin_info->base_x_delta != 0 ||
                origin_info->base_y_delta != 0)
            {
                fprintf(stderr,
                        "%s: building part at (%d,%d) has invalid origin (%d,%d)\n",
                        path, x, y, ox, oy);
                footprint_errors++;
                continue;
            }

            const building_info *bi = Get_BuildingFromBaseTile(origin_tile);
            if (((ox - x) >= bi->width) || ((oy - y) >= bi->height))
            {
                fprintf(stderr,
                        "%s: tile at (%d,%d) lies outside origin footprint at (%d,%d)\n",
                        path, x, y, ox, oy);
                footprint_errors++;
            }
        }
    }

    // For every multi-cell building origin, ensure every expected footprint
    // cell points back to that origin with the same semantic building type.
    for (int y = 0; y < CITY_MAP_HEIGHT; y++)
    {
        for (int x = 0; x < CITY_MAP_WIDTH; x++)
        {
            uint16_t tile = map[y * CITY_MAP_WIDTH + x] & 0x03FF;
            const city_tile_info *info = City_Tileset_Entry_Info(tile);
            if (!is_building_type(info->element_type) ||
                info->base_x_delta != 0 || info->base_y_delta != 0)
                continue;

            const building_info *bi = Get_BuildingFromBaseTile(tile);
            for (int dy = 0; dy < bi->height; dy++)
            {
                for (int dx = 0; dx < bi->width; dx++)
                {
                    int cx = x + dx;
                    int cy = y + dy;
                    if ((cx >= CITY_MAP_WIDTH) || (cy >= CITY_MAP_HEIGHT))
                    {
                        missing_footprint_cells++;
                        continue;
                    }

                    uint16_t part_tile = map[cy * CITY_MAP_WIDTH + cx] & 0x03FF;
                    const city_tile_info *part =
                        City_Tileset_Entry_Info(part_tile);
                    if (((part->element_type & TYPE_MASK) !=
                         (info->element_type & TYPE_MASK)) ||
                        (part->base_x_delta != -dx) ||
                        (part->base_y_delta != -dy))
                    {
                        fprintf(stderr,
                                "%s: incomplete footprint for tile %u at (%d,%d), cell (%d,%d)\n",
                                path, tile, x, y, cx, cy);
                        missing_footprint_cells++;
                    }
                }
            }
        }
    }

    printf("%s\n", path);
    printf("  cells=%d unique_tiles=%u footprint_errors=%u missing_footprint_cells=%u\n",
           CITY_MAP_WIDTH * CITY_MAP_HEIGHT, unique_tiles,
           footprint_errors, missing_footprint_cells);
    printf("  hashes(raw=0x%016llX semantic=0x%016llX)\n",
           (unsigned long long)raw_hash,
           (unsigned long long)semantic_hash);
    printf("  network_cells road=%u rail=%u power=%u\n",
           network_counts[NETWORK_ROAD], network_counts[NETWORK_RAIL],
           network_counts[NETWORK_POWER]);
    printf("  connectors(reciprocal/open/unpaired-contact) road=%u/%u/%u rail=%u/%u/%u power=%u/%u/%u\n",
           network_reciprocal[NETWORK_ROAD], network_open[NETWORK_ROAD],
           network_unpaired[NETWORK_ROAD],
           network_reciprocal[NETWORK_RAIL], network_open[NETWORK_RAIL],
           network_unpaired[NETWORK_RAIL],
           network_reciprocal[NETWORK_POWER], network_open[NETWORK_POWER],
           network_unpaired[NETWORK_POWER]);
    printf("  semantic_cell_counts field=%u forest=%u water=%u residential=%u commercial=%u industrial=%u\n",
           type_counts[TYPE_FIELD], type_counts[TYPE_FOREST],
           type_counts[TYPE_WATER], type_counts[TYPE_RESIDENTIAL],
           type_counts[TYPE_COMMERCIAL], type_counts[TYPE_INDUSTRIAL]);

    return (footprint_errors == 0) && (missing_footprint_cells == 0);
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        fprintf(stderr, "usage: %s map.bin [map.bin ...]\n", argv[0]);
        return 2;
    }

    int result = 0;
    for (int i = 1; i < argc; i++)
    {
        if (!audit_map(argv[i]))
            result = 1;
    }

    return result;
}
