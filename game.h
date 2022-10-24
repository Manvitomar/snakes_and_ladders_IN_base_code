/*
 * game.h
 *
 * Author: Jarrod Bennett
 *
 * Function prototypes for game functions available externally. You may wish
 * to add extra function prototypes here to make other functions available to
 * other files.
 */


#ifndef GAME_H_
#define GAME_H_

#include <stdint.h>
#include <stdbool.h>

// Game board dimensions
#define WIDTH  8
#define HEIGHT 16

// Game objects. Note upper 4 bits indicate type, lower 4 bits indicate the
// identifier number (if applicable)
#define EMPTY_SQUARE    ((uint8_t) 0x00)
#define START_POINT		((uint8_t) 0x10)
#define FINISH_LINE		((uint8_t) 0x20)
#define PLAYER_1		((uint8_t) 0x40)
#define PLAYER_2        ((uint8_t) 0x50)

// Snakes and ladders are represented by a start and end, which must share a
// common identifier to generate the link. A third type is used to indicate
// the 'tunnel' that connects the start and end.
#define SNAKE_START		((uint8_t) 0x80)
#define SNAKE_END		((uint8_t) 0x90)
#define SNAKE_MIDDLE	((uint8_t) 0xA0)

#define LADDER_START	((uint8_t) 0xC0)
#define LADDER_END		((uint8_t) 0xD0)
#define LADDER_MIDDLE	((uint8_t) 0xE0)

// Initialise the display of the board. This creates the internal board
// and also updates the display to show the initialised board.
void initialise_game(bool two_player_game);

// Return the game object at the specified position (x, y). This function does
// not consider the position of the player token since it is not stored on the
// game board.
uint8_t get_object_at(uint8_t x, uint8_t y);

// Extract the object type of a game element.
uint8_t get_object_type(uint8_t object);

// Move the player by the given number of spaces forward.
void move_player_n(uint8_t num_spaces, bool move_player_1);

// Move the player one space in the direction (dx, dy). The player should wrap
// around the display if moved 'off' the display.
void move_player(int8_t dx, int8_t dy, bool move_player_1);

// Flash the player icon on and off. This should be called at a regular
// interval (see where this is called in project.c) to create a consistent
// 500 ms flash.
void flash_player_cursor(void);

// Flash the player 2 icon on and off. This should be called at a regular
// interval (called in project.c two_play_game) to create a consistent
// 500 ms flash
void flash_player_2_cursor(void);

// Returns 1 if the game is over, 0 otherwise.
uint8_t is_game_over(void);

// Simulates dice roll by generating random value from 1-6 (inclusive)
uint8_t roll_dice(void);


// Moves player to end of snake/ladder (if player is at the start of the snake/ladder)
void snake_ladder_func(bool move_player_1);

#endif

