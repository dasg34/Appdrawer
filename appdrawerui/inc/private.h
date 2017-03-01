#ifndef __appdrawerui_H__
#define __appdrawerui_H__

#include <app.h>
#include <Elementary.h>
#include <system_settings.h>
#include <efl_extension.h>
#include <dlog.h>

#ifdef  LOG_TAG
#undef  LOG_TAG
#endif
#define LOG_TAG "appdrawerui"

#if !defined(PACKAGE)
#define PACKAGE "org.yohoho.appdrawer"
#endif

#define DEBUG 1

#if DEBUG == 1
#define dlog_print(...) ;
#endif

#endif /* __appdrawerui_H__ */
