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
#define MAX_DRONES 42

int Gamestate_ProgressCount = 5; // number of loading steps as reported by Gamestate_Load; 0 when missing

struct GamestateResources {
	// This struct is for every resource allocated and used by your gamestate.
	// It gets created on load and then gets passed around to all other function calls.
	ALLEGRO_BITMAP *star, *houses, *drone;
	ALLEGRO_FONT* font;

	struct {
		double x, y, counter, speed, size, deviation;
	} stars[NUM_STARS];

	struct {
		double x, y, rot, speed;
		ALLEGRO_BITMAP* bitmap;
	} santa;

	struct {
		ALLEGRO_SHADER *invert, *circular;
	} shaders;

	struct {
		bool accelerate, brake, left, right;
	} keys;

	struct {
		bool enabled;
		double x, y, counter, angle, left, deviation, speed, rotspeed, timemax, timemin;
	} drones[MAX_DRONES];
};

static double TriangleArea(double x1, double y1, double x2, double y2, double x3, double y3) {
	return fabs((x1 * (y2 - y3) + x2 * (y3 - y1) + x3 * (y1 - y2)) / 2.0);
}

static bool IsInsideTriangle(double x1, double y1, double x2, double y2, double x3, double y3, double x, double y) {
	double A = TriangleArea(x1, y1, x2, y2, x3, y3);
	double A1 = TriangleArea(x, y, x2, y2, x3, y3);
	double A2 = TriangleArea(x1, y1, x, y, x3, y3);
	double A3 = TriangleArea(x1, y1, x2, y2, x, y);
	return fabs(A - (A1 + A2 + A3)) < 0.001;
}

static void GetDroneTriangle(struct Game* game, struct GamestateResources* data, int i, double* x1, double* y1, double* x2, double* y2, double* x3, double* y3) {
	double x = data->drones[i].x;
	double y = data->drones[i].y + cos(data->drones[i].counter * data->drones[i].speed) * data->drones[i].deviation + 0.02;

	*x1 = x;
	*y1 = y;
	*x2 = x + cos(data->drones[i].angle + 0.33) * 0.33;
	*y2 = y + sin(data->drones[i].angle + 0.33) * 0.33 * 1.777;
	*x3 = x + cos(data->drones[i].angle - 0.33) * 0.33;
	*y3 = y + sin(data->drones[i].angle - 0.33) * 0.33 * 1.777;
}

static bool IsSantaInDroneTriangle(struct Game* game, struct GamestateResources* data, int i) {
	// don't judge me
	double x1, y1, x2, y2, x3, y3;
	GetDroneTriangle(game, data, i, &x1, &y1, &x2, &y2, &x3, &y3);
	/*
	al_draw_filled_circle((data->santa.x) * game->viewport.width, (data->santa.y) * game->viewport.height, 15, al_map_rgb(255, 255, 255));
	al_draw_filled_circle((data->santa.x + cos(data->santa.rot) * 0.14) * game->viewport.width, (data->santa.y + sin(data->santa.rot) * 0.14 * 1.7777) * game->viewport.height, 15, al_map_rgb(255, 255, 255));
	al_draw_filled_circle((data->santa.x + cos(data->santa.rot - ALLEGRO_PI / 2.0) * 0.02) * game->viewport.width, (data->santa.y + sin(data->santa.rot - ALLEGRO_PI / 2.0) * 0.02 * 1.7777) * game->viewport.height, 15, al_map_rgb(255, 255, 255));
	al_draw_filled_circle((data->santa.x + cos(data->santa.rot) * 0.07) * game->viewport.width, (data->santa.y + sin(data->santa.rot) * 0.07 * 1.7777) * game->viewport.height, 15, al_map_rgb(255, 255, 255));
	al_draw_filled_circle((data->santa.x + cos(data->santa.rot) * 0.035) * game->viewport.width, (data->santa.y + sin(data->santa.rot) * 0.035 * 1.7777) * game->viewport.height, 15, al_map_rgb(255, 255, 255));
	al_draw_filled_circle((data->santa.x + cos(data->santa.rot) * 0.105) * game->viewport.width, (data->santa.y + sin(data->santa.rot) * 0.105 * 1.7777) * game->viewport.height, 15, al_map_rgb(255, 255, 255));
	*/
	if (IsInsideTriangle(x1, y1, x2, y2, x3, y3, data->santa.x, data->santa.y)) return true;
	if (IsInsideTriangle(x1, y1, x2, y2, x3, y3, data->santa.x + cos(data->santa.rot - ALLEGRO_PI / 2.0) * 0.02, data->santa.y + sin(data->santa.rot - ALLEGRO_PI / 2.0) * 0.02)) return true;

	if (IsInsideTriangle(x1, y1, x2, y2, x3, y3, data->santa.x + cos(data->santa.rot) * 0.14, data->santa.y + sin(data->santa.rot) * 0.14 * 1.7777)) return true;
	if (IsInsideTriangle(x1, y1, x2, y2, x3, y3, data->santa.x + cos(data->santa.rot - ALLEGRO_PI / 2.0) * 0.02 + cos(data->santa.rot) * 0.14, data->santa.y + sin(data->santa.rot - ALLEGRO_PI / 2.0) * 0.02 + sin(data->santa.rot) * 0.14 * 1.7777)) return true;

	if (IsInsideTriangle(x1, y1, x2, y2, x3, y3, data->santa.x + cos(data->santa.rot) * 0.07, data->santa.y + sin(data->santa.rot) * 0.07 * 1.7777)) return true;
	if (IsInsideTriangle(x1, y1, x2, y2, x3, y3, data->santa.x + cos(data->santa.rot - ALLEGRO_PI / 2.0) * 0.02 + cos(data->santa.rot) * 0.07, data->santa.y + sin(data->santa.rot - ALLEGRO_PI / 2.0) * 0.02 + sin(data->santa.rot) * 0.07 * 1.7777)) return true;

	if (IsInsideTriangle(x1, y1, x2, y2, x3, y3, data->santa.x + cos(data->santa.rot) * 0.035, data->santa.y + sin(data->santa.rot) * 0.035 * 1.7777)) return true;
	if (IsInsideTriangle(x1, y1, x2, y2, x3, y3, data->santa.x + cos(data->santa.rot - ALLEGRO_PI / 2.0) * 0.02 + cos(data->santa.rot) * 0.035, data->santa.y + sin(data->santa.rot - ALLEGRO_PI / 2.0) * 0.02 + sin(data->santa.rot) * 0.035 * 1.7777)) return true;

	if (IsInsideTriangle(x1, y1, x2, y2, x3, y3, data->santa.x + cos(data->santa.rot) * 0.105, data->santa.y + sin(data->santa.rot) * 0.105 * 1.7777)) return true;
	if (IsInsideTriangle(x1, y1, x2, y2, x3, y3, data->santa.x + cos(data->santa.rot - ALLEGRO_PI / 2.0) * 0.02 + cos(data->santa.rot) * 0.105, data->santa.y + sin(data->santa.rot - ALLEGRO_PI / 2.0) * 0.02 + sin(data->santa.rot) * 0.105 * 1.7777)) return true;
	return false;
}

void Gamestate_Logic(struct Game* game, struct GamestateResources* data, double delta) {
	// Here you should do all your game logic as if <delta> seconds have passed.
	for (int i = 0; i < NUM_STARS; i++) {
		data->stars[i].counter += delta * data->stars[i].speed;
	}
	for (int i = 0; i < MAX_DRONES; i++) {
		data->drones[i].counter += delta;
		if (data->drones[i].left > 0) {
			data->drones[i].left -= delta;
			data->drones[i].angle -= delta * data->drones[i].rotspeed;
			if (data->drones[i].left <= 0) {
				data->drones[i].left = (rand() / (double)RAND_MAX * (data->drones[i].timemax - data->drones[i].timemin) + data->drones[i].timemin) * ((rand() % 2) ? 1 : -1);
			}
		} else if (data->drones[i].left < 0) {
			data->drones[i].left += delta;
			data->drones[i].angle += delta * data->drones[i].rotspeed;
			if (data->drones[i].left >= 0) {
				data->drones[i].left = (rand() / (double)RAND_MAX * (data->drones[i].timemax - data->drones[i].timemin) + data->drones[i].timemin) * ((rand() % 2) ? 1 : -1);
			}
		}
	}

	double dspeed = 0;
	if (data->keys.accelerate) {
		dspeed += 0.03;
	}
	if (data->keys.brake) {
		dspeed += (data->santa.speed > 0) ? -0.02 : -0.01;
	}
	data->santa.speed = fmin(1, fmax(-0.5, data->santa.speed + dspeed));

	data->santa.speed *= 0.975;

	data->santa.x += cos(data->santa.rot) * data->santa.speed * 0.005;
	data->santa.y += sin(data->santa.rot) * data->santa.speed * 0.005;

	double dangle = 0;
	if (data->keys.left) {
		dangle += -0.025;
	}
	if (data->keys.right) {
		dangle += 0.025;
	}
	data->santa.rot += dangle;
	if (data->santa.rot > ALLEGRO_PI * 2) {
		data->santa.rot -= ALLEGRO_PI * 2;
	}
	if (data->santa.rot < 0) {
		data->santa.rot += ALLEGRO_PI * 2;
	}

	data->santa.x = fmax(0, fmin(data->santa.x, 1));
	data->santa.y = fmax(0, fmin(data->santa.y, 1));

	if (data->santa.x > 0.99 && data->santa.y < 0.2) Gamestate_Start(game, data);
}

static void DrawTexturedRectangle(float x1, float y1, float x2, float y2, ALLEGRO_COLOR color) {
	ALLEGRO_VERTEX vtx[4];
	int ii;

	vtx[0].x = x1;
	vtx[0].y = y1;
	vtx[0].u = 0.0;
	vtx[0].v = 0.0;
	vtx[1].x = x1;
	vtx[1].y = y2;
	vtx[1].u = 0.0;
	vtx[1].v = 1.0;
	vtx[2].x = x2;
	vtx[2].y = y2;
	vtx[2].u = 1.0;
	vtx[2].v = 1.0;
	vtx[3].x = x2;
	vtx[3].y = y1;
	vtx[3].u = 1.0;
	vtx[3].v = 0.0;

	for (ii = 0; ii < 4; ii++) {
		vtx[ii].color = color;
		vtx[ii].z = 0;
	}

	al_draw_prim(vtx, 0, 0, 0, 4, ALLEGRO_PRIM_TRIANGLE_FAN);
}

void Gamestate_Draw(struct Game* game, struct GamestateResources* data) {
	// Draw everything to the screen here.
	DrawVerticalGradientRect(0, 0, game->viewport.width, game->viewport.height,
		al_map_rgb(0, 0, 16 + 0), al_map_rgb(0, 0, 64 + 0));

	for (int i = 0; i < NUM_STARS; i++) {
		double shininess = (1 - (cos(data->stars[i].counter * 4.2) + 1) * 0.1) * 0.8;
		al_draw_tinted_scaled_rotated_bitmap(data->star, al_map_rgb_f(shininess, shininess, shininess), al_get_bitmap_width(data->star) / 2, al_get_bitmap_height(data->star) / 2,
			data->stars[i].x * game->viewport.width, data->stars[i].y * game->viewport.height, data->stars[i].size * 0.8, data->stars[i].size * 0.8,
			sin(data->stars[i].counter) * data->stars[i].deviation, 0);
	}

	al_draw_bitmap(data->houses, 0, 1221, 0);

	al_use_shader(data->shaders.circular);
	DrawTexturedRectangle(game->viewport.width * 0.96, 0, game->viewport.width * 1.06, game->viewport.height * 0.2, al_premul_rgba(19, 209, 45, 222));
	al_use_shader(NULL);
	al_draw_text(data->font, al_map_rgb(19, 209, 45), game->viewport.width * (0.98 + cos(game->time * 3) * 0.003), game->viewport.height * 0.077, ALLEGRO_ALIGN_CENTER, ">");

	for (int i = 0; i < MAX_DRONES; i++) {
		if (!data->drones[i].enabled) continue;

		double x = data->drones[i].x;
		double y = data->drones[i].y + cos(data->drones[i].counter * data->drones[i].speed) * data->drones[i].deviation;

		double x1, y1, x2, y2, x3, y3;
		GetDroneTriangle(game, data, i, &x1, &y1, &x2, &y2, &x3, &y3);
		al_draw_filled_triangle(x1 * game->viewport.width, y1 * game->viewport.height,
			x2 * game->viewport.width, y2 * game->viewport.height,
			x3 * game->viewport.width, y3 * game->viewport.height,
			IsSantaInDroneTriangle(game, data, i) ? al_premul_rgba(255, 168, 255, 192) : al_premul_rgba(77, 168, 255, 192));

		al_draw_rotated_bitmap(data->drone, al_get_bitmap_width(data->drone) / 2, al_get_bitmap_height(data->drone) / 2,
			game->viewport.width * x, game->viewport.height * y, 0, 0);
	}

	al_draw_rotated_bitmap(data->santa.bitmap, 115, 160,
		data->santa.x * game->viewport.width, data->santa.y * game->viewport.height,
		data->santa.rot, (fabs(fmod(data->santa.rot + ALLEGRO_PI / 2, ALLEGRO_PI * 2)) > ALLEGRO_PI) ? ALLEGRO_FLIP_VERTICAL : 0);
}

void Gamestate_ProcessEvent(struct Game* game, struct GamestateResources* data, ALLEGRO_EVENT* ev) {
	// Called for each event in Allegro event queue.
	// Here you can handle user input, expiring timers etc.
	if ((ev->type == ALLEGRO_EVENT_KEY_DOWN) && (ev->keyboard.keycode == ALLEGRO_KEY_ESCAPE)) {
		UnloadCurrentGamestate(game); // mark this gamestate to be stopped and unloaded
		// When there are no active gamestates, the engine will quit.
	}
	if ((ev->type == ALLEGRO_EVENT_KEY_DOWN) && (ev->keyboard.keycode == ALLEGRO_KEY_UP)) {
		data->keys.accelerate = true;
	}
	if ((ev->type == ALLEGRO_EVENT_KEY_UP) && (ev->keyboard.keycode == ALLEGRO_KEY_UP)) {
		data->keys.accelerate = false;
	}
	if ((ev->type == ALLEGRO_EVENT_KEY_DOWN) && (ev->keyboard.keycode == ALLEGRO_KEY_DOWN)) {
		data->keys.brake = true;
	}
	if ((ev->type == ALLEGRO_EVENT_KEY_UP) && (ev->keyboard.keycode == ALLEGRO_KEY_DOWN)) {
		data->keys.brake = false;
	}
	if ((ev->type == ALLEGRO_EVENT_KEY_DOWN) && (ev->keyboard.keycode == ALLEGRO_KEY_LEFT)) {
		data->keys.left = true;
	}
	if ((ev->type == ALLEGRO_EVENT_KEY_UP) && (ev->keyboard.keycode == ALLEGRO_KEY_LEFT)) {
		data->keys.left = false;
	}
	if ((ev->type == ALLEGRO_EVENT_KEY_DOWN) && (ev->keyboard.keycode == ALLEGRO_KEY_RIGHT)) {
		data->keys.right = true;
	}
	if ((ev->type == ALLEGRO_EVENT_KEY_UP) && (ev->keyboard.keycode == ALLEGRO_KEY_RIGHT)) {
		data->keys.right = false;
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

	data->santa.bitmap = al_load_bitmap(GetDataFilePath(game, "santa.png"));
	progress(game);

	data->drone = al_load_bitmap(GetDataFilePath(game, "drone.png"));
	progress(game);

	data->font = al_load_font(GetDataFilePath(game, "fonts/ComicMono.ttf"), 92, 0);
	progress(game);

	data->shaders.invert = CreateShader(game, GetDataFilePath(game, "shaders/vertex.glsl"), GetDataFilePath(game, "shaders/invert.glsl"));
	data->shaders.circular = CreateShader(game, GetDataFilePath(game, "shaders/vertex.glsl"), GetDataFilePath(game, "shaders/circular_gradient.glsl"));

	return data;
}

void Gamestate_Unload(struct Game* game, struct GamestateResources* data) {
	// Called when the gamestate library is being unloaded.
	// Good place for freeing all allocated memory and resources.
	al_destroy_bitmap(data->star);
	al_destroy_bitmap(data->houses);
	al_destroy_bitmap(data->santa.bitmap);
	al_destroy_bitmap(data->drone);
	DestroyShader(game, data->shaders.invert);
	DestroyShader(game, data->shaders.circular);
	al_destroy_font(data->font);
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

	data->santa.x = 0.055;
	data->santa.y = 0.7;
	data->santa.rot = -ALLEGRO_PI / 2.0;
	data->santa.speed = 0;

	data->drones[0].enabled = true;
	data->drones[0].x = 0.5;
	data->drones[0].y = 0.4;
	data->drones[0].counter = rand() / (double)RAND_MAX * ALLEGRO_PI;
	data->drones[0].angle = -ALLEGRO_PI / 2;
	data->drones[0].left = 4;
	data->drones[0].deviation = 0.005;
	data->drones[0].speed = 4;
	data->drones[0].rotspeed = 0.333;
	data->drones[0].timemin = 2;
	data->drones[0].timemax = 5;
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
