#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include "shapes.h"

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

#define ASSERT(x, msg) do {if (!(x)) {printf("(%s:%d) assertion %s failed: %s\n", __FILE__, __LINE__, #x, msg);}} while (0)

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
#define BOARD_SIZE 10

#define SELECTION_SIZE 3

#define NOT_DRAGGING -1
#define MOUSE_DRAG_PADDING 20

Shape shape_from_template(unsigned int template_id, unsigned int color) {
  Shape template = shape_templates[template_id];
  return (Shape) {template.width, template.height, color, template.data};
  }

/* Colors */

const SDL_Color colors[] = {
  (SDL_Color) {0, 0, 0, 0}, /* unused */
  (SDL_Color) {255, 53, 53, 255},
  (SDL_Color) {72, 153, 94, 255},
  (SDL_Color) {62, 92, 169, 255},
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
  int dragging_shape;
  
  int score;
  } GameContext;

void generate_selection(GameContext *ctx);

void init(GameContext *ctx) {
  srand(time(NULL));
  
  ctx->window_size[WIDTH] = 1280;
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
  
  generate_selection(ctx);
  
  /* no shape is currently being dragged */
  ctx->dragging_shape = NOT_DRAGGING;
  }

int clear_solved(uint8_t *board) {
  int score = 0;
  bool is_full = false;
  
  /* X */
  for (int row=0; row<BOARD_SIZE; row++) {
    is_full = true;
    
    for (int x=0; x<BOARD_SIZE; x++) {
      if (!board[row * BOARD_SIZE + x]) {
        is_full = false;
        break;
        }
      }
    
    if (is_full) {
      score += BOARD_SIZE;
      memset(board + row * BOARD_SIZE, 0, sizeof(uint8_t) * BOARD_SIZE);
      }
    }
  
  /* Y */
  for (int column=0; column<BOARD_SIZE; column++) {
    is_full = true;
    
    for (int y=0; y<BOARD_SIZE; y++) {
      if (!board[y * BOARD_SIZE + column]) {
        is_full = false;
        break;
        }
      }
    
    if (is_full) {
      score += BOARD_SIZE;
      for (int y=0; y<BOARD_SIZE; y++)
        board[y * BOARD_SIZE + column] = 0;
      }
    }
  
  return score;
  }

void generate_selection(GameContext *ctx) {
  /* Generate selection*/
  for (int i=0; i<SELECTION_SIZE; i ++) {
    ctx->selection[i] = shape_from_template(randint(0, NUM_TEMPLATES-1), randint(1, NUM_COLORS-1));
    }
  }

void draw_block(GameContext *ctx, int screen_x, int screen_y, SDL_Color color) {
  SDL_SetRenderDrawColor(ctx->renderer, color.r, color.g, color.b, 255);
  SDL_RenderFillRect(ctx->renderer, &(SDL_Rect) {screen_x, screen_y, BLOCK_SIZE_PX, BLOCK_SIZE_PX});
  
  SDL_Texture *texture = ctx->textures[TEXTURE_BLOCK];
  
  SDL_SetTextureAlphaMod(texture, 72);
  SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
  Blit(ctx->renderer, texture, screen_x, screen_y);
  }

void draw_shape(GameContext *ctx, Shape shape, int screen_x, int screen_y) {
  for (int block_x=0; block_x<shape.width; block_x ++) {
    for (int block_y=0; block_y<shape.height; block_y ++) {
      if (shape.data[block_y * shape.width + block_x] != '1') continue;
      
      draw_block(ctx, block_x * BLOCK_SIZE_PX + screen_x, block_y * BLOCK_SIZE_PX + screen_y, colors[shape.color]);
      }
    }
  }

bool shape_is_hovered(Shape shape, int screen_x, int screen_y, int mouse_x, int mouse_y) {
  int width_px = shape.width * BLOCK_SIZE_PX;
  int height_px = shape.height * BLOCK_SIZE_PX;
  
  /* Rectangle check */
  if (!(mouse_x > screen_x && mouse_x < screen_x + width_px)
   || !(mouse_y > screen_y && mouse_y < screen_y + height_px))
    return false;
  
  /* Precise check */
  int block_x = floor((float) (mouse_x - screen_x) / (float) BLOCK_SIZE_PX);
  int block_y = floor((float) (mouse_y - screen_y) / (float) BLOCK_SIZE_PX);
  
  ASSERT(block_x >= 0, "block_x !>= 0");
  ASSERT(block_y >= 0, "block_y !>= 0");
  ASSERT(block_x < shape.width, "block_x > shape.width");
  ASSERT(block_y < shape.height, "block_y > shape.height");
  
  if (shape.data[block_y * shape.width + block_x] != '1')
    return false;
  
  return true;
  }

bool place_shape(uint8_t *board, Shape shape, int block_x, int block_y) {
  /* Check if the shape can be placed */
  for (int shape_x=0; shape_x < shape.width; shape_x ++) {
    for (int shape_y=0; shape_y < shape.height; shape_y ++) {
      if (shape.data[shape_y * shape.width + shape_x] != '1') continue;
      if (block_x + shape_x >= BOARD_SIZE || block_y + shape_y >= BOARD_SIZE) return false;
      
      int index = (block_y + shape_y) * BOARD_SIZE + (block_x + shape_x);
      if (board[index]) return false;
      }
    }
  
  for (int shape_x=0; shape_x < shape.width; shape_x ++) {
    for (int shape_y=0; shape_y < shape.height; shape_y ++) {
      if (shape.data[shape_y * shape.width + shape_x] != '1') continue;
      
      int index = (block_y + shape_y) * BOARD_SIZE + (block_x + shape_x);
      board[index] = (uint8_t) shape.color;
      }
    }
  
  return true;
  }

bool frame(GameContext *ctx) {
  bool just_clicked = false;
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_QUIT) return true;
    if (event.type == SDL_MOUSEBUTTONDOWN) {
      if (event.button.button == SDL_BUTTON_LEFT) just_clicked = true;
      }
    }
  
  int board_position[2] = {
    ctx->window_size[WIDTH] / 2 - (BLOCK_SIZE_PX * BOARD_SIZE) / 2,
    100
    };
  
  int mouse_position[2] = {0};
  uint32_t button_mask = SDL_GetMouseState(&mouse_position[X], &mouse_position[Y]);
  
  SDL_SetRenderDrawColor(ctx->renderer, 0, 0, 0, 255);
  SDL_RenderClear(ctx->renderer);
  
  /* Draw the board */
  {
    int screen_x, screen_y, x, y, index;
    for (int x=0; x<BOARD_SIZE; x ++) {
      for (int y=0; y<BOARD_SIZE; y ++) {
        screen_x = BLOCK_SIZE_PX * x + board_position[X];
        screen_y = BLOCK_SIZE_PX * y + board_position[Y];
        index = y * BOARD_SIZE + x;
        
        if (ctx->board[index])
          draw_block(ctx, screen_x, screen_y, colors[ctx->board[index]]);
        else
          Blit(ctx->renderer, ctx->textures[TEXTURE_BLOCK_EMPTY], screen_x, screen_y);
        }
      }
    }
  
  /* Place the dragged shape if the mouse is released and the shape is in bounds */
  {
    if (ctx->dragging_shape != NOT_DRAGGING && !(button_mask & SDL_BUTTON(1))) {
      const Shape shape = ctx->selection[ctx->dragging_shape];
      
      int screen_x = mouse_position[X] - shape.width * BLOCK_SIZE_PX / 2;
      int screen_y = mouse_position[Y] - shape.height * BLOCK_SIZE_PX - MOUSE_DRAG_PADDING;
      
      int block_x = round((float) (screen_x - board_position[X]) / (float) BLOCK_SIZE_PX);
      int block_y = round((float) (screen_y - board_position[Y]) / (float) BLOCK_SIZE_PX);
      
      int index = block_y * BOARD_SIZE + block_x;
      
      if (index < BOARD_SIZE * BOARD_SIZE) {
        if (place_shape(ctx->board, shape, block_x, block_y)) {
          ctx->score += clear_solved(ctx->board);
          ctx->selection[ctx->dragging_shape].color = 0;
          
          printf("%d\n", ctx->score);
          }
        }
      
      ctx->dragging_shape = NOT_DRAGGING;
      }
    }
  
  /* Draw and update the selection */
  {
    const int padding = 100;
    int center_x, screen_x, screen_y;
    Shape shape;
    
    for (int i=0; i<SELECTION_SIZE; i ++) {
      shape = ctx->selection[i];
      
      if (shape.color == 0) continue;
      
      if (ctx->dragging_shape == i) {
        screen_x = mouse_position[X] - shape.width * BLOCK_SIZE_PX / 2;
        screen_y = mouse_position[Y] - shape.height * BLOCK_SIZE_PX - MOUSE_DRAG_PADDING;
        
        draw_shape(ctx, shape, screen_x, screen_y);
        }
      else {
        center_x = (ctx->window_size[WIDTH] / 2) - (SELECTION_SIZE * padding / 2) + (i * padding) + padding / 2;
        screen_x = center_x - shape.width * BLOCK_SIZE_PX / 2;
        screen_y = board_position[Y] + BOARD_SIZE * BLOCK_SIZE_PX + 10;
        
        draw_shape(ctx, shape, screen_x, screen_y);
        
        if (shape_is_hovered(shape, screen_x, screen_y, mouse_position[X], mouse_position[Y])) {
          if (just_clicked) {
            ctx->dragging_shape = i;
            }
          }
        }
      }
    }
  
  /* Regenerate the selection when all the blocks are used */
  {
    bool do_generate = true;
    for (int i=0; i<SELECTION_SIZE; i ++) {
      if (ctx->selection[i].color) {
        do_generate = false;
        break;
        }
      }
    
    if (do_generate)
      generate_selection(ctx);
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