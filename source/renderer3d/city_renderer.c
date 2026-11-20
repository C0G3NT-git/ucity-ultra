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

static void rotate_x(float angle, float *x, float *y, float *z)
{
    float c = cosf(angle);
    float s = sinf(angle);
    float old_y = *y;
    float old_z = *z;
    *y = c * old_y - s * old_z;
    *z = s * old_y + c * old_z;
    (void)x;
}

static void rotate_y(float angle, float *x, float *y, float *z)
{
    float c = cosf(angle);
    float s = sinf(angle);
    float old_x = *x;
    float old_z = *z;
    *x = c * old_x + s * old_z;
    *z = -s * old_x + c * old_z;
    (void)y;
}

static void camera_to_world(float *x, float *y, float *z,
                            float yaw, float pitch)
{
    const float degrees_to_radians = 0.01745329252f;
    rotate_x(-pitch * degrees_to_radians, x, y, z);
    rotate_y(-yaw * degrees_to_radians, x, y, z);
}

int CityRenderer3D_PickCell(int screen_x, int screen_y, int width, int height,
                            float yaw, float pitch, float distance,
                            int *out_x, int *out_y)
{
    if ((width <= 0) || (height <= 0) || (screen_x < 0) ||
        (screen_x >= width) || (screen_y < 0) || (screen_y >= height) ||
        (out_x == 0) || (out_y == 0))
        return 0;

    float aspect = (float)width / (float)height;
    float ndc_x = 2.0f * (float)screen_x / (float)width - 1.0f;
    float ndc_y = 1.0f - 2.0f * (float)screen_y / (float)height;
    float ray_x = ndc_x * 0.42f * aspect;
    float ray_y = ndc_y * 0.42f;
    float ray_z = -1.0f;

    float origin_x = 0.0f, origin_y = 0.0f, origin_z = distance;
    camera_to_world(&origin_x, &origin_y, &origin_z, yaw, pitch);
    origin_x += CITY_VIEW_WIDTH / 2.0f;
    origin_z += CITY_VIEW_HEIGHT / 2.0f;
    camera_to_world(&ray_x, &ray_y, &ray_z, yaw, pitch);

    if (ray_y >= 0.0f)
        return 0;
    float t = -origin_y / ray_y;
    if (t < 0.0f)
        return 0;

    float world_x = origin_x + ray_x * t;
    float world_z = origin_z + ray_z * t;
    int cell_x = (int)floorf(world_x);
    int cell_y = (int)floorf(world_z);
    if ((cell_x < 0) || (cell_x >= CITY_VIEW_WIDTH) ||
        (cell_y < 0) || (cell_y >= CITY_VIEW_HEIGHT))
        return 0;

    *out_x = cell_x;
    *out_y = cell_y;
    return 1;
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

static void draw_selection(const city_view_cell *cell, int x, int z)
{
    float x0 = (float)cell->footprint_origin_x;
    float z0 = (float)cell->footprint_origin_y;
    float x1 = x0 + cell->footprint_width;
    float z1 = z0 + cell->footprint_height;
    float y = 0.08f;

    if ((cell->footprint_origin_x < 0) || (cell->footprint_origin_y < 0))
    {
        x0 = (float)x;
        z0 = (float)z;
        x1 = x0 + 1.0f;
        z1 = z0 + 1.0f;
    }
    if ((cell->kind >= CITY_VIEW_RESIDENTIAL) &&
        (cell->kind <= CITY_VIEW_POWER_PLANT))
    {
        int ox = cell->footprint_origin_x;
        int oz = cell->footprint_origin_y;
        y = 1.0f + (float)((cell->kind * 7 + ox * 3 + oz) % 7) * 0.34f;
        if (cell->kind == CITY_VIEW_POWER_PLANT)
            y += 1.2f;
        y += 0.04f;
    }

    glLineWidth(2.5f);
    color(1.0f, 0.88f, 0.22f);
    glBegin(GL_LINE_LOOP);
    glVertex3f(x0, y, z0);
    glVertex3f(x1, y, z0);
    glVertex3f(x1, y, z1);
    glVertex3f(x0, y, z1);
    glEnd();
    glLineWidth(1.0f);
}

void CityRenderer3D_Draw(int width, int height, float yaw, float pitch,
                         float distance, int selected_x, int selected_y)
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

    if ((selected_x >= 0) && (selected_x < CITY_VIEW_WIDTH) &&
        (selected_y >= 0) && (selected_y < CITY_VIEW_HEIGHT))
    {
        const city_view_cell *selected =
            &cells[selected_y * CITY_VIEW_WIDTH + selected_x];
        draw_selection(selected, selected_x, selected_y);
    }
}
