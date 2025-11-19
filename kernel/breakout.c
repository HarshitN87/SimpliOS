#include "breakout.h"
#include "shell.h"
#include <stdint.h>

// External dependencies
extern volatile unsigned short* vga_buffer;
extern const int VGA_COLS;
extern const int VGA_ROWS;
extern void term_clear(void);
extern uint32_t pit_get_ticks(void);
extern void term_print(const char* str);
extern void shell_print_prompt(void);
extern void term_set_col(int col);
extern void term_set_row(int row);
extern void term_putc(char c);

// Global main loop hook from kernel.c
extern volatile void (*g_main_loop_hook)(void);

// Game Constants
#define PADDLE_WIDTH 10
#define PADDLE_CHAR 220 // Bottom half block
#define BALL_CHAR 'O'
#define BRICK_CHAR 178 // Medium shade
#define BRICK_ROWS 5
#define BRICK_COLS 10
#define BRICK_WIDTH 6
#define BRICK_HEIGHT 1
#define GAME_SPEED 8 // Higher is slower (ticks per frame)
#define PADDLE_SPEED 2 // Pixels per frame

// Colors
#define COLOR_PADDLE 0x0E // Yellow
#define COLOR_BALL 0x0F   // White
#define COLOR_BRICK 0x0C  // Light Red
#define COLOR_BG 0x00     // Black
#define COLOR_TEXT 0x0F   // White

// Game State
typedef struct {
    int x, y;
    int dx, dy;
} ball_t;

typedef struct {
    int x;
    int width;
} paddle_t;

typedef struct {
    int x, y;
    int active;
} brick_t;

static ball_t ball;
static paddle_t paddle;
static brick_t bricks[BRICK_ROWS][BRICK_COLS];
static int game_running;
static int score;
static int lives;
static int waiting_for_start;
static uint32_t last_tick = 0;

// Input State
static int move_left = 0;
static int move_right = 0;

// Input Handler
static void breakout_input_handler(char c, uint8_t arrow) {
    if (c == 'q') {
        game_running = 0;
    } else if (c == ' ') {
        waiting_for_start = 0;
    }
    
    if (arrow == 2) { // Left
        move_left = 1;
        move_right = 0;
    } else if (arrow == 3) { // Right
        move_right = 1;
        move_left = 0;
    } else {
        move_left = 0;
        move_right = 0;
    }
}

// Helper to draw a character at x,y with color
static void draw_char(int x, int y, char c, uint8_t color) {
    if (x >= 0 && x < VGA_COLS && y >= 0 && y < VGA_ROWS) {
        vga_buffer[y * VGA_COLS + x] = ((uint16_t)color << 8) | c;
    }
}

// Reset Ball Position
static void reset_ball(void) {
    ball.x = VGA_COLS / 2;
    ball.y = VGA_ROWS - 3;
    ball.dx = 1;
    ball.dy = -1;
    paddle.x = (VGA_COLS - paddle.width) / 2;
    waiting_for_start = 1;
}

// Initialize Game
static void init_game(void) {
    // Init Paddle
    paddle.width = PADDLE_WIDTH;
    
    // Init Bricks
    int start_x = (VGA_COLS - (BRICK_COLS * (BRICK_WIDTH + 1))) / 2;
    for (int r = 0; r < BRICK_ROWS; r++) {
        for (int c = 0; c < BRICK_COLS; c++) {
            bricks[r][c].x = start_x + c * (BRICK_WIDTH + 1);
            bricks[r][c].y = 2 + r * (BRICK_HEIGHT + 1);
            bricks[r][c].active = 1;
        }
    }
    
    score = 0;
    lives = 3;
    game_running = 1;
    move_left = 0;
    move_right = 0;
    
    reset_ball();
    last_tick = pit_get_ticks();
}

// Update Game Logic
static void update_game(void) {
    // Update Paddle
    if (move_left && paddle.x > 0) {
        paddle.x -= PADDLE_SPEED;
        if (paddle.x < 0) paddle.x = 0;
        move_left = 0; // Reset for single step per keypress
    }
    if (move_right && paddle.x < VGA_COLS - paddle.width) {
        paddle.x += PADDLE_SPEED;
        if (paddle.x > VGA_COLS - paddle.width) paddle.x = VGA_COLS - paddle.width;
        move_right = 0;
    }
    
    if (waiting_for_start) {
        // Keep ball on paddle if waiting
        ball.x = paddle.x + paddle.width / 2;
        ball.y = VGA_ROWS - 3;
        return;
    }
    
    // Update Ball
    ball.x += ball.dx;
    ball.y += ball.dy;
    
    // Wall Collisions
    if (ball.x <= 0 || ball.x >= VGA_COLS - 1) {
        ball.dx = -ball.dx;
    }
    if (ball.y <= 0) {
        ball.dy = -ball.dy;
    }
    
    // Paddle Collision
    if (ball.y == VGA_ROWS - 2) { // Paddle is at bottom - 1
        if (ball.x >= paddle.x && ball.x < paddle.x + paddle.width) {
            ball.dy = -ball.dy;
            // Optional: Change angle based on where it hit the paddle
        }
    }
    
    // Floor Collision (Life Lost)
    if (ball.y >= VGA_ROWS) {
        lives--;
        if (lives > 0) {
            reset_ball();
        } else {
            game_running = 0;
        }
    }
    
    // Brick Collision
    for (int r = 0; r < BRICK_ROWS; r++) {
        for (int c = 0; c < BRICK_COLS; c++) {
            if (bricks[r][c].active) {
                brick_t* b = &bricks[r][c];
                if (ball.x >= b->x && ball.x < b->x + BRICK_WIDTH &&
                    ball.y == b->y) {
                    bricks[r][c].active = 0;
                    ball.dy = -ball.dy;
                    score += 10;
                    return; // Handle one collision per frame
                }
            }
        }
    }
}

// Render Game
static void render_game(void) {
    term_clear();
    
    // Draw Status Bar
    term_set_row(0);
    term_set_col(0);
    term_print("Score: ");
    // Print score
    char num_str[12];
    int num = score;
    int pos = 0;
    if (num == 0) {
        num_str[pos++] = '0';
    } else {
        while (num > 0) {
            num_str[pos++] = '0' + (num % 10);
            num /= 10;
        }
    }
    for (int i = pos - 1; i >= 0; i--) {
        term_putc(num_str[i]);
    }
    
    term_print("  Lives: ");
    term_putc('0' + lives);
    
    if (waiting_for_start) {
        term_print("  PRESS SPACE TO START");
    }
    
    // Draw Bricks
    for (int r = 0; r < BRICK_ROWS; r++) {
        for (int c = 0; c < BRICK_COLS; c++) {
            if (bricks[r][c].active) {
                for (int i = 0; i < BRICK_WIDTH; i++) {
                    draw_char(bricks[r][c].x + i, bricks[r][c].y, BRICK_CHAR, COLOR_BRICK);
                }
            }
        }
    }
    
    // Draw Paddle
    for (int i = 0; i < paddle.width; i++) {
        draw_char(paddle.x + i, VGA_ROWS - 2, PADDLE_CHAR, COLOR_PADDLE);
    }
    
    // Draw Ball
    draw_char(ball.x, ball.y, BALL_CHAR, COLOR_BALL);
}

// Game Loop Hook
static void breakout_game_step(void) {
    if (!game_running) {
        // Cleanup
        g_main_loop_hook = 0;
        shell_set_input_handler(0);
        term_clear();
        term_print("Game Over! Final Score: ");
        
        // Print score
        char num_str[12];
        int num = score;
        int pos = 0;
        if (num == 0) {
            num_str[pos++] = '0';
        } else {
            while (num > 0) {
                num_str[pos++] = '0' + (num % 10);
                num /= 10;
            }
        }
        for (int i = pos - 1; i >= 0; i--) {
            term_putc(num_str[i]);
        }
        term_print("\n");
        
        shell_print_prompt();
        return;
    }

    uint32_t current_tick = pit_get_ticks();
    if (current_tick - last_tick >= GAME_SPEED) {
        update_game();
        render_game();
        last_tick = current_tick;
    }
}

// Main Game Command
int cmd_breakout(int argc, char* argv[]) {
    (void)argc; (void)argv;
    
    term_clear();
    term_print("Starting Breakout... Press 'q' to quit.\n");
    
    init_game();
    shell_set_input_handler(breakout_input_handler);
    
    // Hook into the main loop
    g_main_loop_hook = breakout_game_step;
    
    return 0;
}
