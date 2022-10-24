/*
 * project.c
 *
 * Main file
 *
 * Authors: Peter Sutton, Luke Kamols, Jarrod Bennett
 * Modified by <Arjun Srikanth>
 */ 


#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#define F_CPU 8000000UL
#include <util/delay.h>

#include "game.h"
#include "display.h"
#include "ledmatrix.h"
#include "buttons.h"
#include "serialio.h"
#include "terminalio.h"
#include "timer0.h"

// Function prototypes - these are defined below (after main()) in the order
// given here
void initialise_hardware(void);
void start_screen(void);
void new_game(void);
void play_game(void);
void two_play_game(void);
void handle_game_over(void);

// Check if player has moved (for player flash implementation)
bool player_moved = false;
uint8_t moves = 0;

// Dice value and start roll
uint8_t dice_value = 0;
bool start_roll = false;

// Two player game - true/false
bool two_player_game = false;

/////////////////////////////// main //////////////////////////////////
int main(void) {
	// Setup hardware and call backs. This will turn on 
	// interrupts.
	initialise_hardware();
	
	// Show the splash screen message. Returns when display
	// is complete.
	two_player_game = false;
	start_screen();
	
	// Loop forever and continuously play the game.
	while(1) {
		new_game();
		if (two_player_game)
		{
			two_play_game();
		}else {
			play_game();
		}
		handle_game_over();
	}
}

void initialise_hardware(void) {
	ledmatrix_setup();
	init_button_interrupts();
	// Setup serial port for 19200 baud communication with no echo
	// of incoming characters
	init_serial_stdio(19200,0);
	
	init_timer0();
	
	// Turn on global interrupts
	sei();
}

void start_screen(void) {
	// Clear terminal screen and output a message
	clear_terminal();
	move_terminal_cursor(10,10);
	printf_P(PSTR("Snakes and Ladders"));
	move_terminal_cursor(10,12);
	printf_P(PSTR("CSSE2010/7201 A2 by ARJUN SRIKANTH - 46915474"));

	// Select One player or two player game	
	move_terminal_cursor(10,14);
	printf_P(PSTR("Press '1' or 's'/'S' for single player"));

	move_terminal_cursor(10,16);
	printf_P(PSTR("Press '2' for two player"));
	// Output the static start screen and wait for a push button 
	// to be pushed or a serial input of 's'
	start_display();
	
	// Wait until a button is pressed, or 's' is pressed on the terminal
	while(1) {
		// First check for if a 's' is pressed
		// There are two steps to this
		// 1) collect any serial input (if available)
		// 2) check if the input is equal to the character 's'
		seven_seg_display(moves, dice_value); // Display 00 during start_screen

		char serial_input = -1;
		if (serial_input_available()) {
			serial_input = fgetc(stdin);
		}
		// If the serial input is 's', then exit the start screen
		// Start a single player game
		if (serial_input == 's' || serial_input == 'S' || serial_input == '1') {
			break;
		}

		// If the serial input is '2', then start a two player game
		if (serial_input == '2') {
			two_player_game = true;
			break;
		}
		// Next check for any button presses
		int8_t btn = button_pushed();
		if (btn != NO_BUTTON_PUSHED) {
			break;
		}
	}
}

void new_game(void) {
	// Clear the serial terminal
	clear_terminal();
	
	// Initialise the game and display
	initialise_game(two_player_game);
	
	// Clear a button push or serial input if any are waiting
	// (The cast to void means the return value is ignored.)
	(void)button_pushed();
	clear_serial_input_buffer();
}

void play_game(void) {
	
	uint32_t last_flash_time, current_time;
	uint32_t last_roll_time; // Last time roll_dice() was called
	uint8_t btn; // The button pushed
	
	last_flash_time = get_current_time();
	last_roll_time = get_current_time();
	
	moves = 0;
	dice_value = 0;

	// We play the game until it's over
	while(!is_game_over()) {
				
		// We need to check if any button has been pushed, this will be
		// NO_BUTTON_PUSHED if no button has been pushed
		btn = button_pushed();
		
		if ((btn == BUTTON0_PUSHED) & !start_roll) {
			// If button 0 is pushed, move the player 1 space forward
			// YOU WILL NEED TO IMPLEMENT THIS FUNCTION
			move_player_n(1, true);
			moves+=1;
		}

		if ((btn == BUTTON1_PUSHED) & !start_roll) {
			move_player_n(2, true);
			moves+=1;
		}

		char serial_input = -1;
		if (serial_input_available()) {
			serial_input = fgetc(stdin);
		}

		if ((serial_input == 's' || serial_input == 'S') & !start_roll) {
			move_player(0, -1, true);
			player_moved = true;
		}

		if ((serial_input == 'w' || serial_input == 'W') & !start_roll) {
			move_player(0, 1, true);
			player_moved = true;
		}

		if ((serial_input == 'd' || serial_input == 'D') & !start_roll) {
			move_player(1, 0, true);
			player_moved = true;
		}

		if ((serial_input == 'a' || serial_input == 'A') & !start_roll) {
			move_player(-1, 0, true);
			player_moved = true;
		}

		current_time = get_current_time();

		if (((btn == BUTTON2_PUSHED)|(serial_input == 'r' || serial_input == 'R')) & !start_roll) {
			start_roll = true;
			printf_P(PSTR("Dice Rolling..."));
		}else if (((btn == BUTTON2_PUSHED)|(serial_input == 'r' || serial_input == 'R')) & start_roll) {
			start_roll = false;
			printf_P(PSTR("Dice Stoppped"));
			move_player_n(dice_value, true);
			moves += 1;
		}

		if (start_roll) {
			if (current_time >= last_roll_time + 100) {
				dice_value = roll_dice();
				last_roll_time = current_time;
			}
		}

		// Hold player flash for 500ms after movement
		if (player_moved) {
			current_time = 0;
		}
		if (current_time >= last_flash_time + 500) {
			// 500ms (0.5 second) has passed since the last time we
			// flashed the cursor, so flash the cursor
			flash_player_cursor();
			
			// Update the most recent time the cursor was flashed
			last_flash_time = current_time;
		}

		player_moved = false;

		snake_ladder_func(true);
		seven_seg_display(moves, dice_value);
	}
	// We get here if the game is over.
	handle_game_over();
}

void two_play_game(void) {
	uint32_t last_flash_time, current_time;
	uint32_t last_roll_time; // Last time roll_dice() was called
	uint8_t btn; // The button pushed
	
	last_flash_time = get_current_time();
	last_roll_time = get_current_time();
	
	// True = move player 1, False = move player 2
	bool move_player_1 = true;

	// Count moves separately
	uint8_t player_1_moves = 0;
	uint8_t player_2_moves = 0;
	
	// moves = player_1_moves or player_2_moves
	moves = player_1_moves;
	dice_value = 0;
	
	while(!is_game_over()) {
	
		// We need to check if any button has been pushed, this will be
		// NO_BUTTON_PUSHED if no button has been pushed
		btn = button_pushed();
		
		if ((btn == BUTTON0_PUSHED) & !start_roll) {
			// If button 0 is pushed, move the player 1 space forward
			// YOU WILL NEED TO IMPLEMENT THIS FUNCTION
			move_player_n(1, move_player_1);
			move_player_1 = !move_player_1;

			if (move_player_1) {
				player_1_moves += 1;
				moves = player_1_moves;
			} else {
				player_2_moves += 1;
				moves = player_2_moves;
			}
		}

		if ((btn == BUTTON1_PUSHED) & !start_roll) {
			move_player_n(2, move_player_1);
			move_player_1 = !move_player_1;

			if (move_player_1) {
				player_1_moves += 1;
				moves = player_1_moves;
			} else {
				player_2_moves += 1;
				moves = player_2_moves;
			}

		}

		char serial_input = -1;
		if (serial_input_available()) {
			serial_input = fgetc(stdin);
		}

		if ((serial_input == 's' || serial_input == 'S') & !start_roll) {
			move_player(0, -1, move_player_1);
			player_moved = true;
		}

		if ((serial_input == 'w' || serial_input == 'W') & !start_roll) {
			move_player(0, 1, move_player_1);
			player_moved = true;
		}

		if ((serial_input == 'd' || serial_input == 'D') & !start_roll) {
			move_player(1, 0, move_player_1);
			player_moved = true;
		}

		if ((serial_input == 'a' || serial_input == 'A') & !start_roll) {
			move_player(-1, 0, move_player_1);
			player_moved = true;
		}

		current_time = get_current_time();

		if (((btn == BUTTON2_PUSHED)|(serial_input == 'r' || serial_input == 'R')) & !start_roll) {
			start_roll = true;
			printf_P(PSTR("Dice Rolling..."));
		}else if (((btn == BUTTON2_PUSHED)|(serial_input == 'r' || serial_input == 'R')) & start_roll) {
			start_roll = false;
			printf_P(PSTR("Dice Stoppped"));
			move_player_n(dice_value, move_player_1);
			move_player_1 = !move_player_1;
			if (move_player_1) {
				player_1_moves += 1;
				moves = player_1_moves;
			} else {
				player_2_moves += 1;
				moves = player_2_moves;
			}
		}

		if (start_roll) {
			if (current_time >= last_roll_time + 100) {
				dice_value = roll_dice();
				last_roll_time = current_time;
			}
		}

		// Hold player flash for 500ms after movement
		if (player_moved) {
			current_time = 0;
		}
		if (current_time >= last_flash_time + 500) {
			// 500ms (0.5 second) has passed since the last time we
			// flashed the cursor, so flash the cursor
			flash_player_cursor();
			
			// Update the most recent time the cursor was flashed
			last_flash_time = current_time;
		}

		player_moved = false;

		snake_ladder_func(move_player_1);
		// Shows previous player's no. of moves
		seven_seg_display(moves, dice_value);
	}
	// We get here if the game is over.
	handle_game_over();
}

void handle_game_over() {
	clear_terminal();
	move_terminal_cursor(10,14);
	printf_P(PSTR("GAME OVER"));
	move_terminal_cursor(10,15);
	if (is_game_over() == 1) {
		printf_P(PSTR("Player 1 Wins!!"));
	} else {
		printf_P(PSTR("Player 2 Wins!!"));
	}
	move_terminal_cursor(10,16);
	printf_P(PSTR("Press a button to start again"));
	
	while(button_pushed() == NO_BUTTON_PUSHED && !serial_input_available()) {
		; // wait
	}

	char serial_input = -1;
	if (serial_input_available()) {
		serial_input = fgetc(stdin);
	}
	// if the serial input is 's' or 'S', then exit the start screen
	if ((serial_input == 's' || serial_input == 'S') || button_pushed != NO_BUTTON_PUSHED ) {
		main();
	}
	
}
