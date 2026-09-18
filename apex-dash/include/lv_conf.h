/**
 * @file lv_conf.h
 * Configuration file for LVGL v9 in Apex-Dash Firmware (ESP-IDF)
 */

#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

/*====================
   COLOR SETTINGS
 *====================*/
#define LV_COLOR_DEPTH 16

/*=========================
   MEMORY SETTINGS
 *=========================*/
#define LV_USE_STDLIB_MALLOC LV_STDLIB_BUILTIN
#define LV_USE_STDLIB_STRING LV_STDLIB_BUILTIN
#define LV_USE_STDLIB_SPRINTF LV_STDLIB_BUILTIN

#define LV_MEM_SIZE (64 * 1024)

/*====================
   HAL SETTINGS
 *====================*/
#define LV_DEF_REFR_PERIOD 16    /* ~60 Hz */
#define LV_DPI_DEF 130

/*=======================
   OPERATING SYSTEM
 *=======================*/
#define LV_USE_OS LV_OS_FREERTOS

/*========================
   RENDERING & DRAWING
 *========================*/
#define LV_DRAW_BUF_STRIDE_ALIGN 1
#define LV_DRAW_BUF_ALIGN 4

/*==================
   LOG SETTINGS
 *==================*/
#define LV_USE_LOG 1
#define LV_LOG_LEVEL LV_LOG_LEVEL_WARN
#define LV_LOG_PRINTF 1

/*==================
   FONT USAGE
 *==================*/
#define LV_FONT_MONTSERRAT_10 1
#define LV_FONT_MONTSERRAT_12 1
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_16 1
#define LV_FONT_MONTSERRAT_18 1
#define LV_FONT_MONTSERRAT_20 1
#define LV_FONT_MONTSERRAT_24 1
#define LV_FONT_MONTSERRAT_28 1
#define LV_FONT_MONTSERRAT_32 1
#define LV_FONT_MONTSERRAT_36 1
#define LV_FONT_MONTSERRAT_40 1
#define LV_FONT_MONTSERRAT_48 1

#define LV_FONT_DEFAULT &lv_font_montserrat_14

/*==================
   WIDGET USAGE
 *==================*/
#define LV_USE_LABEL 1
#define LV_USE_BAR 1
#define LV_USE_LINE 1
#define LV_USE_ARC 1
#define LV_USE_CANVAS 1
#define LV_USE_TABLE 1

/*==================
   DRIVERS (SDL disabled on embedded)
 *==================*/
#define LV_USE_SDL 0

#endif /* LV_CONF_H */
