#include <private_notitle.h>
#include <tizen.h>

typedef struct widget_instance_data {
	Evas_Object *win;
	Evas_Object *grid;
	Evas_Object *box;
	Evas_Object *plus_icon;
	Evas_Object *edit_btn;
	Eina_List *app_list;
	Eina_Bool keep_icon_select;
	int _w, _h;
} widget_instance_data_s;

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
_popup_timeout_cb(void *data, Evas_Object *obj, void *event_info)
{
   evas_object_del(obj);
}


static void
_popup_toast_open(Evas_Object *win)
{
   Evas_Object *popup = elm_popup_add(win);
   elm_object_style_set(popup, "toast");
   elm_popup_timeout_set(popup, 2.0);
   elm_object_text_set(popup, "Icon already exists");
   evas_object_size_hint_weight_set(popup, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_smart_callback_add(popup, "timeout", _popup_timeout_cb, NULL);
   evas_object_show(popup);
}

static void
_app_grid_rearrange(widget_instance_data_s *wid)
{
   Evas_Object *temp_data;
   Eina_List *l;
   int list_count;

   EINA_LIST_FOREACH(wid->app_list, l, temp_data)
      elm_grid_unpack(wid->grid, temp_data);

   list_count = eina_list_count(wid->app_list);
   for (int i = 0; i < list_count; i++)
      {
         temp_data = eina_list_nth(wid->app_list, i);
         elm_grid_pack(wid->grid, temp_data, i % 4, i / 4, 1, 1);

         elm_grid_pack(wid->grid, wid->plus_icon, list_count % 4, list_count / 4, 1, 1);
         evas_object_show(wid->plus_icon);
      }

   if (list_count == 0)
      {
         elm_grid_pack(wid->grid, wid->plus_icon, 0, 0, 1, 1);
         evas_object_show(wid->plus_icon);
      }
}

static void
_delete_clicked_cb(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   widget_instance_data_s *wid = data;

   elm_grid_unpack(wid->grid, obj);
   evas_object_del(obj);
   elm_grid_unpack(wid->grid, wid->plus_icon);

   wid->app_list = eina_list_remove(wid->app_list, obj);
   char *label = evas_object_data_get(obj, "text");
   free(label);
   _app_grid_rearrange(wid);
}


static void
_icon_clicked_cb(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   char *id = data;

   app_control_h app_control;
   app_control_create(&app_control);
   app_control_set_app_id(app_control, id);
   app_control_send_launch_request(app_control, NULL, NULL);
   app_control_destroy(app_control);
}

static void
_app_control_reply_cb(app_control_h request, app_control_h reply, app_control_result_e result, void *data)
{
   widget_instance_data_s *wid = data;
   Evas_Object *obj;
   Eina_List *l;
   char *path, *label, *id;
   int count;

   wid->keep_icon_select = EINA_FALSE;
   app_control_get_extra_data(reply, "app_icon", &path);
   app_control_get_extra_data(reply, "app_label", &label);
   app_control_get_extra_data(reply, "app_id", &id);

   EINA_LIST_FOREACH(wid->app_list, l, obj)
      {
         const char *list_label;
         list_label = evas_object_data_get(obj, "text");
         dlog_print(DLOG_ERROR, LOG_TAG, "list_label = %s", list_label);
         if (!strcmp(label, list_label))
            {
               free(path);
               free(label);
               _popup_toast_open(wid->win);
               return;
            }
      }

   Evas_Object *app_obj = my_layout_add(wid->grid, "edje/app_icon_notitle.edj", "main");
   evas_object_size_hint_weight_set(app_obj, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(app_obj, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_layout_signal_callback_add(app_obj, "delete,clicked", "", _delete_clicked_cb, wid);
   elm_layout_signal_callback_add(app_obj, "icon,clicked", "", _icon_clicked_cb, id);
   elm_layout_signal_emit(app_obj, "delete,show", "");
   evas_object_data_set(app_obj, "text", label);
   evas_object_show(app_obj);

   Evas_Object *img = elm_image_add(app_obj);
   evas_object_size_hint_weight_set(img, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(img, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_image_file_set(img, path, NULL);
   elm_object_part_content_set(app_obj, "icon", img);

   wid->app_list = eina_list_append(wid->app_list, app_obj);

   count = eina_list_count(wid->app_list);
   elm_grid_pack(wid->grid, app_obj, (count - 1) % 4, (count - 1) / 4, 1, 1);

   elm_grid_unpack(wid->grid, wid->plus_icon);
   evas_object_hide(wid->plus_icon);
   if (abs(wid->_w - wid->_h) < 10)
      {
         if (count < 16)
            {
               elm_grid_pack(wid->grid, wid->plus_icon, count % 4, count / 4, 1, 1);
               evas_object_show(wid->plus_icon);
            }
      }
   else
      {
         if (count < 8)
            {
               elm_grid_pack(wid->grid, wid->plus_icon, count % 4, count / 4, 1, 1);
               evas_object_show(wid->plus_icon);
            }
      }

   free(path);
}

static void
_plus_clicked_cb(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   widget_instance_data_s *wid = data;
   app_control_h app_control;

   wid->keep_icon_select = EINA_TRUE;

   app_control_create(&app_control);
   app_control_set_app_id(app_control, "org.yohoho.appdrawer.ui");
   app_control_set_operation(app_control, "SELECT_APP");
   app_control_send_launch_request(app_control, _app_control_reply_cb, wid);
   app_control_destroy(app_control);
}

static void
_done_clicked_cb(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   widget_instance_data_s *wid = data;
   Eina_List *l;
   Evas_Object *app_obj;

   elm_layout_signal_emit(wid->plus_icon, "plus,hide", "");

   if (!wid->app_list)
      return;

   EINA_LIST_FOREACH(wid->app_list, l, app_obj)
      elm_layout_signal_emit(app_obj, "delete,hide", "");
}

static void
_edit_clicked_cb(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   widget_instance_data_s *wid = data;
   Eina_List *l;
   Evas_Object *app_obj;
   int list_count;

   elm_layout_signal_emit(wid->plus_icon, "plus,show", "");

   if (!wid->app_list)
      {
         elm_grid_pack(wid->grid, wid->plus_icon, 0, 0, 1, 1);
         return;
      }

   EINA_LIST_FOREACH(wid->app_list, l, app_obj)
      elm_layout_signal_emit(app_obj, "delete,show", "");

   list_count = eina_list_count(wid->app_list);

   if (abs(wid->_w - wid->_h) < 10)
      {
         if (list_count >= 16)
           return;
      }
   else
      {
         if (list_count >= 8)
            return;
      }

   elm_grid_pack(wid->grid, wid->plus_icon, list_count % 4, list_count / 4, 1, 1);
}

static int
widget_instance_create(widget_context_h context, bundle *content, int w, int h, void *user_data)
{
	widget_instance_data_s *wid = (widget_instance_data_s*) calloc(1, sizeof(widget_instance_data_s));
	int ret;

	if (content != NULL) {
		/* Recover the previous status with the bundle object. */

	}

	/* Window */
	ret = widget_app_get_elm_win(context, &wid->win);
	if (ret != WIDGET_ERROR_NONE) {
		dlog_print(DLOG_ERROR, LOG_TAG, "failed to get window. err = %d", ret);
		return WIDGET_ERROR_FAULT;
	}

	wid->_w = w;
	wid->_h = h;
	evas_object_resize(wid->win, w, h);

	dlog_print(DLOG_ERROR, LOG_TAG, "w = %d h : %d", w, h);

	Evas_Object *v_box = elm_box_add(wid->win);
   evas_object_size_hint_weight_set(v_box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(wid->win, v_box);
   evas_object_show(v_box);

   Evas_Object *edit_btn = my_layout_add(v_box, "edje/edit_btn.edj", "main");
   wid->edit_btn = edit_btn;
   evas_object_size_hint_weight_set(edit_btn, EVAS_HINT_EXPAND, 0);
   evas_object_size_hint_align_set(edit_btn, 1.0, EVAS_HINT_FILL);
   elm_layout_signal_emit(edit_btn, "mouse,clicked,*", "bg");
   elm_layout_signal_callback_add(edit_btn, "edit,done", "", _done_clicked_cb, wid);
   elm_layout_signal_callback_add(edit_btn, "edit,edit", "", _edit_clicked_cb, wid);
   evas_object_show(edit_btn);
   elm_box_pack_end(v_box, edit_btn);

   wid->box = elm_box_add(v_box);
   elm_box_horizontal_set(wid->box, EINA_TRUE);
   evas_object_size_hint_weight_set(wid->box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(wid->box, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(wid->box);
   elm_box_pack_end(v_box, wid->box);

	wid->grid = elm_grid_add(wid->win);
	evas_object_size_hint_weight_set(wid->grid, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(wid->grid, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_show(wid->grid);
	elm_box_pack_end(wid->box, wid->grid);


   if (abs(w - h) < 10) // 4 x 4
      elm_grid_size_set(wid->grid, 4, 4);
   else // 4 x 2
      elm_grid_size_set(wid->grid, 4, 2);

   wid->plus_icon = my_layout_add(wid->grid, "edje/app_icon_notitle.edj", "plus");
   evas_object_size_hint_weight_set(wid->plus_icon, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(wid->plus_icon, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_layout_signal_callback_add(wid->plus_icon, "plus,clicked", "", _plus_clicked_cb, wid);
   evas_object_show(wid->plus_icon);
   elm_grid_pack(wid->grid, wid->plus_icon, 0, 0, 1, 1);

	/* Show window after base gui is set up */
	evas_object_show(wid->win);

	widget_app_context_set_tag(context, wid);
	return WIDGET_ERROR_NONE;
}

static int
widget_instance_destroy(widget_context_h context, widget_app_destroy_type_e reason, bundle *content, void *user_data)
{
	widget_instance_data_s *wid = NULL;
	widget_app_context_get_tag(context,(void**)&wid);

	if (wid->win)
		evas_object_del(wid->win);

	free(wid);

	return WIDGET_ERROR_NONE;
}

static int
widget_instance_pause(widget_context_h context, void *user_data)
{
   widget_instance_data_s *wid = NULL;
   widget_app_context_get_tag(context,(void**)&wid);
   const char *state;

   if (wid->keep_icon_select)
      return WIDGET_ERROR_NONE;

   state = edje_object_part_state_get(elm_layout_edje_get(wid->edit_btn), "text", NULL);

   if (!strcmp(state, "done"))
      elm_layout_signal_emit(wid->edit_btn, "mouse,clicked,*", "bg");
   dlog_print(DLOG_ERROR, LOG_TAG, "widget_instance_pause");
	return WIDGET_ERROR_NONE;

}

static int
widget_instance_resume(widget_context_h context, void *user_data)
{
   dlog_print(DLOG_ERROR, LOG_TAG, "widget_instance_resume");
	return WIDGET_ERROR_NONE;
}

static int
widget_instance_update(widget_context_h context, bundle *content,
                             int force, void *user_data)
{
	/* Take necessary actions when widget instance should be updated. */
	return WIDGET_ERROR_NONE;
}

static int
widget_instance_resize(widget_context_h context, int w, int h, void *user_data)
{
	/* Take necessary actions when the size of widget instance was changed. */
   dlog_print(DLOG_ERROR, LOG_TAG, "widget_instance_resize");
	return WIDGET_ERROR_NONE;
}

static void
widget_app_lang_changed(app_event_info_h event_info, void *user_data)
{
	/* APP_EVENT_LANGUAGE_CHANGED */
	char *locale = NULL;
	app_event_get_language(event_info, &locale);
	elm_language_set(locale);
	free(locale);
}

static void
widget_app_region_changed(app_event_info_h event_info, void *user_data)
{
	/* APP_EVENT_REGION_FORMAT_CHANGED */
}

static widget_class_h
widget_app_create(void *user_data)
{
	/* Hook to take necessary actions before main event loop starts.
	   Initialize UI resources.
	   Make a class for widget instance.
	*/
	app_event_handler_h handlers[5] = {NULL, };

	widget_app_add_event_handler(&handlers[APP_EVENT_LANGUAGE_CHANGED],
		APP_EVENT_LANGUAGE_CHANGED, widget_app_lang_changed, user_data);
	widget_app_add_event_handler(&handlers[APP_EVENT_REGION_FORMAT_CHANGED],
		APP_EVENT_REGION_FORMAT_CHANGED, widget_app_region_changed, user_data);

	widget_instance_lifecycle_callback_s ops = {
		.create = widget_instance_create,
		.destroy = widget_instance_destroy,
		.pause = widget_instance_pause,
		.resume = widget_instance_resume,
		.update = widget_instance_update,
		.resize = widget_instance_resize,
	};

	return widget_app_class_create(ops, user_data);
}

static void
widget_app_terminate(void *user_data)
{
	/* Release all resources. */
}

int
main(int argc, char *argv[])
{
	widget_app_lifecycle_callback_s ops = {0,};
	int ret;

	ops.create = widget_app_create;
	ops.terminate = widget_app_terminate;

	ret = widget_app_main(argc, argv, &ops, NULL);
	if (ret != WIDGET_ERROR_NONE) {
		dlog_print(DLOG_ERROR, LOG_TAG, "widget_app_main() is failed. err = %d", ret);
	}

	return ret;
}


