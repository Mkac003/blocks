#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>


/* ========== UTILS ========== */

int randint(int minimum_number, int max_number) {
  return rand() % (max_number + 1 - minimum_number) + minimum_number;
  }

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

#define BLOCK_SIZE_PX 32
#define BOARD_SIZE 8

#define SELECTION_SIZE 3
#define SELECTION_PAD 130

#define MOUSE_DRAG_OFFSET 50

typedef struct {
  int w;
  int h;
  char *data;
  } Tetromino;

typedef struct {
  int tetromino;
  int color;
  
  bool is_dragging;
  } Shape;

const Tetromino tetrominos[] = {
  (Tetromino) {
    1, 1,
    "1",
    },
  (Tetromino) {
    2, 2,
    "1111",
    },
  (Tetromino) {
    2, 2,
    "1111",
    },
  (Tetromino) {
    3, 2,
    "111111",
    },
  };
const int N_TETROMINOS = (sizeof(tetrominos) / sizeof(tetrominos[0]));

const SDL_Color block_pallete[] = {
  (SDL_Color) {0, 0, 0, 0}, // unused
  (SDL_Color) {255, 0, 0, 0},
  (SDL_Color) {0, 255, 0, 0},
  (SDL_Color) {0, 0, 255, 0},
  };
const int N_COLORS = (sizeof(block_pallete) / sizeof(block_pallete[0]));

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
  
  int screen_width;
  int screen_height;
  
  SDL_Texture *textures[NUM_TEXTURES];
  
  Block board[BOARD_SIZE*BOARD_SIZE];
  int board_x;
  int board_y;
  
  Shape shape_selection[SELECTION_SIZE];
  bool mouse_is_pressed;
  } GameContext;

/* ========== GAME ========== */

void generate_shape_selection(GameContext *ctx);

GameContext *init_game() {
  srand(time(NULL));
  
  GameContext *ctx = new(GameContext);
  
  ctx->screen_width = 1280;
  ctx->screen_height = 720;
  
  ctx->win = SDL_CreateWindow("Blocks", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, ctx->screen_width, ctx->screen_height, 0);
  ctx->rnd = SDL_CreateRenderer(ctx->win, -1, SDL_RENDERER_ACCELERATED);
  
  ctx->textures[TEXTURE_BLOCK] = IMG_LoadTexture(ctx->rnd, "res/block.png");
  ctx->textures[TEXTURE_BLOCK_EMPTY] = IMG_LoadTexture(ctx->rnd, "res/block_empty.png");
  
  ctx->board_x = 0;
  ctx->board_y = 0;
  
  generate_shape_selection(ctx);
  
  return ctx;
  }

void generate_shape_selection(GameContext *ctx) {
  for (int i=0; i<SELECTION_SIZE; i++) {
    ctx->shape_selection[i] = (Shape) {randint(0, N_TETROMINOS-1), randint(1, N_COLORS-1), false};
    }
  }

void draw_block(GameContext *ctx, int screen_x, int screen_y, SDL_Color tint) {
  SDL_SetRenderDrawColor(ctx->rnd, tint.r, tint.g, tint.b, 255);
  SDL_RenderFillRect(ctx->rnd, &(SDL_Rect) {screen_x, screen_y, BLOCK_SIZE_PX, BLOCK_SIZE_PX});

  SDL_SetTextureAlphaMod(ctx->textures[TEXTURE_BLOCK], 128);
  SDL_SetTextureBlendMode(ctx->textures[TEXTURE_BLOCK], SDL_BLENDMODE_BLEND);
  Blit(ctx->rnd, ctx->textures[TEXTURE_BLOCK], screen_x, screen_y);
  }

void draw_shape(GameContext *ctx, Shape shape, int screen_x, int screen_y) {
  Tetromino tetromino = tetrominos[shape.tetromino];
  
  for (int block_x=0; block_x<tetromino.w; block_x++) {
    for (int block_y=0; block_y<tetromino.h; block_y++) {
      if (tetromino.data[block_y * tetromino.h + block_x] == '1') {
        draw_block(ctx, BLOCK_SIZE_PX * block_x + screen_x, BLOCK_SIZE_PX * block_y + screen_y, block_pallete[shape.color]);
        }
      }
    }
  }

void draw_shape_centered(GameContext *ctx, Shape shape, int screen_x, int screen_y) {
  screen_x -= BLOCK_SIZE_PX * tetrominos[shape.tetromino].w / 2;
  screen_y -= BLOCK_SIZE_PX * tetrominos[shape.tetromino].h / 2;
  draw_shape(ctx, shape, screen_x, screen_y);
  }

bool centered_shape_is_hovered(Shape shape, int screen_x, int screen_y, int mouse_x, int mouse_y) {
  screen_x -= BLOCK_SIZE_PX * tetrominos[shape.tetromino].w / 2;
  screen_y -= BLOCK_SIZE_PX * tetrominos[shape.tetromino].h / 2;
  
  int px_width = BLOCK_SIZE_PX * tetrominos[shape.tetromino].w;
  int px_height = BLOCK_SIZE_PX * tetrominos[shape.tetromino].h;
  
  if (mouse_x > screen_x && mouse_x < screen_x+px_width) {
    if (mouse_y > screen_y && mouse_y < screen_y+px_height) {
      return true;
      }
    }
  return false;
  }

bool place_shape(GameContext *ctx, Shape shape, int block_x, int block_y) {
  if (block_x >= BOARD_SIZE || block_y >= BOARD_SIZE) return false;
  
  Tetromino tetromino = tetrominos[shape.tetromino];
  
  for (int rx=0; rx<tetromino.w; rx++) {
    for (int ry=0; ry<tetromino.h; ry++) {
      if (tetromino.data[rx * tetromino.h + ry] != '1') continue;
      
      if (board_get_at(ctx->board, block_x+rx, block_y+ry)) return false;
      }
    }
  
  for (int rx=0; rx<tetromino.w; rx++) {
    for (int ry=0; ry<tetromino.h; ry++) {
      if (tetromino.data[rx * tetromino.h + ry] != '1') continue;
      
      board_set_at(ctx->board, block_x+rx, block_y+ry, shape.color);
      }
    }
  
  return true;
  }

bool update_and_draw(GameContext *ctx) {
  // Update
  SDL_Event event;
  bool mouse_just_pressed = false;
  
  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_QUIT) {
      return true;
      }
    else if (event.type == SDL_MOUSEBUTTONDOWN) {
      if (event.button.button == SDL_BUTTON_LEFT) {
        ctx->mouse_is_pressed = true;
        mouse_just_pressed = true;
        }
      }
    else if (event.type == SDL_MOUSEBUTTONUP) {
      if (event.button.button == SDL_BUTTON_LEFT) {
        ctx->mouse_is_pressed = false;
        }
      }
    }
  
  ctx->board_x = ctx->screen_width / 2 - (BLOCK_SIZE_PX * BOARD_SIZE) / 2;
  ctx->board_y = 150;
  
  int mouse_x, mouse_y;
  SDL_GetMouseState(&mouse_x, &mouse_y);
  
  bool mouse_is_pressed = ctx->mouse_is_pressed;
  
  // Clear
  SDL_SetRenderDrawColor(ctx->rnd, 0, 0, 0, 255);
  SDL_RenderClear(ctx->rnd);
  
  // Draw the board
  int screen_x, screen_y;
  for (int block_x=0; block_x<BOARD_SIZE; block_x++) {
    for (int block_y=0; block_y<BOARD_SIZE; block_y++) {
      Block block = board_get_at(ctx->board, block_x, block_y);
      
      screen_x = (BLOCK_SIZE_PX)*block_x+ctx->board_x;
      screen_y = (BLOCK_SIZE_PX)*block_y+ctx->board_y;
      
      if (block) {
        SDL_Color tint = block_pallete[block];
        draw_block(ctx, screen_x, screen_y, tint);
        }
      else {
        Blit(ctx->rnd, ctx->textures[TEXTURE_BLOCK_EMPTY], screen_x, screen_y);
        }
      }
    }
  
  // draw_shape(ctx, (Shape) {0, 1}, 400, 400);
  
  
  for (int i=0;i<SELECTION_SIZE; i++) {
    const Shape shape = ctx->shape_selection[i];
    
    if (shape.is_dragging) {
      if (!mouse_is_pressed) {
        ctx->shape_selection[i].is_dragging = false;
        
        Tetromino tetromino = tetrominos[shape.tetromino];
        
        int shape_x = floor(((float) mouse_x - (float) ctx->board_x) / (float) BLOCK_SIZE_PX - tetromino.w/2);
        int shape_y = floor(((float) (mouse_y - MOUSE_DRAG_OFFSET) - (float) ctx->board_y) / (float) BLOCK_SIZE_PX - tetromino.h/2);
        
        place_shape(ctx, shape, shape_x, shape_y);
        }
      
      draw_shape_centered(ctx, shape, mouse_x, mouse_y-MOUSE_DRAG_OFFSET);
      }
    else {
      int shape_center_x = (ctx->screen_width/2-(SELECTION_PAD/2*SELECTION_SIZE)+SELECTION_PAD/2)+(i*SELECTION_PAD);
      int shape_center_y = ctx->board_y + BOARD_SIZE*BLOCK_SIZE_PX + 100;
      
      if (mouse_just_pressed && centered_shape_is_hovered(shape, shape_center_x, shape_center_y, mouse_x, mouse_y)) {
        ctx->shape_selection[i].is_dragging = true;
        }
      
      draw_shape_centered(ctx, shape, (ctx->screen_width/2-(SELECTION_PAD/2*SELECTION_SIZE)+SELECTION_PAD/2)+(i*SELECTION_PAD), ctx->board_y + BOARD_SIZE*BLOCK_SIZE_PX+100);
      }
    }
  
  // Refresh
  SDL_RenderPresent(ctx->rnd);
  
  return false;
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