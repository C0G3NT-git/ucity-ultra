// SPDX-License-Identifier: GPL-3.0-only

#include <math.h>
#include <stdint.h>
#include <stdio.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <ugba/ugba.h>

#include "random.h"
#include "renderer3d/city_renderer.h"
#include "room_game/room_game.h"
#include "room_scenarios/room_scenarios.h"
#include "simulation/common.h"

static void set_window_title(SDL_Window *window, int simulation_running)
{
    char title[128];
    snprintf(title, sizeof(title), "µCity Advance 3D — %s | %s | 1–6 scenarios",
             Room_Game_Get_City_Name(),
             simulation_running ? "simulation running" : "Space: run simulation");
    SDL_SetWindowTitle(window, title);
}

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    UGBA_InitHeadless(NULL, NULL);
    rand_fast_set_seed(0x12345678);
    Room_Game_SetAnimationsEnabled(0);
    Room_Game_Graphics_New_Set(1);
    Simulation_DisastersSetEnabled(0);
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
    Room_Scenarios_LoadCity(selected_scenario);
    set_window_title(window, 0);

    float yaw = 35.0f;
    float pitch = 50.0f;
    float distance = 100.0f;
    int dragging = 0;
    int running = 1;
    int simulation_running = 0;
    uint32_t next_simulation_step = SDL_GetTicks() + 750;
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
                Room_Scenarios_LoadCity(selected_scenario);
                simulation_running = 0;
                set_window_title(window, simulation_running);
            }
            else if ((event.type == SDL_KEYDOWN) &&
                     (event.key.keysym.sym == SDLK_SPACE))
            {
                simulation_running = !simulation_running;
                next_simulation_step = SDL_GetTicks() + 750;
                set_window_title(window, simulation_running);
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

        uint32_t now = SDL_GetTicks();
        if (simulation_running && ((int32_t)(now - next_simulation_step) >= 0))
        {
            Simulation_SimulateAll();
            next_simulation_step = now + 750;
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
