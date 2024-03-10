#if !defined(_DATA_H_)
#define _DATA_H_

#include <time.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#define FORCE_INIT 0

/*
 * TRACK:
 * -50...-25...0...25...50
 * HALF: 15
 * FULL: 25
 * DOUBLE: 35
 * Period: time for full task
 */

#define TRACK_SIZE 100
#define TRACK_HALF 25
#define TRACK_FULL 50
#define TRACK_DOUBLE 100

#define DATA_FILE_PATH "/home/owner/media/Others/task_tracker.txt"
#define TRACK_LIMIT 7
#define BUF_SIZE 256

typedef struct appdata appdata_s;

typedef struct color {
	int red;
	int green;
	int blue;
} color_s;


typedef struct track {
	time_t period; // TRACK_FULL sec minimum
	time_t last_full;
	char name[30];
	color_s style[3]; // 1-3 track color
	appdata_s *appdata;
} track_s;


typedef struct track_data {
	int track_number;
	track_s trackes[TRACK_LIMIT];
	int left_hand;
} track_data_s;


int32_t get_track_from_time(track_s *track);
time_t add_track_to_time(time_t period, time_t last_full, int32_t track_diff);
void init_data(track_data_s *track_data, appdata_s *ad);
void save_data(FILE* data_file, track_data_s *track_data);
void load_data(FILE* data_file, appdata_s *ad, track_data_s *track_data);
bool read_or_create_data(appdata_s *ad, track_data_s *track_data);

#endif // _DATA_H_
