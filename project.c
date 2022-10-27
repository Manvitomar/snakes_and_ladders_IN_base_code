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
void terminal_start_screen(void);
void start_screen(void);
void new_game(void);
void play_game(void);
void two_play_game(void);
void game_pause(void);
void handle_game_over(void);

// Check if player has moved (for player flash implementation)
bool player_moved = false;
uint8_t moves = 0;

// Dice value and start roll
uint8_t dice_value = 0;
bool start_roll = false;

// Two player game - true/false
bool two_player_game = false;

// Board number (board select)
uint8_t board_number;
static const uint8_t TOTAL_BOARDS = 2;

// Difficulty select
uint8_t difficulty;

// Timed game forfeit booleans
bool p1_wins = false;
bool p2_wins = false;

/////////////////////////////// main //////////////////////////////////
int main(void) {
	// Setup hardware and call backs. This will turn on 
	// interrupts.
	initialise_hardware();
	
	// Show the splash screen message. Returns when display
	// is complete.
	two_player_game = false;
	board_number = 1;
	difficulty = 0;
	p1_wins = false;
	p2_wins = false;
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

void terminal_start_screen(void) {
	hide_cursor();
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

	move_terminal_cursor(10, 18);
	printf_P(PSTR("Press 'b'/'B' to toggle between the boards"));
	move_terminal_cursor(10, 20);
	
	printf_P(PSTR("BOARD: %d"), board_number);

	move_terminal_cursor(10, 22);
	printf_P(PSTR("For easy difficulty, press 'e'/'E'"));
	move_terminal_cursor(10, 23);
	printf_P(PSTR("For medium difficulty, press 'm'/'M'"));
	move_terminal_cursor(10, 24);
	printf_P(PSTR("For hard difficulty, press 'h'/'H'"));

}

void start_screen(void) {
	// Clear terminal screen and output a message
	terminal_start_screen();
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

		if (serial_input == 'b' || serial_input == 'B') {
			if (board_number == TOTAL_BOARDS) {
				board_number = 1;
			} else {
				board_number += 1;
			}
			terminal_start_screen();
		}

		if (serial_input == 'e' || serial_input == 'E') {
			difficulty = 0;
		}

		if (serial_input == 'm' || serial_input == 'M') {
			difficulty = 1;
		}

		if (serial_input == 'h' || serial_input == 'H') {
			difficulty = 2;
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
	initialise_game(two_player_game, board_number);
	
	// Clear a button push or serial input if any are waiting
	// (The cast to void means the return value is ignored.)
	(void)button_pushed();
	clear_serial_input_buffer();
}

void play_game(void) {
	
	uint32_t last_flash_time, current_time;
	uint32_t last_roll_time; // Last time roll_dice() was called

	// last_decrement_time - records last time the player_game_time was decremented
	// player_game_time - is the total time the player has to complete the game
	uint32_t last_decrement_time, player_game_time;

	uint8_t btn; // The button pushed
	
	last_flash_time = get_current_time();
	last_roll_time = get_current_time();
	last_decrement_time = get_current_time();
	
	if (difficulty == 1) {
		player_game_time = 90000;
	} else if (difficulty == 2) {
		player_game_time = 45000;
	} else {
		player_game_time = 45000000; // 12.5 hours
	}
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

		if (btn == BUTTON3_PUSHED || serial_input == 'p' || serial_input == 'P') {
			game_pause();
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
			clear_terminal();
			move_terminal_cursor(10, 14);
			printf_P(PSTR("Dice Rolling..."));
		}else if (((btn == BUTTON2_PUSHED)|(serial_input == 'r' || serial_input == 'R')) & start_roll) {
			start_roll = false;
			clear_terminal();
			move_terminal_cursor(10, 14);
			printf_P(PSTR("Dice Stopped. Value: %d"), dice_value);
			move_player_n(dice_value, true);
			moves += 1;
		}

		if (start_roll) {
			if (current_time >= last_roll_time + 100) {
				dice_value = roll_dice();
				last_roll_time = current_time;
			}
		}

		if (player_game_time < 100) {
			break;
		}

		if (current_time >= last_decrement_time + 100) {
			player_game_time -= 100;
			last_decrement_time = current_time;
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

		if (difficulty > 0) {
			move_terminal_cursor(10, 16);
			printf_P(PSTR("Time Left: %d"), player_game_time/1000);
			if (player_game_time < 10000){
				move_terminal_cursor(22, 16);
				printf_P(PSTR(".%d"), (player_game_time%1000)/100);
			}
		}
	}
	// We get here if the game is over.
	handle_game_over();
}

void two_play_game(void) {
	uint32_t last_flash_time, current_time;
	uint32_t last_roll_time; // Last time roll_dice() was called
	
	// pn_last_decrement_time - records last time the player_game_time was decremented
	// pn_player_game_time - is the time the player has to complete the game
	uint32_t p1_last_decrement_time, p1_game_time;
	uint32_t p2_last_decrement_time, p2_game_time;

	uint8_t btn; // The button pushed
	
	last_flash_time = get_current_time();
	last_roll_time = get_current_time();
	p1_last_decrement_time = get_current_time();
	p2_last_decrement_time = get_current_time();

	if (difficulty == 1) {
		p1_game_time = 90000;
		p2_game_time = 90000;
	} else if (difficulty == 2) {
		p1_game_time = 45000;
		p2_game_time = 45000;
	} else {
		p1_game_time = 45000000; // 12.5 hours
		p2_game_time = 45000000; 
	}
	
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
		
		if (btn == BUTTON3_PUSHED || serial_input == 'p' || serial_input == 'P') {
			game_pause();
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
			clear_terminal();
			move_terminal_cursor(10, 14);
			printf_P(PSTR("Dice Rolling..."));

		}else if (((btn == BUTTON2_PUSHED)|(serial_input == 'r' || serial_input == 'R')) & start_roll) {
			start_roll = false;
			clear_terminal();
			move_terminal_cursor(10, 14);
			printf_P(PSTR("Dice Stopped. Value: %d"), dice_value);
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

		if (p1_game_time < 100) {
			p2_wins = true;
			break;
		} else if(p2_game_time < 100) {
			p1_wins = true;
			break;
		}

		if (move_player_1) {
			if (current_time >= p1_last_decrement_time + 100) {
				p1_game_time -= 100;
				p1_last_decrement_time = current_time;
			}
		} else {
			if (current_time >= p2_last_decrement_time + 100) {
				p2_game_time -= 100;
				p2_last_decrement_time = current_time;
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
			flash_player_2_cursor();
			// Update the most recent time the cursor was flashed
			last_flash_time = current_time;
		}

		player_moved = false;

		
		if (difficulty > 0) {
			move_terminal_cursor(10, 16);
			if (move_player_1){
				printf_P(PSTR("P1 time left: %d"), p1_game_time/1000);
				if (p1_game_time < 10000) {
					move_terminal_cursor(25, 16);
					printf_P(PSTR(".%d"), (p1_game_time%1000)/100);
				}
			} else {
				printf_P(PSTR("P2 time left: %d"), p2_game_time/1000);
				if (p2_game_time < 10000) {
					move_terminal_cursor(25, 16);
					printf_P(PSTR(".%d"), (p2_game_time%1000)/100);
				}
			}
		}
		snake_ladder_func(move_player_1);
		// Shows previous player's no. of moves
		seven_seg_display(moves, dice_value);

	}
	// We get here if the game is over.
	handle_game_over();
}

void game_pause(void) {
	move_terminal_cursor(10, 12);
	printf_P(PSTR("GAME PAUSED. Press 'p'/'P' to continue game"));
	while (1) {
		char serial_input = -1;
		if (serial_input_available()){
			serial_input = fgetc(stdin);
		}

		if (serial_input == 'p' || serial_input == 'P') {
			return;
		}
		seven_seg_display(moves, dice_value);
		init_button_interrupts();
	}
	clear_to_end_of_line();
}

void handle_game_over() {
	clear_terminal();
	move_terminal_cursor(10,14);
	printf_P(PSTR("GAME OVER"));
	move_terminal_cursor(10,15);
	if (is_game_over() == 1 || p1_wins) {
		printf_P(PSTR("Player 1 Wins!!"));
	} else if (is_game_over() == 2 || p2_wins){
		printf_P(PSTR("Player 2 Wins!!"));
	} else {
		printf_P(PSTR("No one wins :("));
	}
	move_terminal_cursor(10,16);
	printf_P(PSTR("Press a button or 's'/'S' to start again"));
	
	while(button_pushed() == NO_BUTTON_PUSHED && !serial_input_available()) {
		; // wait
	}

	char serial_input = -1;
	if (serial_input_available()) {
		serial_input = fgetc(stdin);
	}
	// if the serial input is 's' or 'S', then exit the start screen
	if ((serial_input == 's' || serial_input == 'S') || button_pushed()) {
		main();
	} else {
		handle_game_over();
	}
	
}
