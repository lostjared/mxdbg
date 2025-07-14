#include "SDL.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define WINDOW_WIDTH 960
#define WINDOW_HEIGHT 720
#define MAP_WIDTH 80
#define MAP_HEIGHT 80
#define FOV 60
#define NUM_RAYS WINDOW_WIDTH
#define MOVE_SPEED 0.05f
#define TURN_SPEED 0.03f

extern SDL_Surface *CreateSurface(int w, int h);
extern void SetPixel(void *buffer, int x, int y, unsigned int pitch, unsigned int color);

int worldMap[MAP_HEIGHT][MAP_WIDTH];

typedef struct {
    float x, y;     
    float angle;    
} Player;

Player player = {5.0f, 5.0f, 0.7f}; 

float degToRad(float deg) {
    return deg * M_PI / 180.0f;
}

void generateMap() {
    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            worldMap[y][x] = 1;
        }
    }

    for (int y = 2; y < 12; y++) {
        for (int x = 2; x < 12; x++) worldMap[y][x] = 0;
    }

    for (int x = 12; x < 20; x++) {
        for (int y = 7; y < 9; y++) worldMap[y][x] = 0;
    }

    for (int y = 5; y < 15; y++) {
        for (int x = 20; x < 32; x++) worldMap[y][x] = 0;
    }
    worldMap[10][26] = 1;

    for (int y = 15; y < 25; y++) {
        for (int x = 25; x < 28; x++) worldMap[y][x] = 0;
    }

    for (int y = 25; y < 35; y++) {
        for (int x = 15; x < 45; x++) worldMap[y][x] = 0;
    }
    worldMap[30][22] = 1;
    worldMap[30][37] = 1;

    for (int x = 45; x < 55; x++) {
        for (int y = 30; y < 32; y++) worldMap[y][x] = 0;
    }
    for (int y = 20; y < 30; y++) {
        for (int x = 53; x < 55; x++) worldMap[y][x] = 0;
    }

    for (int y = 10; y < 20; y++) {
        for (int x = 50; x < 60; x++) worldMap[y][x] = 0;
    }

    for (int x = 32; x < 50; x++) {
        for (int y = 12; y < 14; y++) worldMap[y][x] = 0;
    }
}

void castRays(SDL_Surface *surface) {
    if(SDL_LockSurface(surface) < 0) {
        fprintf(stderr, "Error on lock: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }
    SDL_FillRect(surface, 0, SDL_MapRGBA(surface->format, 0, 0, 0, 0xFF));
    float rayAngle = player.angle - degToRad(FOV / 2.0f);
    float angleStep = degToRad(FOV) / NUM_RAYS;
    
    for(int ray = 0; ray < NUM_RAYS; ray++) {
        float rayX = player.x;
        float rayY = player.y;
        float rayDirX = cosf(rayAngle);
        float rayDirY = sinf(rayAngle);
        
        int mapX = (int)rayX;
        int mapY = (int)rayY;
        
        float sideDistX, sideDistY;
        
        float deltaDistX = (rayDirX == 0) ? 1e30 : fabsf(1 / rayDirX);
        float deltaDistY = (rayDirY == 0) ? 1e30 : fabsf(1 / rayDirY);
        
        int stepX, stepY;
        
        if (rayDirX < 0) {
            stepX = -1;
            sideDistX = (rayX - mapX) * deltaDistX;
        } else {
            stepX = 1;
            sideDistX = (mapX + 1.0f - rayX) * deltaDistX;
        }
        
        if (rayDirY < 0) {
            stepY = -1;
            sideDistY = (rayY - mapY) * deltaDistY;
        } else {
            stepY = 1;
            sideDistY = (mapY + 1.0f - rayY) * deltaDistY;
        }
        
        int hit = 0;
        int side; 
        
        while (!hit) {
            if (sideDistX < sideDistY) {
                sideDistX += deltaDistX;
                mapX += stepX;
                side = 0;
            } else {
                sideDistY += deltaDistY;
                mapY += stepY;
                side = 1;
            }
            
            if (mapX < 0 || mapX >= MAP_WIDTH || mapY < 0 || mapY >= MAP_HEIGHT || 
                worldMap[mapY][mapX] == 1) {
                hit = 1;
            }
        }
        
        float perpWallDist;
        if (side == 0) {
            perpWallDist = (mapX - rayX + (1 - stepX) / 2) / rayDirX;
        } else {
            perpWallDist = (mapY - rayY + (1 - stepY) / 2) / rayDirY;
        }
        
        
        Uint8 shade = (Uint8)(255 / (1 + perpWallDist * perpWallDist * 0.1f));
        Uint32 wallColor;
        
        if(side == 0) {
            wallColor = SDL_MapRGBA(surface->format, shade, shade/2, shade/4, 255);
        } else {
            wallColor = SDL_MapRGBA(surface->format, shade/2, shade, shade/4, 255);
        }
        
        int wallHeight = (int)(WINDOW_HEIGHT / perpWallDist);
        int wallTop = (WINDOW_HEIGHT - wallHeight) / 2;
        int wallBottom = wallTop + wallHeight;

        if (wallTop < 0) wallTop = 0;
        if (wallBottom > surface->h) wallBottom = surface->h;
        
        for(int y = 0; y < wallTop; y++) {
                SetPixel(surface->pixels, ray, y, surface->pitch, SDL_MapRGBA(surface->format, 32, 32, 64, 255));
        }
        
        for(int y = wallTop; y < wallBottom; y++) {
                SetPixel(surface->pixels, ray, y, surface->pitch, wallColor);
        }
        
        for(int y = wallBottom; y < WINDOW_HEIGHT; y++) {
                SetPixel(surface->pixels, ray, y, surface->pitch, SDL_MapRGBA(surface->format, 16, 16, 32, 255));
        }
        
        rayAngle += angleStep;
    }
    
    SDL_UnlockSurface(surface);
}

void handleInput(const Uint8 *keys) {
    float newX = player.x;
    float newY = player.y;
    
    if(keys[SDL_SCANCODE_W]) {
        newX += cosf(player.angle) * MOVE_SPEED;
        newY += sinf(player.angle) * MOVE_SPEED;
    }
    if(keys[SDL_SCANCODE_S]) {
        newX -= cosf(player.angle) * MOVE_SPEED;
        newY -= sinf(player.angle) * MOVE_SPEED;
    }
    if(keys[SDL_SCANCODE_A]) {
        newX += cosf(player.angle - M_PI/2) * MOVE_SPEED;
        newY += sinf(player.angle - M_PI/2) * MOVE_SPEED;
    }
    if(keys[SDL_SCANCODE_D]) {
        newX += cosf(player.angle + M_PI/2) * MOVE_SPEED;
        newY += sinf(player.angle + M_PI/2) * MOVE_SPEED;
    }
    
    if(newX >= 0 && newX < MAP_WIDTH && newY >= 0 && newY < MAP_HEIGHT) {
        if(worldMap[(int)newY][(int)newX] == 0) {
            player.x = newX;
            player.y = newY;
        }
    }
    
    if(keys[SDL_SCANCODE_LEFT]) {
        player.angle -= TURN_SPEED;
    }
    if(keys[SDL_SCANCODE_RIGHT]) {
        player.angle += TURN_SPEED;
    }
}

int main(int argc, char **argv) {
    generateMap();
    if(SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "Error initializing SDL: %s\n", SDL_GetError());
        return EXIT_FAILURE;
    }
    SDL_Window *window = SDL_CreateWindow("Raycasting Demo", 100, 100, WINDOW_WIDTH, WINDOW_HEIGHT, 0);
    if(!window) {
        fprintf(stderr, "Error creating window: %s\n", SDL_GetError());
        return EXIT_FAILURE;
    }
    
    SDL_Surface *main_surface = SDL_GetWindowSurface(window);
    if(!main_surface) {
        fprintf(stderr, "Error getting Surface: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        exit(EXIT_FAILURE);
    }
    SDL_Surface *off_surface = CreateSurface(WINDOW_WIDTH, WINDOW_HEIGHT);
    
    if(!off_surface) {
        fprintf(stderr, "Error creating offscreen surface: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        exit(EXIT_FAILURE);
    }

    SDL_Event e;
    int active = 1;
    
    printf("Controls:\n");
    printf("WASD - Move\n");
    printf("Arrow Keys - Turn\n");
    printf("ESC - Quit\n");
    
    while(active) {
        const Uint8 *keys = SDL_GetKeyboardState(NULL);
        
        while(SDL_PollEvent(&e)) {
            switch(e.type) {
                case SDL_QUIT:
                    active = 0;
                    break;
                case SDL_KEYDOWN:
                    if(e.key.keysym.sym == SDLK_ESCAPE)
                        active = 0;
                    break;
            }
        }
        
        handleInput(keys);
        castRays(off_surface);
        
        SDL_BlitSurface(off_surface, NULL, main_surface, NULL);
        SDL_UpdateWindowSurface(window);
        SDL_Delay(16); 
    }   
    SDL_FreeSurface(off_surface);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return EXIT_SUCCESS;
}