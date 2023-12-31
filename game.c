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
#include <stdbool.h>
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

static const uint8_t custom_layout[HEIGHT][WIDTH] = 
{
	{FINISH_LINE, SNAKE_START | 5, 0, 0, 0, LADDER_END | 4, 0, 0},
	{0, SNAKE_MIDDLE, 0, 0, LADDER_MIDDLE, 0, 0, 0},
	{0, SNAKE_MIDDLE, 0, LADDER_MIDDLE, 0, 0, 0, 0},
	{0, SNAKE_MIDDLE, LADDER_START | 4, SNAKE_START | 4, 0, 0, 0, 0},
	{0, SNAKE_END | 5, 0, SNAKE_MIDDLE, LADDER_END | 3, 0, 0, 0},
	{0, 0, 0, SNAKE_END | 4, 0, LADDER_MIDDLE, 0, 0},
	{0, 0, 0, 0, 0, 0, LADDER_START | 3, 0},
	{0, 0, 0, 0, SNAKE_START | 3, 0, 0, 0},
	{0, SNAKE_START | 2, 0, 0, 0, SNAKE_MIDDLE, 0, 0},
	{0, SNAKE_END | 2, 0, 0, 0, 0, SNAKE_END | 3, LADDER_END | 2},
	{0, 0, 0, 0, 0, 0, 0, LADDER_MIDDLE},
	{0, SNAKE_START | 1, 0, 0, 0, 0, 0, LADDER_START | 2},
	{0, LADDER_END | 1, SNAKE_MIDDLE, 0, 0, 0, 0, 0},
	{0, LADDER_MIDDLE, 0, SNAKE_MIDDLE, 0, 0, 0, 0},
	{0, LADDER_MIDDLE, 0, 0, SNAKE_MIDDLE, 0, 0, 0},
	{START_POINT, LADDER_START | 1, 0, 0, 0, SNAKE_END | 1, 0, 0}
};


// The player(s) is not stored in the board itself to avoid overwriting game
// elements when the player is moved.
int8_t player_1_x;
int8_t player_1_y;
int8_t player_2_x;
int8_t player_2_y;

// For flashing the player icon
uint8_t player_visible;

// For flashing the player 2 icon
uint8_t player_2_visible;

// Added by student (Arjun Srikanth)
// true if player moved up a row, false if not
bool p1_moved_up = false;
bool p2_moved_up = false;

void initialise_game(bool two_player_game, uint8_t board_number) {
	
	// initialise the display we are using.
	initialise_display();
		
	// start the player icon at the bottom left of the display
	// NOTE: (for INternal students) the LED matrix uses a different coordinate
	// system
	player_1_x = 0;
	player_1_y = 0;
	player_2_x = 0;
	player_2_y = 0;


	player_visible = 0;

	// go through and initialise the state of the playing_field
	if (board_number == 1) {
		for (int x = 0; x < WIDTH; x++) {
			for (int y = 0; y < HEIGHT; y++) {
				// initialise this square based on the starting layout
				// the indices here are to ensure the starting layout
				// could be easily visualised when declared
				board[x][y] = starting_layout[HEIGHT - 1 - y][x];
				update_square_colour(x, y, get_object_type(board[x][y]));
			}
		}
	} else if(board_number == 2) {
		for (int x = 0; x < WIDTH; x++) {
			for (int y = 0; y < HEIGHT; y++) {
				// initialise this square based on the starting layout
				// the indices here are to ensure the starting layout
				// could be easily visualised when declared
				board[x][y] = custom_layout[HEIGHT - 1 - y][x];
				update_square_colour(x, y, get_object_type(board[x][y]));
			}
		}
	}
	
	update_square_colour(player_1_x, player_1_y, PLAYER_1);
	if (two_player_game) {
		player_2_visible = 0;
		update_square_colour(player_2_x, player_2_y, PLAYER_2);
	}
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
void move_player_n(uint8_t num_spaces, bool move_player_1) {
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
	int8_t prev_player_x, prev_player_y;
	int8_t player_x, player_y;
	if (move_player_1) {
		prev_player_x = player_1_x;
		prev_player_y = player_1_y;
		player_x = player_1_x;
		player_y = player_1_y;
	}
	else {
		prev_player_x = player_2_x;
		prev_player_y = player_2_y;
		player_x = player_2_x;
		player_y = player_2_y;
	}

	int8_t diff = player_x - num_spaces;
	// Boolean values to check player's position
	
	//	Player is at the end of the row (right side)
	bool player_end_right = (player_x == WIDTH-1);
	
	//	Player is at the end of the row (left side)
	bool player_end_left = ((player_x==0) & (player_y%2));

	if(( player_end_right | player_end_left) & ((move_player_1 && !p1_moved_up) || (!move_player_1 && !p2_moved_up))) {
		if(move_player_1) {
			p1_moved_up = true;
		} else {
			p2_moved_up = true;
		}
		player_y += 1;

		if (player_end_left) {
			player_x += num_spaces-1;
		} else {
			player_x -= num_spaces-1;
		}
	}else if(player_y%2) {
		if (move_player_1) {
			p1_moved_up = false;
		} else {
			p2_moved_up = false;
		}

		// Make sure the player doesn't go past the end point
		if ((diff < 0) & (player_y == HEIGHT-1)) {
			player_x = 0;
		} else if (diff < 0) {
			int8_t remaining_steps = num_spaces - (player_x+1);
			int8_t dx = player_x - remaining_steps;
			player_x -= dx;
			player_y += 1;
			if (move_player_1) {
				p1_moved_up = true;
			} else {
				p2_moved_up = true;
			}

		} else {
			player_x -= num_spaces;
		}
	}else{
		if (move_player_1) {
			p1_moved_up = false;
		} else {
			p2_moved_up = false;
		}

		if(player_x + num_spaces > WIDTH-1) {
			// Steps to end of row
			int8_t x_steps = WIDTH - player_x;

			// remaining steps from x += x_steps, y += 1
			int8_t remaining_steps = num_spaces - (x_steps - 1);
			
			// number of x_steps player needs to take to reach new point
			int8_t dx = x_steps - remaining_steps;

			player_x += dx;
			player_y += 1;
			if (move_player_1) {
				p1_moved_up = true;
			} else {
				p2_moved_up = true;
			}

		}else {
			player_x += num_spaces;
		}
	}

	// Check if player is at end tile
	if((prev_player_x == 0) & (prev_player_y == HEIGHT-1)) {
		player_x = prev_player_x;
		player_y = prev_player_y;
	}
	
	update_square_colour(prev_player_x, prev_player_y, get_object_at(prev_player_x, prev_player_y));
	if (move_player_1){
		player_1_x = player_x;
		player_1_y = player_y;
		update_square_colour(player_1_x, player_1_y, PLAYER_1);

	} else {
		player_2_x = player_x;
		player_2_y = player_y;
		update_square_colour(player_2_x, player_2_y, PLAYER_2);
	}
}

// Move the player one space in the direction (dx, dy). The player should wrap
// around the display if moved 'off' the display.
void move_player(int8_t dx, int8_t dy, bool move_player_1) {
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
	int8_t prev_player_x, prev_player_y;
	int8_t player_x, player_y;
	
	if (move_player_1) {
		prev_player_x = player_1_x;
		prev_player_y = player_1_y;
		player_x = player_1_x;
		player_y = player_1_y;
	} else {
		prev_player_x = player_2_x;
		prev_player_y = player_2_y;
		player_x = player_2_x;
		player_y = player_2_y;
	}

	if (player_x + dx >= WIDTH) {
		player_x = 0;
	} else if (player_y + dy >= HEIGHT) {
		player_y = 0;
	} else if (player_x + dx < 0) {
		player_x = WIDTH - 1;
	} else if (player_y + dy < 0) {
		player_y = HEIGHT - 1;
	} else {
		player_x += dx;
		player_y += dy;
	}

	if (move_player_1) {
		player_1_x = player_x;
		player_1_y = player_y;
		update_square_colour(player_1_x, player_1_y, PLAYER_1);
	} else {
		player_2_x = player_x;
		player_2_y = player_y;
		update_square_colour(player_2_x, player_2_y, PLAYER_2);
	}
	update_square_colour(prev_player_x, prev_player_y, get_object_at(prev_player_x, prev_player_y));	
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

void flash_player_2_cursor(void) {
	if (player_2_visible) {
		// we need to flash the player off, it should be replaced by
		// the colour of the object which is at that location
		uint8_t object_at_cursor = get_object_at(player_2_x, player_2_y);
		update_square_colour(player_2_x, player_2_y, object_at_cursor);
	} else {
		// we need to flash the player on
		update_square_colour(player_2_x, player_2_y, PLAYER_2);
	}
	player_2_visible = 1 - player_2_visible; //alternate between 0 and 1

}
// Returns 1 if the game is over, 0 otherwise.
uint8_t is_game_over(void) {
	// YOUR CODE HERE
	uint8_t object = get_object_at(player_1_x, player_1_y);
	// Detect if the game is over i.e. if player 1 has won.
	if (get_object_type(object) == FINISH_LINE) {
		return 1;
	}
	// Check if player 2 has won
	if (player_2_x >= 0 && player_2_y >= 0) {
		object = get_object_at(player_2_x, player_2_y);
		if (get_object_type(object) == FINISH_LINE) {
			return 2;
		}
	
	}
	return 0;
}

// Simulates dice roll by generating random value from 1-6 (inclusive)
uint8_t roll_dice(void) {
	uint8_t dice_value = rand() % 6; // 0 to 5
	return dice_value + 1; // 1 to 6 (inclusive)
}

// Moves player to end of snake/ladder (if player is at the start of the snake/ladder)
void snake_ladder_func(bool move_player_1){
	// Get the object at the player's position
	int8_t player_x, player_y;
	
	if (move_player_1) {
		player_x = player_1_x;
		player_y = player_1_y;
	} else {
		player_x = player_2_x;
		player_y = player_2_y;
	}

	uint8_t object = get_object_at(player_x, player_y);
	uint8_t object_type = get_object_type(object);
	uint8_t object_identifier = get_object_identifier(object);
	
	uint8_t tempObject;

	// Check if the object is either a SNAKE_START or LADDER_START
	if (object_type == SNAKE_START) {

		// Loop through board to find the corresponding snake end
		for (int i = 0; i < WIDTH; i++) {
			for (int j = 0; j < HEIGHT; j++) {
				tempObject = get_object_at(i, j);
				
				// Move player to SNAKE_END
				if (get_object_type(tempObject) == SNAKE_END && object_identifier == get_object_identifier(tempObject)) {
					move_player(i-player_x, j-player_y, move_player_1);
				}				
			}
		}
	} else if (object_type == LADDER_START) {

		// Loop through board to find the corresponding ladder end
		for (int i = 0; i < WIDTH; i++) {
			for (int j = 0; j < HEIGHT; j++) {
				tempObject = get_object_at(i, j);

				// Move player to LADDER_END
				if (get_object_type(tempObject) == LADDER_END  && object_identifier == get_object_identifier(tempObject)) {
					move_player(i-player_x, j-player_y, move_player_1);
				}				
			}
		}
	}
}