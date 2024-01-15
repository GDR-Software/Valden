#ifndef __KEYCODES__
#define __KEYCODES__

#pragma once

//
// these are the key numbers that should be passed to KeyEvent
//

// normal keys should be passed as lowercased ascii

enum {
    KEY_TAB = 9,
	KEY_ENTER = 13,
	KEY_ESCAPE = 27,
	KEY_SPACE = 32,

	KEY_QUOTE = '\'',
	KEY_PLUS = '+',
	KEY_COMMA = ',',
	KEY_MINUS = '-',
	KEY_DOT = '.',
	KEY_SLASH = '/',
	KEY_SEMICOLON = ';',
	KEY_EQUAL = '=',
	KEY_BACKSLASH = '\\',
	KEY_UNDERSCORE = '_',
	KEY_BRACKET_OPEN = '[',
	KEY_BRACKET_CLOSE = ']',

	KEY_A = 'a',
	KEY_B = 'b',
	KEY_C = 'c',
	KEY_D = 'd',
	KEY_E = 'e',
	KEY_F = 'f',
	KEY_G = 'g',
	KEY_H = 'h',
	KEY_I = 'i',
	KEY_J = 'j',
	KEY_K = 'k',
	KEY_L = 'l',
	KEY_M = 'm',
	KEY_N = 'n',
	KEY_O = 'o',
	KEY_P = 'p',
	KEY_Q = 'q',
	KEY_R = 'r',
	KEY_S = 's',
	KEY_T = 't',
	KEY_U = 'u',
	KEY_V = 'v',
	KEY_W = 'w',
	KEY_X = 'x',
	KEY_Y = 'y',
	KEY_Z = 'z',

	// following definitions must not be changed
	KEY_BACKSPACE = 127,

	KEY_COMMAND = 128,
	KEY_CAPSLOCK,
	KEY_POWER,
	KEY_PAUSE,

	KEY_UP,
	KEY_DOWN,
	KEY_LEFT,
	KEY_RIGHT,

	KEY_ALT,
	KEY_CTRL,
	KEY_SHIFT,
	KEY_INSERT,
	KEY_DELETE,
	KEY_PAGEDOWN,
	KEY_PAGEUP,
	KEY_HOME,
	KEY_END,

	KEY_F1,
	KEY_F2,
	KEY_F3,
	KEY_F4,
	KEY_F5,
	KEY_F6,
	KEY_F7,
	KEY_F8,
	KEY_F9,
	KEY_F10,
	KEY_F11,
	KEY_F12,

	KEY_KP_HOME,
	KEY_KP_UP,
	KEY_KP_PAGEUP,
	KEY_KP_LEFT,
	KEY_KP_5,
	KEY_KP_RIGHT,
	KEY_KP_END,
	KEY_KP_DOWN,
	KEY_KP_PAGEDOWN,
	KEY_KP_ENTER,
	KEY_KP_INSERT,
	KEY_KP_DELETE,
	KEY_KP_SLASH,
	KEY_KP_MINUS,
	KEY_KP_PLUS,
	KEY_KP_NUMLOCK,
	KEY_KP_STAR,
	KEY_KP_EQUALS,

    KEY_MOUSE_LEFT,
    KEY_MOUSE_MIDDLE,
    KEY_MOUSE_RIGHT,

	KEY_WHEEL_DOWN,
	KEY_WHEEL_UP,

	KEY_JOY1,
	KEY_JOY2,
	KEY_JOY3,
	KEY_JOY4,
	KEY_JOY5,
	KEY_JOY6,
	KEY_JOY7,
	KEY_JOY8,
	KEY_JOY9,
	KEY_JOY10,
	KEY_JOY11,
	KEY_JOY12,
	KEY_JOY13,
	KEY_JOY14,
	KEY_JOY15,
	KEY_JOY16,
	KEY_JOY17,
	KEY_JOY18,
	KEY_JOY19,
	KEY_JOY20,
	KEY_JOY21,
	KEY_JOY22,
	KEY_JOY23,
	KEY_JOY24,
	KEY_JOY25,
	KEY_JOY26,
	KEY_JOY27,
	KEY_JOY28,
	KEY_JOY29,
	KEY_JOY30,
	KEY_JOY31,
	KEY_JOY32,

	KEY_AUX1,
	KEY_AUX2,
	KEY_AUX3,
	KEY_AUX4,
	KEY_AUX5,
	KEY_AUX6,
	KEY_AUX7,
	KEY_AUX8,
	KEY_AUX9,
	KEY_AUX10,
	KEY_AUX11,
	KEY_AUX12,
	KEY_AUX13,
	KEY_AUX14,
	KEY_AUX15,
	KEY_AUX16,
	// end of compatibility list

	KEY_SUPER,
	KEY_COMPOSE,
	KEY_MODE,
	KEY_HELP,
	KEY_SCREENSHOT,
	KEY_SYSREQ,
	KEY_SCROLLOCK,
	KEY_BREAK,
	KEY_MENU,
	KEY_EURO,
	KEY_UNDO,

	// Gamepad controls
	// Ordered to match SDL2 game controller buttons and axes
	// Do not change this order without also changing IN_GamepadMove() in SDL_input.c
	KEY_PAD0_A,
	KEY_PAD0_B,
	KEY_PAD0_X,
	KEY_PAD0_Y,
	KEY_PAD0_BACK,
	KEY_PAD0_GUIDE,
	KEY_PAD0_START,
	KEY_PAD0_LEFTSTICK_CLICK,
	KEY_PAD0_RIGHTSTICK_CLICK,
	KEY_PAD0_LEFTSHOULDER,
	KEY_PAD0_RIGHTSHOULDER,
	KEY_PAD0_DPAD_UP,
	KEY_PAD0_DPAD_DOWN,
	KEY_PAD0_DPAD_LEFT,
	KEY_PAD0_DPAD_RIGHT,

	KEY_PAD0_LEFTSTICK_LEFT,
	KEY_PAD0_LEFTSTICK_RIGHT,
	KEY_PAD0_LEFTSTICK_UP,
	KEY_PAD0_LEFTSTICK_DOWN,
	KEY_PAD0_RIGHTSTICK_LEFT,
	KEY_PAD0_RIGHTSTICK_RIGHT,
	KEY_PAD0_RIGHTSTICK_UP,
	KEY_PAD0_RIGHTSTICK_DOWN,
	KEY_PAD0_LEFTTRIGGER,
	KEY_PAD0_RIGHTTRIGGER,

	KEY_PAD0_MISC1,    /* Xbox Series X share button, PS5 microphone button, Nintendo Switch Pro capture button, Amazon Luna microphone button */
	KEY_PAD0_PADDLE1,  /* Xbox Elite paddle P1 */
	KEY_PAD0_PADDLE2,  /* Xbox Elite paddle P3 */
	KEY_PAD0_PADDLE3,  /* Xbox Elite paddle P2 */
	KEY_PAD0_PADDLE4,  /* Xbox Elite paddle P4 */
	KEY_PAD0_TOUCHPAD, /* PS4/PS5 touchpad button */

	// Pseudo-key that brings the console down
	KEY_CONSOLE,

    NUMKEYS
};

typedef uint32_t keynum_t;

// The menu code needs to get both key and char events, but
// to avoid duplicating the paths, the char events are just
// distinguished by or'ing in K_CHAR_FLAG (ugly)
#define	K_CHAR_FLAG		1024

#endif
