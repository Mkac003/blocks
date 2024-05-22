#include <stdlib.h>
#include <stdio.h>
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

int clampi(int v, int mi, int mx) {
  if (v < mi) return mi;
  if (v > mx) return mx;
  return v;
  }

SDL_Color AdjustColorBrightness(SDL_Color color, int f) {
  return (SDL_Color) {
                      clampi((int) color.r + f, 0, 255),
                      clampi((int) color.g + f, 0, 255),
                      clampi((int) color.b + f, 0, 255),
                      color.a,
                      };
  }

#define ASSERT(x, msg) do {if (!(x)) {printf("(%s:%d) assertion %s failed: %s\n", __FILE__, __LINE__, #x, msg);}} while (0)

typedef unsigned char bool;
#define true 1
#define false 0

/* ========== MAIN ========== */

#define FPS 60

/* indexing macros */

#define WIDTH  0
#define HEIGHT 1

#define X 0
#define Y 1

/* texture constants */
#define NUM_TEXTURES 16

#define TEXTURE_BLOCK            0
#define TEXTURE_BLOCK_EMPTY      1
#define TEXTURE_RESTART          2
#define TEXTURE_7SEGMENT_0       3
#define TEXTURE_7SEGMENT_1       4
#define TEXTURE_7SEGMENT_2       5
#define TEXTURE_7SEGMENT_3       6
#define TEXTURE_7SEGMENT_4       7
#define TEXTURE_7SEGMENT_5       8
#define TEXTURE_7SEGMENT_6       9
#define TEXTURE_7SEGMENT_7       10
#define TEXTURE_7SEGMENT_8       11
#define TEXTURE_7SEGMENT_9       12
#define TEXTURE_7SEGMENT_BG      13
#define TEXTURE_7SEGMENT_MINUS   14

/* ... */
const uint8_t block_texture_colors[] = {
  253, /* TOP */
  159, /* RIGHT */
  112, /* BOTTOM */
  224, /* LEFT */
  200, /* MIDDLE */
  92,  /* BORDER */
  };

#define BLOCK_TEXTURE_SIDE_PX 6

#define BLOCK_ALPHA_MOD 72
#define BLOCK_SIZE_PX 32
#define BOARD_SIZE 8

#define SELECTION_SIZE 3

#define NOT_DRAGGING -1
#define MOUSE_DRAG_PADDING 20

Shape shape_from_template(unsigned int template_id, unsigned int color) {
  Shape template = shape_templates[template_id];
  return (Shape) {template.width, template.height, color, template.data};
  }

/* Colors */

const SDL_Color colors[] = {
  (SDL_Color) {255, 255, 255, 255}, /* used for highliting */
  (SDL_Color) {255, 53, 53, 255},
  (SDL_Color) {72, 153, 94, 255},
  (SDL_Color) {62, 92, 169, 255},
  };

const int NUM_COLORS = sizeof(colors) / sizeof(colors[0]);

/* ... */

typedef enum {
  GAME_MAIN_MENU,
  GAME_PLAYING,
  } GameState;

typedef enum {
  PLAYING,
  GAME_OVER_ANIMATION,
  } PlayingState;

typedef struct {
  /* general */
  SDL_Window *window;
  SDL_Renderer *renderer;
  SDL_Texture *textures[NUM_TEXTURES];
  
  GameState state;
  int window_size[2];
  
  uint64_t start_frame;
  uint64_t end_frame;
  uint64_t last;
  
  float dt;
  
  /* ingame stuff */
  uint8_t board[BOARD_SIZE * BOARD_SIZE];
  
  Shape selection[SELECTION_SIZE];
  int dragging_shape;
  
  PlayingState playing_state;
  
  float game_over_anim_timer;
  int game_over_squares_left;
  
  int score;
  } GameContext;

void draw_block_rect(GameContext *ctx, int x, int y, int w, int h, SDL_Color color) {
  /* crazy but it works */
  SDL_Texture *block_texture = ctx->textures[TEXTURE_BLOCK];
  
  SDL_SetRenderDrawColor(ctx->renderer, color.r, color.g, color.b, 255);
  SDL_RenderFillRect(ctx->renderer, &(SDL_Rect) {x, y, w, h});
  
  SDL_SetTextureAlphaMod(block_texture, BLOCK_ALPHA_MOD);
  SDL_SetTextureBlendMode(block_texture, SDL_BLENDMODE_BLEND);
  
  SDL_RenderCopy(ctx->renderer, block_texture, &(SDL_Rect) {0, 0, BLOCK_TEXTURE_SIDE_PX, BLOCK_TEXTURE_SIDE_PX}, &(SDL_Rect) {x, y, BLOCK_TEXTURE_SIDE_PX, BLOCK_TEXTURE_SIDE_PX});
  SDL_SetRenderDrawColor(ctx->renderer, block_texture_colors[0], block_texture_colors[0], block_texture_colors[0], BLOCK_ALPHA_MOD);
  SDL_RenderFillRect(ctx->renderer, &(SDL_Rect) {x+BLOCK_TEXTURE_SIDE_PX, y, w-BLOCK_TEXTURE_SIDE_PX*2, BLOCK_TEXTURE_SIDE_PX});
  
  SDL_RenderCopy(ctx->renderer, block_texture, &(SDL_Rect) {BLOCK_SIZE_PX-BLOCK_TEXTURE_SIDE_PX, 0, BLOCK_TEXTURE_SIDE_PX, BLOCK_TEXTURE_SIDE_PX}, &(SDL_Rect) {x+w-BLOCK_TEXTURE_SIDE_PX, y, BLOCK_TEXTURE_SIDE_PX, BLOCK_TEXTURE_SIDE_PX});
  SDL_SetRenderDrawColor(ctx->renderer, block_texture_colors[1], block_texture_colors[1], block_texture_colors[1], BLOCK_ALPHA_MOD);
  SDL_RenderFillRect(ctx->renderer, &(SDL_Rect) {x+w-BLOCK_TEXTURE_SIDE_PX, y+BLOCK_TEXTURE_SIDE_PX, BLOCK_TEXTURE_SIDE_PX, h-BLOCK_TEXTURE_SIDE_PX*2});
  
  SDL_RenderCopy(ctx->renderer, block_texture, &(SDL_Rect) {BLOCK_SIZE_PX-BLOCK_TEXTURE_SIDE_PX, BLOCK_SIZE_PX-BLOCK_TEXTURE_SIDE_PX, BLOCK_TEXTURE_SIDE_PX, BLOCK_TEXTURE_SIDE_PX}, &(SDL_Rect) {x+w-BLOCK_TEXTURE_SIDE_PX, y+h-BLOCK_TEXTURE_SIDE_PX, BLOCK_TEXTURE_SIDE_PX, BLOCK_TEXTURE_SIDE_PX});
  SDL_SetRenderDrawColor(ctx->renderer, block_texture_colors[2], block_texture_colors[2], block_texture_colors[2], BLOCK_ALPHA_MOD);
  SDL_RenderFillRect(ctx->renderer, &(SDL_Rect) {x+BLOCK_TEXTURE_SIDE_PX, y+h-BLOCK_TEXTURE_SIDE_PX, w-BLOCK_TEXTURE_SIDE_PX*2, BLOCK_TEXTURE_SIDE_PX});
  
  SDL_RenderCopy(ctx->renderer, block_texture, &(SDL_Rect) {0, BLOCK_SIZE_PX-BLOCK_TEXTURE_SIDE_PX, BLOCK_TEXTURE_SIDE_PX, BLOCK_TEXTURE_SIDE_PX}, &(SDL_Rect) {x, y+h-BLOCK_TEXTURE_SIDE_PX, BLOCK_TEXTURE_SIDE_PX, BLOCK_TEXTURE_SIDE_PX});
  SDL_SetRenderDrawColor(ctx->renderer, block_texture_colors[3], block_texture_colors[3], block_texture_colors[3], BLOCK_ALPHA_MOD);
  SDL_RenderFillRect(ctx->renderer, &(SDL_Rect) {x, y+BLOCK_TEXTURE_SIDE_PX, BLOCK_TEXTURE_SIDE_PX, h-BLOCK_TEXTURE_SIDE_PX*2});
  
  SDL_SetRenderDrawColor(ctx->renderer, block_texture_colors[4], block_texture_colors[4], block_texture_colors[4], BLOCK_ALPHA_MOD);
  SDL_RenderFillRect(ctx->renderer, &(SDL_Rect) {x+BLOCK_TEXTURE_SIDE_PX, y+BLOCK_TEXTURE_SIDE_PX, w-BLOCK_TEXTURE_SIDE_PX*2, h-BLOCK_TEXTURE_SIDE_PX*2});
  
  SDL_SetRenderDrawColor(ctx->renderer, color.r, color.g, color.b, 255);
  SDL_RenderDrawRect(ctx->renderer, &(SDL_Rect) {x+BLOCK_TEXTURE_SIDE_PX, y+BLOCK_TEXTURE_SIDE_PX, w-BLOCK_TEXTURE_SIDE_PX*2, h-BLOCK_TEXTURE_SIDE_PX*2});
  
  SDL_SetRenderDrawColor(ctx->renderer, block_texture_colors[5], block_texture_colors[5], block_texture_colors[5], BLOCK_ALPHA_MOD);
  SDL_RenderDrawRect(ctx->renderer, &(SDL_Rect) {x+BLOCK_TEXTURE_SIDE_PX, y+BLOCK_TEXTURE_SIDE_PX, w-BLOCK_TEXTURE_SIDE_PX*2, h-BLOCK_TEXTURE_SIDE_PX*2});
  }

bool button_frame(GameContext *ctx, int x, int y, int w, int h, int texture_id, int color_id, int mouse_position[2], bool just_clicked) {
  SDL_Color color = colors[color_id];
  bool retval = false;
  
  if (mouse_position[0] > x && mouse_position[0] < x+w) {
    if (mouse_position[1] > y && mouse_position[1] < y+h) {
      color = AdjustColorBrightness(color, 35);
      
      if (just_clicked) retval = true;
      }
    }
  
  draw_block_rect(ctx, x, y, w, h, color);
  
  SDL_Texture *texture = ctx->textures[texture_id];
  
  int texture_width, texture_height;
  if (SDL_QueryTexture(texture, NULL, NULL, &texture_width, &texture_height))
    handle_sdl_error();
  
  Blit(ctx->renderer, texture, x+(w/2-texture_width/2), y+(h/2-texture_height/2));
  
  return retval;
  }

void segment_display_frame(GameContext *ctx, int x, int y, int value, int digits) {
  int digit;
  int starting_x = x + digits * 22;
  int screen_x = starting_x;
  
  bool draw_zero = false;
  if (value == 0)
    draw_zero = true;
  
  while (screen_x >= x) {
    Blit(ctx->renderer, ctx->textures[TEXTURE_7SEGMENT_BG], screen_x, y);
    if (value) {
      digit = value % 10;
      Blit(ctx->renderer, ctx->textures[TEXTURE_7SEGMENT_0 + digit], screen_x, y);
      value /= 10;
      }
    screen_x -= 22;
    }
  
  if (draw_zero)
    Blit(ctx->renderer, ctx->textures[TEXTURE_7SEGMENT_0], starting_x, y);
  }

void generate_selection(GameContext *ctx);

void clear_board(uint8_t *board) {
  memset(board, 0, sizeof(board[0]) * BOARD_SIZE * BOARD_SIZE);
  }

void init(GameContext *ctx) {
  srand(time(NULL));
  
  ctx->window_size[WIDTH] = 1280;
  ctx->window_size[HEIGHT] = 720;
  
  /* Create the window and SDL renderer */
  ctx->window = SDL_CreateWindow("blocks", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, ctx->window_size[WIDTH], ctx->window_size[HEIGHT], 0);
  if (!ctx->window) handle_sdl_error();
  
  ctx->renderer = SDL_CreateRenderer(ctx->window, -1, SDL_RENDERER_ACCELERATED);
  if (!ctx->renderer) handle_sdl_error();
  
  SDL_SetRenderDrawBlendMode(ctx->renderer, SDL_BLENDMODE_BLEND);
  
  ctx->last = SDL_GetPerformanceCounter();
  
  /* Load textures */
  ctx->textures[TEXTURE_BLOCK]       = IMG_LoadTexture(ctx->renderer, "res/block.png");
  ctx->textures[TEXTURE_BLOCK_EMPTY] = IMG_LoadTexture(ctx->renderer, "res/block_empty.png");
  ctx->textures[TEXTURE_RESTART]     = IMG_LoadTexture(ctx->renderer, "res/restart_button.png");
  
  ctx->textures[TEXTURE_7SEGMENT_0] = IMG_LoadTexture(ctx->renderer, "res/7seg0.png");
  ctx->textures[TEXTURE_7SEGMENT_1] = IMG_LoadTexture(ctx->renderer, "res/7seg1.png");
  ctx->textures[TEXTURE_7SEGMENT_2] = IMG_LoadTexture(ctx->renderer, "res/7seg2.png");
  ctx->textures[TEXTURE_7SEGMENT_3] = IMG_LoadTexture(ctx->renderer, "res/7seg3.png");
  ctx->textures[TEXTURE_7SEGMENT_4] = IMG_LoadTexture(ctx->renderer, "res/7seg4.png");
  ctx->textures[TEXTURE_7SEGMENT_5] = IMG_LoadTexture(ctx->renderer, "res/7seg5.png");
  ctx->textures[TEXTURE_7SEGMENT_6] = IMG_LoadTexture(ctx->renderer, "res/7seg6.png");
  ctx->textures[TEXTURE_7SEGMENT_7] = IMG_LoadTexture(ctx->renderer, "res/7seg7.png");
  ctx->textures[TEXTURE_7SEGMENT_8] = IMG_LoadTexture(ctx->renderer, "res/7seg8.png");
  ctx->textures[TEXTURE_7SEGMENT_9] = IMG_LoadTexture(ctx->renderer, "res/7seg9.png");
  ctx->textures[TEXTURE_7SEGMENT_BG] = IMG_LoadTexture(ctx->renderer, "res/7segbg.png");
  ctx->textures[TEXTURE_7SEGMENT_MINUS] = IMG_LoadTexture(ctx->renderer, "res/7segminus.png");
  
  if (!ctx->textures[0]) handle_sdl_error();
  
  /* Clear the board */
  clear_board(ctx->board);
  
  generate_selection(ctx);
  
  /* no shape is currently being dragged */
  ctx->dragging_shape = NOT_DRAGGING;
  
  /* start in the main menu */
  ctx->state = GAME_MAIN_MENU;
  
  ctx->playing_state = PLAYING;
  }

void get_solved(uint8_t *board, bool *rows, bool *columns) {
  bool is_full;
  
  /* X */
  for (int row=0; row<BOARD_SIZE; row++) {
    is_full = true;
    
    for (int x=0; x<BOARD_SIZE; x++) {
      if (!board[row * BOARD_SIZE + x]) {
        is_full = false;
        break;
        }
      }
    
    if (is_full) rows[row] = true;
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
    
    if (is_full) columns[column] = true;
    }
  }

void clear_solved(uint8_t *board, int *cleared_x, int *cleared_y) {
  bool rows[BOARD_SIZE] = {false};
  bool columns[BOARD_SIZE] = {false};
  
  get_solved(board, rows, columns);
  
  for (int row=0; row<BOARD_SIZE; row++) {
    if (!rows[row]) continue;
    memset(board + row * BOARD_SIZE, 0, sizeof(uint8_t) * BOARD_SIZE);
    
    (*cleared_x) ++;
    }
  
  for (int column=0; column<BOARD_SIZE; column++) {
    if (!columns[column]) continue;
    for (int y=0; y<BOARD_SIZE; y++)
      board[y * BOARD_SIZE + column] = 0;
    
    (*cleared_y) ++;
    }
  }

void generate_selection(GameContext *ctx) {
  /* Generate selection */
  for (int i=0; i<SELECTION_SIZE; i ++) {
    ctx->selection[i] = shape_from_template(randint(0, NUM_TEMPLATES-1), randint(1, NUM_COLORS-1));
    }
  }

void draw_block(GameContext *ctx, int screen_x, int screen_y, SDL_Color color) {
  SDL_SetRenderDrawColor(ctx->renderer, color.r, color.g, color.b, 255);
  SDL_RenderFillRect(ctx->renderer, &(SDL_Rect) {screen_x, screen_y, BLOCK_SIZE_PX, BLOCK_SIZE_PX});
  
  SDL_Texture *texture = ctx->textures[TEXTURE_BLOCK];
  
  SDL_SetTextureAlphaMod(texture, BLOCK_ALPHA_MOD);
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

bool can_place_shape(uint8_t *board, Shape shape, int block_x, int block_y) {
  if (block_x < 0 || block_y < 0 || block_x + shape.width-1 > BOARD_SIZE || block_y + shape.height-1 > BOARD_SIZE)
    return false;
  
  for (int shape_x=0; shape_x < shape.width; shape_x ++) {
    for (int shape_y=0; shape_y < shape.height; shape_y ++) {
      if (shape.data[shape_y * shape.width + shape_x] != '1') continue;
      if (block_x + shape_x >= BOARD_SIZE || block_y + shape_y >= BOARD_SIZE) return false;
      
      int index = (block_y + shape_y) * BOARD_SIZE + (block_x + shape_x);
      if (board[index]) return false;
      }
    }
  
  return true;
  }

bool can_place_shape_anywhere(uint8_t *board, Shape shape) {
  for (int block_x = 0; block_x < BOARD_SIZE; block_x ++) {
    for (int block_y = 0; block_y < BOARD_SIZE; block_y ++) {
      if (can_place_shape(board, shape, block_x, block_y)) return true;
      }
    }
  
  return false;
  }

bool place_shape(uint8_t *board, Shape shape, int block_x, int block_y) {
  /* Check if the shape can be placed */
  if (!can_place_shape(board, shape, block_x, block_y)) return false;
  
  for (int shape_x=0; shape_x < shape.width; shape_x ++) {
    for (int shape_y=0; shape_y < shape.height; shape_y ++) {
      if (shape.data[shape_y * shape.width + shape_x] != '1') continue;
      
      int index = (block_y + shape_y) * BOARD_SIZE + (block_x + shape_x);
      board[index] = (uint8_t) shape.color;
      }
    }
  
  return true;
  }

void frame_playing(GameContext *ctx, int board_position[2], int mouse_position[2], uint32_t button_mask, bool just_clicked) {
  uint8_t predicted_board[BOARD_SIZE * BOARD_SIZE] = {0};
  memcpy(predicted_board, ctx->board, sizeof(uint8_t) * BOARD_SIZE * BOARD_SIZE);
  
  bool do_restart = button_frame(ctx,
    board_position[X] + BOARD_SIZE * BLOCK_SIZE_PX - (BLOCK_SIZE_PX * 2),
    board_position[Y] - BLOCK_SIZE_PX - 8,
    BLOCK_SIZE_PX * 2, BLOCK_SIZE_PX,
    TEXTURE_RESTART, 1,
    mouse_position,
    just_clicked);
  
  if (do_restart) {
    ctx->score = 0;
    generate_selection(ctx);
    clear_board(ctx->board);
    }
  
  segment_display_frame(ctx, board_position[X], board_position[Y] - 40, ctx->score, 4);
  
  bool can_place;
  int block_x, block_y;
  const Shape drag_shape = ctx->selection[ctx->dragging_shape];
  
  /* Place the dragged shape if the mouse is released and the shape is in bounds */
  {
    int screen_x = mouse_position[X] - drag_shape.width * BLOCK_SIZE_PX / 2;
    int screen_y = mouse_position[Y] - drag_shape.height * BLOCK_SIZE_PX - MOUSE_DRAG_PADDING;
    
    block_x = round((float) (screen_x - board_position[X]) / (float) BLOCK_SIZE_PX);
    block_y = round((float) (screen_y - board_position[Y]) / (float) BLOCK_SIZE_PX);
    
    can_place = place_shape(predicted_board, drag_shape, block_x, block_y);
    
    if (ctx->dragging_shape != NOT_DRAGGING && !(button_mask & SDL_BUTTON(1))) {
      int index = block_y * BOARD_SIZE + block_x;
      
      if (index < BOARD_SIZE * BOARD_SIZE) {
        if (place_shape(ctx->board, drag_shape, block_x, block_y)) {
          int cleared_x = 0;
          int cleared_y = 0;
          clear_solved(ctx->board, &cleared_x, &cleared_y);
          
          int score_x = cleared_x * BOARD_SIZE;
          int score_y = cleared_y * BOARD_SIZE;
          
          ctx->score += score_x + score_y + (score_x * score_y / 2);
          
          ctx->selection[ctx->dragging_shape].color = 0;
          }
        }
      
      ctx->dragging_shape = NOT_DRAGGING;
      }
    }
  
  if (ctx->playing_state == GAME_OVER_ANIMATION) {
    ctx->game_over_anim_timer -= ctx->dt;
    
    if (ctx->game_over_anim_timer <= 0) {
      ctx->game_over_anim_timer = 50;
      ctx->board[BOARD_SIZE * BOARD_SIZE - ctx->game_over_squares_left] = 1;
      ctx->game_over_squares_left --;
      
      if (ctx->game_over_squares_left <= 0) {
        ctx->playing_state = PLAYING;
        clear_board(ctx->board);
        generate_selection(ctx);
        ctx->score = 0;
        }
      }
    }
  
  /* Draw the board */
  {
    /* get the blocks that should be highlighted */
    bool rows[BOARD_SIZE] = {false};
    bool columns[BOARD_SIZE] = {false};
    
    if (ctx->playing_state != GAME_OVER_ANIMATION)
      get_solved(predicted_board, rows, columns);
    
    SDL_SetRenderDrawColor(ctx->renderer, 200, 200, 200, 255);
    SDL_RenderDrawRect(ctx->renderer, &(SDL_Rect) {board_position[X]-1, board_position[Y]-1, BOARD_SIZE * BLOCK_SIZE_PX+2, BOARD_SIZE * BLOCK_SIZE_PX+2});
    
    int screen_x, screen_y, x, y, index;
    bool highlight;
    
    for (int x=0; x<BOARD_SIZE; x ++) {
      for (int y=0; y<BOARD_SIZE; y ++) {
        screen_x = BLOCK_SIZE_PX * x + board_position[X];
        screen_y = BLOCK_SIZE_PX * y + board_position[Y];
        index = y * BOARD_SIZE + x;
        
        highlight = rows[y] || columns[x];
        
        if (can_place) {
          int x_rel = x - block_x;
          int y_rel = y - block_y;
          
          if (x_rel >= 0 && y_rel >= 0 && x_rel < drag_shape.width && y_rel < drag_shape.height) {
            if (drag_shape.data[y_rel * drag_shape.width + x_rel] == '1') highlight = true;
            }
          }
        
        if (highlight)
          draw_block(ctx, screen_x, screen_y, colors[0]);
        else if (ctx->board[index])
          draw_block(ctx, screen_x, screen_y, colors[ctx->board[index]]);
        else
          Blit(ctx->renderer, ctx->textures[TEXTURE_BLOCK_EMPTY], screen_x, screen_y);
        }
      }
    }
  
  /* Draw and update the selection area */
  {
    const int padding = 10;
    int center_x, screen_x, screen_y;
    Shape shape;
    int selection_width = 0;
    
    /* Calculate the total width of the selection area */
    for (int i=0; i<SELECTION_SIZE; i ++) {
      shape = ctx->selection[i];
      selection_width += shape.width * BLOCK_SIZE_PX + padding;
      }
    
    selection_width -= padding;
    screen_x = board_position[X] + BOARD_SIZE * BLOCK_SIZE_PX / 2 - selection_width / 2;
    
    /* Draw and update it */
    for (int i=0; i<SELECTION_SIZE; i ++) {
      shape = ctx->selection[i];
      
      if (shape.color == 0) goto skip;
      
      if (ctx->dragging_shape == i) {
        int drag_screen_x = mouse_position[X] - shape.width * BLOCK_SIZE_PX / 2;
        screen_y = mouse_position[Y] - shape.height * BLOCK_SIZE_PX - MOUSE_DRAG_PADDING;
        
        draw_shape(ctx, shape, drag_screen_x, screen_y);
        }
      else {
        screen_y = board_position[Y] + BOARD_SIZE * BLOCK_SIZE_PX + padding;
        
        draw_shape(ctx, shape, screen_x, screen_y);
        
        if (shape_is_hovered(shape, screen_x, screen_y, mouse_position[X], mouse_position[Y]) && just_clicked) {
          ctx->dragging_shape = i;
          }
        }
      
      skip:
      screen_x += shape.width * BLOCK_SIZE_PX + padding;
      }
    }
  
  /* Regenerate the selection when all the blocks are used and switch to game over state when there arent any moves left */
  {
    bool do_generate = true;
    
    Shape shape;
    for (int i=0; i<SELECTION_SIZE; i ++) {
      shape = ctx->selection[i];
      
      if (shape.color) {
        do_generate = false;
        break;
        }
      }
    
    if (do_generate)
      generate_selection(ctx);
    
    bool can_place_anything = false;
    for (int i=0; i<SELECTION_SIZE; i++) {
      shape = ctx->selection[i];
      if (!shape.color) continue;
      if (can_place_shape_anywhere(ctx->board, shape))
        can_place_anything = true;
      }
    
    if (!can_place_anything && ctx->playing_state != GAME_OVER_ANIMATION) {
      ctx->playing_state = GAME_OVER_ANIMATION;
      ctx->game_over_squares_left = BOARD_SIZE * BOARD_SIZE;
      ctx->game_over_anim_timer = 0;
      }
    }
  }

bool frame(GameContext *ctx) {
  /* Delta Time*/
  ctx->start_frame = SDL_GetPerformanceCounter();
  ctx->last = ctx->start_frame;
  ctx->dt = (float) ((ctx->start_frame - ctx->last) * 1000 / (float) SDL_GetPerformanceFrequency());
  
  /* ... */
  
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
  
  if (ctx->state == GAME_PLAYING) {
    frame_playing(ctx, board_position, mouse_position, button_mask, just_clicked);
    }
  else if (ctx->state == GAME_MAIN_MENU) ctx->state = GAME_PLAYING;
  
  SDL_RenderPresent(ctx->renderer);
  ctx->end_frame = SDL_GetPerformanceCounter();
  
  return false;
  }

void stop(GameContext *ctx) {
  SDL_DestroyRenderer(ctx->renderer);
  SDL_DestroyWindow(ctx->window);
  free(ctx);
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