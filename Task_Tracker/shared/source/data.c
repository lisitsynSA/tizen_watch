#include "data.h"

int32_t get_track_from_time(track_s *track)
{

	int64_t losses = (time(NULL) - track->last_full) * TRACK_SIZE / track->period;
	if (losses > TRACK_SIZE)
	{
		losses = TRACK_SIZE;
	}
	if (losses < 0)
	{
		losses = 0;
	}
	return TRACK_SIZE - losses;
}

time_t add_track_to_time(time_t period, time_t last_full, int32_t track_diff)
{
	int64_t losses = (time(NULL) - last_full) * TRACK_SIZE / period;
	if (track_diff > 0)
	{
		if (losses - track_diff < 0)
		{
			return time(NULL);
		}
		if (losses > TRACK_SIZE) // but + TRACK_diff in interval
		{
			return time(NULL) - period * (TRACK_SIZE - track_diff) / TRACK_SIZE;
		}
	}
	if (track_diff < 0)
	{
		if (losses < 0)
		{
			return time(NULL) + period * (track_diff) / TRACK_SIZE;
		}
		if (losses - track_diff > TRACK_SIZE)
		{
			return time(NULL) - period * TRACK_SIZE / TRACK_SIZE;
		}
	}
	return last_full + period * track_diff / TRACK_SIZE;
}

void init_data(track_data_s *track_data, appdata_s *ad)
{
	// watch on the left hand
	track_data->left_hand = 1;
	// simple workout task
	track_data->track_number = 1;
	strcpy(track_data->trackes[0].name, "Обзор");
	track_data->trackes[0].period = 60*60*3; // full task period
	track_data->trackes[0].last_full = time(NULL);
	track_data->trackes[0].appdata = ad;
	track_data->trackes[0].style[0].red = 255;
	track_data->trackes[0].style[0].green = 0;
	track_data->trackes[0].style[0].blue = 255;
	track_data->trackes[0].style[1].red = 255;
	track_data->trackes[0].style[1].green = 255;
	track_data->trackes[0].style[1].blue = 0;
	track_data->trackes[0].style[2].red = 0;
	track_data->trackes[0].style[2].green = 255;
	track_data->trackes[0].style[2].blue = 0;
	// food task
	track_data->track_number = 2;
	strcpy(track_data->trackes[1].name, "Треня");
	track_data->trackes[1].period = 60*60*24*7; // full task period
	track_data->trackes[1].last_full = time(NULL);
	track_data->trackes[1].appdata = ad;
	track_data->trackes[1].style[0].red = 0;
	track_data->trackes[1].style[0].green = 0;
	track_data->trackes[1].style[0].blue = 255;
	track_data->trackes[1].style[1].red = 0;
	track_data->trackes[1].style[1].green = 200;
	track_data->trackes[1].style[1].blue = 200;
	track_data->trackes[1].style[2].red = 0;
	track_data->trackes[1].style[2].green = 255;
	track_data->trackes[1].style[2].blue = 255;
	// clean task
	track_data->track_number = 3;
	strcpy(track_data->trackes[2].name, "Уборка");
	track_data->trackes[2].period = 60*60*24*7; // full task period
	track_data->trackes[2].last_full = time(NULL);
	track_data->trackes[2].appdata = ad;
	track_data->trackes[2].style[0].red = 0;
	track_data->trackes[2].style[0].green = 0;
	track_data->trackes[2].style[0].blue = 255;
	track_data->trackes[2].style[1].red = 0;
	track_data->trackes[2].style[1].green = 200;
	track_data->trackes[2].style[1].blue = 200;
	track_data->trackes[2].style[2].red = 0;
	track_data->trackes[2].style[2].green = 255;
	track_data->trackes[2].style[2].blue = 255;
	// metatask task
	track_data->track_number = 4;
	strcpy(track_data->trackes[3].name, "Пересмотр");
	track_data->trackes[3].period = 60*60*24*7; // full task period
	track_data->trackes[3].last_full = time(NULL);
	track_data->trackes[3].appdata = ad;
	track_data->trackes[3].style[0].red = 255;
	track_data->trackes[3].style[0].green = 0;
	track_data->trackes[3].style[0].blue = 255;
	track_data->trackes[3].style[1].red = 0;
	track_data->trackes[3].style[1].green = 255;
	track_data->trackes[3].style[1].blue = 255;
	track_data->trackes[3].style[2].red = 0;
	track_data->trackes[3].style[2].green = 255;
	track_data->trackes[3].style[2].blue = 0;
	// test fast task
	track_data->track_number = 5;
	strcpy(track_data->trackes[4].name, "Быстрый тест");
	track_data->trackes[4].period = TRACK_FULL; // TRACK_FULL second - full task period
	track_data->trackes[4].last_full = time(NULL);
	track_data->trackes[4].appdata = ad;
	track_data->trackes[4].style[0].red = 100;
	track_data->trackes[4].style[0].green = 100;
	track_data->trackes[4].style[0].blue = 100;
	track_data->trackes[4].style[1].red = 0;
	track_data->trackes[4].style[1].green = 200;
	track_data->trackes[4].style[1].blue = 200;
	track_data->trackes[4].style[2].red = 0;
	track_data->trackes[4].style[2].green = 255;
	track_data->trackes[4].style[2].blue = 0;
}

static void char_replace(char *string, char orig, char repl)
{
	while (*string != 0)
	{
		if (*string == orig)
			*string = repl;
		string++;
	}
}

void save_data(FILE* data_file, track_data_s *track_data)
{
	fprintf(data_file, "%d\n", track_data->track_number);
	fprintf(data_file, "%d\n", track_data->left_hand);
	int i = 0;
	for (i = 0; i < track_data->track_number; i++)
	{
		char_replace(track_data->trackes[i].name, ' ', '_');
		fprintf(data_file, "%s %lu %lu", track_data->trackes[i].name, track_data->trackes[i].period, track_data->trackes[i].last_full);
		fprintf(data_file, " %d %d %d %d %d %d %d %d %d\n",
				track_data->trackes[i].style[0].red, track_data->trackes[i].style[0].green, track_data->trackes[i].style[0].blue,
				track_data->trackes[i].style[1].red, track_data->trackes[i].style[1].green, track_data->trackes[i].style[1].blue,
				track_data->trackes[i].style[2].red, track_data->trackes[i].style[2].green, track_data->trackes[i].style[2].blue);
		char_replace(track_data->trackes[i].name, '_', ' ');
	}
}

void load_data(FILE* data_file, appdata_s *ad, track_data_s *track_data)
{
	fscanf(data_file, "%d", &(track_data->track_number));
	fscanf(data_file, "%d", &(track_data->left_hand));
	int i = 0;
	for (i = 0; i < track_data->track_number; i++)
	{
		fscanf(data_file, "%s %lu %lu", track_data->trackes[i].name, &(track_data->trackes[i].period), &(track_data->trackes[i].last_full));
		fscanf(data_file, " %d %d %d %d %d %d %d %d %d\n",
				&(track_data->trackes[i].style[0].red), &(track_data->trackes[i].style[0].green), &(track_data->trackes[i].style[0].blue),
				&(track_data->trackes[i].style[1].red), &(track_data->trackes[i].style[1].green), &(track_data->trackes[i].style[1].blue),
				&(track_data->trackes[i].style[2].red), &(track_data->trackes[i].style[2].green), &(track_data->trackes[i].style[2].blue));
		char_replace(track_data->trackes[i].name, '_', ' ');
		track_data->trackes[i].appdata = ad;
	}
}

bool read_or_create_data(appdata_s *ad, track_data_s *track_data)
{
	FILE* data_file = fopen(DATA_FILE_PATH, "r");
	if (data_file == NULL || FORCE_INIT)
	{
		data_file = fopen(DATA_FILE_PATH, "w");
		if (data_file == NULL)
		{
			return false;
		} else {
			init_data(track_data, ad);
			save_data(data_file, track_data);
			fclose(data_file);
		}
	} else {
		load_data(data_file, ad, track_data);
		fclose(data_file);
	}
	return true;
}
