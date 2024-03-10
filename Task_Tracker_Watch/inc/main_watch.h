#if !defined(_MAIN_WATCH_H_)
#define _MAIN_WATCH_H_

#include <watch_app.h>
#include <watch_app_efl.h>
#include <Elementary.h>
#include <dlog.h>
#include <app.h>
#include "../../Task_Tracker/shared/source/data.h"

#if !defined(PACKAGE)
#define PACKAGE "org.example.task_tracker_watch"
#endif

#ifdef  LOG_TAG
#undef  LOG_TAG
#endif
#define LOG_TAG "task_tracker_watch"

#define API_OPEN_WEATHER 23419869bf7b6bd28c71d2228f429d56
#define WEATHER_ICON_NUMBER 18

#define BLACK_ANGLE 2
#define ANGLE_STEP (M_PI/(TRACK_SIZE+2*BLACK_ANGLE))
#define FONT 15
#define WORD_ANGLE_STEP (M_PI/50)
#define WEATHER_SIZE 40

#endif // _MAIN_WATCH_H_
