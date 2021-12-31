/*! \file game.c
 *  \brief Gameplay gamestate.
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

#define NUM_STARS 42

struct GamestateResources {
	// This struct is for every resource allocated and used by your gamestate.
	// It gets created on load and then gets passed around to all other function calls.
	ALLEGRO_BITMAP *star, *houses;
	struct {
		double x, y, counter, speed, size, deviation;
	} stars[NUM_STARS];
};

int Gamestate_ProgressCount = 1; // number of loading steps as reported by Gamestate_Load; 0 when missing

void Gamestate_Logic(struct Game* game, struct GamestateResources* data, double delta) {
	// Here you should do all your game logic as if <delta> seconds have passed.
	for (int i = 0; i < NUM_STARS; i++) {
		data->stars[i].counter += delta * data->stars[i].speed;
	}
}

void Gamestate_Draw(struct Game* game, struct GamestateResources* data) {
	// Draw everything to the screen here.
	DrawVerticalGradientRect(0, 0, game->viewport.width, game->viewport.height,
		al_map_rgb(0, 0, 16), al_map_rgb(0, 0, 64));

	for (int i = 0; i < NUM_STARS; i++) {
		double shininess = (1 - (cos(data->stars[i].counter * 4.2) + 1) * 0.1) * 0.8;
		al_draw_tinted_scaled_rotated_bitmap(data->star, al_map_rgb_f(shininess, shininess, shininess), al_get_bitmap_width(data->star) / 2, al_get_bitmap_height(data->star) / 2,
			data->stars[i].x * game->viewport.width, data->stars[i].y * game->viewport.height, data->stars[i].size, data->stars[i].size,
			sin(data->stars[i].counter) * data->stars[i].deviation, 0);
	}

	al_draw_bitmap(data->houses, 0, 0, 0);
}

void Gamestate_ProcessEvent(struct Game* game, struct GamestateResources* data, ALLEGRO_EVENT* ev) {
	// Called for each event in Allegro event queue.
	// Here you can handle user input, expiring timers etc.
	if ((ev->type == ALLEGRO_EVENT_KEY_DOWN) && (ev->keyboard.keycode == ALLEGRO_KEY_ESCAPE)) {
		UnloadCurrentGamestate(game); // mark this gamestate to be stopped and unloaded
		// When there are no active gamestates, the engine will quit.
	}
}

void* Gamestate_Load(struct Game* game, void (*progress)(struct Game*)) {
	// Called once, when the gamestate library is being loaded.
	// Good place for allocating memory, loading bitmaps etc.
	//
	// Keep in mind that there's no OpenGL context available here. If you want to prerender something,
	// create VBOs, etc. do it in Gamestate_PostLoad.

	struct GamestateResources* data = calloc(1, sizeof(struct GamestateResources));
	data->star = al_load_bitmap(GetDataFilePath(game, "gwiazdka.png"));
	progress(game); // report that we progressed with the loading, so the engine can move a progress bar

	data->houses = al_load_bitmap(GetDataFilePath(game, "domki.png"));
	progress(game);

	return data;
}

void Gamestate_Unload(struct Game* game, struct GamestateResources* data) {
	// Called when the gamestate library is being unloaded.
	// Good place for freeing all allocated memory and resources.
	al_destroy_bitmap(data->star);
	free(data);
}

void Gamestate_Start(struct Game* game, struct GamestateResources* data) {
	// Called when this gamestate gets control. Good place for initializing state,
	// playing music etc.
	for (int i = 0; i < NUM_STARS; i++) {
		data->stars[i].x = rand() / (double)RAND_MAX;
		data->stars[i].y = rand() / (double)RAND_MAX;
		data->stars[i].counter = rand() / (double)RAND_MAX * ALLEGRO_PI;
		data->stars[i].size = rand() / (double)RAND_MAX * 0.5 + 0.75;
		data->stars[i].speed = rand() / (double)RAND_MAX * 0.1 + 1;
		data->stars[i].deviation = rand() / (double)RAND_MAX;
	}
}

void Gamestate_Stop(struct Game* game, struct GamestateResources* data) {
	// Called when gamestate gets stopped. Stop timers, music etc. here.
}

// Optional endpoints:

void Gamestate_PostLoad(struct Game* game, struct GamestateResources* data) {
	// This is called in the main thread after Gamestate_Load has ended.
	// Use it to prerender bitmaps, create VBOs, etc.
}

void Gamestate_Pause(struct Game* game, struct GamestateResources* data) {
	// Called when gamestate gets paused (so only Draw is being called, no Logic nor ProcessEvent)
	// Pause your timers and/or sounds here.
}

void Gamestate_Resume(struct Game* game, struct GamestateResources* data) {
	// Called when gamestate gets resumed. Resume your timers and/or sounds here.
}

void Gamestate_Reload(struct Game* game, struct GamestateResources* data) {
	// Called when the display gets lost and not preserved bitmaps need to be recreated.
	// Unless you want to support mobile platforms, you should be able to ignore it.
}
