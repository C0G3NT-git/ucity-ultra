// SPDX-License-Identifier: GPL-3.0-only

#ifndef CITY_RENDERER_3D_CITY_RENDERER_H__
#define CITY_RENDERER_3D_CITY_RENDERER_H__

void CityRenderer3D_Draw(int width, int height, float yaw, float pitch,
                         float distance, int selected_x, int selected_y);
int CityRenderer3D_PickCell(int screen_x, int screen_y, int width, int height,
                            float yaw, float pitch, float distance,
                            int *out_x, int *out_y);

#endif // CITY_RENDERER_3D_CITY_RENDERER_H__
