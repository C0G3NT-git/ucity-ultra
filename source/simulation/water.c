// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright (c) 2021, Antonio Niño Díaz

#include <stdlib.h>

#include "city_model/city_view.h"
#include "random.h"
#include "room_game/draw_common.h"

void Simulation_WaterAnimate(void)
{
    int count = (rand_fast() & 31) + 1;

    for (int j = 0; j < CITY_VIEW_HEIGHT; j++)
    {
        for (int i = 0; i < CITY_VIEW_WIDTH; i++)
        {
            if (count > 0)
            {
                count--;
                continue;
            }

            count = (rand_fast() & 31) + 1;

            uint8_t phase;
            if (CityView_AdvanceWaterAnimation(i, j, &phase))
                CityMapDrawWaterPhase(phase, i, j);
        }
    }
}
