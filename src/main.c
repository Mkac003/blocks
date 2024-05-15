#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

/* ========== UTILS ========== */

void handle_sdl_error() {
  printf("SDL ERROR: %s\n", SDL_GetError());
  }

int randint(int minimum_number, int max_number) {
  return rand() % (max_number + 1 - minimum_number) + minimum_number;
  }

void Blit(SDL_Renderer *renderer, SDL_Texture *texture, int x, int y) {
  int w, h;
  if (SDL_QueryTexture(texture, NULL, NULL, &w, &h)) handle_sdl_error();
  if (SDL_RenderCopy(renderer, texture, NULL, &(SDL_Rect) {x, y, w, h})) handle_sdl_error();
  }

/* ========== MAIN ========== */

/* indexing macros */

#define WIDTH  0
#define HEIGHT 1

#define X 0
#define Y 1

/* texture constants */
#define NUM_TEXTURES 16

#define TEXTURE_BLOCK        0
#define TEXTURE_BLOCK_EMPTY  1

/* ... */
#define BLOCK_SIZE_PX 32
#define BOARD_SIZE 8

typedef struct {
  SDL_Window *window;
  SDL_Renderer *renderer;
  
  int window_size[2];
  
  SDL_Texture *textures[NUM_TEXTURES];
  uint8_t board[BOARD_SIZE * BOARD_SIZE];
  } GameContext;

void init(GameContext *ctx) {
  ctx->window_size[WIDTH] = 720;
  ctx->window_size[HEIGHT] = 720;
  
  /* Create the window and SDL renderer */
  ctx->window = SDL_CreateWindow("blocks", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, ctx->window_size[WIDTH], ctx->window_size[HEIGHT], 0);
  if (!ctx->window) handle_sdl_error();
  
  ctx->renderer = SDL_CreateRenderer(ctx->window, -1, SDL_RENDERER_ACCELERATED);
  if (!ctx->renderer) handle_sdl_error();
  
  /* Load textures */
  ctx->textures[TEXTURE_BLOCK]       = IMG_LoadTexture(ctx->renderer, "res/block.png");
  ctx->textures[TEXTURE_BLOCK_EMPTY] = IMG_LoadTexture(ctx->renderer, "res/block_empty.png");
  
  if (!ctx->textures[0]) handle_sdl_error();
  
  /* Clear the board */
  memset(ctx->board, 0, sizeof(ctx->board[0]) * BOARD_SIZE * BOARD_SIZE);
  }

bool frame(GameContext *ctx) {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_QUIT) return true;
    }
  
  int board_position[2] = {
    ctx->window_size[WIDTH]  / 2 - (BLOCK_SIZE_PX * BOARD_SIZE) / 2,
    100
    };
  
  SDL_SetRenderDrawColor(ctx->renderer, 0, 0, 0, 255);
  SDL_RenderClear(ctx->renderer);
  
  int screen_x, screen_y, x, y;
  for (int x=0; x<BOARD_SIZE; x++) {
    for (int y=0; y<BOARD_SIZE; y++) {
      screen_x = BLOCK_SIZE_PX * x + board_position[X];
      screen_y = BLOCK_SIZE_PX * y + board_position[Y];
      
      Blit(ctx->renderer, ctx->textures[TEXTURE_BLOCK_EMPTY], screen_x, screen_y);
      }
    }
  
  SDL_RenderPresent(ctx->renderer);
  
  return false;
  }

void stop(GameContext *ctx) {
  SDL_DestroyRenderer(ctx->renderer);
  SDL_DestroyWindow(ctx->window);
  }

int main() {
  GameContext *ctx = malloc(sizeof(GameContext));
  
  init(ctx);
  
  while (1) {
    if (frame(ctx)) break;
    }
  
  stop(ctx);
  
  return 0;
  }