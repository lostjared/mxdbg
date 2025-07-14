#include "SDL.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#define WINDOW_WIDTH 960
#define WINDOW_HEIGHT 720
#define MAP_WIDTH 160
#define MAP_HEIGHT 160
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

Player player = {2.0f, 2.0f, 0.0f};

float degToRad(float deg) {
    return deg * M_PI / 180.0f;
}

void generateMap() {
    for(int y = 0; y < MAP_HEIGHT; y++) {
        for(int x = 0; x < MAP_WIDTH; x++) {
            if(x == 0 || x == MAP_WIDTH-1 || y == 0 || y == MAP_HEIGHT-1) {
                worldMap[y][x] = 1;
            } else if((x % 10 == 0 && y % 10 < 8) || (y % 15 == 0 && x % 15 < 12)) {
                worldMap[y][x] = 1;
            } else {
                worldMap[y][x] = 0;
            }
        }
    }
}

void castRays(SDL_Surface *surface) {
    if(SDL_LockSurface(surface) < 0) {
        fprintf(stderr, "Error on lock");
        exit(EXIT_FAILURE);
    }
    
    for(int y = 0; y < WINDOW_HEIGHT; y++) {
        for(int x = 0; x < WINDOW_WIDTH; x++) {
            SetPixel(surface->pixels, x, y, surface->pitch, 
                    SDL_MapRGBA(surface->format, 0, 0, 0, 255));
        }
    }
    
    float rayAngle = player.angle - degToRad(FOV / 2.0f);
    float angleStep = degToRad(FOV) / NUM_RAYS;
    
    for(int ray = 0; ray < NUM_RAYS; ray++) {
        float rayX = player.x;
        float rayY = player.y;
        float rayDirX = cosf(rayAngle);
        float rayDirY = sinf(rayAngle);
        
        float distance = 0.0f;
        int hit = 0;
        int wallType = 1;
        
        while(!hit) {
            rayX += rayDirX * 0.01f;
            rayY += rayDirY * 0.01f;
            distance += 0.01f;
            
            int mapX = (int)rayX;
            int mapY = (int)rayY;
            
            if(mapX < 0 || mapX >= MAP_WIDTH || mapY < 0 || mapY >= MAP_HEIGHT || 
               worldMap[mapY][mapX] == 1) {
                hit = 1;
                if(mapX >= 0 && mapX < MAP_WIDTH && mapY >= 0 && mapY < MAP_HEIGHT) {
                    wallType = worldMap[mapY][mapX];
                }
            }
        }
        
        distance *= cosf(rayAngle - player.angle);
        if (distance < 0.01f) distance = 0.01f; 

        int wallHeight = (int)(WINDOW_HEIGHT / distance);
        int wallTop = (WINDOW_HEIGHT - wallHeight) / 2;
        int wallBottom = wallTop + wallHeight;

        if (wallTop < 0) wallTop = 0;
        if (wallBottom > surface->h) wallBottom = surface->h;

        Uint8 shade = (Uint8)(255 / (1 + distance * distance * 0.1f));
        Uint32 wallColor;
        
        if(fabs(rayDirX) > fabs(rayDirY)) {
            wallColor = SDL_MapRGBA(surface->format, shade, shade/2, shade/4, 255);
        } else {
            wallColor = SDL_MapRGBA(surface->format, shade/2, shade, shade/4, 255);
        }
        
        for(int y = 0; y < wallTop; y++) {
                SetPixel(surface->pixels, ray, y, surface->pitch,
                         SDL_MapRGBA(surface->format, 32, 32, 64, 255));
        }
        
        for(int y = wallTop; y < wallBottom; y++) {
                SetPixel(surface->pixels, ray, y, surface->pitch, wallColor);
        }
        
        for(int y = wallBottom; y < WINDOW_HEIGHT; y++) {
                SetPixel(surface->pixels, ray, y, surface->pitch,
                         SDL_MapRGBA(surface->format, 16, 16, 32, 255));
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
    
    SDL_Window *window = SDL_CreateWindow("Raycasting Demo", 100, 100, 
                                         WINDOW_WIDTH, WINDOW_HEIGHT, 0);
    if(!window) {
        fprintf(stderr, "Error creating window: %s\n", SDL_GetError());
        return EXIT_FAILURE;
    }
    
    SDL_Surface *main_surface = SDL_GetWindowSurface(window);
    SDL_Surface *off_surface = CreateSurface(WINDOW_WIDTH, WINDOW_HEIGHT);
    
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