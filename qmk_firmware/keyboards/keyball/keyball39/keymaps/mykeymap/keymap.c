/*
Copyright 2022 @Yowkees
Copyright 2022 MURAOKA Taro (aka KoRoN, @kaoriya)

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include QMK_KEYBOARD_H
#include "quantum.h"

/*
#ifdef OLED_ENABLE
#include "lib/oledkit/oledkit.h"

void oledkit_render_info_user(void) {
    keyball_oled_render_keyinfo();
    keyball_oled_render_ballinfo();
}
#endif

layer_state_t layer_state_set_user(layer_state_t state) {
    // Auto enable scroll mode when the highest layer is 2
    keyball_set_scroll_mode(get_highest_layer(state) == 2);
    return state;
}
*/
enum {
    NONE = 0,
    SINGLE_TAP = 1,
    SINGLE_HOLD = 2,
    DOUBLE_TAP = 3,
    DOUBLE_HOLD = 4,
    DOUBLE_SINGLE_TAP = 5, //send two single taps
    TRIPLE_TAP = 6,
    TRIPLE_HOLD = 7,
    TRIPLE_SINGLE_TAP = 8, //send three single taps
    MORE_TAP = 9
};

enum {
    TD_TRIPLE_BRACE_START = 0,
    TD_TRIPLE_BRACE_END,
    TD_B,
    TD_C,
    TD_E,
    TD_G,
    TD_H,
    TD_I,
    TD_N,
    TD_R,
    TD_T,
    TD_V,
    TD_X,
    TD_Y,
    TD_Z
};

/*
 * Return an integer that corresponds to what kind of tap dance should be executed.
 *
 * How to figure out tap dance state: interrupted and pressed.
 *
 * Interrupted: If the state of a dance dance is "interrupted", that means that another key has been hit
 *  under the tapping term. This is typically indicitive that you are trying to "tap" the key.
 *
 * Pressed: Whether or not the key is still being pressed. If this value is true, that means the tapping term
 *  has ended, but the key is still being pressed down. This generally means the key is being "held".
 *
 * One thing that is currenlty not possible with qmk software in regards to tap dance is to mimic the "permissive hold"
 *  feature. In general, advanced tap dances do not work well if they are used with commonly typed letters.
 *  For example "A". Tap dances are best used on non-letter keys that are not hit while typing letters.
 *
 * Good places to put an advanced tap dance:
 *  z,q,x,j,k,v,b, any function key, home/end, comma, semi-colon
 *
 * Criteria for "good placement" of a tap dance key:
 *  Not a key that is hit frequently in a sentence
 *  Not a key that is used frequently to double tap, for example 'tab' is often double tapped in a terminal, or
 *    in a web form. So 'tab' would be a poor choice for a tap dance.
 *  Letters used in common words as a double. For example 'p' in 'pepper'. If a tap dance function existed on the
 *    letter 'p', the word 'pepper' would be quite frustating to type.
 *
 * For the third point, there does exist the 'DOUBLE_SINGLE_TAP', however this is not fully tested
 *
 */
int cur_dance (tap_dance_state_t *state) {
    if (state->count == 1) {
        if (state->interrupted || !state->pressed) {
            return SINGLE_TAP;
        } else {
            // key has not been interrupted, but they key is still held. Means you want to send a 'HOLD'.
            return SINGLE_HOLD;
        }
    }
    else if (state->count == 2) {
        /*
        * DOUBLE_SINGLE_TAP is to distinguish between typing "pepper", and actually wanting a double tap
        * action when hitting 'pp'. Suggested use case for this return value is when you want to send two
        * keystrokes of the key, and not the 'double tap' action/macro.
        */
        if (state->interrupted) {
            return DOUBLE_SINGLE_TAP;
        } else if (state->pressed) {
            return DOUBLE_HOLD;
        } else {
            return DOUBLE_TAP;
        }
    }
    // Assumes no one is trying to type the same letter three times (at least not quickly).
    // If your tap dance key is 'KC_W', and you want to type "www." quickly - then you will need to add
    // an exception here to return a 'TRIPLE_SINGLE_TAP', and define that enum just like 'DOUBLE_SINGLE_TAP'
    else if (state->count == 3) {
        if (state->interrupted) {
            return TRIPLE_SINGLE_TAP;
        } else if (state->pressed) {
            return TRIPLE_HOLD;
        } else {
            return TRIPLE_TAP;
        }
    }
    else if (state->count > 3) {
        return MORE_TAP;
    }
    else return 99; //magic number. At some point this method will expand to work for more presses
};

typedef struct {
    bool is_press_action;
    int state;
} tap;

//instanalize an instance of 'tap' for the 'x' tap dance.
static tap btap_state = {
    .is_press_action = true,
    .state = NONE
};
static tap ctap_state = {
    .is_press_action = true,
    .state = NONE
};
static tap etap_state = {
    .is_press_action = true,
    .state = NONE
};
static tap gtap_state = {
    .is_press_action = true,
    .state = NONE
};
static tap htap_state = {
    .is_press_action = true,
    .state = NONE
};
static tap itap_state = {
    .is_press_action = true,
    .state = NONE
};
static tap ntap_state = {
    .is_press_action = true,
    .state = NONE
};
static tap rtap_state = {
    .is_press_action = true,
    .state = NONE
};
static tap ttap_state = {
    .is_press_action = true,
    .state = NONE
};
static tap vtap_state = {
    .is_press_action = true,
    .state = NONE
};
static tap xtap_state = {
    .is_press_action = true,
    .state = NONE
};
static tap ytap_state = {
    .is_press_action = true,
    .state = NONE
};
static tap ztap_state = {
    .is_press_action = true,
    .state = NONE
};

void repeat_key_x_time(uint16_t keycode, int x) {
    for (int i = 0; i < x; i++) {
        register_code(keycode);
    }

    // less than register_code because unregister_code in hoge_reset
    for (int i = 1; i < x; i++) {
        unregister_code(keycode);
    }
}

void b_finished (tap_dance_state_t *state, void *user_data) {
    btap_state.state = cur_dance(state);
    switch (btap_state.state) {
        case SINGLE_TAP:
            register_code(KC_B);
            break;
        case SINGLE_HOLD:
            register_code(KC_LSFT);
            register_code(KC_LBRC);
            break;
        case DOUBLE_TAP:
            repeat_key_x_time(KC_B, 2);
            break;
        case DOUBLE_HOLD:
            register_code(KC_B);
            break;
        // description of DOUBLE_SINGLE_TAP
        // case TD_SINGLE_TAP: register_code(KC_F); break;
        // case TD_SINGLE_HOLD: register_code(KC_LCTL); break;
        // case TD_DOUBLE_TAP: register_code(KC_ESC); break;
        // case TD_DOUBLE_HOLD: register_code(KC_LALT); break;
        // Last case is for fast typing. Assuming your key is `f`:
        // For example, when typing the word `buffer`, and you want to make sure that you send `ff` and not `Esc`.
        // In order to type `ff` when typing fast, the next character will have to be hit within the `TAPPING_TERM`, which by default is 200ms.
        // case TD_DOUBLE_SINGLE_TAP: tap_code(KC_X); register_code(KC_X); break;
        case DOUBLE_SINGLE_TAP:
            repeat_key_x_time(KC_B, 2);
            break;
        case TRIPLE_TAP:
            repeat_key_x_time(KC_B, 3);
            break;
        case TRIPLE_HOLD:
            register_code(KC_B);
            break;
        case TRIPLE_SINGLE_TAP:
            repeat_key_x_time(KC_B, 3);
            break;
        case MORE_TAP:
            repeat_key_x_time(KC_B, state->count);
            break;
        default:
            break;
    }
};
void b_reset (tap_dance_state_t *state, void *user_data) {
    switch (btap_state.state) {
        case SINGLE_TAP:
            unregister_code(KC_B);
            break;
        case SINGLE_HOLD:
            unregister_code(KC_LSFT);
            unregister_code(KC_LBRC);
            break;
        case DOUBLE_TAP:
            unregister_code(KC_B);
            break;
        case DOUBLE_HOLD:
            unregister_code(KC_B);
            break;
        case DOUBLE_SINGLE_TAP:
            unregister_code(KC_B);
            break;
        case TRIPLE_TAP:
            unregister_code(KC_B);
            break;
        case TRIPLE_HOLD:
            unregister_code(KC_B);
            break;
        case TRIPLE_SINGLE_TAP:
            unregister_code(KC_B);
            break;
        case MORE_TAP:
            unregister_code(KC_B);
            break;
        default:
            break;
    }
    btap_state.state = NONE;
};

void c_finished (tap_dance_state_t *state, void *user_data) {
    ctap_state.state = cur_dance(state);
    switch (ctap_state.state) {
        case SINGLE_TAP:
            register_code(KC_C);
            break;
        case SINGLE_HOLD:
            register_code(KC_LCTL);
            register_code(KC_C);
            break;
        case DOUBLE_TAP:
            repeat_key_x_time(KC_C, 2);
            break;
        case DOUBLE_HOLD:
            register_code(KC_C);
            break;
        case DOUBLE_SINGLE_TAP:
            repeat_key_x_time(KC_C, 2);
            break;
        case TRIPLE_TAP:
            repeat_key_x_time(KC_C, 3);
            break;
        case TRIPLE_HOLD:
            register_code(KC_C);
            break;
        case TRIPLE_SINGLE_TAP:
            repeat_key_x_time(KC_C, 3);
            break;
        case MORE_TAP:
            repeat_key_x_time(KC_C, state->count);
            break;
        default:
            break;
    }
};
void c_reset (tap_dance_state_t *state, void *user_data) {
    switch (ctap_state.state) {
        case SINGLE_TAP:
            unregister_code(KC_C);
            break;
        case SINGLE_HOLD:
            unregister_code(KC_LCTL);
            unregister_code(KC_C);
            break;
        case DOUBLE_TAP:
            unregister_code(KC_C);
            break;
        case DOUBLE_HOLD:
            unregister_code(KC_C);
            break;
        case DOUBLE_SINGLE_TAP:
            unregister_code(KC_C);
            break;
        case TRIPLE_TAP:
            unregister_code(KC_C);
            break;
        case TRIPLE_HOLD:
            unregister_code(KC_C);
            break;
        case TRIPLE_SINGLE_TAP:
            unregister_code(KC_C);
            break;
        case MORE_TAP:
            unregister_code(KC_C);
            break;
        default:
            break;
    }
    ctap_state.state = NONE;
};

void e_finished (tap_dance_state_t *state, void *user_data) {
    etap_state.state = cur_dance(state);
    switch (etap_state.state) {
        case SINGLE_TAP:
            register_code(KC_E);
            break;
        case SINGLE_HOLD:
            register_code(KC_ESC);
            break;
        case DOUBLE_TAP:
            repeat_key_x_time(KC_E, 2);
            break;
        case DOUBLE_HOLD:
            register_code(KC_E);
            break;
        case DOUBLE_SINGLE_TAP:
            repeat_key_x_time(KC_E, 2);
            break;
        case TRIPLE_TAP:
            repeat_key_x_time(KC_E, 3);
            break;
        case TRIPLE_HOLD:
            register_code(KC_E);
            break;
        case TRIPLE_SINGLE_TAP:
            repeat_key_x_time(KC_E, 3);
            break;
        case MORE_TAP:
            repeat_key_x_time(KC_E, state->count);
            break;
        default:
            break;
    }
};
void e_reset (tap_dance_state_t *state, void *user_data) {
    switch (etap_state.state) {
        case SINGLE_TAP:
            unregister_code(KC_E);
            break;
        case SINGLE_HOLD:
            unregister_code(KC_ESC);
            break;
        case DOUBLE_TAP:
            unregister_code(KC_E);
            break;
        case DOUBLE_HOLD:
            unregister_code(KC_E);
            break;
        case DOUBLE_SINGLE_TAP:
            unregister_code(KC_E);
            break;
        case TRIPLE_TAP:
            unregister_code(KC_E);
            break;
        case TRIPLE_HOLD:
            unregister_code(KC_E);
            break;
        case TRIPLE_SINGLE_TAP:
            unregister_code(KC_E);
            break;
        case MORE_TAP:
            unregister_code(KC_E);
            break;
        default:
            break;
    }
    etap_state.state = NONE;
};

void g_finished (tap_dance_state_t *state, void *user_data) {
    gtap_state.state = cur_dance(state);
    switch (gtap_state.state) {
        case SINGLE_TAP:
            register_code(KC_G);
            break;
        case SINGLE_HOLD:
            register_code(KC_LSFT);
            register_code(KC_9);
            break;
        case DOUBLE_TAP:
            repeat_key_x_time(KC_G, 2);
            break;
        case DOUBLE_HOLD:
            register_code(KC_G);
            break;
        case DOUBLE_SINGLE_TAP:
            repeat_key_x_time(KC_G, 2);
            break;
        case TRIPLE_TAP:
            repeat_key_x_time(KC_G, 3);
            break;
        case TRIPLE_HOLD:
            register_code(KC_G);
            break;
        case TRIPLE_SINGLE_TAP:
            repeat_key_x_time(KC_G, 3);
            break;
        case MORE_TAP:
            repeat_key_x_time(KC_G, state->count);
            break;
        default:
            break;
    }
};
void g_reset (tap_dance_state_t *state, void *user_data) {
    switch (gtap_state.state) {
        case SINGLE_TAP:
            unregister_code(KC_G);
            break;
        case SINGLE_HOLD:
            unregister_code(KC_LSFT);
            unregister_code(KC_9);
            break;
        case DOUBLE_TAP:
            unregister_code(KC_G);
            break;
        case DOUBLE_HOLD:
            unregister_code(KC_G);
            break;
        case DOUBLE_SINGLE_TAP:
            unregister_code(KC_G);
            break;
        case TRIPLE_TAP:
            unregister_code(KC_G);
            break;
        case TRIPLE_HOLD:
            unregister_code(KC_G);
            break;
        case TRIPLE_SINGLE_TAP:
            unregister_code(KC_G);
            break;
        case MORE_TAP:
            unregister_code(KC_G);
            break;
        default:
            break;
    }
    gtap_state.state = NONE;
};

void h_finished (tap_dance_state_t *state, void *user_data) {
    htap_state.state = cur_dance(state);
    switch (htap_state.state) {
        case SINGLE_TAP:
            register_code(KC_H);
            break;
        case SINGLE_HOLD:
            register_code(KC_LSFT);
            register_code(KC_0);
            break;
        case DOUBLE_TAP:
            repeat_key_x_time(KC_H, 2);
            break;
        case DOUBLE_HOLD:
            register_code(KC_H);
            break;
        case DOUBLE_SINGLE_TAP:
            repeat_key_x_time(KC_H, 2);
            break;
        case TRIPLE_TAP:
            repeat_key_x_time(KC_H, 3);
            break;
        case TRIPLE_HOLD:
            register_code(KC_H);
            break;
        case TRIPLE_SINGLE_TAP:
            repeat_key_x_time(KC_H, 3);
            break;
        case MORE_TAP:
            repeat_key_x_time(KC_H, state->count);
            break;
        default:
            break;
    }
};
void h_reset (tap_dance_state_t *state, void *user_data) {
    switch (htap_state.state) {
        case SINGLE_TAP:
            unregister_code(KC_H);
            break;
        case SINGLE_HOLD:
            unregister_code(KC_LSFT);
            unregister_code(KC_0);
            break;
        case DOUBLE_TAP:
            unregister_code(KC_H);
            break;
        case DOUBLE_HOLD:
            unregister_code(KC_H);
            break;
        case DOUBLE_SINGLE_TAP:
            unregister_code(KC_H);
            break;
        case TRIPLE_TAP:
            unregister_code(KC_H);
            break;
        case TRIPLE_HOLD:
            unregister_code(KC_H);
            break;
        case TRIPLE_SINGLE_TAP:
            unregister_code(KC_H);
            break;
        case MORE_TAP:
            unregister_code(KC_H);
            break;
        default:
            break;
    }
    htap_state.state = NONE;
};

void i_finished (tap_dance_state_t *state, void *user_data) {
    itap_state.state = cur_dance(state);
    switch (itap_state.state) {
        case SINGLE_TAP:
            register_code(KC_I);
            break;
        case SINGLE_HOLD:
            register_code(KC_ESC);
            break;
        case DOUBLE_TAP:
            repeat_key_x_time(KC_I, 2);
            break;
        case DOUBLE_HOLD:
            register_code(KC_I);
            break;
        case DOUBLE_SINGLE_TAP:
            repeat_key_x_time(KC_I, 2);
            break;
        case TRIPLE_TAP:
            repeat_key_x_time(KC_I, 3);
            break;
        case TRIPLE_HOLD:
            register_code(KC_I);
            break;
        case TRIPLE_SINGLE_TAP:
            repeat_key_x_time(KC_I, 3);
            break;
        case MORE_TAP:
            repeat_key_x_time(KC_I, state->count);
            break;
        default:
            break;
    }
};
void i_reset (tap_dance_state_t *state, void *user_data) {
    switch (itap_state.state) {
        case SINGLE_TAP:
            unregister_code(KC_I);
            break;
        case SINGLE_HOLD:
            unregister_code(KC_ESC);
            break;
        case DOUBLE_TAP:
            unregister_code(KC_I);
            break;
        case DOUBLE_HOLD:
            unregister_code(KC_I);
            break;
        case DOUBLE_SINGLE_TAP:
            unregister_code(KC_I);
            break;
        case TRIPLE_TAP:
            unregister_code(KC_I);
            break;
        case TRIPLE_HOLD:
            unregister_code(KC_I);
            break;
        case TRIPLE_SINGLE_TAP:
            unregister_code(KC_I);
            break;
        case MORE_TAP:
            unregister_code(KC_I);
            break;
        default:
            break;
    }
    itap_state.state = NONE;
};

void n_finished (tap_dance_state_t *state, void *user_data) {
    ntap_state.state = cur_dance(state);
    switch (ntap_state.state) {
        case SINGLE_TAP:
            register_code(KC_N);
            break;
        case SINGLE_HOLD:
            register_code(KC_LSFT);
            register_code(KC_RBRC);
            break;
        case DOUBLE_TAP:
            repeat_key_x_time(KC_N, 2);
            break;
        case DOUBLE_HOLD:
            register_code(KC_N);
            break;
        case DOUBLE_SINGLE_TAP:
            repeat_key_x_time(KC_N, 2);
            break;
        case TRIPLE_TAP:
            repeat_key_x_time(KC_N, 3);
            break;
        case TRIPLE_HOLD:
            register_code(KC_N);
            break;
        case TRIPLE_SINGLE_TAP:
            repeat_key_x_time(KC_N, 3);
            break;
        case MORE_TAP:
            repeat_key_x_time(KC_N, state->count);
            break;
        default:
            break;
    }
};
void n_reset (tap_dance_state_t *state, void *user_data) {
    switch (ntap_state.state) {
        case SINGLE_TAP:
            unregister_code(KC_N);
            break;
        case SINGLE_HOLD:
            unregister_code(KC_LSFT);
            unregister_code(KC_RBRC);
            break;
        case DOUBLE_TAP:
            unregister_code(KC_N);
            break;
        case DOUBLE_HOLD:
            unregister_code(KC_N);
            break;
        case DOUBLE_SINGLE_TAP:
            unregister_code(KC_N);
            break;
        case TRIPLE_TAP:
            unregister_code(KC_N);
            break;
        case TRIPLE_HOLD:
            unregister_code(KC_N);
            break;
        case TRIPLE_SINGLE_TAP:
            unregister_code(KC_N);
            break;
        case MORE_TAP:
            unregister_code(KC_N);
            break;
        default:
            break;
    }
    ntap_state.state = NONE;
};

void r_finished (tap_dance_state_t *state, void *user_data) {
    rtap_state.state = cur_dance(state);
    switch (rtap_state.state) {
        case SINGLE_TAP:
            register_code(KC_R);
            break;
        case SINGLE_HOLD:
            register_code(KC_LCTL);
            register_code(KC_R);
            break;
        case DOUBLE_TAP:
            repeat_key_x_time(KC_R, 2);
            break;
        case DOUBLE_HOLD:
            register_code(KC_R);
            break;
        case DOUBLE_SINGLE_TAP:
            repeat_key_x_time(KC_R, 2);
            break;
        case TRIPLE_TAP:
            repeat_key_x_time(KC_R, 3);
            break;
        case TRIPLE_HOLD:
            register_code(KC_R);
            break;
        case TRIPLE_SINGLE_TAP:
            repeat_key_x_time(KC_R, 3);
            break;
        case MORE_TAP:
            repeat_key_x_time(KC_R, state->count);
            break;
        default:
            break;
    }
};
void r_reset (tap_dance_state_t *state, void *user_data) {
    switch (rtap_state.state) {
        case SINGLE_TAP:
            unregister_code(KC_R);
            break;
        case SINGLE_HOLD:
            unregister_code(KC_LCTL);
            unregister_code(KC_R);
            break;
        case DOUBLE_TAP:
            unregister_code(KC_R);
            break;
        case DOUBLE_HOLD:
            unregister_code(KC_R);
            break;
        case DOUBLE_SINGLE_TAP:
            unregister_code(KC_R);
            break;
        case TRIPLE_TAP:
            unregister_code(KC_R);
            break;
        case TRIPLE_HOLD:
            unregister_code(KC_R);
            break;
        case TRIPLE_SINGLE_TAP:
            unregister_code(KC_R);
            break;
        case MORE_TAP:
            unregister_code(KC_R);
            break;
        default:
            break;
    }
    rtap_state.state = NONE;
};

void t_finished (tap_dance_state_t *state, void *user_data) {
    ttap_state.state = cur_dance(state);
    switch (ttap_state.state) {
        case SINGLE_TAP:
            register_code(KC_T);
            break;
        case SINGLE_HOLD:
            register_code(KC_LBRC);
            break;
        case DOUBLE_TAP:
            repeat_key_x_time(KC_T, 2);
            break;
        case DOUBLE_HOLD:
            register_code(KC_T);
            break;
        case DOUBLE_SINGLE_TAP:
            repeat_key_x_time(KC_T, 2);
            break;
        case TRIPLE_TAP:
            repeat_key_x_time(KC_T, 3);
            break;
        case TRIPLE_HOLD:
            register_code(KC_T);
            break;
        case TRIPLE_SINGLE_TAP:
            repeat_key_x_time(KC_T, 3);
            break;
        case MORE_TAP:
            repeat_key_x_time(KC_T, state->count);
            break;
        default:
            break;
    }
};
void t_reset (tap_dance_state_t *state, void *user_data) {
    switch (ttap_state.state) {
        case SINGLE_TAP:
            unregister_code(KC_T);
            break;
        case SINGLE_HOLD:
            unregister_code(KC_LBRC);
            break;
        case DOUBLE_TAP:
            unregister_code(KC_T);
            break;
        case DOUBLE_HOLD:
            unregister_code(KC_T);
            break;
        case DOUBLE_SINGLE_TAP:
            unregister_code(KC_T);
            break;
        case TRIPLE_TAP:
            unregister_code(KC_T);
            break;
        case TRIPLE_HOLD:
            unregister_code(KC_T);
            break;
        case TRIPLE_SINGLE_TAP:
            unregister_code(KC_T);
            break;
        case MORE_TAP:
            unregister_code(KC_T);
            break;
        default:
            break;
    }
    ttap_state.state = NONE;
};

void v_finished (tap_dance_state_t *state, void *user_data) {
    vtap_state.state = cur_dance(state);
    switch (vtap_state.state) {
        case SINGLE_TAP:
            register_code(KC_V);
            break;
        case SINGLE_HOLD:
            register_code(KC_LCTL);
            register_code(KC_V);
            break;
        case DOUBLE_TAP:
            repeat_key_x_time(KC_V, 2);
            break;
        case DOUBLE_HOLD:
            register_code(KC_V);
            break;
        case DOUBLE_SINGLE_TAP:
            repeat_key_x_time(KC_V, 2);
            break;
        case TRIPLE_TAP:
            repeat_key_x_time(KC_V, 3);
            break;
        case TRIPLE_HOLD:
            register_code(KC_V);
            break;
        case TRIPLE_SINGLE_TAP:
            repeat_key_x_time(KC_V, 3);
            break;
        case MORE_TAP:
            repeat_key_x_time(KC_V, state->count);
            break;
        default:
            break;
    }
};
void v_reset (tap_dance_state_t *state, void *user_data) {
    switch (vtap_state.state) {
        case SINGLE_TAP:
            unregister_code(KC_V);
            break;
        case SINGLE_HOLD:
            unregister_code(KC_LCTL);
            unregister_code(KC_V);
            break;
        case DOUBLE_TAP:
            unregister_code(KC_V);
            break;
        case DOUBLE_HOLD:
            unregister_code(KC_V);
            break;
        case DOUBLE_SINGLE_TAP:
            unregister_code(KC_V);
            break;
        case TRIPLE_TAP:
            unregister_code(KC_V);
            break;
        case TRIPLE_HOLD:
            unregister_code(KC_V);
            break;
        case TRIPLE_SINGLE_TAP:
            unregister_code(KC_V);
            break;
        case MORE_TAP:
            unregister_code(KC_V);
            break;
        default:
            break;
    }
    vtap_state.state = NONE;
};

void x_finished (tap_dance_state_t *state, void *user_data) {
    xtap_state.state = cur_dance(state);
    switch (xtap_state.state) {
        case SINGLE_TAP:
            register_code(KC_X);
            break;
        case SINGLE_HOLD:
            register_code(KC_LCTL);
            register_code(KC_X);
            break;
        case DOUBLE_TAP:
            repeat_key_x_time(KC_X, 2);
            break;
        case DOUBLE_HOLD:
            register_code(KC_X);
            break;
        case DOUBLE_SINGLE_TAP:
            repeat_key_x_time(KC_X, 2);
            break;
        case TRIPLE_TAP:
            repeat_key_x_time(KC_X, 3);
            break;
        case TRIPLE_HOLD:
            register_code(KC_X);
            break;
        case TRIPLE_SINGLE_TAP:
            repeat_key_x_time(KC_X, 3);
            break;
        case MORE_TAP:
            repeat_key_x_time(KC_X, state->count);
            break;
        default:
            break;
    }
};
void x_reset (tap_dance_state_t *state, void *user_data) {
    switch (xtap_state.state) {
        case SINGLE_TAP:
            unregister_code(KC_X);
            break;
        case SINGLE_HOLD:
            unregister_code(KC_LCTL);
            unregister_code(KC_X);
            break;
        case DOUBLE_TAP:
            unregister_code(KC_X);
            break;
        case DOUBLE_HOLD:
            unregister_code(KC_X);
            break;
        case DOUBLE_SINGLE_TAP:
            unregister_code(KC_X);
            break;
        case TRIPLE_TAP:
            unregister_code(KC_X);
            break;
        case TRIPLE_HOLD:
            unregister_code(KC_X);
            break;
        case TRIPLE_SINGLE_TAP:
            unregister_code(KC_X);
            break;
        case MORE_TAP:
            unregister_code(KC_X);
            break;
        default:
            break;
    }
    xtap_state.state = NONE;
};

void y_finished (tap_dance_state_t *state, void *user_data) {
    ytap_state.state = cur_dance(state);
    switch (ytap_state.state) {
        case SINGLE_TAP:
            register_code(KC_Y);
            break;
        case SINGLE_HOLD:
            register_code(KC_RBRC);
            break;
        case DOUBLE_TAP:
            repeat_key_x_time(KC_Y, 2);
            break;
        case DOUBLE_HOLD:
            register_code(KC_Y);
            break;
        case DOUBLE_SINGLE_TAP:
            repeat_key_x_time(KC_Y, 2);
            break;
        case TRIPLE_TAP:
            repeat_key_x_time(KC_Y, 3);
            break;
        case TRIPLE_HOLD:
            register_code(KC_Y);
            break;
        case TRIPLE_SINGLE_TAP:
            repeat_key_x_time(KC_Y, 3);
            break;
        case MORE_TAP:
            repeat_key_x_time(KC_Y, state->count);
            break;
        default:
            break;
    }
};
void y_reset (tap_dance_state_t *state, void *user_data) {
    switch (ytap_state.state) {
        case SINGLE_TAP:
            unregister_code(KC_Y);
            break;
        case SINGLE_HOLD:
            unregister_code(KC_RBRC);
            break;
        case DOUBLE_TAP:
            unregister_code(KC_Y);
            break;
        case DOUBLE_HOLD:
            unregister_code(KC_Y);
            break;
        case DOUBLE_SINGLE_TAP:
            unregister_code(KC_Y);
            break;
        case TRIPLE_TAP:
            unregister_code(KC_Y);
            break;
        case TRIPLE_HOLD:
            unregister_code(KC_Y);
            break;
        case TRIPLE_SINGLE_TAP:
            unregister_code(KC_Y);
            break;
        case MORE_TAP:
            unregister_code(KC_Y);
            break;
        default:
            break;
    }
    ytap_state.state = NONE;
};

void z_finished (tap_dance_state_t *state, void *user_data) {
    ztap_state.state = cur_dance(state);
    switch (ztap_state.state) {
        case SINGLE_TAP:
            register_code(KC_Z);
            break;
        case SINGLE_HOLD:
            register_code(KC_LCTL);
            register_code(KC_Z);
            break;
        case DOUBLE_TAP:
            repeat_key_x_time(KC_Z, 2);
            break;
        case DOUBLE_HOLD:
            register_code(KC_Z);
            break;
        case DOUBLE_SINGLE_TAP:
            repeat_key_x_time(KC_Z, 2);
            break;
        case TRIPLE_TAP:
            repeat_key_x_time(KC_Z, 3);
            break;
        case TRIPLE_HOLD:
            register_code(KC_Z);
            break;
        case TRIPLE_SINGLE_TAP:
            repeat_key_x_time(KC_Z, 3);
            break;
        case MORE_TAP:
            repeat_key_x_time(KC_Z, state->count);
            break;
        default:
            break;
    }
};
void z_reset (tap_dance_state_t *state, void *user_data) {
    switch (ztap_state.state) {
        case SINGLE_TAP:
            unregister_code(KC_Z);
            break;
        case SINGLE_HOLD:
            unregister_code(KC_LCTL);
            unregister_code(KC_Z);
            break;
        case DOUBLE_TAP:
            unregister_code(KC_Z);
            break;
        case DOUBLE_HOLD:
            unregister_code(KC_Z);
            break;
        case DOUBLE_SINGLE_TAP:
            unregister_code(KC_Z);
            break;
        case TRIPLE_TAP:
            unregister_code(KC_Z);
            break;
        case TRIPLE_HOLD:
            unregister_code(KC_Z);
            break;
        case TRIPLE_SINGLE_TAP:
            unregister_code(KC_Z);
            break;
        case MORE_TAP:
            unregister_code(KC_Z);
            break;
        default:
            break;
    }
    ztap_state.state = NONE;
};

void triple_brace_start(tap_dance_state_t *state, void *user_data) {
    if(state->count == 1){
        register_code(KC_LSFT);
        register_code(KC_9);
        unregister_code(KC_9);
        unregister_code(KC_LSFT);
    }else if(state->count == 2){
        register_code(KC_LBRC);
        unregister_code(KC_LBRC);
    }else if(state->count == 3){
        register_code(KC_LSFT);
        register_code(KC_LBRC);
        unregister_code(KC_LBRC);
        unregister_code(KC_LSFT);
    }
    reset_tap_dance(state);
}

void triple_brace_end(tap_dance_state_t *state, void *user_data) {
    if(state->count == 1){
        register_code(KC_LSFT);
        register_code(KC_0);
        unregister_code(KC_0);
        unregister_code(KC_LSFT);
    }else if(state->count == 2){
        register_code(KC_RBRC);
        unregister_code(KC_RBRC);
    }else if(state->count == 3){
        register_code(KC_LSFT);
        register_code(KC_RBRC);
        unregister_code(KC_RBRC);
        unregister_code(KC_LSFT);
    }
    reset_tap_dance(state);
}

tap_dance_action_t tap_dance_actions[] = {
    [TD_TRIPLE_BRACE_START] = ACTION_TAP_DANCE_FN(triple_brace_start),
    [TD_TRIPLE_BRACE_END] = ACTION_TAP_DANCE_FN(triple_brace_end),
    [TD_B] = ACTION_TAP_DANCE_FN_ADVANCED(NULL, b_finished, b_reset),
    [TD_C] = ACTION_TAP_DANCE_FN_ADVANCED(NULL, c_finished, c_reset),
    [TD_E] = ACTION_TAP_DANCE_FN_ADVANCED(NULL, e_finished, e_reset),
    [TD_G] = ACTION_TAP_DANCE_FN_ADVANCED(NULL, g_finished, g_reset),
    [TD_H] = ACTION_TAP_DANCE_FN_ADVANCED(NULL, h_finished, h_reset),
    [TD_I] = ACTION_TAP_DANCE_FN_ADVANCED(NULL, i_finished, i_reset),
    [TD_N] = ACTION_TAP_DANCE_FN_ADVANCED(NULL, n_finished, n_reset),
    [TD_R] = ACTION_TAP_DANCE_FN_ADVANCED(NULL, r_finished, r_reset),
    [TD_T] = ACTION_TAP_DANCE_FN_ADVANCED(NULL, t_finished, t_reset),
    [TD_V] = ACTION_TAP_DANCE_FN_ADVANCED(NULL, v_finished, v_reset),
    [TD_X] = ACTION_TAP_DANCE_FN_ADVANCED(NULL, x_finished, x_reset),
    [TD_Y] = ACTION_TAP_DANCE_FN_ADVANCED(NULL, y_finished, y_reset),
    [TD_Z] = ACTION_TAP_DANCE_FN_ADVANCED(NULL, z_finished, z_reset)
};

// clang-format off
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
  // keymap for default
  [0] = LAYOUT_universal(
    KC_A     , KC_B     , KC_C     , KC_D     , KC_T     ,                            KC_Y     , KC_U     , KC_I     , KC_O     , KC_P     ,
    KC_A     , KC_S     , KC_D     , KC_F     , KC_G     ,                            KC_H     , KC_J     , KC_K     , KC_L     , KC_MINS  ,
    KC_Z     , KC_X     , KC_C     , KC_V     , KC_B     ,                            KC_N     , KC_M     , KC_COMM  , KC_DOT   , KC_SLSH  ,
    KC_LCTL  , KC_LGUI  , KC_LALT  ,LSFT_T(KC_LNG2),LT(1,KC_SPC),LT(3,KC_LNG1),KC_BSPC,LT(2,KC_ENT),LSFT_T(KC_LNG2),KC_RALT,KC_RGUI, KC_RSFT
  ),

  [1] = LAYOUT_universal(
    KC_F1    , KC_F2    , KC_F3    , KC_F4    , KC_RBRC  ,                            KC_F6    , KC_F7    , KC_F8    , KC_F9    , KC_F10   ,
    KC_F5    , KC_EXLM  , S(KC_6)  ,S(KC_INT3), S(KC_8)  ,                           S(KC_INT1), KC_BTN1  , KC_PGUP  , KC_BTN2  , KC_SCLN  ,
    S(KC_EQL),S(KC_LBRC),S(KC_7)   , S(KC_2)  ,S(KC_RBRC),                            KC_LBRC  , KC_DLR   , KC_PGDN  , KC_BTN3  , KC_F11   ,
    KC_INT1  , KC_EQL   , S(KC_3)  , _______  , _______  , _______  ,      TO(2)    , TO(0)    , _______  , KC_RALT  , KC_RGUI  , KC_F12
  ),

  [2] = LAYOUT_universal(
    KC_TAB   , KC_7     , KC_8     , KC_9     , KC_MINS  ,                            KC_NUHS  , _______  , KC_BTN3  , _______  , KC_BSPC  ,
   S(KC_QUOT), KC_4     , KC_5     , KC_6     ,S(KC_SCLN),                            S(KC_9)  , KC_BTN1  , KC_UP    , KC_BTN2  , KC_QUOT  ,
    KC_SLSH  , KC_1     , KC_2     , KC_3     ,S(KC_MINS),                           S(KC_NUHS), KC_LEFT  , KC_DOWN  , KC_RGHT  , _______  ,
    KC_ESC   , KC_0     , KC_DOT   , KC_DEL   , KC_ENT   , KC_BSPC  ,      _______  , _______  , _______  , _______  , _______  , _______
  ),

  [3] = LAYOUT_universal(
    RGB_TOG  , AML_TO   , AML_I50  , AML_D50  , _______  ,                            _______  , _______  , SSNP_HOR , SSNP_VRT , SSNP_FRE ,
    RGB_MOD  , RGB_HUI  , RGB_SAI  , RGB_VAI  , SCRL_DVI ,                            _______  , _______  , _______  , _______  , _______  ,
    RGB_RMOD , RGB_HUD  , RGB_SAD  , RGB_VAD  , SCRL_DVD ,                            CPI_D1K  , CPI_D100 , CPI_I100 , CPI_I1K  , KBC_SAVE ,
    QK_BOOT  , KBC_RST  , _______  , _______  , _______  , _______  ,      _______  , _______  , _______  , _______  , KBC_RST  , QK_BOOT
  ),
};
// clang-format on

layer_state_t layer_state_set_user(layer_state_t state) {
    // Auto enable scroll mode when the highest layer is 3
    keyball_set_scroll_mode(get_highest_layer(state) == 3);
    return state;
}

#ifdef OLED_ENABLE

#    include "lib/oledkit/oledkit.h"

void oledkit_render_info_user(void) {
    keyball_oled_render_keyinfo();
    keyball_oled_render_ballinfo();
    keyball_oled_render_layerinfo();
}
#endif
