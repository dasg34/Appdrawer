#ifndef _APPDRAWERLIB_H_
#define _APPDRAWERLIB_H_

/**
 * This header file is included to define _EXPORT_.
 */
#include <stdbool.h>
#include <tizen.h>
#include <Elementary.h>
#include <widget_app.h>
#include <widget_app_efl.h>
#include <Elementary.h>
#include <dlog.h>

#ifdef  LOG_TAG
#undef  LOG_TAG
#endif
#define LOG_TAG "appdrawer"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct widget_instance_data {
   Evas_Object *win;
   Evas_Object *grid;
   Evas_Object *box;
   Evas_Object *plus_icon;
   Evas_Object *edit_btn;
   Eina_List *app_list;
   Eina_Bool keep_icon_select;
   Eina_Bool no_label;
   widget_context_h context;
   bundle *bundle;
   int app_count;
   int _w, _h;
} widget_instance_data_s;

EXPORT_API Evas_Object *my_layout_add(Evas_Object *parent, const char *edj_name, const char *group);

EXPORT_API void _done_clicked_cb(void *data, Evas_Object *obj, const char *emission, const char *source);

EXPORT_API void _edit_clicked_cb(void *data, Evas_Object *obj, const char *emission, const char *source);

EXPORT_API int _bundle_prev_state_set(widget_instance_data_s *wid);

EXPORT_API void _plus_clicked_cb(void *data, Evas_Object *obj, const char *emission, const char *source);

#ifdef __cplusplus
}
#endif
#endif // _APPDRAWERLIB_H_

