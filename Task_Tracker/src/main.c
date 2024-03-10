#include "main.h"

typedef struct appdata {
	Evas_Object *win;
	Evas_Object *naviframe;
	Eext_Circle_Surface *circle_surface;
	Evas_Object *circle_genlist;
	Evas_Object *TRACK_rect[TRACK_SIZE];
	Ecore_Timer *sec_timer;

	track_data_s track_data;
	track_s *cur_track;
} appdata_s;

void update_track(track_s *track)
{
	int track_val = get_track_from_time(track);
	int i = 0;
	double alpha = 0.0;
	for (i = 0; i < TRACK_SIZE/2; ++i) {
		alpha = 0.3 + 0.7*(i < track_val);
		evas_object_color_set(track->appdata->TRACK_rect[i],
				alpha*2*(track->style[0].red*(TRACK_SIZE/2 - i) + track->style[1].red*i)/TRACK_SIZE,
				alpha*2*(track->style[0].green*(TRACK_SIZE/2 - i) + track->style[1].green*i)/TRACK_SIZE,
				alpha*2*(track->style[0].blue*(TRACK_SIZE/2 - i) + track->style[1].blue*i)/TRACK_SIZE,
				255);
	}
	for (i = TRACK_SIZE/2; i < TRACK_SIZE; ++i) {
		alpha = 0.3 + 0.7*(i < track_val);
		evas_object_color_set(track->appdata->TRACK_rect[i],
				alpha*2*(track->style[1].red*(TRACK_SIZE - i) + track->style[2].red*(i - TRACK_SIZE/2))/TRACK_SIZE,
				alpha*2*(track->style[1].green*(TRACK_SIZE - i) + track->style[2].green*(i - TRACK_SIZE/2))/TRACK_SIZE,
				alpha*2*(track->style[1].blue*(TRACK_SIZE - i) + track->style[2].blue*(i - TRACK_SIZE/2))/TRACK_SIZE,
				255);
	}
}

/*
 * @brief Function will be operated when window deletion is requested
 * @param[in] data The data to be passed to the callback function
 * @param[in] obj The Evas object handle to be passed to the callback function
 * @param[in] event_info The system event information
 */
static void
win_delete_request_cb(void *data, Evas_Object *obj, void *event_info)
{
	ui_app_exit();
}

/*
 * @brief Function to get absolute edje resource path
 * @param[in] edj_file_in The relative path of edje resource
 * @param[in] edj_path_out The pointer variable to write absolute path of edje resource
 * @param[in] edj_path_max The maximum size of path_out variable
 */
static void
app_get_resource(const char *edj_file_in, char *edj_path_out, int edj_path_max)
{
	char *res_path = app_get_resource_path();
	if (res_path) {
		snprintf(edj_path_out, edj_path_max, "%s%s", res_path, edj_file_in);
		free(res_path);
	}
}

/*
 * @brief Function will be operated when manipulating setting value is finished
 * @param[in] data The data to be passed to the callback function
 * @param[in] it The item to be popped from naviframe
 */
static Eina_Bool
_setting_finished_cb(void *data, Elm_Object_Item *it)
{
	appdata_s *ad = data;

	/* Activate Rotary Event */
	eext_rotary_object_event_activated_set(ad->circle_genlist, EINA_TRUE);
	return EINA_TRUE;
}

/*
 * @brief Function will be operated when button is clicked
 * @param[in] data The data to be passed to the callback function
 * @param[in] obj The Evas object handle to be passed to the callback function
 * @param[in] event_info The system event information
 */
static void
_button_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
	appdata_s *ad = data;
	elm_naviframe_item_pop(ad->naviframe);
}

/*
 * @brief Function will be operated when "Information" menu is clicked
 * @param[in] data The data to be passed to the callback function
 * @param[in] obj The Evas object handle to be passed to the callback function
 * @param[in] event_info The system event information
 */
static void
_setting_information_cb(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
	char edj_path[PATH_MAX] = {0, };
	appdata_s *ad = data;
	Evas_Object *naviframe = ad->naviframe;
	Evas_Object *layout = NULL;
	Evas_Object *label = NULL;
	Evas_Object *button = NULL;
	Evas_Object *scroller = NULL;
	Evas_Object *circle_scroller = NULL;
	Elm_Object_Item *nf_it = NULL;

	/* Unhighlight Item */
	elm_genlist_item_selected_set((Elm_Object_Item *)event_info, EINA_FALSE);

	app_get_resource(EDJ_FILE, edj_path, (int)PATH_MAX);

	layout = elm_layout_add(naviframe);
	elm_layout_file_set(layout, edj_path, "info_layout");
	evas_object_show(layout);

	scroller = elm_scroller_add(layout);
	evas_object_show(scroller);

	elm_object_part_content_set(layout, "elm.swallow.content", scroller);

	/* Create Circle Scroller */
	circle_scroller = eext_circle_object_scroller_add(scroller, ad->circle_surface);

	/* Set Scroller Policy */
	eext_circle_object_scroller_policy_set(circle_scroller, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_AUTO);

	/* Activate Rotary Event */
	eext_rotary_object_event_activated_set(circle_scroller, EINA_TRUE);

	label = elm_label_add(scroller);
	elm_label_line_wrap_set(label, ELM_WRAP_MIXED);
	elm_object_text_set(label, "<br/><br/> Здесь будет подробное описание трекера задач :) <br/><br/><br/>");
	evas_object_show(label);

	elm_object_content_set(scroller, label);

	button = elm_button_add(layout);
	elm_object_style_set(button, "bottom");
	elm_object_text_set(button, "OK");
	elm_object_part_content_set(layout, "elm.swallow.button", button);
	evas_object_smart_callback_add(button, "clicked", _button_clicked_cb, ad);
	evas_object_show(button);

	nf_it = elm_naviframe_item_push(naviframe, _("Slider"), NULL, NULL, layout, "empty");
	elm_naviframe_item_pop_cb_set(nf_it, _setting_finished_cb, ad);
}

static void
_half_plus_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
	track_s *track = data;
	track->last_full = add_track_to_time(track->period, track->last_full, TRACK_HALF);
	update_track(track);
}

static void
_full_plus_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
	track_s *track = data;
	track->last_full = add_track_to_time(track->period, track->last_full, TRACK_FULL);
	update_track(track);
}

static void
_double_plus_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
	track_s *track = data;
	track->last_full = add_track_to_time(track->period, track->last_full, TRACK_DOUBLE);
	update_track(track);
}


static void
_track_changed_cb(void *data, Evas_Object *obj, void *event_info)
{
	track_s *track = data;
	int value = 0;

	/* Get Circle Object Value */
	value = eext_circle_object_value_get(obj);

	track->last_full = add_track_to_time(track->period, track->last_full, value - 1);
	eext_circle_object_value_set(obj, 1.0);
	update_track(track);
}

static void
_setting_test_cb(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{

	char edj_path[PATH_MAX] = {0, };
	track_s *track = data;
	Evas_Object *naviframe = track->appdata->naviframe;
	Evas_Object *layout = NULL;
	Evas_Object *box = NULL;
	Elm_Object_Item *nf_it = NULL;
	Evas_Object *button_box = NULL;
	Evas_Object *button_half_plus = NULL;
	Evas_Object *button_full_plus = NULL;
	Evas_Object *button_double_plus = NULL;
	Evas_Object *slider = NULL;
	int i = 0;

	/* set current track */
	track->appdata->cur_track = track;
	/* Unhighlight Item */
	elm_genlist_item_selected_set((Elm_Object_Item *)event_info, EINA_FALSE);

	app_get_resource(EDJ_FILE, edj_path, (int)PATH_MAX);

	layout = elm_layout_add(naviframe);
	elm_layout_file_set(layout, edj_path, "test_layout");
	evas_object_show(layout);

	box = elm_box_add(layout);
	elm_box_horizontal_set(box, EINA_TRUE);
	elm_box_padding_set(box, 0, 0);
	evas_object_show(box);

	elm_object_part_content_set(layout, "elm.swallow.content", box);

	for (i = 0; i < TRACK_SIZE; ++i) {
		track->appdata->TRACK_rect[i] = evas_object_rectangle_add(evas_object_evas_get(layout));
		evas_object_size_hint_min_set(track->appdata->TRACK_rect[i], 3, 20);
		evas_object_show(track->appdata->TRACK_rect[i]);
		elm_box_pack_end(box, track->appdata->TRACK_rect[i]);
	}
	update_track(track);

	/* Add Buttons */
	button_box = elm_box_add(layout);
	elm_box_align_set(button_box, EINA_FALSE, EINA_TRUE);
	//elm_box_horizontal_set(button_box, EINA_TRUE);
	elm_box_padding_set(button_box, 0, 5);
	evas_object_show(button_box);
	elm_object_part_content_set(layout, "elm.swallow.button", button_box);

	button_half_plus = elm_button_add(layout);
	elm_object_text_set(button_half_plus, "±");
	elm_object_style_set(button_half_plus, "bottom");
	elm_object_scale_set (button_half_plus, 0.5);
	elm_layout_sizing_restricted_eval(button_half_plus, EINA_TRUE, EINA_TRUE);
	evas_object_smart_callback_add(button_half_plus, "clicked", _half_plus_clicked_cb, track);
	evas_object_show(button_half_plus);
	elm_box_pack_end(button_box, button_half_plus);

	button_full_plus = elm_button_add(layout);
	elm_object_text_set(button_full_plus, "+");
	elm_object_style_set(button_full_plus, "bottom");
	elm_object_scale_set (button_full_plus, 0.5);
	evas_object_smart_callback_add(button_full_plus, "clicked", _full_plus_clicked_cb, track);
	evas_object_show(button_full_plus);
	elm_box_pack_end(button_box, button_full_plus);

	button_double_plus = elm_button_add(layout);
	elm_object_text_set(button_double_plus, "++");
	elm_object_style_set(button_double_plus, "bottom");
	elm_object_scale_set (button_double_plus, 0.5);
	evas_object_smart_callback_add(button_double_plus, "clicked", _double_plus_clicked_cb, track);
	evas_object_show(button_double_plus);
	elm_box_pack_end(button_box, button_double_plus);

	/* Create Circle Slider */
	slider = eext_circle_object_slider_add(layout, NULL);
	/* Set Circle Slider Range, Value and Step */
	eext_circle_object_value_min_max_set(slider, 0.0, 2.0);
	eext_circle_object_value_set(slider, 1.0);
	eext_circle_object_slider_step_set(slider, 1.0);
	/* Activate Rotary Event */
	eext_rotary_object_event_activated_set(slider, EINA_TRUE);
	evas_object_smart_callback_add(slider, "value,changed", _track_changed_cb, track);

	/* Create text label */
	elm_object_part_text_set(layout, "elm.text.slider", track->name);

	nf_it = elm_naviframe_item_push(naviframe, _("Slider"), NULL, NULL, layout, "empty");
	elm_naviframe_item_pop_cb_set(nf_it, _setting_finished_cb, track->appdata);
}

static Eina_Bool
_timer_cd(void *data)
{
	dlog_print(DLOG_INFO, LOG_TAG, "[TIMER]");
	appdata_s *ad = data;
	if (ad->cur_track != NULL)
	{
		update_track(ad->cur_track);
	}
	return EINA_TRUE;
}

typedef struct _item_data {
	int index;
	char *name;
	Elm_Object_Item *item;
} item_data;

/*
 * @brief Function to get string on genlist title item's text part
 * @param[in] data The data to be passed to the callback function
 * @param[in] obj The Evas object handle to be passed to the callback function
 * @param[in] part The name of text part
 * @param[out] char* A string with the characters to use as genlist title item's text part
 */
static char*
_gl_title_text_get(void *data, Evas_Object *obj, const char *part)
{
	char buf[1024];

	snprintf(buf, 1023, "%s", "Трекер задач");

	return strdup(buf);
}

/*
 * @brief Function to get string on genlist item's text part
 * @param[in] data The data to be passed to the callback function
 * @param[in] obj The Evas object handle to be passed to the callback function
 * @param[in] part The name of text part
 * @param[out] char* A string with the characters to use as genlist item's text part
 */
static char *
_gl_main_text_get(void *data, Evas_Object *obj, const char *part)
{
	char buf[1024];
	item_data *id = data;

	if (!strcmp(part, "elm.text")) {
		snprintf(buf, 1023, "%s", id->name);
		return strdup(buf);
	}
	return NULL;
}

/*
 * @brief Function will be operated when genlist is deleted.
 * @param[in] data The data to be passed to the callback function
 * @param[in] obj The Evas object handle to be passed to the callback function
 */
static void _gl_del(void *data, Evas_Object *obj)
{
	// FIXME: Unrealized callback can be called after this.
	// Accessing Item_Data can be dangerous on unrealized callback.
	item_data *id = data;
	if (id) free(id);
}

/*
 * @brief Function will be operated when naviframe pop its own item
 * @param[in] data The data to be passed to the callback function
 * @param[in] it The item to be popped from naviframe
 */
static Eina_Bool
_naviframe_pop_cb(void *data, Elm_Object_Item *it)
{
	ui_app_exit();
	return EINA_FALSE;
}

/*
 * @brief Function to create gui object
 * @param[in] ad The data structure to manage gui object
 */
static void
create_list_view(appdata_s *ad)
{
	Evas_Object *genlist = NULL;
	Evas_Object *naviframe = ad->naviframe;
	Elm_Object_Item *nf_it = NULL;
	Elm_Genlist_Item_Class *itc = elm_genlist_item_class_new();
	Elm_Genlist_Item_Class *titc = elm_genlist_item_class_new();
	Elm_Genlist_Item_Class *pitc = elm_genlist_item_class_new();
	item_data *id = NULL;
	int index = 0;

	/* Create Genlist */
	genlist = elm_genlist_add(naviframe);
	elm_genlist_mode_set(genlist, ELM_LIST_COMPRESS);
	evas_object_smart_callback_add(genlist, "selected", NULL, NULL);

	/* Create Circle Genlist */
	ad->circle_genlist = eext_circle_object_genlist_add(genlist, ad->circle_surface);

	/* Set Scroller Policy */
	eext_circle_object_genlist_scroller_policy_set(ad->circle_genlist, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_AUTO);

	/* Activate Rotary Event */
	eext_rotary_object_event_activated_set(ad->circle_genlist, EINA_TRUE);

	/* Genlist Item Style */
	itc->item_style = "default";
	itc->func.text_get = _gl_main_text_get;
	itc->func.del = _gl_del;

	/* Genlist Title Item Style */
	titc->item_style = "title";
	titc->func.text_get = _gl_title_text_get;
	titc->func.del = _gl_del;

	/* Genlist Padding Item Style */
	pitc->item_style = "padding";

	/* Title Item Here */
	elm_genlist_item_append(genlist, titc, NULL, NULL, ELM_GENLIST_ITEM_NONE, NULL, ad);

	/* Main Menu Items Here */
	int i = 0;
	for (i = 0; i < ad->track_data.track_number; i++)
	{
		id = calloc(sizeof(item_data), 1);
		id->index = index++;
		id->name = ad->track_data.trackes[i].name;
		id->item = elm_genlist_item_append(genlist, itc, id, NULL, ELM_GENLIST_ITEM_NONE, _setting_test_cb, &(ad->track_data.trackes[i]));
	}

	// Add help in menu
	id = calloc(sizeof(item_data), 1);
	id->index = index++;
	id->name = "О приложении";
	id->item = elm_genlist_item_append(genlist, itc, id, NULL, ELM_GENLIST_ITEM_NONE, _setting_information_cb, ad);

	/* Set timer */
	ad->cur_track = NULL;
	ad->sec_timer = ecore_timer_loop_add(1.0, _timer_cd, ad);

	/* Padding Item Here */
	elm_genlist_item_append(genlist, pitc, NULL, NULL, ELM_GENLIST_ITEM_NONE, NULL, ad);

	nf_it = elm_naviframe_item_push(naviframe, NULL, NULL, NULL, genlist, "empty");
	elm_naviframe_item_pop_cb_set(nf_it, _naviframe_pop_cb, ad->win);
}

/*
 * @brief Function to create base gui structure
 * @param[in] ad The data structure to manage gui object
 */
static void
create_base_gui(appdata_s *ad)
{
	Evas_Object *conform = NULL;

	/* Window */
	ad->win = elm_win_util_standard_add(PACKAGE, PACKAGE);
	elm_win_autodel_set(ad->win, EINA_TRUE);

	evas_object_smart_callback_add(ad->win, "delete,request", win_delete_request_cb, NULL);

	/* Conformant */
	conform = elm_conformant_add(ad->win);
	evas_object_size_hint_weight_set(conform, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_win_resize_object_add(ad->win, conform);
	evas_object_show(conform);

	/* Naviframe */
	ad->naviframe = elm_naviframe_add(conform);
	elm_object_content_set(conform, ad->naviframe);

	/* Eext Circle Surface*/
	ad->circle_surface = eext_circle_surface_naviframe_add(ad->naviframe);

	/* Read data */
	if (!read_or_create_data(ad, &ad->track_data)) {
		dlog_print(DLOG_ERROR, LOG_TAG, "Can't create data");
		ui_app_exit();
	}

	/* Main View */
	create_list_view(ad);

	eext_object_event_callback_add(ad->naviframe, EEXT_CALLBACK_BACK, eext_naviframe_back_cb, NULL);
	eext_object_event_callback_add(ad->naviframe, EEXT_CALLBACK_MORE, eext_naviframe_more_cb, NULL);

	/* Show window after base gui is set up */
	evas_object_show(ad->win);
}

/*
 * @brief Hook to take necessary actions before main event loop starts
 * Initialize UI resources and application's data
 * If this function return true, the main loop of application starts
 * If this function return false, the application is terminated
 * @param[in] user_data The data to be passed to the callback function
 */
static bool app_create(void *user_data)
{
	/* Hook to take necessary actions before main event loop starts
	   Initialize UI resources and application's data
	   If this function returns true, the main loop of application starts
	   If this function returns false, the application is terminated */
	appdata_s *ad = user_data;

	create_base_gui(ad);

	return true;
}

/*
 * @brief This callback function is called when another application
 * sends the launch request to the application
 * @param[in] app_control The handle to the app_control
 * @param[in] user_data The data to be passed to the callback function
 */
static void app_control(app_control_h app_control, void *user_data)
{
	/* Handle the launch request. */

}

/*
 * @brief This callback function is called each time
 * the application is completely obscured by another application
 * and becomes invisible to the user
 * @param[in] user_data The data to be passed to the callback function
 */
static void app_pause(void *user_data)
{
	dlog_print(DLOG_INFO, LOG_TAG, "[PAUSE]");
	appdata_s *ad = user_data;
	/* Take necessary actions when application becomes invisible. */
	FILE* data_file = fopen(DATA_FILE_PATH, "w");
	if (data_file == NULL)
	{
		dlog_print(DLOG_ERROR, LOG_TAG, "Can't write data");
	} else {
		dlog_print(DLOG_INFO, LOG_TAG, "Save data");
		save_data(data_file, &ad->track_data);
		fclose(data_file);
	}
	/* Delete timer */
	ecore_timer_del(ad->sec_timer);
	ad->sec_timer = NULL;
}

/*
 * @brief This callback function is called each time
 * the application becomes visible to the user
 * @param[in] user_data The data to be passed to the callback function
 */
static void app_resume(void *user_data)
{
	dlog_print(DLOG_INFO, LOG_TAG, "[RESUME]");
	/* Take necessary actions when application becomes visible. */
	appdata_s *ad = user_data;
	/* Set timer */
	if (ad->sec_timer == NULL)
	{
		ad->sec_timer = ecore_timer_loop_add(1.0, _timer_cd, ad);
	}
}

/*
 * @brief This callback function is called once after the main loop of the application exits
 * @param[in] user_data The data to be passed to the callback function
 */
static void app_terminate(void *user_data)
{
	/* Release all resources. */
}

/*
 * @brief This function will be called when the language is changed
 * @param[in] event_info The system event information
 * @param[in] user_data The user data to be passed to the callback function
 */
static void ui_app_lang_changed(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_LANGUAGE_CHANGED*/
	char *locale = NULL;
	system_settings_get_value_string(SYSTEM_SETTINGS_KEY_LOCALE_LANGUAGE, &locale);
	elm_language_set(locale);
	free(locale);
	return;
}

/*
 * @brief This function will be called when the orientation is changed
 * @param[in] event_info The system event information
 * @param[in] user_data The user data to be passed to the callback function
 */
static void ui_app_orient_changed(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_DEVICE_ORIENTATION_CHANGED*/
	return;
}

/*
 * @brief This function will be called when the region is changed
 * @param[in] event_info The system event information
 * @param[in] user_data The user data to be passed to the callback function
 */
static void ui_app_region_changed(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_REGION_FORMAT_CHANGED*/
}

/*
 * @brief This function will be called when the battery is low
 * @param[in] event_info The system event information
 * @param[in] user_data The user data to be passed to the callback function
 */
static void ui_app_low_battery(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_LOW_BATTERY*/
}

/*
 * @brief This function will be called when the memory is low
 * @param[in] event_info The system event information
 * @param[in] user_data The user data to be passed to the callback function
 */
static void ui_app_low_memory(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_LOW_MEMORY*/
}

/*
 * @brief Main function of the application
 * @param[in] argc The argument count
 * @param[in] argv The argument vector
 */
int main(int argc, char *argv[])
{
	appdata_s ad = {0, };
	int ret;

	ui_app_lifecycle_callback_s event_callback = {0, };
	app_event_handler_h handlers[5] = {NULL, };

	event_callback.create = app_create;
	event_callback.terminate = app_terminate;
	event_callback.pause = app_pause;
	event_callback.resume = app_resume;
	event_callback.app_control = app_control;

	ui_app_add_event_handler(&handlers[APP_EVENT_LOW_BATTERY], APP_EVENT_LOW_BATTERY, ui_app_low_battery, &ad);
	ui_app_add_event_handler(&handlers[APP_EVENT_LOW_MEMORY], APP_EVENT_LOW_MEMORY, ui_app_low_memory, &ad);
	ui_app_add_event_handler(&handlers[APP_EVENT_DEVICE_ORIENTATION_CHANGED], APP_EVENT_DEVICE_ORIENTATION_CHANGED, ui_app_orient_changed, &ad);
	ui_app_add_event_handler(&handlers[APP_EVENT_LANGUAGE_CHANGED], APP_EVENT_LANGUAGE_CHANGED, ui_app_lang_changed, &ad);
	ui_app_add_event_handler(&handlers[APP_EVENT_REGION_FORMAT_CHANGED], APP_EVENT_REGION_FORMAT_CHANGED, ui_app_region_changed, &ad);
	ui_app_remove_event_handler(handlers[APP_EVENT_LOW_MEMORY]);

	ret = ui_app_main(argc, argv, &event_callback, &ad);
	if (ret != APP_ERROR_NONE)
		dlog_print(DLOG_ERROR, LOG_TAG, "ui_app_main() is failed. err = %d", ret);

	return ret;
}
