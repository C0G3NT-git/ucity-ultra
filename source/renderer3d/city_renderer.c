// SPDX-License-Identifier: GPL-3.0-only

#include <stdint.h>
#include <math.h>

#include <SDL2/SDL_opengl.h>

#include "city_model/city_view.h"
#include "renderer3d/city_renderer.h"

static void color(float r, float g, float b)
{
    glColor3f(r, g, b);
}

static void quad(float x0, float y0, float z0,
                 float x1, float y1, float z1,
                 float x2, float y2, float z2,
                 float x3, float y3, float z3)
{
    glBegin(GL_QUADS);
    glVertex3f(x0, y0, z0);
    glVertex3f(x1, y1, z1);
    glVertex3f(x2, y2, z2);
    glVertex3f(x3, y3, z3);
    glEnd();
}

static void draw_ground(int x, int z, city_view_kind kind)
{
    if (kind == CITY_VIEW_WATER)
        color(0.16f, 0.43f, 0.59f);
    else if (kind == CITY_VIEW_FOREST)
        color(0.24f, 0.42f, 0.25f);
    else
        color(0.43f, 0.59f, 0.35f);
    quad(x, 0, z, x + 1, 0, z, x + 1, 0, z + 1, x, 0, z + 1);
}

static void draw_box(float x0, float z0, float x1, float z1, float height,
                     float r, float g, float b)
{
    float y0 = 0.015f;
    float y1 = y0 + height;
    color(r * 0.66f, g * 0.66f, b * 0.66f);
    quad(x0, y0, z0, x0, y0, z1, x0, y1, z1, x0, y1, z0);
    quad(x1, y0, z1, x1, y0, z0, x1, y1, z0, x1, y1, z1);
    color(r * 0.78f, g * 0.78f, b * 0.78f);
    quad(x1, y0, z0, x0, y0, z0, x0, y1, z0, x1, y1, z0);
    quad(x0, y0, z1, x1, y0, z1, x1, y1, z1, x0, y1, z1);
    color(r, g, b);
    quad(x0, y1, z0, x0, y1, z1, x1, y1, z1, x1, y1, z0);
}

static void draw_network_path(float x, float z, uint8_t connections,
                              float half_width, float y,
                              float r, float g, float b)
{
    color(r, g, b);
    float cx = x + 0.5f;
    float cz = z + 0.5f;
    quad(cx - half_width, y, cz - half_width,
         cx + half_width, y, cz - half_width,
         cx + half_width, y, cz + half_width,
         cx - half_width, y, cz + half_width);

    if (connections & CITY_VIEW_DIR_NORTH)
        quad(cx - half_width, y, z, cx + half_width, y, z,
             cx + half_width, y, cz, cx - half_width, y, cz);
    if (connections & CITY_VIEW_DIR_EAST)
        quad(cx, y, cz - half_width, x + 1, y, cz - half_width,
             x + 1, y, cz + half_width, cx, y, cz + half_width);
    if (connections & CITY_VIEW_DIR_SOUTH)
        quad(cx - half_width, y, cz, cx + half_width, y, cz,
             cx + half_width, y, z + 1, cx - half_width, y, z + 1);
    if (connections & CITY_VIEW_DIR_WEST)
        quad(x, y, cz - half_width, cx, y, cz - half_width,
             cx, y, cz + half_width, x, y, cz + half_width);
}

static void draw_tree(int x, int z)
{
    float cx = x + 0.5f;
    float cz = z + 0.5f;
    draw_box(cx - 0.07f, cz - 0.07f, cx + 0.07f, cz + 0.07f,
             0.6f, 0.34f, 0.22f, 0.12f);
    color(0.18f, 0.38f, 0.17f);
    glBegin(GL_TRIANGLES);
    for (int side = 0; side < 6; side++)
    {
        float a0 = (float)side * 1.04719755f;
        float a1 = (float)(side + 1) * 1.04719755f;
        glVertex3f(cx, 2.3f, cz);
        glVertex3f(cx + 0.48f * cosf(a0), 0.55f,
                   cz + 0.48f * sinf(a0));
        glVertex3f(cx + 0.48f * cosf(a1), 0.55f,
                   cz + 0.48f * sinf(a1));
    }
    glEnd();
}

static void draw_building(const city_view_cell *cell, int cell_x, int cell_z)
{
    if ((cell->footprint_origin_x < 0) || (cell->footprint_origin_y < 0) ||
        (cell->footprint_origin_x >= CITY_VIEW_WIDTH) ||
        (cell->footprint_origin_y >= CITY_VIEW_HEIGHT) ||
        (cell->footprint_origin_x + cell->footprint_width > CITY_VIEW_WIDTH) ||
        (cell->footprint_origin_y + cell->footprint_height > CITY_VIEW_HEIGHT))
        return;

    if ((cell->footprint_origin_x == cell_x) &&
        (cell->footprint_origin_y == cell_z))
    {
        // Only the footprint origin owns the full building shape.
        int x = cell->footprint_origin_x;
        int z = cell->footprint_origin_y;
        if ((cell->kind != CITY_VIEW_FIELD) &&
            (cell->kind != CITY_VIEW_FOREST) &&
            (cell->kind != CITY_VIEW_WATER) &&
            (cell->kind != CITY_VIEW_RADIATION))
        {
            float height = 1.0f + (float)((cell->kind * 7 + x * 3 + z) % 7) * 0.34f;
            float r = 0.72f, g = 0.57f, b = 0.38f;
            if ((cell->kind >= CITY_VIEW_RESIDENTIAL) &&
                (cell->kind <= CITY_VIEW_COMMERCIAL))
            {
                r = 0.77f; g = 0.65f; b = 0.48f;
            }
            else if (cell->kind == CITY_VIEW_POWER_PLANT)
            {
                r = 0.46f; g = 0.48f; b = 0.52f; height += 1.2f;
            }
            else if (cell->kind == CITY_VIEW_AIRPORT)
            {
                r = 0.65f; g = 0.68f; b = 0.65f;
            }
            else if (cell->kind == CITY_VIEW_PORT)
            {
                r = 0.62f; g = 0.47f; b = 0.31f;
            }
            draw_box((float)x + 0.1f, (float)z + 0.1f,
                     (float)x + cell->footprint_width - 0.1f,
                     (float)z + cell->footprint_height - 0.1f,
                     height, r, g, b);
        }
    }
}

void CityRenderer3D_Draw(int width, int height, float yaw, float pitch,
                         float distance)
{
    if ((width <= 0) || (height <= 0))
        return;

    float aspect = (float)width / (float)height;
    float near_clip = 0.1f;
    float far_clip = 300.0f;
    float half_height = near_clip * 0.42f;
    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(-half_height * aspect, half_height * aspect,
              -half_height, half_height, near_clip, far_clip);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0, 0, -distance);
    glRotatef(pitch, 1, 0, 0);
    glRotatef(yaw, 0, 1, 0);
    glTranslatef(-CITY_VIEW_WIDTH / 2.0f, 0, -CITY_VIEW_HEIGHT / 2.0f);

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    static city_view_cell cells[CITY_VIEW_CELL_COUNT];
    if (!CityView_CopySnapshot(cells, CITY_VIEW_CELL_COUNT))
        return;

    for (int z = 0; z < CITY_VIEW_HEIGHT; z++)
    {
        for (int x = 0; x < CITY_VIEW_WIDTH; x++)
        {
            const city_view_cell *cell = &cells[z * CITY_VIEW_WIDTH + x];
            draw_ground(x, z, cell->kind);
        }
    }

    for (int z = 0; z < CITY_VIEW_HEIGHT; z++)
    {
        for (int x = 0; x < CITY_VIEW_WIDTH; x++)
        {
            const city_view_cell *cell = &cells[z * CITY_VIEW_WIDTH + x];
            if (cell->kind == CITY_VIEW_FOREST)
                draw_tree(x, z);
            draw_building(cell, x, z);
            if (cell->networks & CITY_VIEW_NETWORK_ROAD)
                draw_network_path((float)x, (float)z, cell->road_connections,
                                  0.23f, 0.025f, 0.19f, 0.20f, 0.21f);
            if (cell->networks & CITY_VIEW_NETWORK_RAIL)
                draw_network_path((float)x, (float)z, cell->rail_connections,
                                  0.07f, 0.045f, 0.66f, 0.61f, 0.51f);
            if (cell->networks & CITY_VIEW_NETWORK_POWER)
                draw_network_path((float)x, (float)z, cell->power_connections,
                                  0.035f, 0.13f, 0.88f, 0.76f, 0.24f);
        }
    }
}
