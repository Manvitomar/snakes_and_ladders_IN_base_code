/*
 * game.c
 *
 * Functionality related to the game state and features.
 *
 * Author: Jarrod Bennett
 */ 


#include "game.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include "display.h"
#include "terminalio.h"

uint8_t board[WIDTH][HEIGHT];

// The initial game layout. Note that this is laid out in such a way that
// starting_layout[x][y] does not correspond to an (x,y) coordinate but is a
// better visual representation (but still somewhat messy).
// In our reference system, (0,0) is the bottom left, but (0,0) in this array
// is the top left.
static const uint8_t starting_layout[HEIGHT][WIDTH] =
{
	{FINISH_LINE, 0, 0, 0, 0, 0, 0, 0},
	{0, SNAKE_START | 4, 0, 0, LADDER_END | 4, 0, 0, 0},
	{0, SNAKE_MIDDLE, 0, LADDER_MIDDLE, 0, 0, 0, 0},
	{0, SNAKE_MIDDLE, LADDER_START | 4, 0, 0, 0, 0, 0},
	{0, SNAKE_END | 4, 0, 0, 0, 0, SNAKE_START | 3, 0},
	{0, 0, 0, 0, LADDER_END | 3, 0, SNAKE_MIDDLE, 0},
	{SNAKE_START | 2, 0, 0, 0, LADDER_MIDDLE, 0, SNAKE_MIDDLE, 0},
	{0, SNAKE_MIDDLE, 0, 0, LADDER_START | 3, 0, SNAKE_END | 3, 0},
	{0, 0, SNAKE_END | 2, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, SNAKE_START | 1, 0, 0, 0, LADDER_END | 1},
	{0, LADDER_END | 2, 0, SNAKE_MIDDLE, 0, 0, LADDER_MIDDLE, 0},
	{0, LADDER_MIDDLE, 0, SNAKE_MIDDLE, 0, LADDER_START | 1, 0, 0},
	{0, LADDER_START | 2, 0, SNAKE_MIDDLE, 0, 0, 0, 0},
	{START_POINT, 0, 0, SNAKE_END | 1, 0, 0, 0, 0}
};

// The player is not stored in the board itself to avoid overwriting game
// elements when the player is moved.
int8_t player_1_x;
int8_t player_1_y;

// For flashing the player icon
uint8_t player_visible;

// Added by student (Arjun Srikanth)
// 1 if player moved up a row, 0 if not
int8_t moved_up = 0;

void initialise_game(void) {
	
	// initialise the display we are using.
	initialise_display();
		
	// start the player icon at the bottom left of the display
	// NOTE: (for INternal students) the LED matrix uses a different coordinate
	// system
	player_1_x = 0;
	player_1_y = 0;
	
	player_visible = 0;

	// go through and initialise the state of the playing_field
	for (int x = 0; x < WIDTH; x++) {
		for (int y = 0; y < HEIGHT; y++) {
			// initialise this square based on the starting layout
			// the indices here are to ensure the starting layout
			// could be easily visualised when declared
			board[x][y] = starting_layout[HEIGHT - 1 - y][x];
			update_square_colour(x, y, get_object_type(board[x][y]));
		}
	}
	
	update_square_colour(player_1_x, player_1_y, PLAYER_1);
}

// Return the game object at the specified position (x, y). This function does
// not consider the position of the player token since it is not stored on the
// game board.
uint8_t get_object_at(uint8_t x, uint8_t y) {
	// check the bounds, anything outside the bounds
	// will be considered empty
	if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT) {
		return EMPTY_SQUARE;
	} else {
		//if in the bounds, just index into the array
		return board[x][y];
	}
}

// Extract the object type of a game element (the upper 4 bits).
uint8_t get_object_type(uint8_t object) {
	return object & 0xF0;
}

// Get the identifier of a game object (the lower 4 bits). Not all objects
// have an identifier, in which case 0 will be returned.
uint8_t get_object_identifier(uint8_t object) {
	return object & 0x0F;
}

// Move the player by the given number of spaces forward.
void move_player_n(uint8_t num_spaces) {
	/* suggestions for implementation:
	 * 1: remove the display of the player at the current location
	 *		(and replace it with whatever object is at that location).
	 * 2: update the positional knowledge of the player, this will include
	 *		variables player_1_x and player_1_y since you will need to consider
	 *		what should happen if the player reaches the end of the current
	 *		row. You will need to consider which direction the player should
	 *		move based on the current row. A loop can be useful to move the
	 *		player one space at a time but is not essential.
	 * 3: display the player at the new location
	 *
	 * FOR "Flashing Player Icon"
	 * 4: reset the player cursor flashing cycle. See project.c for how the
	 *		cursor is flashed.
	 */
	// YOUR CODE HERE
	int8_t prev_player_x = player_1_x;
	int8_t prev_player_y = player_1_y;
	if(((player_1_x == 7) | ((player_1_x==0) & (player_1_y%2))) & (moved_up == 0)) {
		moved_up = 1;
		player_1_y += 1;
	}else if(player_1_y%2) {
		moved_up = 0;
		if (player_1_x - num_spaces < 0) {
			player_1_x -=1;
		}else {
			player_1_x -= num_spaces;
		}
	}else{
		moved_up = 0;
		if(player_1_x + num_spaces > 7) {
			player_1_x += 1;
		}else {
			player_1_x += num_spaces;
		}
	}
	update_square_colour(prev_player_x, prev_player_y, get_object_at(prev_player_x, prev_player_y));
	update_square_colour(player_1_x, player_1_y, PLAYER_1);
}

// Move the player one space in the direction (dx, dy). The player should wrap
// around the display if moved 'off' the display.
void move_player(int8_t dx, int8_t dy) {
	/* suggestions for implementation:
	 * 1: remove the display of the player at the current location
	 *		(and replace it with whatever object is at that location).
	 * 2: update the positional knowledge of the player, this will include
	 *		variables player_1_x and player_1_y and cursor_visible. Make sure
	 *		you consider what should happen if the player moves off the board.
	 *		(The player should wrap around to the current row/column.)
	 * 3: display the player at the new location
	 *
	 * FOR "Flashing Player Icon"
	 * 4: reset the player cursor flashing cycle. See project.c for how the
	 *		cursor is flashed.
	 */	
	// YOUR CODE HERE

}

// Flash the player icon on and off. This should be called at a regular
// interval (see where this is called in project.c) to create a consistent
// 500 ms flash.
void flash_player_cursor(void) {
	
	if (player_visible) {
		// we need to flash the player off, it should be replaced by
		// the colour of the object which is at that location
		uint8_t object_at_cursor = get_object_at(player_1_x, player_1_y);
		update_square_colour(player_1_x, player_1_y, object_at_cursor);
		
	} else {
		// we need to flash the player on
		update_square_colour(player_1_x, player_1_y, PLAYER_1);
	}
	player_visible = 1 - player_visible; //alternate between 0 and 1
}

// Returns 1 if the game is over, 0 otherwise.
uint8_t is_game_over(void) {
	// YOUR CODE HERE
	// Detect if the game is over i.e. if a player has won.
	return 0;
}
