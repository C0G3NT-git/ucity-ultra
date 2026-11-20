// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright (c) 2021, Antonio Niño Díaz

#ifndef ROOM_SCENARIOS_ROOM_SCENARIOS_H__
#define ROOM_SCENARIOS_ROOM_SCENARIOS_H__

void Room_Scenarios_Load(void);
void Room_Scenarios_Unload(void);

void Room_Scenarios_Handle(void);

// Loads a built-in scenario's map and starting simulation state.
int Room_Scenarios_LoadCity(int scenario_index);

#endif // ROOM_SCENARIOS_ROOM_SCENARIOS_H__
