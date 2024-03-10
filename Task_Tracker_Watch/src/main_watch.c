#include <tizen.h>
#include <cairo.h>
#include <math.h>
#include <device/battery.h>
#include <device/callback.h>
#include <curl/curl.h>
#include <main_watch.h>
#include <net_connection.h>
#include <string.h>

typedef struct weather {
	uint64_t dt;
	int temp;
	int index;
} weather_s;

typedef struct appdata {

	/* Variables for basic UI contents */
	Evas_Object *win;
	Evas_Object *img;
	Evas_Object *timeLabel;
	Evas_Object *button_app;

	/* Variables for watch size information */
	int width;
	int height;

	/* Weather */
	weather_s weather_data[WEATHER_SIZE];
	cairo_surface_t* weather_icons[WEATHER_ICON_NUMBER];
	time_t update_weather_time;

	/* Variables for cairo image backend contents */
	cairo_t *cairo;
	cairo_surface_t *surface;
	unsigned char *pixels;
	int reload_data;

	track_data_s track_data;
} appdata_s;

char *month_name[12] = { "Январь", "Февраль", "Март", "Апрель",
						 "Май", "Июнь", "Июль", "Август",
						 "Сентябрь", "Октябрь", "Ноябрь", "Декабрь" };
int month_lenght[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
char *week_name[8] = { "Воскресенье", "Понедельник", "Вторник", "Среда", "Четверг", "Пятница", "Суббота", "Воскресенье"};
char *short_week_name[8] = { "Вc", "Пн", "Вт", "Ср", "Чт", "Пт", "Сб", "Вc" };


static void
draw_calendar(cairo_t *cairo, double x, double y, int day, int week, int month, int year)
{
	if (week == 0) {
		week = 7;
	}
	int first_week = 7 + (week - 7 - (day - 1)) % 7;
	char* week_buf = NULL;
	char day_buf[] = "00";
	cairo_text_extents_t extents;
	double y_start = y/1.8;

	char info[128] = {};
	sprintf(info, "%s %d", month_name[month], year);
	int info_len = 0;
	while (info[info_len] != 0) {
		info_len++;
	}
	cairo_set_source_rgba(cairo, 1.0, 1.0, 1.0, 1.0);
	cairo_text_extents(cairo, info, &extents);
	cairo_set_font_size(cairo, (FONT + 1));
	cairo_move_to(cairo, x - (info_len * (FONT + 1) / 6.0), y_start - FONT*1.4);
	cairo_show_text(cairo, info);
	cairo_stroke(cairo);
	int curr_row = 0;
	int month_len = month_lenght[month] + ((month == 1) && (year%4 == 0));
	int last_row = 0;

	for (int i = 1; i <= 7; i++)
	{
		if (i > 5) {
			cairo_set_source_rgba(cairo, 1.0, 0.7, 0.7, 1.0);
		} else {
			cairo_set_source_rgba(cairo, 1.0, 1.0, 1.0, 1.0);
		}
		if (i == week) {
			cairo_set_source_rgba(cairo, 0.7, 1.0, 0.7, 1.0);
		}
		week_buf = short_week_name[i];
		cairo_text_extents(cairo, week_buf, &extents);
		cairo_set_font_size(cairo, FONT + 3*(i == week));
		cairo_move_to(cairo, x/2 + FONT/2 + x*(i - 1)/7 - FONT*(i == week)/5, y_start);
		cairo_show_text(cairo, week_buf);
		cairo_stroke(cairo);
		if (i > 5) {
			cairo_set_source_rgba(cairo, 1.0, 0.7, 0.7, 1.0);
		} else {
			cairo_set_source_rgba(cairo, 1.0, 1.0, 1.0, 1.0);
		}
		for (int row = 1; row <= 6; row++) {
			int printed_day = i + (row - 1) * 7 - first_week + 1;
			if (0 < printed_day && printed_day <= month_len) {
				if (printed_day == day) {
					cairo_set_source_rgba(cairo, 0.7, 1.0, 0.7, 1.0);
					curr_row = row;
				}
				sprintf(day_buf, "%2d", printed_day);
				cairo_text_extents(cairo, day_buf, &extents);
				cairo_set_font_size(cairo, FONT);
				cairo_move_to(cairo, x/2 + FONT/2 + x*(i - 1)/7, y_start + row * FONT + FONT / 2);
				cairo_show_text(cairo, day_buf);
				cairo_stroke(cairo);
				if (printed_day == day) {
					cairo_set_line_width(cairo, 2);
					cairo_rectangle(cairo, x/2 + FONT/3 + x*(i - 1)/7, y_start + row * FONT - FONT/2.5 , FONT*1.3, FONT);
					cairo_stroke(cairo);
					if (i > 5) {
						cairo_set_source_rgba(cairo, 1.0, 0.7, 0.7, 1.0);
					} else {
						cairo_set_source_rgba(cairo, 1.0, 1.0, 1.0, 1.0);
					}
				}
				if (last_row < row)
					last_row = row;
			}
		}
	}
	int curr_thursday = day + 4 - week;
	int days_from_year_start = 0;
	for (int i = 0; i < month; i++) {
		days_from_year_start += month_lenght[i];
	}
	days_from_year_start += ((month > 1) && (year % 4 == 0));
	int curr_week = (days_from_year_start + curr_thursday + 6)/7;

	for (int row = 1; row <= last_row; row++) {
		int week_number = curr_week - curr_row + row;
		cairo_set_font_size(cairo, FONT - 5 + 4*(row == curr_row));
		cairo_set_source_rgba(cairo, 0.7, 1.0, 0.7, 1.0);
		sprintf(day_buf, "%2d", week_number);
		cairo_text_extents(cairo, day_buf, &extents);
		cairo_move_to(cairo, x/2 + FONT/2 + x, y_start + row * FONT + 0.4*FONT + (row == curr_row)*0.1*FONT);
		cairo_show_text(cairo, day_buf);
		cairo_stroke(cairo);
	}
}

static void
draw_track(cairo_t *cairo, double x, double y, double rad, int track, color_s style[3], int left)
{
	int i = 0;
	double alpha = 0.0;
	if (left)
	{
		// M_PI/2 -> 3*M_PI/2
		for (i = 0; i < TRACK_SIZE/2; ++i)
		{
			alpha = 0.3 + 0.7*(i < track);
			cairo_set_source_rgba(cairo,
					alpha*2*(style[0].red*(TRACK_SIZE/2 - i) + style[1].red*i)/(TRACK_SIZE*255.0) +
						((track < TRACK_HALF) ? (TRACK_HALF - track)/(1.0*TRACK_HALF) : 0),
					alpha*2*(style[0].green*(TRACK_SIZE/2 - i) + style[1].green*i)/(TRACK_SIZE*255.0),
					alpha*2*(style[0].blue*(TRACK_SIZE/2 - i) + style[1].blue*i)/(TRACK_SIZE*255.0),
					(i < 49*TRACK_SIZE/100) ? 1.0 : 0.0);
			cairo_arc(cairo, x, y, rad, M_PI/2 + ANGLE_STEP*(i + BLACK_ANGLE), M_PI/2 + ANGLE_STEP*(i + 1 + BLACK_ANGLE));
			cairo_stroke(cairo);
		}
		for (i = TRACK_SIZE/2; i < TRACK_SIZE; ++i)
		{
			alpha = 0.3 + 0.7*(i < track);
			cairo_set_source_rgba(cairo,
					alpha*2*(style[1].red*(TRACK_SIZE - i) + style[2].red*(i - TRACK_SIZE/2))/(TRACK_SIZE*255.0) +
					((track < TRACK_HALF) ? (TRACK_HALF - track)/(1.0*TRACK_HALF) : 0),
					alpha*2*(style[1].green*(TRACK_SIZE - i) + style[2].green*(i - TRACK_SIZE/2))/(TRACK_SIZE*255.0),
					alpha*2*(style[1].blue*(TRACK_SIZE - i) + style[2].blue*(i - TRACK_SIZE/2))/(TRACK_SIZE*255.0),
					(i > 52*TRACK_SIZE/100) ? 1.0 : 0.0);
			cairo_arc(cairo, x, y, rad, M_PI/2 + ANGLE_STEP*(i + BLACK_ANGLE), M_PI/2 + ANGLE_STEP*(i + 1 + BLACK_ANGLE));
			cairo_stroke(cairo);
		}
	} else {
		// -M_PI/2 -> M_PI/2
		for (i = 0; i < TRACK_SIZE/2; ++i)
		{
			alpha = 0.3 + 0.7*(i < track);
			cairo_set_source_rgba(cairo,
					alpha*2*(style[0].red*(TRACK_SIZE/2 - i) + style[1].red*i)/(TRACK_SIZE*255.0) +
					((track < TRACK_HALF) ? (TRACK_HALF - track)/(1.0*TRACK_HALF) : 0),
					alpha*2*(style[0].green*(TRACK_SIZE/2 - i) + style[1].green*i)/(TRACK_SIZE*255.0),
					alpha*2*(style[0].blue*(TRACK_SIZE/2 - i) + style[1].blue*i)/(TRACK_SIZE*255.0),
					1.0);
			cairo_arc(cairo, x, y, rad, M_PI/2 - ANGLE_STEP*(i + 1 + BLACK_ANGLE), M_PI/2 - ANGLE_STEP*(i + BLACK_ANGLE));
			cairo_stroke(cairo);
		}
		for (i = TRACK_SIZE/2; i < TRACK_SIZE; ++i)
		{
			alpha = 0.3 + 0.7*(i < track);
			cairo_set_source_rgba(cairo,
					alpha*2*(style[1].red*(TRACK_SIZE - i) + style[2].red*(i - TRACK_SIZE/2))/(TRACK_SIZE*255.0) +
					((track < TRACK_HALF) ? (TRACK_HALF - track)/(1.0*TRACK_HALF) : 0),
					alpha*2*(style[1].green*(TRACK_SIZE - i) + style[2].green*(i - TRACK_SIZE/2))/(TRACK_SIZE*255.0),
					alpha*2*(style[1].blue*(TRACK_SIZE - i) + style[2].blue*(i - TRACK_SIZE/2))/(TRACK_SIZE*255.0),
					1.0);
			cairo_arc(cairo, x, y, rad, M_PI/2 - ANGLE_STEP*(i + 1 + BLACK_ANGLE), M_PI/2 - ANGLE_STEP*(i + BLACK_ANGLE));
			cairo_stroke(cairo);
		}
	}
}

static void
draw_battery(cairo_t *cairo, double x, double y, double rad, int battery)
{
	double alpha = 0.3;
	// 5*M_PI/4 ->  3*M_PI/2 -> 7*M_PI/4
	double shift_angle = battery * M_PI/400.0;

	if (battery < 50) {
		cairo_set_source_rgba(cairo,
				alpha*(1.0),
				alpha*(battery / 50.0),
				0.0, 1.0);
		cairo_arc(cairo, x, y, rad, 5*M_PI/4, 7*M_PI/4);
		cairo_stroke(cairo);

		cairo_set_source_rgba(cairo,
				1.0,
				battery / 50.0,
				0.0, 1.0);
		cairo_arc(cairo, x, y, rad, 3*M_PI/2 - shift_angle, 3*M_PI/2 + shift_angle);
		cairo_stroke(cairo);
	} else {
		cairo_set_source_rgba(cairo,
				alpha*(2.0 - battery / 50.0),
				alpha*(1.0),
				0.0, 1.0);
		cairo_arc(cairo, x, y, rad, 5*M_PI/4, 7*M_PI/4);
		cairo_stroke(cairo);

		cairo_set_source_rgba(cairo,
				2.0 - battery / 50.0,
				1.0,
				0.0, 1.0);
		cairo_arc(cairo, x, y, rad, 3*M_PI/2 - shift_angle, 3*M_PI/2 + shift_angle);
		cairo_stroke(cairo);
	}

	cairo_set_source_rgba(cairo, 1.0, 1.0, 1.0, 1.0);
	char battery_string[] = "100%";
	sprintf(battery_string, "%d%%", battery);
	cairo_text_extents_t extents;
	cairo_text_extents(cairo, battery_string, &extents);
	cairo_set_font_size(cairo, FONT);
	cairo_move_to(cairo, x - FONT, y/3);
	cairo_show_text(cairo, battery_string);
	cairo_stroke(cairo);
}

static void
draw_name(cairo_t *cairo, double x, double y, double rad, char* name, char* rest_time, int left)
{
	int len = 0;
	char* cur = name;
	while (*cur != 0)
	{
		len++;
		cur += 2;
	}
	char word[] = "**";
	int i = 0;
	cairo_select_font_face(cairo, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
	cairo_set_font_size(cairo, FONT);
	cairo_text_extents_t extents;
	for (i = 0; i < len; i++)
	{
		cairo_text_extents(cairo, word, &extents);
		if (left)
		{
			word[0] = name[i * 2];
			word[1] = name[i * 2 + 1];
			cairo_move_to(cairo, x + rad*sin(M_PI + (len - i + 2)*WORD_ANGLE_STEP + BLACK_ANGLE*ANGLE_STEP) - FONT/4,
								 y + rad*cos((len - i + 2)*WORD_ANGLE_STEP + BLACK_ANGLE*ANGLE_STEP));
		} else {
			word[0] = name[(len - i - 1) * 2];
			word[1] = name[(len - i - 1) * 2 + 1];
			cairo_move_to(cairo, x - rad*sin(M_PI + (len - i + 2)*WORD_ANGLE_STEP + BLACK_ANGLE*ANGLE_STEP) - FONT/3,
								 y + rad*cos((len - i + 2)*WORD_ANGLE_STEP + BLACK_ANGLE*ANGLE_STEP));
		}
		cairo_show_text(cairo, word);
		cairo_stroke(cairo);
	}
	cairo_text_extents(cairo, rest_time, &extents);
	if (left)
	{
		cairo_move_to(cairo, x + rad*sin(M_PI + WORD_ANGLE_STEP + BLACK_ANGLE*ANGLE_STEP) - 2*FONT/3,
							 y + rad*cos(WORD_ANGLE_STEP + BLACK_ANGLE*ANGLE_STEP));
	} else {
		cairo_move_to(cairo, x - rad*sin(M_PI + BLACK_ANGLE*ANGLE_STEP) - FONT/3,
							 y + rad*cos(BLACK_ANGLE*ANGLE_STEP));
	}
	cairo_show_text(cairo, rest_time);
	cairo_stroke(cairo);


}

static void
print_rest_time(char rest_time[16], track_s* track)
{
	time_t rest = track->last_full + track->period - time(NULL);
	if (rest <= 0) {
		sprintf(rest_time, "!!!");
	} else {
		int d = rest/(3600*24);
		if (d > 0) {
			sprintf(rest_time, "%02dd", d);
		} else {
			int h = rest/3600;
			if (h > 0) {
				sprintf(rest_time, "%02dh", h);
			} else {
				int m = rest/60;
				if (m > 0) {
					sprintf(rest_time, "%02dm", m);
				} else {
					sprintf(rest_time, "%02lds", rest);
				}
			}
		}
	}
}

struct MemoryStruct {
  char *memory;
  size_t size;
};
struct MemoryStruct chunk;


static size_t
WriteMemoryCb(void *contents, size_t size, size_t nmemb, void *userp)
{
    dlog_print(DLOG_INFO, LOG_TAG, "[WriteMemoryCallback]");
	size_t realsize = size * nmemb;
	struct MemoryStruct *mem = (struct MemoryStruct *)userp;

	mem->memory = realloc(mem->memory, mem->size + realsize + 1);
	if(mem->memory == NULL) {
		return 0;
	}

	memcpy(&(mem->memory[mem->size]), contents, realsize);
	mem->size += realsize;
	mem->memory[mem->size] = 0;

	return realsize;
}

int index2icon_arr[WEATHER_ICON_NUMBER] =
					{1,  2,  3,  4,  9,  10,  11,  13,  50,
					-1, -2, -3, -4, -9, -10, -11, -13, -50};

static int
icon2index(int icon) {
	int index = 0;
	while (index2icon_arr[index] != icon)
		index++;
	return index;
}

static int
index2icon(int index) {
	return index2icon_arr[index];
}

static void
get_weather(void *data)
{
	//dlog_print(DLOG_INFO, LOG_TAG, "[CURL] Get weather...");
	appdata_s *ad = data;
	CURL *curl;

	chunk.memory = malloc(1);
	chunk.size = 0;

	/* Initialize the curl session */
	//dlog_print(DLOG_INFO, LOG_TAG, "[CURL] curl_easy_init...");
	curl = curl_easy_init();

	/* Download the header */
	if (!curl) {
		/* Display failure message */
		//dlog_print(DLOG_ERROR, LOG_TAG, "[CURL] curl empty");
		//ad->weather_data[0].temp = -90;
		return;
	}

	/* Set URL to get */
	CURLcode error_code = curl_easy_setopt(curl, CURLOPT_URL, "http://api.openweathermap.org/data/2.5/forecast?q=Moscow,RU&lang=RU&appid=23419869bf7b6bd28c71d2228f429d56");

	/* Verify the SSL certificate */
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);

	/* Verify the host name in the SSL certificate */
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

	/* Follow HTTP 3xx redirects */
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

	/* Callback for writing data */
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCb);

	/* Data pointer to pass to the write callback */
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

	/* Timeout for the entire request */
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);

	/* Switch off the progress meter */
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);

	/* Perform a blocking file transfer */
	dlog_print(DLOG_INFO, LOG_TAG, "[CURL] curl_easy_perform...");
	error_code = curl_easy_perform(curl);
	dlog_print(DLOG_INFO, LOG_TAG, "[CURL] curl_easy_perform(curl): %s (%d)",
			   curl_easy_strerror(error_code), error_code);
	if (error_code == CURLE_ABORTED_BY_CALLBACK) {
		/* Clean up and display cancel message */
		//dlog_print(DLOG_ERROR, LOG_TAG, "[CURL] error_code");
		//ad->weather_data[0].temp = -80;
		return;
	}
	else if (error_code != CURLE_OK) {
		/* Display failure message */
		//dlog_print(DLOG_ERROR, LOG_TAG, "[CURL] curl_easy_perform ERROR");
		//ad->weather_data[0].temp= -70;
		return;
	}

	//dlog_print(DLOG_INFO, LOG_TAG, "[CURL] curl_easy_cleanup(ad->curl)");
	curl_easy_cleanup(curl);

	// DECODE

	ad->update_weather_time = time(NULL);

	double temp = 0;
	int icon = 0;
	char* cur = chunk.memory;

	for (int i = 0; i < WEATHER_SIZE; i++) {
		// "dt":1621674000
		// 0123456789
		cur = strstr(cur, "\"dt\"");
		cur += 5;
		sscanf(cur, "%llu", &(ad->weather_data[i].dt));

		// "temp":270.81
		// 0123456789
		cur = strstr(cur, "\"temp\"");
		cur += 7;
		sscanf(cur, "%lf", &temp);
		ad->weather_data[i].temp = temp - 273.15;

		// "icon":"01n"
		// 01234567890
		cur = strstr(cur, "\"icon\"");
		cur += 8;
		sscanf(cur, "%d", &icon);
		if (cur[2] == 'n') {
			icon = -icon;
		}
		ad->weather_data[i].index = icon2index(icon);
		//dlog_print(DLOG_INFO, LOG_TAG, "[TEMP] DT %llu TEMP: %d ICON: %d)",
		//		ad->weather_data[i].dt, ad->weather_data[i].temp, ad->weather_data[i].index);
	}

	free(chunk.memory);
	return;
}

static void
weather_thread_run_cb(void *data, Ecore_Thread *thread)
{
	get_weather(data);

    /*
       ecore_thread_feedback() invokes download_feedback_cb()
       registered by ecore_thread_feedback_run()
    */
    ecore_thread_feedback(thread, data);
}

static void
weather_feedback_cb(void *data, Ecore_Thread *thread, void *msg_data)
{
    if (msg_data == NULL) {
        dlog_print(DLOG_ERROR, LOG_TAG, "[WEATHER THREAD] msg_data is NULL");

        return;
    }
}

static void
weather_thread_end_cb(void *data, Ecore_Thread *thread)
{
    dlog_print(DLOG_INFO, LOG_TAG, "[WEATHER THREAD] thread end!");
}

static void
weather_thread_cancel_cb(void *data, Ecore_Thread *thread)
{
    dlog_print(DLOG_ERROR, LOG_TAG, "[WEATHER THREAD] thread cancel!");
}

static Eina_Bool
start_weather(void *data)
{
    appdata_s *ad = data;
    // one hour update
    if (time(NULL) - ad->update_weather_time < 60*60) {
    	return EINA_TRUE;
    }
    Ecore_Thread *thread;

    /* Create a thread that communicates with the main thread */
    thread = ecore_thread_feedback_run(weather_thread_run_cb, weather_feedback_cb,
    								   weather_thread_end_cb, weather_thread_cancel_cb,
                                       ad, EINA_FALSE);
	return EINA_TRUE;
}

static int
get_cur_weather(weather_s *weather_data, uint64_t time)
{
	for (int i = 0; i < WEATHER_SIZE; i++) {
		if (time <= weather_data[i].dt)
			return i;
	}
	return -1;
}

static void
draw_weather(cairo_t *cairo, double x, double y, weather_s *weather_data, int cur_weather, int hour24, time_t update_time)
{
	char weather_text[BUF_SIZE];
	cairo_set_source_rgba(cairo, 1.0, 1.0, 1.0, 1.0);
	cairo_text_extents_t extents;
	cairo_set_font_size(cairo, FONT + 1);

	sprintf(weather_text, "%+d °C", weather_data[cur_weather].temp);
	cairo_text_extents(cairo, weather_text, &extents);
	cairo_move_to(cairo, x, 1.5*y);
	cairo_show_text(cairo, weather_text);
	cairo_stroke(cairo);

	unsigned weather_hour = (hour24 + (weather_data[cur_weather].dt/3600) - (time(NULL)/3600)) % 24;

	cairo_set_font_size(cairo, FONT - 6);
	sprintf(weather_text, "%02u", weather_hour);
	cairo_text_extents(cairo, weather_text, &extents);
	cairo_move_to(cairo, 0.98*x, 1.43*y);
	cairo_show_text(cairo, weather_text);
	cairo_stroke(cairo);

	time_t last = time(NULL) - update_time;
	if (last <= 0) {
		sprintf(weather_text, "UPD");
	} else {
		int d = last/(3600*24);
		if (d > 0) {
			sprintf(weather_text, "%02dd", d);
		} else {
			int h = last/3600;
			if (h > 0) {
				sprintf(weather_text, "%02dh", h);
			} else {
				int m = last/60;
				if (m > 0) {
					sprintf(weather_text, "%02dm", m);
				} else {
					sprintf(weather_text, "%02lds", last);
				}
			}
		}
	}
	cairo_text_extents(cairo, weather_text, &extents);
	cairo_move_to(cairo, 0.98*x, 1.55*y);
	cairo_show_text(cairo, weather_text);
	cairo_stroke(cairo);
	cairo_set_font_size(cairo, FONT + 1);

	if (cur_weather <= WEATHER_SIZE - 8) {
		int weather_min = 100;
		int weather_max = -100;
		for (int i = 0; i < 8; i++) {
			if (weather_data[cur_weather + i].temp > weather_max)
				weather_max = weather_data[cur_weather + i].temp;
			if (weather_data[cur_weather + i].temp < weather_min)
				weather_min = weather_data[cur_weather + i].temp;
		}
		int rain = 0;
		for (int i = 0; i < 4; i++) {
			if (weather_data[cur_weather + i].index == 5 || weather_data[cur_weather + i].index == 14)
				rain += 1;
			if (weather_data[cur_weather + i].index == 6 || weather_data[cur_weather + i].index == 15)
				rain += 2;
		}

		double rain_step = y*0.02;
		cairo_set_line_width(cairo, 5);
		cairo_set_source_rgba(cairo, 1.0, 0.0, 0.0, 0.5);
		cairo_move_to(cairo, 0.75*x, 1.525*y);
		cairo_line_to(cairo, 0.75*x, 1.525*y - rain_step*8);
		cairo_close_path(cairo);
		cairo_stroke(cairo);
		if (rain > 0) {
			cairo_set_source_rgba(cairo, 0.0, 0.0, 1.0, 1.0);
			cairo_move_to(cairo, 0.75*x, 1.525*y);
			cairo_line_to(cairo, 0.75*x, 1.525*y - rain*rain_step);
			cairo_close_path(cairo);
			cairo_stroke(cairo);
		}
		cairo_set_source_rgba(cairo, 1.0, 1.0, 1.0, 1.0);


		sprintf(weather_text, "%+d °C", weather_min);
		cairo_text_extents(cairo, weather_text, &extents);
		cairo_move_to(cairo, 0.3*x, 1.4*y);
		cairo_rotate(cairo, M_PI_4);
		cairo_show_text(cairo, weather_text);
		cairo_stroke(cairo);
		cairo_rotate(cairo, -M_PI_4);

		sprintf(weather_text, "%+d °C", weather_max);
		cairo_text_extents(cairo, weather_text, &extents);
		cairo_move_to(cairo, 1.5*x, 1.6*y);
		cairo_rotate(cairo, -M_PI_4);
		cairo_show_text(cairo, weather_text);
		cairo_stroke(cairo);
		cairo_rotate(cairo, M_PI_4);
	}

}

/*
 * @brief Update a watch face screen
 * @param[in] ad application's data structure
 * @param[in] watch_time current time information for update
 * @param[in] ambient ambient mode status
 */
static void
update_watch(appdata_s *ad, watch_time_h watch_time, int ambient)
{
	/* Variables for time */
	char watch_text[BUF_SIZE];
	int hour24, minute, second, day, week, month, year;
	double x, y;
	x = ad->width/2;
	y = ad->height/2;

	if (watch_time == NULL)
		return;

	if (ad->reload_data > 0)
	{
		if (!read_or_create_data(ad, &ad->track_data)) {
			dlog_print(DLOG_ERROR, LOG_TAG, "Can't create data");
		}
		ad->reload_data--;
	}

	/* Get current time */
	watch_time_get_hour24(watch_time, &hour24);
	watch_time_get_minute(watch_time, &minute);
	watch_time_get_second(watch_time, &second);
	watch_time_get_day(watch_time, &day);
	watch_time_get_day_of_week(watch_time, &week);
	watch_time_get_month(watch_time, &month);
	watch_time_get_year(watch_time, &year);

	/* Set Background color as light blue */
	cairo_set_source_rgba(ad->cairo, 0, 0, 0, 1);
	// Add weather image
	int cur_weather = get_cur_weather(ad->weather_data, time(NULL) - 60*60);
	if (cur_weather != -1) {// second % WEATHER_ICON_NUMBER
		cairo_set_source_surface(ad->cairo, ad->weather_icons[ad->weather_data[cur_weather].index], 0.8*x, 1.37*y);
	}
	cairo_set_operator(ad->cairo, CAIRO_OPERATOR_SOURCE);

	cairo_paint(ad->cairo);

	/* Variables for display time */
	cairo_line_cap_t line_cap_style;
	cairo_line_join_t line_join_style;


	line_cap_style = CAIRO_LINE_CAP_SQUARE;
	line_join_style = CAIRO_LINE_JOIN_BEVEL;
	cairo_set_line_width(ad->cairo, 10);
	cairo_set_line_join(ad->cairo, line_join_style);
	cairo_set_line_cap(ad->cairo, line_cap_style);

	int track = 0;
	track = get_track_from_time(&ad->track_data.trackes[0]);
	draw_track(ad->cairo, x, y, ad->height/2 - 10, track, ad->track_data.trackes[0].style, ad->track_data.left_hand ? 0 : 1);

	track = get_track_from_time(&ad->track_data.trackes[1]);
	draw_track(ad->cairo, x, y, ad->height/2 - 25, track, ad->track_data.trackes[1].style, ad->track_data.left_hand ? 0 : 1);

	track = get_track_from_time(&ad->track_data.trackes[2]);
	draw_track(ad->cairo, x, y, ad->height/2 - 25, track, ad->track_data.trackes[2].style, ad->track_data.left_hand ? 1 : 0);

	track = get_track_from_time(&ad->track_data.trackes[3]);
	draw_track(ad->cairo, x, y, ad->height/2 - 10, track, ad->track_data.trackes[3].style, ad->track_data.left_hand ? 1 : 0);


	char rest_time[16] = {};
	cairo_set_source_rgba(ad->cairo, 1.0 - ad->track_data.trackes[0].style[0].red,
								 	 1.0 - ad->track_data.trackes[0].style[0].green,
									 1.0 - ad->track_data.trackes[0].style[0].blue, 1.0);
	print_rest_time(rest_time, &ad->track_data.trackes[0]);
	draw_name(ad->cairo, x, y, ad->height/2 - 6, ad->track_data.trackes[0].name, rest_time, ad->track_data.left_hand ? 0 : 1);

	cairo_set_source_rgba(ad->cairo, 1.0 - ad->track_data.trackes[1].style[0].red,
								 	 1.0 - ad->track_data.trackes[1].style[0].green,
									 1.0 - ad->track_data.trackes[1].style[0].blue, 1.0);
	print_rest_time(rest_time, &ad->track_data.trackes[1]);
	draw_name(ad->cairo, x, y, ad->height/2 - 21, ad->track_data.trackes[1].name, rest_time, ad->track_data.left_hand ? 0 : 1);

	cairo_set_source_rgba(ad->cairo, 1.0 - ad->track_data.trackes[2].style[0].red,
								 	 1.0 - ad->track_data.trackes[2].style[0].green,
									 1.0 - ad->track_data.trackes[2].style[0].blue, 1.0);
	print_rest_time(rest_time, &ad->track_data.trackes[2]);
	draw_name(ad->cairo, x, y, ad->height/2 - 21, ad->track_data.trackes[2].name, rest_time,  ad->track_data.left_hand ? 1 : 0);

	cairo_set_source_rgba(ad->cairo, 1.0 - ad->track_data.trackes[3].style[0].red,
								 	 1.0 - ad->track_data.trackes[3].style[0].green,
									 1.0 - ad->track_data.trackes[3].style[0].blue, 1.0);
	print_rest_time(rest_time, &ad->track_data.trackes[3]);
	draw_name(ad->cairo, x, y, ad->height/2 - 6, ad->track_data.trackes[3].name, rest_time,  ad->track_data.left_hand ? 1 : 0);

	/* Render stacked cairo APIs on cairo context's surface */
	cairo_surface_flush(ad->surface);

	/* Display this cairo watch on screen */
	evas_object_image_data_update_add(ad->img, 0, 0, ad->width, ad->height);
	evas_object_show(ad->img);

	/* Draw battery */
	line_cap_style = CAIRO_LINE_CAP_ROUND;
	cairo_set_line_cap(ad->cairo, line_cap_style);
	int battery = 0;
	device_battery_get_percent(&battery);
	draw_battery(ad->cairo, x, y, ad->height/2 - 40, battery);

	/* Draw calendar */
	draw_calendar(ad->cairo, x, y, day, week - 1, month - 1, year);
	//draw_calendar(ad->cairo, x, y, 1 + second % 28, second % 7, second % 12, year);

	if (cur_weather != -1) {
		draw_weather(ad->cairo, x, y, ad->weather_data, cur_weather, hour24, ad->update_weather_time);
	}

	/* Set label text */
	/*if (!ambient) {
		snprintf(watch_text, BUF_SIZE, "<align=center>%d %s %d</br>%s</br>%02d:%02d:%02d</align>",
			day, month_name[month - 1], year, week_name[week - 1], hour24, minute, second);
	} else {
		snprintf(watch_text, BUF_SIZE, "<align=center>%d %s %d</br>%s</br>%02d:%02d</align>",
			day, month_name[month - 1], year, week_name[week - 1], hour24, minute);
	}*/
	if (!ambient) {
		snprintf(watch_text, BUF_SIZE, "<align=center>%02d:%02d:%02d</align>",
				 hour24, minute, second);
	} else {
		snprintf(watch_text, BUF_SIZE, "<align=center>%02d:%02d</align>",
				 hour24, minute);
	}
	// Set time font size
	if (cur_weather == -1) {
		elm_object_scale_set(ad->timeLabel, 1.5);
	} else {
		elm_object_scale_set(ad->timeLabel, 1.1);
	}
	/* Set label */
	elm_object_text_set(ad->timeLabel, watch_text);
}

static void
_button_app_cb(void *data, Evas_Object *obj, void *event_info)
{
	app_control_h app_control;

	app_control_create(&app_control);
	app_control_set_operation(app_control, APP_CONTROL_OPERATION_DEFAULT);
	app_control_set_app_id(app_control, "org.example.task_tracker");

	if (app_control_send_launch_request(app_control, NULL, NULL) == APP_CONTROL_ERROR_NONE)
	    dlog_print(DLOG_INFO, LOG_TAG, "Succeeded to launch a calculator app.");
	else
	    dlog_print(DLOG_ERROR, LOG_TAG, "Failed to launch a calculator app.");

	app_control_destroy(app_control);
}

/*
 * Create and display the selected watch face screen
   ( User can change watch face in Wearable's Settings
	 Select watch face option in 'Settings' - 'Display' - 'Watch face' menu )
 * In this function,
 * First, get the watch application's elm_win
 * Second, create an evas object image as a destination surface
 * Then, create cairo context and surface using evas object image's data
 * Third, Create a label for display the given time as a text type
 * Last, call update_watch() function to draw and display the first screen
 */
static void
create_base_gui(appdata_s *ad)
{
	int ret;
	watch_time_h watch_time = NULL;

	/*
	 * Window
	 * Get the watch application's elm_win.
	 * elm_win is mandatory to manipulate window
	 */
	ret = watch_app_get_elm_win(&ad->win);
	if (ret != APP_ERROR_NONE) {
		dlog_print(DLOG_ERROR, LOG_TAG, "failed to get window. err = %d", ret);
		return;
	}

	evas_object_resize(ad->win, ad->width, ad->height);

	/* Create image */
	ad->img = evas_object_image_filled_add(evas_object_evas_get(ad->win));
	evas_object_image_size_set(ad->img, ad->width, ad->height);
	evas_object_resize(ad->img, ad->width, ad->height);
	evas_object_image_content_hint_set(ad->img, EVAS_IMAGE_CONTENT_HINT_DYNAMIC);
	evas_object_image_colorspace_set(ad->img, EVAS_COLORSPACE_ARGB8888);
	evas_object_show(ad->img);

	/* Create Cairo context */
	ad->pixels = (unsigned char*) evas_object_image_data_get(ad->img, 1);
	ad->surface = cairo_image_surface_create_for_data(ad->pixels,
					CAIRO_FORMAT_ARGB32, ad->width, ad->height, ad->width * 4);
	ad->cairo = cairo_create(ad->surface);

	/* Create Labels */
	ad->timeLabel = elm_label_add(ad->win);
	evas_object_text_font_set(ad->timeLabel, "Sans", 30);
	evas_object_resize(ad->timeLabel, ad->width, ad->height/2);
	evas_object_move(ad->timeLabel, 0, ad->height/1.85);
	evas_object_show(ad->timeLabel);

	/* Create App Button */
	Evas_Object *button_app = elm_button_add(ad->win);
	elm_object_text_set(button_app, "Трекер");
	elm_object_style_set(button_app, "bottom");
	elm_object_scale_set (button_app, 0.5);
	elm_layout_sizing_restricted_eval(button_app, EINA_TRUE, EINA_TRUE);
	evas_object_smart_callback_add(button_app, "clicked", _button_app_cb, NULL);
	evas_object_move(button_app, ad->width/2, ad->height - 60);
	evas_object_show(button_app);
	ad->button_app = button_app;

	ret = watch_time_get_current_time(&watch_time);
	if (ret != APP_ERROR_NONE)
		dlog_print(DLOG_ERROR, LOG_TAG, "failed to get current time. err = %d", ret);


	for (int i = 0; i < WEATHER_SIZE; i++) {
		ad->weather_data[i].dt = 0;
		ad->weather_data[i].index = 0;
		ad->weather_data[i].index = 0;
	}
	//start_weather(ad);
	ecore_timer_loop_add(10, start_weather, (void*)ad);

	/* Load weather images */
	char image_filepath[256];
	char *resource_path = app_get_resource_path();
	for (int index = 0; index < WEATHER_ICON_NUMBER; index++) {
		snprintf(image_filepath, 256, "%s/%d.png", resource_path, index2icon(index));
		ad->weather_icons[index] = cairo_image_surface_create_from_png(image_filepath);
	}
	free(resource_path);

	/* Display first screen of watch */
	update_watch(ad, watch_time, 0);

	/* Destroy variable */
	watch_time_delete(watch_time);

	/* Show window after base gui is set up */
	evas_object_show(ad->win);
}

/*
 * @brief Hook to take necessary actions before main event loop starts
 * @param[in] width watch application's given elm_win's width
 * @param[in] height watch application's given elm_win's height
 * @param[in] data application's data structure
 * Initialize UI resources and application's data
 * If this function returns true, the main loop of application starts
 * If this function returns false, the application is terminated
 */
static bool
app_create(int width, int height, void *data)
{
	appdata_s *ad = data;
	ad->width = width;
	ad->height = height;
	if (!read_or_create_data(ad, &ad->track_data)) {
		dlog_print(DLOG_ERROR, LOG_TAG, "Can't create data");
	}
	create_base_gui(ad);
	return true;
}

/*
 * @brief This callback function is called when another application
 * sends the launch request to the application
 */
static void
app_control(app_control_h app_control, void *data)
{
	/* Handle the launch request. */
}

/*
 * @brief This callback function is called each time
 * the application is completely obscured by another application
 * and becomes invisible to the user
 */
static void
app_pause(void *data)
{
	/* Take necessary actions when application becomes invisible. */
}

/*
 * @brief This callback function is called each time
 * the application becomes visible to the user
 */
static void
app_resume(void *data)
{
	/* Take necessary actions when application becomes visible. */
	appdata_s *ad = data;
	if (!read_or_create_data(ad, &ad->track_data)) {
		dlog_print(DLOG_ERROR, LOG_TAG, "Can't create data");
	}
	ad->reload_data = 3;
}

/*
 * @brief This callback function is called once after the main loop of the application exits
 */
static void
app_terminate(void *data)
{
	/* Release all resources. */
	appdata_s *ad = data;
	/* Destroy cairo surface and context */
	cairo_surface_destroy(ad->surface);
	cairo_destroy(ad->cairo);
}

/*
 * @brief This function will be called at each second.
 */
static void
app_time_tick(watch_time_h watch_time, void *data)
{
	/* Called at each second while your app is visible. Update watch UI. */
	appdata_s *ad = data;
	update_watch(ad, watch_time, 0);
}

/*
 * @brief This function will be called at each minute.
 */
static void
app_ambient_tick(watch_time_h watch_time, void *data)
{
	/* Called at each minute while the device is in ambient mode. Update watch UI. */
	appdata_s *ad = data;
	update_watch(ad, watch_time, 1);
}

/*
 * @brief This function will be called when the ambient mode is changed
 */
static void
app_ambient_changed(bool ambient_mode, void *data)
{
	/* Update your watch UI to conform to the ambient mode */
	appdata_s *ad = data;
	if (ambient_mode)
	{
		evas_object_hide(ad->button_app);
	} else {
		evas_object_show(ad->button_app);
	}
}

/*
 * @brief This function will be called when the language is changed
 */
static void
watch_app_lang_changed(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_LANGUAGE_CHANGED*/
	char *locale = NULL;
	app_event_get_language(event_info, &locale);
	elm_language_set(locale);
	free(locale);
	return;
}

/*
 * @brief This function will be called when the region is changed
 */
static void
watch_app_region_changed(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_REGION_FORMAT_CHANGED*/
}

/*
 * @brief Main function of the application
 */
int
main(int argc, char *argv[])
{
	appdata_s ad = {0,};
	int ret = 0;

	watch_app_lifecycle_callback_s event_callback = {0,};
	app_event_handler_h handlers[5] = {NULL, };

	event_callback.create = app_create;
	event_callback.terminate = app_terminate;
	event_callback.pause = app_pause;
	event_callback.resume = app_resume;
	event_callback.app_control = app_control;
	event_callback.time_tick = app_time_tick;
	event_callback.ambient_tick = app_ambient_tick;
	event_callback.ambient_changed = app_ambient_changed;

	watch_app_add_event_handler(&handlers[APP_EVENT_LANGUAGE_CHANGED],
		APP_EVENT_LANGUAGE_CHANGED, watch_app_lang_changed, &ad);
	watch_app_add_event_handler(&handlers[APP_EVENT_REGION_FORMAT_CHANGED],
		APP_EVENT_REGION_FORMAT_CHANGED, watch_app_region_changed, &ad);

	ret = watch_app_main(argc, argv, &event_callback, &ad);
	if (ret != APP_ERROR_NONE)
		dlog_print(DLOG_ERROR, LOG_TAG, "watch_app_main() is failed. err = %d", ret);

	return ret;
}

