#include <private.h>
#include <tizen.h>

static int
widget_instance_create(widget_context_h context, bundle *content, int w, int h, void *user_data)
{
	widget_instance_data_s *wid = (widget_instance_data_s*) calloc(1, sizeof(widget_instance_data_s));
	int ret;

   if (content != NULL)
      wid->bundle = bundle_dup(content);
   else
      wid->bundle = bundle_create();

   wid->context = context;

   wid->no_label = EINA_FALSE;

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

   if (abs(w - h) < 10) // 4 x 3
      elm_grid_size_set(wid->grid, 4, 3);
   else // 4 x 1
      elm_grid_size_set(wid->grid, 4, 1);

   wid->plus_icon = my_layout_add(wid->grid, "edje/app_icon.edj", "plus");
   evas_object_size_hint_weight_set(wid->plus_icon, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(wid->plus_icon, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_layout_signal_callback_add(wid->plus_icon, "plus,clicked", "", _plus_clicked_cb, wid);
   evas_object_show(wid->plus_icon);

   wid->app_count = _bundle_prev_state_set(wid);

   if (wid->app_count <= 0)
      {
         elm_layout_signal_emit(edit_btn, "mouse,clicked,*", "bg");

         elm_grid_pack(wid->grid, wid->plus_icon, 0, 0, 1, 1);
      }
   else
      elm_layout_signal_emit(wid->plus_icon, "plus,hide", "");

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


