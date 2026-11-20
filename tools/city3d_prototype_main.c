// SPDX-License-Identifier: GPL-3.0-only

#include <math.h>
#include <stdint.h>
#include <stdio.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <ugba/ugba.h>

#include "city_model/city_view.h"
#include "maps/city/scenario_0_rock_river_bin.h"
#include "maps/city/scenario_1_boringtown_bin.h"
#include "maps/city/scenario_2_portville_bin.h"
#include "maps/city/scenario_3_newdale_bin.h"
#include "maps/city/scenario_4_central_bin.h"
#include "maps/city/scenario_5_futura_bin.h"
#include "renderer3d/city_renderer.h"

static const char *scenario_names[] = {
    "Rock River", "Boringtown", "Portville", "Newdale", "Central", "Futura"
};
static const uint8_t *scenario_maps[] = {
    scenario_0_rock_river_bin,
    scenario_1_boringtown_bin,
    scenario_2_portville_bin,
    scenario_3_newdale_bin,
    scenario_4_central_bin,
    scenario_5_futura_bin,
};

static uint16_t scenario_map[CITY_VIEW_CELL_COUNT];

static void load_scenario(int index)
{
    const uint8_t *source = scenario_maps[index];
    for (int i = 0; i < CITY_VIEW_CELL_COUNT; i++)
        scenario_map[i] = source[i * 2] | ((uint16_t)source[i * 2 + 1] << 8);
    CityView_ImportLegacyMap(scenario_map);
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
    int selected_scenario = 4;
    load_scenario(selected_scenario);
    SDL_SetWindowTitle(window, "µCity Advance 3D — Central (1–6: select scenario)");

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
            else if ((event.type == SDL_KEYDOWN) &&
                     (event.key.keysym.sym >= SDLK_1) &&
                     (event.key.keysym.sym <= SDLK_6))
            {
                selected_scenario = event.key.keysym.sym - SDLK_1;
                load_scenario(selected_scenario);
                char title[96];
                snprintf(title, sizeof(title),
                         "µCity Advance 3D — %s (1–6: select scenario)",
                         scenario_names[selected_scenario]);
                SDL_SetWindowTitle(window, title);
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
