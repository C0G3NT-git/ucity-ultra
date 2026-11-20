// SPDX-License-Identifier: GPL-3.0-only
//
// Run a scenario map through the existing simulation in a fresh headless
// process and print deterministic per-step summaries. This tool does not open
// the interactive save file or write to SRAM.

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <ugba/ugba.h>

#include "date.h"
#include "money.h"
#include "random.h"
#include "room_bank/room_bank.h"
#include "room_game/draw_common.h"
#include "room_game/room_game.h"
#include "room_game/text_messages.h"
#include "simulation/budget.h"
#include "simulation/building_count.h"
#include "simulation/calculate_stats.h"
#include "simulation/common.h"
#include "simulation/pollution.h"
#include "simulation/power.h"
#include "simulation/services.h"
#include "simulation/technology.h"
#include "simulation/traffic.h"
#include "simulation/happiness.h"

#define CITY_CELLS (CITY_MAP_WIDTH * CITY_MAP_HEIGHT)
#define TEST_MAP_FUNDS 99999999
#define TEST_MAP_SEED 0x4D595DF4D0F33173ULL
#define FAST_SEED 0x12345678

static uint16_t map[CITY_CELLS];

static int read_map(const char *path)
{
    uint8_t bytes[CITY_CELLS * 2];
    FILE *file = fopen(path, "rb");
    if (file == NULL)
    {
        perror(path);
        return 0;
    }

    size_t count = fread(bytes, 1, sizeof(bytes), file);
    int extra = fgetc(file);
    fclose(file);

    if ((count != sizeof(bytes)) || (extra != EOF))
    {
        fprintf(stderr, "%s: expected exactly %zu bytes, read %zu\n",
                path, sizeof(bytes), count);
        return 0;
    }

    for (int i = 0; i < CITY_CELLS; i++)
        map[i] = bytes[i * 2] | ((uint16_t)bytes[i * 2 + 1] << 8);

    return 1;
}

static uint64_t hash_buffer(uint64_t hash, const uint8_t *data, size_t size)
{
    for (size_t i = 0; i < size; i++)
        hash = (hash ^ data[i]) * 1099511628211ULL;
    return hash;
}

static uint64_t hash_city_map(void)
{
    uint64_t hash = 14695981039346656037ULL;
    for (int y = 0; y < CITY_MAP_HEIGHT; y++)
    {
        for (int x = 0; x < CITY_MAP_WIDTH; x++)
        {
            uint16_t tile = CityMapGetTile(x, y);
            uint16_t type = CityMapGetType(x, y);
            const uint8_t data[] = {
                tile & 0xFF, tile >> 8,
                type & 0xFF, type >> 8,
            };
            hash = hash_buffer(hash, data, sizeof(data));
        }
    }
    return hash;
}

static uint64_t hash_city_cells(const uint8_t *cells)
{
    return hash_buffer(14695981039346656037ULL, cells, CITY_CELLS);
}

static unsigned int count_nonzero(const uint8_t *cells)
{
    unsigned int count = 0;
    for (int i = 0; i < CITY_CELLS; i++)
    {
        if (cells[i] != 0)
            count++;
    }
    return count;
}

static void print_step(int step)
{
    uint32_t population_r, population_c, population_i;
    int demand_r, demand_c, demand_i;
    Simulation_GetPopulationRCI(&population_r, &population_c, &population_i);
    Simulation_GetDemandRCI(&demand_r, &demand_c, &demand_i);

    uint8_t *traffic = Simulation_TrafficGetMap();
    uint8_t *power = Simulation_PowerDistributionGetMap();
    uint8_t *pollution = Simulation_PollutionGetMap();
    uint8_t *services = Simulation_ServicesGetMap();
    uint8_t *happiness = Simulation_HappinessGetMap();

    printf("%d,%d,%d,%d,%u,%u,%u,%u,%d,%d,%d,%ld,%d,%d,%u,%u,%u,"
           "%016llX,%016llX,%016llX,%016llX,%016llX,%016llX\n",
           step, DateGetYear(), DateGetMonth(), Simulation_GetCityClass(),
           Simulation_GetTotalPopulation(), population_r, population_c,
           population_i, demand_r, demand_c, demand_i, (long)MoneyGet(),
           Simulation_PollutionGetTotal(),
           Simulation_PollutionGetPercentage(),
           Simulation_TrafficGetTrafficJamPercent(),
           count_nonzero(power), count_nonzero(services),
           (unsigned long long)hash_city_map(),
           (unsigned long long)hash_city_cells(traffic),
           (unsigned long long)hash_city_cells(power),
           (unsigned long long)hash_city_cells(pollution),
           (unsigned long long)hash_city_cells(services),
           (unsigned long long)hash_city_cells(happiness));
}

int main(int argc, char **argv)
{
    if ((argc < 2) || (argc > 3))
    {
        fprintf(stderr, "usage: %s map.bin [steps]\n", argv[0]);
        return 2;
    }

    char *endptr = NULL;
    long steps = argc == 3 ? strtol(argv[2], &endptr, 10) : 12;
    if ((steps < 1) || (steps > 240) ||
        (argc == 3 && (endptr == argv[2] || *endptr != '\0')))
    {
        fprintf(stderr, "steps must be an integer from 1 to 240\n");
        return 2;
    }

    if (!read_map(argv[1]))
        return 1;

    // Passing NULL prevents UGBA_InitHeadless() from deriving/loading a .sav
    // file from argv[0]. Everything after initialization lives in fresh memory.
    UGBA_InitHeadless(NULL, NULL);

    rand_fast_set_seed(FAST_SEED);
    rand_slow_set_seed(TEST_MAP_SEED);
    DateSet(0, 1950);
    Room_Game_Set_City_Economy(TEST_MAP_FUNDS, 10, 0, 0);
    Simulation_SetCityClass(CLASS_CITY);
    Simulation_NegativeBudgetCountSet(0);
    Simulation_DisastersSetEnabled(0);
    Room_Game_SetAnimationsEnabled(0);
    Room_Game_Graphics_New_Set(1);
    Technology_SetLevel(TECH_LEVEL_MAX);
    PersistentMessageFlagsReset();
    MessageQueueInit();

    Room_Game_Load_City(map, "Baseline", 0, 0);
    Simulation_CountBuildings();
    Simulation_GraphsResetAll();

    puts("step,year,month,class,population,residential,commercial,industrial,"
         "demand_r,demand_c,demand_i,funds,pollution_total,pollution_pct,"
         "traffic_jam_pct,powered_cells,service_cells,city_hash,traffic_hash,"
         "power_hash,pollution_hash,service_hash,happiness_hash");
    print_step(0);

    for (int step = 1; step <= steps; step++)
    {
        MessageQueueInit(); // Match dismissing any notifications between steps.
        Simulation_SimulateAll();
        print_step(step);
    }

    return 0;
}
