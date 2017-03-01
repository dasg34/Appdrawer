#ifndef __private_H__
#define __private_H__

#include <widget_app.h>
#include <widget_app_efl.h>
#include <Elementary.h>
#include <dlog.h>

#ifdef  LOG_TAG
#undef  LOG_TAG
#endif
#define LOG_TAG "appdrawer"

#if !defined(PACKAGE)
#define PACKAGE "org.yohoho.appdrawer"
#endif

#define DEBUG 1

#if DEBUG == 1
#define dlog_print(...) ;
#endif

#endif /* __private_H__ */
