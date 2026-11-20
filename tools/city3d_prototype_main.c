// SPDX-License-Identifier: GPL-3.0-only

#include <math.h>
#include <stdint.h>
#include <stdio.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <ugba/ugba.h>

#include "city_model/city_view.h"
#include "renderer3d/city_renderer.h"
#include "room_game/tileset_info.h"

static uint16_t demo_map[CITY_VIEW_CELL_COUNT];

static void map_set(int x, int z, uint16_t tile)
{
    if ((x >= 0) && (x < CITY_VIEW_WIDTH) &&
        (z >= 0) && (z < CITY_VIEW_HEIGHT))
        demo_map[z * CITY_VIEW_WIDTH + x] = tile;
}

static void place_building(int x, int z, uint16_t base_tile,
                           int width, int height)
{
    for (int dz = 0; dz < height; dz++)
    {
        for (int dx = 0; dx < width; dx++)
            map_set(x + dx, z + dz, base_tile + dz * width + dx);
    }
}

static void make_demo_city(void)
{
    for (int z = 0; z < CITY_VIEW_HEIGHT; z++)
    {
        for (int x = 0; x < CITY_VIEW_WIDTH; x++)
            map_set(x, z, T_GRASS);
    }

    for (int z = 4; z < 17; z++)
        for (int x = 3; x < 18; x++)
            map_set(x, z, T_WATER);

    for (int z = 4; z < 21; z++)
        for (int x = 44; x < 61; x++)
            if (((x + z) % 4) != 0)
                map_set(x, z, T_FOREST);

    for (int x = 8; x <= 56; x++)
        map_set(x, 34, T_ROAD_LR);
    for (int z = 10; z <= 54; z++)
        map_set(31, z, T_ROAD_TB);
    map_set(31, 34, T_ROAD_TLRB);

    for (int z = 27; z <= 30; z++)
        for (int x = 16; x <= 19; x++)
            map_set(x, z, T_RESIDENTIAL);
    for (int z = 36; z <= 38; z++)
        for (int x = 15; x <= 17; x++)
            map_set(x, z, T_COMMERCIAL);
    for (int z = 28; z <= 30; z++)
        for (int x = 38; x <= 40; x++)
            map_set(x, z, T_INDUSTRIAL);

    place_building(23, 25, T_POLICE_DEPT, 3, 3);
    place_building(35, 39, T_SCHOOL, 3, 2);
    for (int z = 23; z <= 25; z++)
        for (int x = 7; x <= 9; x++)
            map_set(x, z, T_PARK_BIG + (z - 23) * 3 + (x - 7));

    for (int z = 39; z <= 48; z++)
        map_set(46, z, T_POWER_LINES_TB);

    CityView_ImportLegacyMap(demo_map);
}

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    UGBA_InitHeadless(NULL, NULL);
    if (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0)
    {
        fprintf(stderr, "SDL video initialization failed: %s\n", SDL_GetError());
        return 1;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    SDL_Window *window = SDL_CreateWindow(
        "µCity Advance — 3D prototype",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 800,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    if (window == NULL)
    {
        fprintf(stderr, "Window creation failed: %s\n", SDL_GetError());
        return 1;
    }

    SDL_GLContext context = SDL_GL_CreateContext(window);
    if (context == NULL)
    {
        fprintf(stderr, "OpenGL context creation failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        return 1;
    }

    SDL_GL_SetSwapInterval(1);
    glClearColor(0.69f, 0.80f, 0.86f, 1.0f);
    make_demo_city();

    float yaw = 35.0f;
    float pitch = 50.0f;
    float distance = 100.0f;
    int dragging = 0;
    int running = 1;
    while (running)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
                running = 0;
            else if ((event.type == SDL_KEYDOWN) &&
                     (event.key.keysym.sym == SDLK_ESCAPE))
                running = 0;
            else if ((event.type == SDL_KEYDOWN) &&
                     (event.key.keysym.sym == SDLK_r))
            {
                yaw = 35.0f;
                pitch = 50.0f;
                distance = 100.0f;
            }
            else if ((event.type == SDL_MOUSEBUTTONDOWN) &&
                     (event.button.button == SDL_BUTTON_LEFT))
                dragging = 1;
            else if ((event.type == SDL_MOUSEBUTTONUP) &&
                     (event.button.button == SDL_BUTTON_LEFT))
                dragging = 0;
            else if ((event.type == SDL_MOUSEMOTION) && dragging)
            {
                yaw += event.motion.xrel * 0.45f;
                pitch += event.motion.yrel * 0.35f;
                if (pitch < 20.0f) pitch = 20.0f;
                if (pitch > 78.0f) pitch = 78.0f;
            }
            else if (event.type == SDL_MOUSEWHEEL)
            {
                distance *= expf(-event.wheel.y * 0.11f);
                if (distance < 34.0f) distance = 34.0f;
                if (distance > 180.0f) distance = 180.0f;
            }
        }

        int width, height;
        SDL_GL_GetDrawableSize(window, &width, &height);
        CityRenderer3D_Draw(width, height, yaw, pitch, distance);
        SDL_GL_SwapWindow(window);
    }

    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
    return 0;
}
