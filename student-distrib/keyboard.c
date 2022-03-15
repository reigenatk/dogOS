#include "keyboard.h"
  


#define KEYBOARD_PORT 0x60

// Key numbers taken from https://elixir.bootlin.com/linux/latest/source/include/uapi/linux/input-event-codes.h#L136
#define RELEASE_CODE 0x80
#define KEY_RESERVED		0
#define KEY_ESC			1
#define KEY_1			2
#define KEY_2			3
#define KEY_3			4
#define KEY_4			5
#define KEY_5			6
#define KEY_6			7
#define KEY_7			8
#define KEY_8			9
#define KEY_9			10
#define KEY_0			11
#define KEY_MINUS		12
#define KEY_EQUAL		13
#define KEY_BACKSPACE		14
#define KEY_TAB			15
#define KEY_Q			16
#define KEY_W			17
#define KEY_E			18
#define KEY_R			19
#define KEY_T			20
#define KEY_Y			21
#define KEY_U			22
#define KEY_I			23
#define KEY_O			24
#define KEY_P			25
#define KEY_LEFTBRACE		26
#define KEY_RIGHTBRACE		27
#define KEY_ENTER		28
#define KEY_LEFTCTRL		29
#define KEY_A			30
#define KEY_S			31
#define KEY_D			32
#define KEY_F			33
#define KEY_G			34
#define KEY_H			35
#define KEY_J			36
#define KEY_K			37
#define KEY_L			38
#define KEY_SEMICOLON		39
#define KEY_APOSTROPHE		40
#define KEY_GRAVE		41 // backtick
#define KEY_LEFTSHIFT		42
#define KEY_BACKSLASH		43
#define KEY_Z			44
#define KEY_X			45
#define KEY_C			46
#define KEY_V			47
#define KEY_B			48
#define KEY_N			49
#define KEY_M			50
#define KEY_COMMA		51
#define KEY_DOT			52
#define KEY_SLASH		53
#define KEY_RIGHTSHIFT		54
#define KEY_KPASTERISK		55
#define KEY_LEFTALT		56
#define KEY_SPACE		57
#define KEY_CAPSLOCK		58
#define KEY_F1			59
#define KEY_F2			60
#define KEY_F3			61
#define KEY_F4			62
#define KEY_F5			63
#define KEY_F6			64
#define KEY_F7			65
#define KEY_F8			66
#define KEY_F9			67
#define KEY_F10			68
#define KEY_NUMLOCK		69
#define KEY_SCROLLLOCK		70
// #define KEY_KP7			71
// #define KEY_KP8			72
// #define KEY_KP9			73
// #define KEY_KPMINUS		74
// #define KEY_KP4			75
// #define KEY_KP5			76
// #define KEY_KP6			77
// #define KEY_KPPLUS		78
// #define KEY_KP1			79
// #define KEY_KP2			80
// #define KEY_KP3			81
// #define KEY_KP0			82
// #define KEY_KPDOT		83


uint8_t is_caps_lock_toggled = 0;
uint8_t is_shift_key_pressed = 0;

#define NUM_KEYS 70
#define KEYBOARD_IRQ 1

key keys[NUM_KEYS];

uint32_t non_displayable_keys[20] = {KEY_RESERVED, KEY_ESC, KEY_BACKSPACE, KEY_LEFTCTRL, KEY_LEFTSHIFT, KEY_RIGHTSHIFT,
                                     KEY_LEFTALT, KEY_CAPSLOCK, KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6, KEY_F7, KEY_F8, KEY_F9, KEY_F10,
                                     KEY_NUMLOCK, KEY_SCROLLLOCK};

void init_key_vals() {
  // initialize all of the keys that are writable to the screen (letters + numbers + some chars)
  keys[KEY_1] = (key) {'1', '1', '!', '!', 0x01};
  keys[KEY_2] = (key) {'2', '2', '@', '@', 0x01};
  keys[KEY_3] = (key) {'3', '3', '#', '#', 1};
  keys[KEY_4] = (key) {'4', '4', '$', '$', 1};
  keys[KEY_5] = (key) {'5', '5', '%', '%', 1};
  keys[KEY_6] = (key) {'6', '6', '^', '^', 1};
  keys[KEY_7] = (key) {'7', '7', '&', '&', 1};
  keys[KEY_8] = (key) {'8', '8', '*', '*', 1};
  keys[KEY_9] = (key) {'9', '9', '(', '(', 1};
  keys[KEY_0] = (key) {'0', '0', ')', ')', 1};
  keys[KEY_MINUS] = (key) {'-', '-', '_', '_', 1};
  keys[KEY_EQUAL] = (key) {'=', '=', '+', '+', 1};
  keys[KEY_TAB] = (key) {'\t', '\t', '\t', '\t', 1};
  keys[KEY_Q] = (key) {'q', 'Q', 'Q', 'q', 1};
  keys[KEY_W] = (key) { 'w', 'W', 'W', 'w', 1 };
  keys[KEY_E] = (key) {'e', 'E', 'E', 'e', 1};
  keys[KEY_R] = (key) {'r', 'R', 'R', 'r', 1};
  keys[KEY_T] = (key) {'t', 'T', 'T', 't', 1};
  keys[KEY_Y] = (key) {'y', 'Y', 'Y', 'y', 1};
  keys[KEY_U] = (key) {'u', 'U', 'U', 'u', 1};
  keys[KEY_I] = (key) {'i', 'I', 'I', 'i', 1};
  keys[KEY_O] = (key) {'o', 'O', 'O', 'o', 1};
  keys[KEY_P] = (key) {'p', 'P', 'P', 'p', 1};
  keys[KEY_LEFTBRACE] = (key) {'[', '[', '{', '{', 1};
  keys[KEY_RIGHTBRACE] = (key) {']', ']', '}', '}', 1};
  keys[KEY_ENTER] = (key) {'\n', '\n', '\n', '\n', 1};
  keys[KEY_A] = (key) {'a', 'A', 'A', 'a', 1};
  keys[KEY_S] = (key) {'s', 'S', 'S', 's', 1};
  keys[KEY_D] = (key) {'d', 'D', 'D', 'd', 1};
  keys[KEY_F] = (key) {'f', 'F', 'F', 'f', 1};
  keys[KEY_G] = (key) {'g', 'G', 'G', 'g', 1};
  keys[KEY_H] = (key) {'h', 'H', 'H', 'h', 1};
  keys[KEY_J] = (key) {'j', 'J', 'J', 'j', 1};
  keys[KEY_K] = (key) {'k', 'K', 'K', 'k', 1};
  keys[KEY_L] = (key) {'l', 'L', 'L', 'l', 1};
  keys[KEY_SEMICOLON] = (key) {';', ';', ':', ':', 1};
  keys[KEY_APOSTROPHE] = (key) {'\'', '\'', '\"', '\"', 1};
  keys[KEY_GRAVE] = (key) {'`', '`', '~', '~', 1};
  keys[KEY_BACKSLASH] = (key) {'\\', '\\', '|', '|', 1};
  keys[KEY_Z] = (key) {'z', 'Z', 'Z', 'z', 1};
  keys[KEY_X] = (key) {'x', 'X', 'X', 'x', 1};
  keys[KEY_C] = (key) {'c', 'C', 'C', 'c',1};
  keys[KEY_V] = (key) {'v', 'V', 'V', 'v', 1};
  keys[KEY_B] = (key) {'b', 'B', 'B', 'b', 1};
  keys[KEY_N] = (key) {'n', 'N', 'N', 'n', 1};
  keys[KEY_M] = (key) {'m', 'M', 'M', 'm', 1};
  keys[KEY_COMMA] = (key) {',', ',', '<', '<', 1};
  keys[KEY_DOT] = (key) {'.', '.', '>', '>', 1};
  keys[KEY_SLASH] = (key) {'/', '/', '?', '?', 1};
  keys[KEY_SPACE] = (key) {' ', ' ', ' ', ' ', 1};
}

void init_keyboard()
{
  // Keyboard is IRQ1, RTC is IRQ8
  enable_irq(KEYBOARD_IRQ);

  // mark all the non-displayable keys
  int i;
  for (i = 0; i < sizeof(non_displayable_keys) / sizeof(non_displayable_keys[0]); i++)
  {
    keys[i].should_display = 0;
  }
  init_key_vals();
}

void dereference_null() {
  int a = 3 / 0;
}

void keyboard_INT() {
  // IO Port 0x60 is used by the Keyboard. This is just standard, you can look it up
  // To read data from the keyboard we just read from port 0x60 using IN, and then send EOI
  // Port 0x64 is also useful maybe, its the command register

  // Data comes from keyboard controller and is in the form of a scancode, which is usually default scancode set 2.
  // make + break scancodes for press + release. A key releae is the code for the key + 0x80
  // Read each keycode in one at a time.

  // keycodes detailed at https://www.win.tue.nl/~aeb/linux/kbd/scancodes-1.html

  // send eoi immediately so PIC can service other hardware interrupts.
  send_eoi(KEYBOARD_IRQ);

  // disable this IRQ line from sending info to the CPU for a second
  disable_irq(KEYBOARD_IRQ);

  // no more processor interrupts
  cli();

  uint8_t keycode = 0; // keycodes range from 0x0-0x60 ish, let's use 0 which is error code normally to indicate we haven't received anything
	do {
		if (inb(KEYBOARD_PORT) != 0) {
			keycode = inb(KEYBOARD_PORT);
			if (keycode > 0) {
				break;
			}
		}
	} while(1);
  if (keycode == KEY_LEFTSHIFT || keycode == KEY_RIGHTSHIFT) {
    is_shift_key_pressed = 1;
  }
  else if (keycode == KEY_LEFTSHIFT + RELEASE_CODE || keycode == KEY_RIGHTSHIFT + RELEASE_CODE) {
    is_shift_key_pressed = 0;
  }
  else if (keycode == KEY_CAPSLOCK) {
    is_caps_lock_toggled = !is_caps_lock_toggled;
  }
  else {
    if (keycode <= NUM_KEYS) {
      // valid code, process it

      // if the character is not displayable, do not display
      if (keys[keycode].should_display == 0) {
        enable_irq(KEYBOARD_IRQ);
        sti();
        return;
      }

      // press 1 key to test the RTC interrupt functionality
      if (keycode == KEY_1) {
        toggle_run();
      }
      if (keycode == KEY_2) {
        dereference_null();
      }

      uint8_t key_char = 0;
      // if shift and no caps lock
      if (is_shift_key_pressed && !is_caps_lock_toggled) {
        key_char = keys[keycode].shift_char;
      }
      else if (is_shift_key_pressed && is_caps_lock_toggled) {
        // shift + caps
        key_char = keys[keycode].caps_and_shift;
      }
      else if (!is_shift_key_pressed && !is_caps_lock_toggled) {
        key_char = keys[keycode].lowercase_char;
      }
      else if (!is_shift_key_pressed && is_caps_lock_toggled) {
        key_char = keys[keycode].uppercase_char;
      }


      putc(key_char);
    }
  }

  enable_irq(KEYBOARD_IRQ);
  sti();
}
