#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include <SDL2/SDL.h>

#define new(t) calloc(1, sizeof(t))

typedef struct {
  SDL_Window *win;
  SDL_Renderer *rnd;
  } GameContext;

GameContext *init_game() {
  GameContext *ctx = new(GameContext);
  
  ctx->win = SDL_CreateWindow("Blocks", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, 0);
  ctx->rnd = SDL_CreateRenderer(ctx->win, -1, SDL_RENDERER_ACCELERATED);
  
  return ctx;
  }

bool update(GameContext *ctx) {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_QUIT) {
      return true;
      }
    }
  
  return false;
  }

void draw(GameContext *ctx) {
  SDL_SetRenderDrawColor(ctx->rnd, 0, 0, 0, 255);
  SDL_RenderClear(ctx->rnd);
  
  
  SDL_RenderPresent(ctx->rnd);
  }

bool update_and_draw(GameContext *ctx) {
  bool do_quit = update(ctx);
  draw(ctx);
  return do_quit;
  }
  

int main() {
  GameContext *ctx = init_game();
  
  while (1) {
    if (update_and_draw(ctx)) {
      break;
      }
    }
  
  return 0;
  }