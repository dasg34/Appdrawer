#include <app_manager.h>
#include <private.h>
#include <system_info.h>

typedef struct appdata{
	Evas_Object* win;
	Evas_Object* gengrid;
	Evas_Object* conform;
	Elm_Gengrid_Item_Class *gic;
	app_info_filter_h filter;
	app_control_h app_control;
	Ecore_Thread *thread;
	char *operation;
} appdata_s;

static char *
app_res_path_get(const char *res_name)
{
   char *res_path, *path;
   int pathlen;

   res_path = app_get_resource_path();
   if (!res_path)
      {
         dlog_print(DLOG_ERROR, LOG_TAG, "res_path_get ERROR!");
         return NULL;
      }

   pathlen = strlen(res_name) + strlen(res_path) + 1;
   path = malloc(sizeof(char) * pathlen);
   snprintf(path, pathlen, "%s%s", res_path, res_name);
   free(res_path);

   return path;
}

static Evas_Object *
my_layout_add(Evas_Object *parent, const char *edj_name, const char *group)
{
   Evas_Object *layout;
   char *path;

   path = app_res_path_get(edj_name);

   layout = elm_layout_add(parent);
   elm_layout_file_set(layout, path, group);
   free(path);

   return layout;
}

static void
_layout_back_cb(void *data, Evas_Object *obj, void *event_info)
{
	appdata_s *ad = data;

	ecore_thread_cancel(ad->thread);
	elm_win_lower(ad->win);
}

static Evas_Object *
_grid_content_get(void *data, Evas_Object *obj, const char *part)
{
   app_info_h app_info = data;
   char *path, *label;

   if (strcmp(part, "elm.swallow.icon"))
      return NULL;

   app_info_get_icon(app_info, &path);
   app_info_get_label(app_info, &label);

   if (!path || !label)
      return NULL;

   Evas_Object *app_obj = my_layout_add(obj, "edje/app_icon_ui.edj", "main");
   evas_object_size_hint_weight_set(app_obj, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(app_obj, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(app_obj);

   Evas_Object *img = elm_image_add(obj);
   evas_object_size_hint_weight_set(img, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(img, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_image_file_set(img, path, NULL);
   elm_object_part_content_set(app_obj, "icon", img);
   elm_object_part_text_set(app_obj, "text", label);

   free(label);
   free(path);

   return app_obj;
}

static void
_app_select_cb(void *data, Evas_Object *obj, void *event_info)
{
   appdata_s *ad = data;
   Elm_Object_Item *it = event_info;
   app_info_h app_info;
   char *icon, *label, *id;

   app_info = elm_object_item_data_get(it);

   app_info_get_icon(app_info, &icon);
   app_info_get_label(app_info, &label);
   app_info_get_app_id(app_info, &id);

   dlog_print(DLOG_ERROR, LOG_TAG, "icon = %s", icon);
   dlog_print(DLOG_ERROR, LOG_TAG, "label = %s", label);

   app_control_h reply;
   app_control_create(&reply);
   app_control_add_extra_data(reply, "app_icon", icon);
   app_control_add_extra_data(reply, "app_label", label);
   app_control_add_extra_data(reply, "app_id", id);
   app_control_reply_to_launch_request(reply, ad->app_control, APP_CONTROL_RESULT_SUCCEEDED);
   app_control_destroy(reply);
   app_control_destroy(ad->app_control);

   free(icon);
   free(label);
   free(id);
   free(ad->operation);
   ad->operation = NULL;

   ecore_thread_cancel(ad->thread);
   elm_win_lower(ad->win);
}

static bool
_app_info_cb(app_info_h app_info, void *data)
{
   appdata_s *ad = data;
   app_info_h app_clone_info;

   if (ecore_thread_check(ad->thread))
      return false;

   app_info_clone(&app_clone_info, app_info);

   char *path, *label;

   app_info_get_icon(app_info, &path);
   app_info_get_label(app_info, &label);

   ecore_thread_main_loop_begin();
   elm_gengrid_item_append(ad->gengrid, ad->gic, app_clone_info, _app_select_cb, ad);
   ecore_thread_main_loop_end();
   return true;
}

static void
_grid_del(void *data, Evas_Object *obj)
{
   app_info_h app_info = data;

   app_info_destroy(app_info);
}


static void
create_base_gui(appdata_s *ad)
{
	/* Window */
	/* Create and initialize elm_win.
	   elm_win is mandatory to manipulate window. */
	ad->win = elm_win_util_standard_add(PACKAGE, PACKAGE);
	elm_win_conformant_set(ad->win, EINA_TRUE);
	elm_win_autodel_set(ad->win, EINA_TRUE);

	if (elm_win_wm_rotation_supported_get(ad->win)) {
		int rots[4] = { 0, 90, 180, 270 };
		elm_win_wm_rotation_available_rotations_set(ad->win, (const int *)(&rots), 4);
	}

	ad->conform = elm_conformant_add(ad->win);
	elm_win_indicator_mode_set(ad->win, ELM_WIN_INDICATOR_SHOW);
	elm_win_indicator_opacity_set(ad->win, ELM_WIN_INDICATOR_OPAQUE);
	evas_object_size_hint_weight_set(ad->conform, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	eext_object_event_callback_add(ad->conform, EEXT_CALLBACK_BACK, _layout_back_cb, ad);
	elm_win_resize_object_add(ad->win, ad->conform);
	evas_object_show(ad->conform);

	/* Show window after base gui is set up */
	evas_object_show(ad->win);

}

static bool
app_create(void *data)
{
	appdata_s *ad = data;

	elm_config_accel_preference_set("gl");

	create_base_gui(ad);

	return true;
}

static void
_thread_start(void *data, Ecore_Thread *thread)
{
   appdata_s *ad = data;

   app_info_filter_foreach_appinfo(ad->filter, _app_info_cb, ad);
}

static void
_thread_end(void *data, Ecore_Thread *thread)
{
   appdata_s *ad = data;

   app_info_filter_destroy(ad->filter);
   elm_gengrid_item_class_free(ad->gic);

   ad->thread = NULL;
}

static void
app_control(app_control_h app_control, void *data)
{
   appdata_s *ad = data;
   Elm_Gengrid_Item_Class *gic;
   int width;
   char *operation;

   app_control_get_operation(app_control, &operation);

   dlog_print(DLOG_ERROR, LOG_TAG, "operation = %s", operation);

   ad->operation = operation;
   if (strcmp(operation, "SELECT_APP"))
      return;

   ad->gengrid = elm_gengrid_add(ad->conform);
   system_info_get_platform_int("http://tizen.org/feature/screen.width", &width);
   elm_gengrid_item_size_set(ad->gengrid, width / 4, (width / 4) * 1.4);
   elm_gengrid_align_set(ad->gengrid, 0.0, 0.0);
   elm_object_content_set(ad->conform, ad->gengrid);

   app_info_filter_create(&ad->filter);
   app_info_filter_add_bool(ad->filter, PACKAGE_INFO_PROP_APP_TASKMANAGE, true);
   app_info_filter_add_bool(ad->filter, PACKAGE_INFO_PROP_APP_NODISPLAY, false);

   gic = elm_gengrid_item_class_new();
   gic->item_style = "default";
   gic->func.content_get = _grid_content_get;
   gic->func.del = _grid_del;

   ad->gic = gic;

   app_control_clone(&ad->app_control, app_control);

   ad->thread = ecore_thread_run(_thread_start, _thread_end, NULL, ad);
}

static void
app_pause(void *data)
{
	/* Take necessary actions when application becomes invisible. */
}

static void
app_resume(void *data)
{
   appdata_s *ad = data;
   char *path;

   if (ad->operation && !strcmp(ad->operation, "SELECT_APP"))
      return;

   Evas_Object *layout = my_layout_add(ad->conform, "edje/warning.edj", "main");
   evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(layout, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_object_content_set(ad->conform, layout);

   Evas_Object *img = elm_image_add(layout);
   evas_object_size_hint_weight_set(img, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(img, EVAS_HINT_FILL, EVAS_HINT_FILL);
   path = app_res_path_get("images/tutorial.gif");
   elm_image_file_set(img, path, NULL);

   if (elm_image_animated_available_get(img))
      {
         elm_image_animated_set(img, EINA_TRUE);
         elm_image_animated_play_set(img, EINA_TRUE);
      }

   elm_object_part_content_set(layout, "tutorial", img);
   free(path);
}

static void
app_terminate(void *data)
{
	/* Release all resources. */
}

static void
ui_app_lang_changed(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_LANGUAGE_CHANGED*/

	int ret;
	char *language;

	ret = app_event_get_language(event_info, &language);
	if (ret != APP_ERROR_NONE) {
		dlog_print(DLOG_ERROR, LOG_TAG, "app_event_get_language() failed. Err = %d.", ret);
		return;
	}

	if (language != NULL) {
		elm_language_set(language);
		free(language);
	}
}

static void
ui_app_orient_changed(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_DEVICE_ORIENTATION_CHANGED*/
	return;
}

static void
ui_app_region_changed(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_REGION_FORMAT_CHANGED*/
}

static void
ui_app_low_battery(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_LOW_BATTERY*/
}

static void
ui_app_low_memory(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_LOW_MEMORY*/
}

int
main(int argc, char *argv[])
{
	appdata_s ad = {0,};
	int ret = 0;

	ui_app_lifecycle_callback_s event_callback = {0,};
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

	ret = ui_app_main(argc, argv, &event_callback, &ad);
	if (ret != APP_ERROR_NONE) {
		dlog_print(DLOG_ERROR, LOG_TAG, "ui_app_main() is failed. err = %d", ret);
	}

	return ret;
}
