// SPDX-License-Identifier: GPL-3.0-only

#include <stdint.h>

#include "city_model/city_commands.h"

#include "room_game/building_info.h"
#include "room_game/draw_common.h"
#include "room_game/draw_building.h"
#include "room_game/draw_port.h"
#include "room_game/draw_power_lines.h"
#include "room_game/draw_road.h"
#include "room_game/draw_train.h"

static int LegacyBuildingType(city_building_kind kind)
{
    switch (kind)
    {
        case CITY_BUILD_RESIDENTIAL:   return B_Residential;
        case CITY_BUILD_COMMERCIAL:    return B_Commercial;
        case CITY_BUILD_INDUSTRIAL:    return B_Industrial;
        case CITY_BUILD_POLICE:        return B_PoliceDept;
        case CITY_BUILD_FIRE_STATION:  return B_FireDept;
        case CITY_BUILD_HOSPITAL:      return B_Hospital;
        case CITY_BUILD_SMALL_PARK:    return B_ParkSmall;
        case CITY_BUILD_LARGE_PARK:    return B_ParkBig;
        case CITY_BUILD_STADIUM:       return B_Stadium;
        case CITY_BUILD_SCHOOL:        return B_School;
        case CITY_BUILD_HIGH_SCHOOL:   return B_HighSchool;
        case CITY_BUILD_UNIVERSITY:    return B_University;
        case CITY_BUILD_MUSEUM:        return B_Museum;
        case CITY_BUILD_LIBRARY:       return B_Library;
        case CITY_BUILD_TRAIN_STATION: return B_TrainStation;
        case CITY_BUILD_AIRPORT:       return B_Airport;
        case CITY_BUILD_COAL_PLANT:    return B_PowerPlantCoal;
        case CITY_BUILD_OIL_PLANT:     return B_PowerPlantOil;
        case CITY_BUILD_WIND_PLANT:    return B_PowerPlantWind;
        case CITY_BUILD_SOLAR_PLANT:   return B_PowerPlantSolar;
        case CITY_BUILD_NUCLEAR_PLANT: return B_PowerPlantNuclear;
        case CITY_BUILD_FUSION_PLANT:  return B_PowerPlantFusion;
    }

    return -1;
}

int CityCommand_Build(city_building_kind kind, int x, int y, int forced)
{
    int legacy_type = LegacyBuildingType(kind);
    if (legacy_type < 0)
        return 1;

    return MapDrawBuilding(forced, legacy_type, x, y);
}

int CityCommand_Demolish(int x, int y, int forced)
{
    return MapDeleteBuilding(forced, x, y);
}

int CityCommand_BuildFromLegacyType(int legacy_type, int x, int y, int forced)
{
    switch (legacy_type)
    {
        case B_Residential:       return CityCommand_Build(CITY_BUILD_RESIDENTIAL, x, y, forced);
        case B_Commercial:        return CityCommand_Build(CITY_BUILD_COMMERCIAL, x, y, forced);
        case B_Industrial:        return CityCommand_Build(CITY_BUILD_INDUSTRIAL, x, y, forced);
        case B_PoliceDept:        return CityCommand_Build(CITY_BUILD_POLICE, x, y, forced);
        case B_FireDept:          return CityCommand_Build(CITY_BUILD_FIRE_STATION, x, y, forced);
        case B_Hospital:          return CityCommand_Build(CITY_BUILD_HOSPITAL, x, y, forced);
        case B_ParkSmall:         return CityCommand_Build(CITY_BUILD_SMALL_PARK, x, y, forced);
        case B_ParkBig:           return CityCommand_Build(CITY_BUILD_LARGE_PARK, x, y, forced);
        case B_Stadium:           return CityCommand_Build(CITY_BUILD_STADIUM, x, y, forced);
        case B_School:            return CityCommand_Build(CITY_BUILD_SCHOOL, x, y, forced);
        case B_HighSchool:        return CityCommand_Build(CITY_BUILD_HIGH_SCHOOL, x, y, forced);
        case B_University:        return CityCommand_Build(CITY_BUILD_UNIVERSITY, x, y, forced);
        case B_Museum:            return CityCommand_Build(CITY_BUILD_MUSEUM, x, y, forced);
        case B_Library:           return CityCommand_Build(CITY_BUILD_LIBRARY, x, y, forced);
        case B_TrainStation:      return CityCommand_Build(CITY_BUILD_TRAIN_STATION, x, y, forced);
        case B_Airport:           return CityCommand_Build(CITY_BUILD_AIRPORT, x, y, forced);
        case B_PowerPlantCoal:    return CityCommand_Build(CITY_BUILD_COAL_PLANT, x, y, forced);
        case B_PowerPlantOil:     return CityCommand_Build(CITY_BUILD_OIL_PLANT, x, y, forced);
        case B_PowerPlantWind:    return CityCommand_Build(CITY_BUILD_WIND_PLANT, x, y, forced);
        case B_PowerPlantSolar:   return CityCommand_Build(CITY_BUILD_SOLAR_PLANT, x, y, forced);
        case B_PowerPlantNuclear: return CityCommand_Build(CITY_BUILD_NUCLEAR_PLANT, x, y, forced);
        case B_PowerPlantFusion:  return CityCommand_Build(CITY_BUILD_FUSION_PLANT, x, y, forced);
        default:                  return 1;
    }
}

int CityCommand_GrowRCI(city_rci_kind kind, int stage, int style_variant,
                        int x, int y)
{
    if ((stage < 1) || (stage > 3) ||
        (style_variant < 0) || (style_variant > 3))
        return 1;

    int first_stage_type;
    switch (kind)
    {
        case CITY_RCI_RESIDENTIAL: first_stage_type = B_ResidentialS1A; break;
        case CITY_RCI_COMMERCIAL:  first_stage_type = B_CommercialS1A; break;
        case CITY_RCI_INDUSTRIAL:  first_stage_type = B_IndustrialS1A; break;
        default:                   return 1;
    }

    int legacy_type = first_stage_type + ((stage - 1) * 4) + style_variant;
    return MapDrawBuilding(1, legacy_type, x, y);
}

int CityCommand_PlaceNetwork(city_network_kind kind, int x, int y, int forced)
{
    switch (kind)
    {
        case CITY_NETWORK_ROAD:  MapDrawRoad(forced, x, y); return 0;
        case CITY_NETWORK_RAIL:  MapDrawTrain(forced, x, y); return 0;
        case CITY_NETWORK_POWER: MapDrawPowerLines(forced, x, y); return 0;
    }

    return 1;
}

void CityCommand_RemoveNetworkAtCell(int x, int y, int forced)
{
    BuildingRemoveRoadTrainPowerLines(forced, x, y);
}

void CityCommand_BuildPort(int x, int y, int forced)
{
    MapDrawPort(forced, x, y);
}

void CityCommand_DemolishPort(int x, int y, int forced)
{
    MapDeletePort(forced, x, y);
}
