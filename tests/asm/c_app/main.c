#include"SDL.h"
#include<stdio.h>
#define WINDOW_WIDTH 960
#define WINDOW_HEIGHT  720

extern SDL_Surface *CreateSurface(int w, int h);
extern void RandomPixels(SDL_Surface *surface);
extern void SetPixel(void *buffer, int x, int y, int pitch, int color);

int main(int argc, char **argv) {
    if(SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "Error initalizing SDL: %s\n", SDL_GetError());
        return EXIT_FAILURE;
    }
    SDL_Window *window = SDL_CreateWindow("CApp", 100,  100, WINDOW_WIDTH, WINDOW_HEIGHT, 0);
    if(!window) {
        fprintf(stderr, "Error creating winodw: %s\n", SDL_GetError());
        return EXIT_FAILURE;
    }
    SDL_Surface *main_surface = SDL_GetWindowSurface(window);
    SDL_Surface *off_surface = CreateSurface(WINDOW_WIDTH, WINDOW_HEIGHT);
    SDL_Event e;
    int active = 1;
    while(active == 1) {
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
         RandomPixels(off_surface);
         SDL_BlitSurface(off_surface, NULL, main_surface, NULL);
         SDL_UpdateWindowSurface(window);
         SDL_Delay(1);
    }
    SDL_Quit();
    return  EXIT_SUCCESS;
}