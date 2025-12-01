#include <SDL2/SDL.h>
#include <string.h>

#include "../include/signal_handler.h"
#include "../include/input.h"

typedef struct
{
    SDL_Scancode primary;
    SDL_Scancode secondary;
    char symbol;
} DirectionMap;

static const DirectionMap kDirectionMap[] = {
    {SDL_SCANCODE_W, SDL_SCANCODE_UP, 'w'},
    {SDL_SCANCODE_S, SDL_SCANCODE_DOWN, 's'},
    {SDL_SCANCODE_A, SDL_SCANCODE_LEFT, 'a'},
    {SDL_SCANCODE_D, SDL_SCANCODE_RIGHT, 'd'},
};

static char g_direction_stack[4];
static int g_direction_count = 0;
static int g_direction_down[4] = {0};

static void remove_direction(char dir)
{
    for (int i = 0; i < g_direction_count; ++i)
    {
        if (g_direction_stack[i] == dir)
        {
            for (int j = i; j < g_direction_count - 1; ++j)
            {
                g_direction_stack[j] = g_direction_stack[j + 1];
            }
            g_direction_count--;
            break;
        }
    }
}

static void push_direction(char dir)
{
    remove_direction(dir);
    if (g_direction_count < (int)(sizeof(g_direction_stack) / sizeof(g_direction_stack[0])))
    {
        g_direction_stack[g_direction_count++] = dir;
    }
}

static void sync_direction_state(void)
{
    SDL_PumpEvents();
    const Uint8 *state = SDL_GetKeyboardState(NULL);
    if (!state)
    {
        g_direction_count = 0;
        memset(g_direction_down, 0, sizeof(g_direction_down));
        return;
    }

    for (int i = 0; i < (int)(sizeof(kDirectionMap) / sizeof(kDirectionMap[0])); ++i)
    {
        int down = state[kDirectionMap[i].primary];
        if (kDirectionMap[i].secondary != SDL_SCANCODE_UNKNOWN)
        {
            down = down || state[kDirectionMap[i].secondary];
        }

        if (down && !g_direction_down[i])
        {
            push_direction(kDirectionMap[i].symbol);
            g_direction_down[i] = 1;
        }
        else if (!down && g_direction_down[i])
        {
            remove_direction(kDirectionMap[i].symbol);
            g_direction_down[i] = 0;
        }
    }
}

static int translate_key(SDL_Keycode key)
{
    switch (key)
    {
        case SDLK_w:
        case SDLK_UP:
            return 'w';
        case SDLK_s:
        case SDLK_DOWN:
            return 's';
        case SDLK_a:
        case SDLK_LEFT:
            return 'a';
        case SDLK_d:
        case SDLK_RIGHT:
            return 'd';
        case SDLK_q:
        case SDLK_ESCAPE:
            return 'q';
        default:
            return (key >= 0 && key < 128) ? (int)key : -1;
    }
}

void init_input(void)
{
    SDL_StartTextInput();
}

void restore_input(void)
{
    SDL_StopTextInput();
}

int read_input(void)
{
    sync_direction_state();
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_QUIT)
        {
            g_running = 0;
            return -1;
        }

        if (event.type == SDL_KEYDOWN && !event.key.repeat)
        {
            int mapped = translate_key(event.key.keysym.sym);
            if (mapped != -1)
            {
                return mapped;
            }
        }
    }

    return -1;
}

int poll_input(void)
{
    return read_input();
}

int current_direction_key(void)
{
    sync_direction_state();
    if (g_direction_count > 0)
    {
        return g_direction_stack[g_direction_count - 1];
    }
    return -1;
}
