/* See LICENSE file for copyright and license details. */
#ifndef LIBTERMINPUT_H
#define LIBTERMINPUT_H

#include <stddef.h>


/**
 * Flags for supporting incompatible input; the user must
 * set or clear his flag after setting or clearing it on
 * the terminal, and the user must make sure that the
 * terminal support this flag if set.
 */
enum libterminput_flags {
	LIBTERMINPUT_DECSET_1005              = 0x0001,
	LIBTERMINPUT_MACRO_ON_CSI_M           = 0x0002,
	LIBTERMINPUT_PAUSE_ON_CSI_P           = 0x0004,
	LIBTERMINPUT_INS_ON_CSI_AT            = 0x0008,
	LIBTERMINPUT_SEPARATE_BACKTAB         = 0x0010,

	/**
	 * If an ESC is received without anything after it,
	 * return ESC keypress. This is not always desirable
	 * behaviour as the user may manually press ESC to
	 * simulate a keypress that terminal does not support
	 * (yes, this is a real world issue).
	 */
	LIBTERMINPUT_ESC_ON_BLOCK             = 0x0020,

	LIBTERMINPUT_AWAITING_CURSOR_POSITION = 0x0040
};

enum libterminput_mod {
	LIBTERMINPUT_SHIFT = 0x01,
	LIBTERMINPUT_META  = 0x02,
	LIBTERMINPUT_CTRL  = 0x04
};

enum libterminput_key {
	LIBTERMINPUT_SYMBOL,
	LIBTERMINPUT_UP,
	LIBTERMINPUT_DOWN,
	LIBTERMINPUT_RIGHT,
	LIBTERMINPUT_LEFT,
	LIBTERMINPUT_BEGIN,   /* keypad 5 without numlock */
	LIBTERMINPUT_TAB,     /* backtab is interpreted as shift+tab by default */
	LIBTERMINPUT_BACKTAB, /* requires LIBTERMINPUT_SEPARATE_BACKTAB */
	LIBTERMINPUT_F1,
	LIBTERMINPUT_F2,
	LIBTERMINPUT_F3,
	LIBTERMINPUT_F4,
	LIBTERMINPUT_F5,
	LIBTERMINPUT_F6,
	LIBTERMINPUT_F7,
	LIBTERMINPUT_F8,
	LIBTERMINPUT_F9,
	LIBTERMINPUT_F10,
	LIBTERMINPUT_F11,
	LIBTERMINPUT_F12,
	LIBTERMINPUT_HOME,  /* = find */
	LIBTERMINPUT_INS,
	LIBTERMINPUT_DEL,   /* = remove */
	LIBTERMINPUT_END,   /* = select */
	LIBTERMINPUT_PRIOR, /* page up   */
	LIBTERMINPUT_NEXT,  /* page down */
	LIBTERMINPUT_ERASE, /* backspace */
	LIBTERMINPUT_ENTER, /* return    */
	LIBTERMINPUT_ESC,
	LIBTERMINPUT_MACRO,
	LIBTERMINPUT_PAUSE,
	LIBTERMINPUT_KEYPAD_0,
	LIBTERMINPUT_KEYPAD_1,
	LIBTERMINPUT_KEYPAD_2,
	LIBTERMINPUT_KEYPAD_3,
	LIBTERMINPUT_KEYPAD_4,
	LIBTERMINPUT_KEYPAD_5,
	LIBTERMINPUT_KEYPAD_6,
	LIBTERMINPUT_KEYPAD_7,
	LIBTERMINPUT_KEYPAD_8,
	LIBTERMINPUT_KEYPAD_9,
	LIBTERMINPUT_KEYPAD_PLUS,
	LIBTERMINPUT_KEYPAD_MINUS,
	LIBTERMINPUT_KEYPAD_TIMES,
	LIBTERMINPUT_KEYPAD_DIVISION,
	LIBTERMINPUT_KEYPAD_DECIMAL,
	LIBTERMINPUT_KEYPAD_COMMA,
	LIBTERMINPUT_KEYPAD_POINT,
	LIBTERMINPUT_KEYPAD_ENTER,
};

enum libterminput_button {
	LIBTERMINPUT_NO_BUTTON,
	LIBTERMINPUT_BUTTON1,      /* left (assuming right-handed) */
	LIBTERMINPUT_BUTTON2,      /* middle */
	LIBTERMINPUT_BUTTON3,      /* right (assuming right-handed) */
	LIBTERMINPUT_SCROLL_UP,    /* no corresponding release event shall be generated */
	LIBTERMINPUT_SCROLL_DOWN,  /* no corresponding release event shall be generated */
	LIBTERMINPUT_SCROLL_LEFT,  /* may or may not have a corresponding release event */
	LIBTERMINPUT_SCROLL_RIGHT, /* may or may not have a corresponding release event */
	LIBTERMINPUT_XBUTTON1,     /* extended button 1, also known as backward */
	LIBTERMINPUT_XBUTTON2,     /* extended button 2, also known as forward */
	LIBTERMINPUT_XBUTTON3,     /* extended button 3, you probably don't have this button */
	LIBTERMINPUT_XBUTTON4      /* extended button 4, you probably don't have this button */
};

enum libterminput_type {
	LIBTERMINPUT_NONE,
	LIBTERMINPUT_KEYPRESS,
	LIBTERMINPUT_BRACKETED_PASTE_START,
	LIBTERMINPUT_BRACKETED_PASTE_END,
	LIBTERMINPUT_TEXT,
	LIBTERMINPUT_MOUSEEVENT,
	LIBTERMINPUT_TERMINAL_IS_OK,     /* response to CSI 5 n */
	LIBTERMINPUT_TERMINAL_IS_NOT_OK, /* response to CSI 5 n */
	LIBTERMINPUT_CURSOR_POSITION     /* response to CSI 6 n */
};

enum libterminput_event {
	LIBTERMINPUT_PRESS,
	LIBTERMINPUT_RELEASE,
	LIBTERMINPUT_MOTION,
	LIBTERMINPUT_HIGHLIGHT_INSIDE,
	LIBTERMINPUT_HIGHLIGHT_OUTSIDE
};

struct libterminput_keypress {
	enum libterminput_type type;
	enum libterminput_key key;
	unsigned long long int times; /* if .times > 1, next will be the same, but will .times -= 1 */
	enum libterminput_mod mods;
	char symbol[7];               /* use if .key == LIBTERMINPUT_SYMBOL */
};

struct libterminput_text {
	enum libterminput_type type;
	size_t nbytes;
	char bytes[512];
};

struct libterminput_mouseevent {
	enum libterminput_type type;
	enum libterminput_mod mods;      /* Set to 0 for LIBTERMINPUT_HIGHLIGHT_INSIDE and LIBTERMINPUT_HIGHLIGHT_OUTSIDE */
	enum libterminput_button button; /* Set to 1 for LIBTERMINPUT_HIGHLIGHT_INSIDE and LIBTERMINPUT_HIGHLIGHT_OUTSIDE */
	enum libterminput_event event;
	size_t x;
	size_t y;
	size_t start_x; /* Only set for LIBTERMINPUT_HIGHLIGHT_OUTSIDE */
	size_t start_y; /* Only set for LIBTERMINPUT_HIGHLIGHT_OUTSIDE */
	size_t end_x;   /* Only set for LIBTERMINPUT_HIGHLIGHT_OUTSIDE */
	size_t end_y;   /* Only set for LIBTERMINPUT_HIGHLIGHT_OUTSIDE */
};

struct libterminput_position {
	enum libterminput_type type;
	size_t x;
	size_t y;
};

union libterminput_input {
	enum libterminput_type type;
	struct libterminput_keypress keypress;     /* use if .type == LIBTERMINPUT_KEYPRESS */
	struct libterminput_text text;             /* use if .type == LIBTERMINPUT_TEXT */
	struct libterminput_mouseevent mouseevent; /* use if .type == LIBTERMINPUT_MOUSEEVENT */
	struct libterminput_position position;     /* use if .type == LIBTERMINPUT_CURSOR_POSITION */
};


/**
 * This struct should be considered opaque
 */
struct libterminput_state {
	int inited; /* whether the input in initialised, not this struct */
	enum libterminput_mod mods;
	enum libterminput_flags flags;
	char bracketed_paste;
	char mouse_tracking;
	char meta;
	char n;
	size_t stored_head;
	size_t stored_tail;
	char paused;
	char npartial;
	char partial[7];
	char key[44];
	char stored[512];
};


/**
 * Get input from the terminal
 * 
 * @param   fd     The file descriptor to the terminal
 * @param   input  Output parameter for input
 * @param   ctx    State for the terminal, parts of the state may be stored in `input`
 * @return         1 normally, 0 on end of input, -1 on error
 */
int libterminput_read(int fd, union libterminput_input *input, struct libterminput_state *ctx);

inline int
libterminput_is_ready(union libterminput_input *input, struct libterminput_state *ctx)
{
	if (!ctx->inited || ctx->paused || ctx->mouse_tracking)
		return 0;
	if (input->type == LIBTERMINPUT_KEYPRESS && input->keypress.times > 1)
		return 1;
	return ctx->stored_head > ctx->stored_tail;
}

int libterminput_set_flags(struct libterminput_state *ctx, enum libterminput_flags flags);
int libterminput_clear_flags(struct libterminput_state *ctx, enum libterminput_flags flags);


#endif
