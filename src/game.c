#include "game.h"

#include "drawing.h"
#include "pd_api/pd_api_gfx.h"
#include "song.h"
#include "song_player.h"
#include <stdio.h>

float disk_current_angle;

struct GameData data;

const char* fontpath = "/System/Fonts/Asheville-Sans-14-Bold.pft";

int map_select;
int map_index = 1;
float map_select_range;
char map_ids[50][100] = {"Tutorial"};

void game_setup_pd(PlaydateAPI* playdate) {
	data.playdate = playdate;
}

static void get_song(const char* filename, void* userdata) {
	strcpy(map_ids[map_index], filename);
	// strcpy(data.songs[map_index].path, filename);

	int i = 0;
	while (1) {
		if (i > 50) {
			break;
		}
		if (map_ids[map_index][i] == '/') {
			map_ids[map_index][i] = 0;
			break;
		}
		if (map_ids[map_index][i] == 0) {
			break;
		}
		i += 1;
	}
	
	map_index += 1;	
}

static void update_tutorial() {
	if (data.first_update) {
		data.first_update = 0;
		data.playdate->graphics->clear(kColorWhite);
		data.playdate->graphics->drawText("Spin the crank to spin the disk", 50, kASCIIEncoding, 5, 25);
		data.playdate->graphics->drawText("Allign the disk color with the incoming notes", 50, kASCIIEncoding, 5, 55);
		data.playdate->graphics->drawEllipse(5, 85, 16, 16, 2, 0.0f, 0.0f, kColorBlack);
		data.playdate->graphics->fillEllipse(8, 88, 10, 10, 0.0f, 0.0f, kColorBlack);
		data.playdate->graphics->drawText("Click any button when collidoing with disk", 50, kASCIIEncoding, 25, 85);
		data.playdate->graphics->drawBitmap(data.black_x_bitmap, 5, 115, kBitmapUnflipped);
		data.playdate->graphics->drawText("Don't let X's touch the same colour", 50, kASCIIEncoding, 30, 115);
		data.playdate->graphics->drawText("Click any button to continue...", 50, kASCIIEncoding, 5, 145);
	}
	
	PDButtons buttons;
	data.playdate->system->getButtonState(NULL, &buttons, NULL);
	if (buttons > 0) {
		data.state = GAME_STATE_SONG_LIST;
		data.first_update = 1;
	}
}

typedef struct MenuNote {
	float x;
	uint8_t y;
	uint8_t type;
	uint8_t color;
	uint8_t sin_offset;
} MenuNote;
MenuNote menu_notes[15];
int rng_y[12] = {50, 70, 200, 210, 140, 190, 100, 8, 120, 30, 180, 120};
int rng_type[5] = {0, 1, 0, 2, 0};
float sin_vals[11] = {0, 2, 4, 4, 2, 0, -3, -5, -5, -3, 0};

static void update_main_menu() {
	if (data.first_update) {
		data.playdate->sound->fileplayer->loadIntoPlayer(data.fileplayer, "audio/menu");
		data.playdate->sound->fileplayer->play(data.fileplayer, 0);

		data.first_update = 0;

		for (int i = 0; i < 15; ++i) {
			MenuNote* note = &menu_notes[i];
			note->y = (i * 2) % 12;
			note->type = (i * 3) % 5;
			note->color = (i * 7) % 2;
			note->x = i * 50;
			note->sin_offset = i * 3;
		}
	}

	data.playdate->graphics->clear(kColorWhite);
	for (int i = 0; i < 15; ++i) {
		MenuNote* note = &menu_notes[i];
		note->x = (note->x + ((float)rng_y[note->y] / 80.0f) + 1.5f);
		if (note->x > 500) {
			note->x = 0;
			note->y = (note->y + 1) % 12;
			note->type = (note->type + 1) % 5;
			note->color = (note->color + 1) % 2;
		}
		draw_note(&data, 420 - (int)note->x, rng_y[note->y] + sin_vals[((data.frame + note->sin_offset) / 10) % 11], rng_type[note->type], note->color);
	}	
	
	data.playdate->graphics->setDrawMode(kDrawModeNXOR);
	data.playdate->graphics->drawBitmap(data.title_bitmaps[(data.frame / 10) % 2], 200 - (196 / 2), 50, kBitmapUnflipped);
	data.playdate->graphics->setDrawMode(kDrawModeCopy);
	int left = 200 - (data.playdate->graphics->getTextWidth(data.basic_font, "CRANK TO START", 16, kASCIIEncoding, 0) / 2);
	int top = 230 - data.playdate->graphics->getFontHeight(data.basic_font) - ((data.frame / 20) % 2) * 2;
	data.playdate->graphics->setFont(data.basic_font);
	data.playdate->graphics->drawText("CRANK TO START", 16, kASCIIEncoding, left, top);
	data.playdate->graphics->setFont(data.font);
		
  PDButtons buttons;
  data.playdate->system->getButtonState(NULL, &buttons, NULL);
	
	if (buttons > 0) {
		data.playdate->sound->sampleplayer->play(data.sound_effect, 1, 1.0);
		data.state = GAME_STATE_SONG_LIST;
		data.first_update = 1;
	}
}

static void update_song_list() {
	float crank_angle = data.playdate->system->getCrankAngle();
	
	if (data.first_update) {
		data.first_update = 0;
		map_select_range = crank_angle;
	}
	
  PDButtons pressed;
  data.playdate->system->getButtonState(NULL, &pressed, NULL);
	
	int prev = map_select;
	
	// scroll with crank
	int scrolled_with_crank = 0;
	float crank_dist = map_select_range + 37 - crank_angle;
	if (crank_dist < 0.0f) {
		crank_dist += 360.0f;
	}
	if (crank_dist > 37.0f * 2.0f) {
		if (data.playdate->system->getCrankChange() > 0) {		
			map_select += 1;
			if (map_select == map_index) {
				map_select = map_index - 1;
			}
		} else {
			map_select -= 1;
			if (map_select < 0) {
				map_select = 0;
			}
		}
		scrolled_with_crank = 1;
		map_select_range = crank_angle;
	}
	
	data.playdate->graphics->clear(kColorWhite);
	int points[8] = {0, 0, 150, 0, 130, 240, 0, 240};
	data.playdate->graphics->fillPolygon(4, points, kColorBlack, kPolygonFillNonZero);
	data.playdate->graphics->setDrawMode(kDrawModeNXOR);
	for (int i = 0; i < map_index; ++i) {
		data.playdate->graphics->drawText(map_ids[i], 100, kASCIIEncoding, 13, 20 + 30 * i);
	}
	data.playdate->graphics->setDrawMode(kDrawModeCopy);

	int length = 150;
	data.playdate->graphics->fillRect(
		0, 19 + 30 * map_select,
		length + 15, 30,
		kColorXOR
	);
	
	data.playdate->graphics->fillEllipse(length, 19 + 30 * map_select, 30, 30, 0.0f, 360.0f, kColorBlack);
		
	if (pressed & kButtonDown) {
		map_select += 1;
		if (map_select == map_index) {
			map_select = map_index - 1;
		}
	}
	if (pressed & kButtonUp) {
		map_select -= 1;
		if (map_select < 0) {
			map_select = 0;
		}
	}
	if (pressed & kButtonA) {
		if (map_select == 0) {
			data.state = GAME_STATE_TUTORIAL;
			data.first_update = 1;
			return;
		}
		song_open(&data, &data.song_player, map_ids[map_select]);
		data.playdate->sound->fileplayer->stop(data.fileplayer);
		data.state = GAME_STATE_SONG;
		data.first_update = 1;
	}
	if (pressed & kButtonB) {
		data.state = GAME_STATE_MAIN_MENU;
		data.first_update = 1;
	}
}

void update_song() {						
	sp_update(&data, &data.song_player);
	song_update(&data);
	song_draw(&data);
}

static void menu_home_callback(void* userdata) {
	data.state = GAME_STATE_SONG_LIST;
	data.first_update = 1;
	song_close(&data);
}

static void menu_debug_callback(void* userdata) {
	data.debug = data.playdate->system->getMenuItemValue(data.debug_menu);
	data.playdate->system->logToConsole("%d", data.debug);
}

void game_init() {
	song_set_data_ptr(&data);
		
	data.playdate->system->addMenuItem("Quit Song", menu_home_callback, NULL);
	data.debug_menu = data.playdate->system->addCheckmarkMenuItem("Debug", 0, menu_debug_callback, NULL);
	
	data.state = GAME_STATE_MAIN_MENU;
	data.first_update = 1;

	data.fileplayer = data.playdate->sound->fileplayer->newPlayer();
	data.sound_effect = data.playdate->sound->sampleplayer->newPlayer();
	// HACK: this never frees the memory for start.wav
	data.playdate->sound->sampleplayer->setSample(data.sound_effect, data.playdate->sound->sample->load("audio/start"));

	const char* err;
	data.font = data.playdate->graphics->loadFont(fontpath, &err);
	data.basic_font = data.playdate->graphics->loadFont("fonts/Basic.pft", &err);
	data.playdate->graphics->setFont(data.font);

	data.title_bitmaps[0] = data.playdate->graphics->loadBitmap("images/title1.png", &err);
	data.title_bitmaps[1] = data.playdate->graphics->loadBitmap("images/title2.png", &err);
	data.black_x_bitmap = data.playdate->graphics->loadBitmap("x.png", &err);
	data.white_x_bitmap = data.playdate->graphics->loadBitmap("white_x.png", &err);
	data.bg_tile_bitmap = data.playdate->graphics->loadBitmap("bg-tiles.png", &err);
	data.light_grey_bitmap = data.playdate->graphics->loadBitmap("light-grey.png", &err);
	data.clear_bitmap = data.playdate->graphics->loadBitmap("clear.png", &err);
			
	data.playdate->file->listfiles("songs", get_song, NULL, 0);
  
	// song_open(pd, &song_player, "song.4t");
	data.playdate->display->setRefreshRate(50);
}

void game_update() {
	// data.playdate->graphics->clear(kColorWhite);
  
	switch (data.state) {
		case GAME_STATE_MAIN_MENU:
			update_main_menu();
			break;
		case GAME_STATE_SONG_LIST:
			update_song_list();
			break;
		case GAME_STATE_SONG:
			if (data.first_update) {			
				data.playdate->graphics->clear(kColorWhite);
				data.first_update = 0;
			}
			update_song();
			break;
		case GAME_STATE_TUTORIAL:
			update_tutorial();
			break;
		default:
			break;
	}
	
	data.frame += 1;

	if (data.debug) {
		float width = 100;
		float rows = 3;
		data.playdate->system->drawFPS(0, 0);

		data.playdate->graphics->fillRect(400 - width, rows * 18 + 4, width, 240 - (rows * 18 + 4), kColorWhite);
		data.playdate->graphics->drawRect(400 - width, rows * 18 + 4, width, 240 - (rows * 18 + 4), kColorBlack);

		for (int i = data.debug_log_start; i != data.debug_log_end; i = (i + 1) % 10) {
			data.playdate->graphics->drawText(data.debug_log[i], 15, kASCIIEncoding, 400 - width + 2, (rows * 18 + 4) + (18 * ((i - data.debug_log_start + 10) % 10)));
		}

		char buffer[20];
		data.playdate->graphics->fillRect(400 - width, 0, width, rows * 18 + 4, kColorWhite);
		data.playdate->graphics->drawRect(400 - width, 0, width, rows * 18 + 4, kColorBlack);
		if (data.song_player.beat_time > 0.0f) {		
			sprintf(buffer, "time: %d.%d", (int)data.song_player.beat_time, (int)((data.song_player.beat_time - (int)data.song_player.beat_time) * 10.0f + 0.5f));
		} else {
			sprintf(buffer, "time: 0.0");
		}
		data.playdate->graphics->drawText(buffer, 20, kASCIIEncoding, 400 - width + 2, 2 + 18 * 0);
		
		sprintf(buffer, "next: %d", (int)data.debug_next_note);
		data.playdate->graphics->drawText(buffer, 20, kASCIIEncoding, 400 - width + 2, 2 + 18 * 1);

		sprintf(buffer, "pos: %d", data.debug_next_note_position);
		data.playdate->graphics->drawText(buffer, 20, kASCIIEncoding, 400 - width + 2, 2 + 18 * 2);
	}
}

void game_change_state(game_state state) {
	data.state = state;
}

void debug_log(const char* msg) {
	int i = 0;
	for (; i < 14; ++i) {
		if (msg[i] == '\n') {
			break;
		}
		data.debug_log[data.debug_log_end][i] = msg[i];
	}
	data.debug_log[data.debug_log_end][i] = 0;

	data.debug_log_end = (data.debug_log_end + 1) % 10;
	if (data.debug_log_end == data.debug_log_start) {
		data.debug_log_start += 1;
	}
}
