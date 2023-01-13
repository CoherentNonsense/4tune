#ifndef SONG_PLAYER_H
#define SONG_PLAYER_H

#include "pd_api.h"

struct SongPlayer {
  PlaydateAPI* pd;
  float bpm;
  float length;
  float sec_per_beat;
  FilePlayer* current_song;
  float start_tick;
  int song_start;
  float time;
  float beat_time;
  float percentage;
  float offset;
  int playing;
};

void sp_init(struct SongPlayer* player, PlaydateAPI* playdate);
void sp_load(struct SongPlayer* player, FilePlayer* song, float bpm, float offset, float length);
void sp_play(struct SongPlayer* player);
void sp_pause(struct SongPlayer* player);
void sp_stop(struct SongPlayer* player);
void sp_update(struct SongPlayer* player);

#endif