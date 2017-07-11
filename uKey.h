/**　　　　　　 ,-...　　　 　 　 　　　..-.
 *　　　　　 ./:::::＼　　　　 　 　  ／:::::ヽ
 *　　　　  /::::::::::::;ゝ--──-- .._/:::::::ヽ
 *　　　　 /,.-‐''"′                  ＼:::::::|
 *　　　  ／　 　　　　　　　　　　　　  ヽ.::|
 *　　　　/　　　　●　　　 　 　 　 　 　 ヽ|
 *　　 　 l　　　...　　 　 　 　  ●　      l
 *　　　 .|　　　 　　 (_人__丿   ...　    |
 *　 　 　l　　　　　　　　　　　　 　　  l
 *　　　　` .　　　　　　　　 　 　 　　 /
 *　　　　　　`. .__　　　 　 　 　　.／
 *　　　　　　　　　/`'''.‐‐──‐‐‐┬---
 * File      : key.h
 * This file is part of aRTOS
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
 
#ifndef KEY_H_
#define KEY_H_

/**
 * @addtogroup std C lib include
 */

/*@{*/

#include <stdint.h>

/*@}*/

/**
 * @addtogroup Key function configure
 */
 
/*@{*/

/**
 *  实时系统(RTOS)类型
 *  @note 0: No OS
 *        1: RTX4
 *        2: aRTOS
 */
#define KEY_OS_TYPE      (1)

/**
 *  按键数目
 *  @note 包括普通按键,多重组合按键,Shift按键
 */
#define KEY_NUMBER        (3)

/**
 *  是否使用shift按键
 *  @note none
 */
#define USE_SHIFT_KEY     (0)

/**
 *  长按键判断周期
 *  @note 按键扫描周期 * KEY_PRESS_DLY = 长按判定
 */
#define KEY_PRESS_DLY     (20)

/**
 *  长按键判断周期
 *  @note 按键扫描周期 * KEY_PRESS_TMR = 长按发射一次
 */
#define KEY_PRESS_TMR     (3)

/**
 *  按键缓冲区大小
 *  @note none
 */
#define KEY_BUFFER_SIZE   (KEY_NUMBER * 2)

/*@}*/

/**
 * @addtogroup 按照按键数确定bitmap
 */
 
/*@{*/

#if KEY_NUMBER < 9
typedef uint8_t uKey_Bitmap_t;
#elif KEY_NUMBER < 17
typedef uint16_t uKey_Bitmap_t;
#else
typedef uint32_t uKey_Bitmap_t;
#endif

/*@}*/

/**
 * @addtogroup simplify list define
 */
 
/*@{*/

struct __list_t {
  struct __list_t *previous;    /**< 前节点 */
  struct __list_t *next;        /**< 后节点 */
};

/*@}*/

/**
 * @addtogroup Key object define
 */
 
/*@{*/

/**
 * 按键类型
 * @note 描述某key可触发的动作
 */
#define KEY_TYPE_SHORT     0x00  /**< 可短按连发 */
#define KEY_TYPE_LONG      0x01  /**< 不可连发,长按 */
#define KEY_TYPE_MULTI     0x02  /**< 多重按键 */


/**
 * key 触发电平类型
 * @note none
 */
#define KEY_LEVEL_HIGHT     0x01  /**< 高电平触发 */
#define KEY_LEVEL_LOW       0x00  /**< 低电平触发 */


/**
 * key 按下类型
 * @note 描述某key触发的动作类型
 */
#define KEY_IS_NONE      0x00      /**< 保留 */
#define KEY_IS_SP        0x01      /**< 短按 */
#define KEY_IS_LP        0x02      /**< 长按 */
#define KEY_IS_SSP       0x03      /**< shift + 短按 */
#define KEY_IS_SLP       0x04      /**< shift + 长按 */

typedef uint8_t uKey_type_t; 


/**
 * key 对象
 * @note 描述某key的配置
 */
typedef struct uKeyDef {
  uint8_t type;     /**< 按键类型 */
  uint8_t level;    /**< 按键电平 */
  
  uKey_Bitmap_t value;       /**< 按键内部触发值 */
  uKey_Bitmap_t value2;      /**< 按键内部触发值,用于触发多个按键同时按下的值 */
  uKey_Bitmap_t valueLong;   /**< 按键长按内部触发值 */
  
  uint32_t port;  /**< 按键port */
  uint32_t gpio;  /**< 按键GPIO */
  
  void (*callback)(void *obj, uKey_type_t type);  /**< 按键触发回调 */
  
  struct __list_t list;   /**< 对象链表头 */
} uKeyDef_t;

/*@}*/

/**
 * @addtogroup Key addons functions
 */
 
/*@{*/

/**
 * key callback
 * @note none
 */
#define uKey_Callback(name) \
  void name(void *obj, uKey_type_t type)
    

/**
 * key 获取按键值类型
 * @note none
 */
#define SELF_PRESS_TYPE \
  type

/*@}*/

/**
 * @addtogroup Key functions define
 */
 
/*@{*/

extern void uKey_Startup(void);

extern void uKey_ShiftKeySet(uint32_t port, uint32_t gpio);

extern void uKey_ObjInsert(uKeyDef_t *obj);
  
extern uint8_t uKey_GetPressType(uKeyDef_t *obj, uKey_Bitmap_t vaule);

/*@}*/

#endif
