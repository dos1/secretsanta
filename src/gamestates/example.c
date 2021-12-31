/*! \file example.c
 *  \brief Example gamestate.
 */
/*
 * Copyright (c) Sebastian Krzyszkowiak <dos@dosowisko.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "../common.h"
#include <libsuperderpy.h>

struct GamestateResources {
	ALLEGRO_FONT* font;
	int blink_counter;
};

int Gamestate_ProgressCount = 1;

void Gamestate_Logic(struct Game* game, struct GamestateResources* data, double delta) {
	data->blink_counter++;
	if (data->blink_counter >= 60) {
		data->blink_counter = 0;
	}
}

void Gamestate_Draw(struct Game* game, struct GamestateResources* data) {
	if (data->blink_counter < 50) {
		al_draw_text(data->font, al_map_rgb(255, 255, 255), game->viewport.width / 2.0, game->viewport.height / 2.0,
			ALLEGRO_ALIGN_CENTRE, "Nothing to see here, move along!");
	}
}

void Gamestate_ProcessEvent(struct Game* game, struct GamestateResources* data, ALLEGRO_EVENT* ev) {
	if ((ev->type == ALLEGRO_EVENT_KEY_DOWN) && (ev->keyboard.keycode == ALLEGRO_KEY_ESCAPE)) {
		UnloadCurrentGamestate(game); // mark this gamestate to be stopped and unloaded
		// When there are no active gamestates, the engine will quit.
	}
}

void* Gamestate_Load(struct Game* game, void (*progress)(struct Game*)) {
	struct GamestateResources* data = calloc(1, sizeof(struct GamestateResources));
	int flags = al_get_new_bitmap_flags();
	al_set_new_bitmap_flags(flags & ~ALLEGRO_MAG_LINEAR); // disable linear scaling for pixelarty appearance
	data->font = al_create_builtin_font();
	progress(game); // report that we progressed with the loading, so the engine can move a progress bar
	al_set_new_bitmap_flags(flags);
	return data;
}

void Gamestate_Unload(struct Game* game, struct GamestateResources* data) {
	al_destroy_font(data->font);
	free(data);
}

void Gamestate_Start(struct Game* game, struct GamestateResources* data) {
	data->blink_counter = 0;
}

void Gamestate_Stop(struct Game* game, struct GamestateResources* data) {}
