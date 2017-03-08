#include <APPDRAWER.h>
#include <app_manager.h>

static char *
_app_res_path_get(const char *res_name)
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

Evas_Object *
my_layout_add(Evas_Object *parent, const char *edj_name, const char *group)
{
   Evas_Object *layout;
   char *path;

   path = _app_res_path_get(edj_name);

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
_bundle_data_save(widget_instance_data_s *wid)
{
   char **app_id_ary;
   int list_count;

   if (!wid->app_list)
      return;

   list_count = eina_list_count(wid->app_list);
   app_id_ary = (char **)calloc(list_count, sizeof(char *));

   for (int i = 0; i < list_count; i++)
      app_id_ary[i] = eina_list_nth(wid->app_list, i);

   bundle_del(wid->bundle, "app_list");
   int ret = bundle_add_str_array(wid->bundle, "app_list", (const char**)app_id_ary, list_count);
   dlog_print(DLOG_ERROR, LOG_TAG, "ret : %d", ret);
   free(app_id_ary);
   ret = widget_app_context_set_content_info(wid->context, wid->bundle);
   dlog_print(DLOG_ERROR, LOG_TAG, "widget_app_context_set_content_info : %d", ret);
}

static void
_app_grid_rearrange(widget_instance_data_s *wid)
{
   int list_count;
   Eina_List *l, *grid_children;
   Evas_Object *app_obj;

   grid_children = elm_grid_children_get(wid->grid);


   EINA_LIST_FOREACH(grid_children, l, app_obj)
      elm_grid_unpack(wid->grid, app_obj);

   list_count = eina_list_count(grid_children);
   for (int i = 0; i < list_count; i++)
      {
         app_obj = eina_list_nth(grid_children, i);
         elm_grid_pack(wid->grid, app_obj, i % 4, i / 4, 1, 1);

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
   Eina_List *l;
   char *list_data, *app_id;

   app_id = evas_object_data_get(obj, "app_id");
   dlog_print(DLOG_ERROR, LOG_TAG, "app_id = %s", app_id);
   EINA_LIST_FOREACH(wid->app_list, l, list_data)
   {
      dlog_print(DLOG_ERROR, LOG_TAG, "list_data = %s", list_data);
      if (!strcmp(app_id, list_data))
         {
            wid->app_list = eina_list_remove(wid->app_list, list_data);
            break;
         }
   }
   wid->app_count--;

   elm_grid_unpack(wid->grid, obj);
   evas_object_del(obj);
   elm_grid_unpack(wid->grid, wid->plus_icon);

   _bundle_data_save(wid);

   _app_grid_rearrange(wid);

   free(app_id);
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

   free(id);
}

static void
_app_shortcut_delete_hide_all(widget_instance_data_s *wid)
{
   Eina_List *l, *grid_children;
   Evas_Object *app_obj;

   grid_children = elm_grid_children_get(wid->grid);

   if (!grid_children)
      return;

   EINA_LIST_FOREACH(grid_children, l, app_obj)
      elm_layout_signal_emit(app_obj, "delete,hide", "");
}

static void
_app_shortcut_delete_show_all(widget_instance_data_s *wid)
{
   Eina_List *l, *grid_children;
   Evas_Object *app_obj;

   grid_children = elm_grid_children_get(wid->grid);

   if (!grid_children)
      {
         elm_grid_pack(wid->grid, wid->plus_icon, 0, 0, 1, 1);
         return;
      }

   EINA_LIST_FOREACH(grid_children, l, app_obj)
      elm_layout_signal_emit(app_obj, "delete,show", "");
}

static void
_app_shortcut_add(widget_instance_data_s *wid, char *id, Eina_Bool bundle_add)
{
   Eina_List *l;
   app_info_h app_info;
   int count, by4max, by2max;
   char *path, *label, *app_id;
   dlog_print(DLOG_ERROR, LOG_TAG, "id = %s", id);
   if (wid->no_label)
      {
         by4max = 16;
         by2max = 8;
      }
   else
      {
         by4max = 12;
         by2max = 4;
      }

   app_info_create(id, &app_info);
   app_info_get_icon(app_info, &path);
   app_info_get_label(app_info, &label);
   app_info_destroy(app_info);

   dlog_print(DLOG_ERROR, LOG_TAG, "path = %s", path);
   wid->keep_icon_select = EINA_FALSE;

   EINA_LIST_FOREACH(wid->app_list, l, app_id)
      {
         if (!strcmp(id, app_id))
            {
               free(path);
               free(label);
               _popup_toast_open(wid->win);
               return;
            }
      }

   Evas_Object *app_obj;
   if (wid->no_label)
      app_obj = my_layout_add(wid->grid, "edje/app_icon_notitle.edj", "main");
   else
      app_obj = my_layout_add(wid->grid, "edje/app_icon.edj", "main");
   evas_object_size_hint_weight_set(app_obj, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(app_obj, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_layout_signal_callback_add(app_obj, "delete,clicked", "", _delete_clicked_cb, wid);
   elm_layout_signal_callback_add(app_obj, "icon,clicked", "", _icon_clicked_cb, strdup(id));
   elm_layout_signal_emit(app_obj, "delete,show", "");
   evas_object_data_set(app_obj, "app_id", strdup(id));
   if (!wid->no_label)
      elm_object_part_text_set(app_obj, "text", label);
   evas_object_show(app_obj);

   Evas_Object *img = elm_image_add(app_obj);
   evas_object_size_hint_weight_set(img, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(img, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_image_file_set(img, path, NULL);
   elm_object_part_content_set(app_obj, "icon", img);

   wid->app_count++;
   wid->app_list = eina_list_append(wid->app_list, strdup(id));

   count = eina_list_count(wid->app_list);
   elm_grid_pack(wid->grid, app_obj, (count - 1) % 4, (count - 1) / 4, 1, 1);

   if (bundle_add)
      {
         elm_grid_unpack(wid->grid, wid->plus_icon);
         evas_object_hide(wid->plus_icon);
         if (abs(wid->_w - wid->_h) < 10)
            {
               if (count < by4max)
                  {
                     elm_grid_pack(wid->grid, wid->plus_icon, count % 4, count / 4, 1, 1);
                     evas_object_show(wid->plus_icon);
                  }
            }
         else
            {
               if (count < by2max)
                  {
                     elm_grid_pack(wid->grid, wid->plus_icon, count % 4, count / 4, 1, 1);
                     evas_object_show(wid->plus_icon);
                  }
            }
      }
   else
      _app_shortcut_delete_hide_all(wid);

   free(path);
   free(label);
}

static void
_app_control_reply_cb(app_control_h request, app_control_h reply, app_control_result_e result, void *data)
{
   widget_instance_data_s *wid = data;
   char *id;

   app_control_get_extra_data(reply, "app_id", &id);

   _app_shortcut_add(wid, id, EINA_TRUE);
   _bundle_data_save(wid);
   free(id);
}

void
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

void
_done_clicked_cb(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   widget_instance_data_s *wid = data;

   elm_layout_signal_emit(wid->plus_icon, "plus,hide", "");

   _app_shortcut_delete_hide_all(wid);
}

void
_edit_clicked_cb(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   widget_instance_data_s *wid = data;
   int list_count;
   int by4max, by2max;

   if (wid->no_label)
      {
         by4max = 16;
         by2max = 8;
      }
   else
      {
         by4max = 12;
         by2max = 4;
      }

   elm_layout_signal_emit(wid->plus_icon, "plus,show", "");

   _app_shortcut_delete_show_all(wid);

   list_count = eina_list_count(wid->app_list);

   if (abs(wid->_w - wid->_h) < 10)
      {
         if (list_count >= by4max)
            return;
      }
   else
      {
         if (list_count >= by2max)
            return;
      }

   elm_grid_pack(wid->grid, wid->plus_icon, list_count % 4, list_count / 4, 1, 1);
}

int
_bundle_prev_state_set(widget_instance_data_s *wid)
{
   int ary_count;
   char **app_list;
   app_list = (char **)bundle_get_str_array(wid->bundle, "app_list", &ary_count);
   if (!app_list)
      return 0;
   dlog_print(DLOG_ERROR, LOG_TAG, "ary_count = %d", ary_count);
   for (int i = 0; i < ary_count; i++)
      _app_shortcut_add(wid, app_list[i], EINA_FALSE);

   return ary_count;
}
