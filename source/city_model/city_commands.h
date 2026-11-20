// SPDX-License-Identifier: GPL-3.0-only

#ifndef CITY_MODEL_CITY_COMMANDS_H__
#define CITY_MODEL_CITY_COMMANDS_H__

typedef enum {
    CITY_BUILD_RESIDENTIAL,
    CITY_BUILD_COMMERCIAL,
    CITY_BUILD_INDUSTRIAL,
    CITY_BUILD_POLICE,
    CITY_BUILD_FIRE_STATION,
    CITY_BUILD_HOSPITAL,
    CITY_BUILD_SMALL_PARK,
    CITY_BUILD_LARGE_PARK,
    CITY_BUILD_STADIUM,
    CITY_BUILD_SCHOOL,
    CITY_BUILD_HIGH_SCHOOL,
    CITY_BUILD_UNIVERSITY,
    CITY_BUILD_MUSEUM,
    CITY_BUILD_LIBRARY,
    CITY_BUILD_TRAIN_STATION,
    CITY_BUILD_AIRPORT,
    CITY_BUILD_COAL_PLANT,
    CITY_BUILD_OIL_PLANT,
    CITY_BUILD_WIND_PLANT,
    CITY_BUILD_SOLAR_PLANT,
    CITY_BUILD_NUCLEAR_PLANT,
    CITY_BUILD_FUSION_PLANT,
} city_building_kind;

typedef enum {
    CITY_RCI_RESIDENTIAL,
    CITY_RCI_COMMERCIAL,
    CITY_RCI_INDUSTRIAL,
} city_rci_kind;

typedef enum {
    CITY_NETWORK_ROAD,
    CITY_NETWORK_RAIL,
    CITY_NETWORK_POWER,
} city_network_kind;

// Submit a build/demolition command to the current legacy-backed model.
// forced bypasses money/placement restrictions, matching simulation growth
// and disaster behavior. Return values match the legacy drawing operations:
// zero means success, nonzero means rejected.
int CityCommand_Build(city_building_kind kind, int x, int y, int forced);
int CityCommand_Demolish(int x, int y, int forced);

// Transitional entry point for the existing build menu's B_* values.
int CityCommand_BuildFromLegacyType(int legacy_type, int x, int y, int forced);

// Grow an RCI zone into a stage 1–3 building. style_variant is 0–3.
int CityCommand_GrowRCI(city_rci_kind kind, int stage, int style_variant,
                        int x, int y);

int CityCommand_PlaceNetwork(city_network_kind kind, int x, int y, int forced);
void CityCommand_RemoveNetworkAtCell(int x, int y, int forced);
void CityCommand_BuildPort(int x, int y, int forced);
void CityCommand_DemolishPort(int x, int y, int forced);

#endif // CITY_MODEL_CITY_COMMANDS_H__
