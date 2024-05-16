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

#define SELECTION_SIZE 3

/* Shapes */

typedef struct {
  unsigned int width;
  unsigned int height;
  unsigned int color;
  char *data;
  } Shape;

const Shape shape_templates[] = {
  (Shape) {
    2, 2,
    0,
    "1111",
    },
  };

const int NUM_TEMPLATES = sizeof(shape_templates) / sizeof(shape_templates[0]);

Shape shape_from_template(unsigned int template_id, unsigned int color) {
  Shape template = shape_templates[template_id];
  return (Shape) {template.width, template.height, color, template.data};
  }

/* Colors */

const SDL_Color colors[] = {
  (SDL_Color) {0, 0, 0, 0}, /* unused */
  (SDL_Color) {255, 0, 0, 255},
  (SDL_Color) {255, 0, 255, 255},
  (SDL_Color) {0, 0, 255, 255},
  (SDL_Color) {0, 255, 0, 255},
  };

const int NUM_COLORS = sizeof(colors) / sizeof(colors[0]);

/* ... */

typedef struct {
  SDL_Window *window;
  SDL_Renderer *renderer;
  
  int window_size[2];
  
  SDL_Texture *textures[NUM_TEXTURES];
  uint8_t board[BOARD_SIZE * BOARD_SIZE];
  
  Shape selection[SELECTION_SIZE];
  } GameContext;

void init(GameContext *ctx) {
  srand(time(NULL));
  
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
  
  /* Generate selection*/
  for (int i=0; i<SELECTION_SIZE; ++i) {
    ctx->selection[i] = shape_from_template(randint(0, NUM_TEMPLATES-1), randint(1, NUM_COLORS-1));
    }
  }

void draw_block(GameContext *ctx, int screen_x, int screen_y, SDL_Color color) {
  SDL_SetRenderDrawColor(ctx->renderer, color.r, color.g, color.b, 255);
  SDL_RenderFillRect(ctx->renderer, &(SDL_Rect) {screen_x, screen_y, BLOCK_SIZE_PX, BLOCK_SIZE_PX});
  
  SDL_Texture *texture = ctx->textures[TEXTURE_BLOCK];
  
  SDL_SetTextureAlphaMod(texture, 128);
  SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
  Blit(ctx->renderer, texture, screen_x, screen_y);
  }

void draw_shape(GameContext *ctx, Shape shape, int screen_x, int screen_y) {
  for (int block_x=0; block_x<shape.width; ++block_x) {
    for (int block_y=0; block_y<shape.height; ++block_y) {
      if (shape.data[block_y * shape.height + block_x] != '1') continue;
      
      draw_block(ctx, block_x * BLOCK_SIZE_PX + screen_x, block_y * BLOCK_SIZE_PX + screen_y, colors[shape.color]);
      }
    }
  }

bool frame(GameContext *ctx) {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_QUIT) return true;
    }
  
  int board_position[2] = {
    ctx->window_size[WIDTH] / 2 - (BLOCK_SIZE_PX * BOARD_SIZE) / 2,
    100
    };
  
  SDL_SetRenderDrawColor(ctx->renderer, 0, 0, 0, 255);
  SDL_RenderClear(ctx->renderer);
  
  int screen_x, screen_y, x, y;
  for (int x=0; x<BOARD_SIZE; ++x) {
    for (int y=0; y<BOARD_SIZE; ++y) {
      screen_x = BLOCK_SIZE_PX * x + board_position[X];
      screen_y = BLOCK_SIZE_PX * y + board_position[Y];
      
      Blit(ctx->renderer, ctx->textures[TEXTURE_BLOCK_EMPTY], screen_x, screen_y);
      }
    }
  
  /* Draw the selection */
  {
    const int padding = 100;
    int center_x, screen_x;
    Shape shape;
    
    for (int i=0; i<SELECTION_SIZE; ++i) {
      shape = ctx->selection[i];
      
      center_x = (ctx->window_size[WIDTH] / 2) - (SELECTION_SIZE * padding / 2) + (i * padding) + padding / 2;
      screen_x = center_x - shape.width * BLOCK_SIZE_PX / 2;
      
      draw_shape(ctx, shape, screen_x, board_position[Y] + BOARD_SIZE * BLOCK_SIZE_PX + 10);
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