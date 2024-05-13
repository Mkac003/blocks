#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>


/* ========== UTILS ========== */

#define new(t) calloc(1, sizeof(t))

void Blit(SDL_Renderer *renderer, SDL_Texture *texture, int x, int y) {
  int w, h;
  SDL_QueryTexture(texture, NULL, NULL, &w, &h);
  SDL_RenderCopy(renderer, texture, NULL, &(SDL_Rect) {x, y, w, h});
  }

/* ========== MAIN ========== */

#define NUM_TEXTURES 2

#define TEXTURE_BLOCK 0
#define TEXTURE_BLOCK_EMPTY 1

#define BLOCK_SIZE_PX 48
#define BOARD_SIZE 8

const SDL_Color block_pallete[] = {
  (SDL_Color) {255, 0, 0, 0}, // unused
  (SDL_Color) {255, 0, 0, 0},
  (SDL_Color) {0, 255, 0, 0},
  (SDL_Color) {0, 0, 255, 0},
  };

typedef uint8_t Block;

void board_set_at(Block *board, int x, int y, Block value) {
  board[y*BOARD_SIZE+x] = value;
  }

Block board_get_at(Block *board, int x, int y) {
  return board[y*BOARD_SIZE+x];
  }

typedef struct {
  SDL_Window *win;
  SDL_Renderer *rnd;
  SDL_Texture *textures[NUM_TEXTURES];
  
  Block board[BOARD_SIZE*BOARD_SIZE];
  int board_x;
  int board_y;
  } GameContext;

GameContext *init_game() {
  GameContext *ctx = new(GameContext);
  
  ctx->win = SDL_CreateWindow("Blocks", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, 0);
  ctx->rnd = SDL_CreateRenderer(ctx->win, -1, SDL_RENDERER_ACCELERATED);
  
  ctx->textures[TEXTURE_BLOCK] = IMG_LoadTexture(ctx->rnd, "res/block.png");
  ctx->textures[TEXTURE_BLOCK_EMPTY] = IMG_LoadTexture(ctx->rnd, "res/block_empty.png");
  
  ctx->board_x = 100;
  ctx->board_y = 100;
  
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
  // Clear
  SDL_SetRenderDrawColor(ctx->rnd, 0, 0, 0, 255);
  SDL_RenderClear(ctx->rnd);
  
  for (int block_x=0; block_x<BOARD_SIZE; block_x++) {
    for (int block_y=0; block_y<BOARD_SIZE; block_y++) {
      Block block = board_get_at(ctx->board, block_x, block_y);
      
      int screen_x = block_x*BLOCK_SIZE_PX+ctx->board_x;
      int screen_y = block_y*BLOCK_SIZE_PX+ctx->board_y;
      
      if (block) {
        SDL_Color tint = block_pallete[block];
        
        SDL_SetRenderDrawColor(ctx->rnd, tint.r, tint.g, tint.b, 255);
        SDL_RenderFillRect(ctx->rnd, &(SDL_Rect) {screen_x, screen_y, BLOCK_SIZE_PX, BLOCK_SIZE_PX});
        
        SDL_SetTextureAlphaMod(ctx->textures[TEXTURE_BLOCK], 128);
        SDL_SetTextureBlendMode(ctx->textures[TEXTURE_BLOCK], SDL_BLENDMODE_BLEND);
        Blit(ctx->rnd, ctx->textures[TEXTURE_BLOCK], screen_x, screen_y);
        }
      else {
        Blit(ctx->rnd, ctx->textures[TEXTURE_BLOCK_EMPTY], screen_x, screen_y);
        }
      }
    }
  
  // Refresh
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