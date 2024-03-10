#if !defined(_MAIN_H_)
#define _MAIN_H_

#if !defined(PACKAGE)
#define PACKAGE "org.example.dashboard"
#endif

#ifdef  LOG_TAG
#undef  LOG_TAG
#endif
#define LOG_TAG "task_tracker"

#include <app.h>
#include <Elementary.h>
#include <system_settings.h>
#include <efl_extension.h>
#include <dlog.h>
#include <stdio.h>
#include <string.h>
#include "../shared/source/data.h"

#define EDJ_FILE "edje/setting.edj"
#define GRP_MAIN "main"


#endif // _MAIN_H_
