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
 * File      : uKey.c
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
 *  
 *  Change Log :
 *  2017/6/23 [FlandreUNX] 源码构建.
 *    基于线程的按键扫描,使用回调隔离操作
 *    支持最大32个普通按键或31普通+1Shift按键.
 *    支持每个按键的短按连发或长按功能,包括Shift+按键的短按连发/长按功能.
 *    支持每个按键的触发电平类型.
 *  2017/7/7 [FlandreUNX] 添加虚拟按键.
 *    增加对多个按键同时按下时,触发一个新的按键值.
 *    
 */

#include "./uKey.h"

/**
 * @addtogroup hal
 */
 
/*@{*/

#include "stm32f0xx.h"                  // Device header

/*@}*/

/**
 * @addtogroup os include
 */

/*@{*/

#if KEY_OS_TYPE == 1
#include "cmsis_os.h"                   // ARM::CMSIS:RTOS:Keil RTX
#endif

/*@}*/

/**
 * @addtogroup Key var
 */
 
/*@{*/

/**
 * 前次按键扫描值
 * @note none
 */
static uKey_Bitmap_t preScanKey = 0;                     

/**
 * 前次按键读取值
 * @note none
 */
static uKey_Bitmap_t preReadKey = 0;  

/**
 * 按键掩码
 * @note none
 */
static uKey_Bitmap_t keyMask = 0;

#if USE_SHIFT_KEY == 1
/**
 * shift按键记录
 * @note none
 */
static uKey_Bitmap_t keyShift = 0;

/**
 * shift按键对象
 * @note none
 */
static uKeyDef_t key_ShiftObj = {
  .port = 0,
  .value = 0x01
};
#endif

static uKey_Bitmap_t keyPressTmr = KEY_PRESS_DLY;

/**
 * 长按键编码掩码
 * @note none
 */
static uKey_Bitmap_t key_LongMask;

/**
 * 按键对象集合
 * @note none
 */
static struct __list_t keysList;

/*@}*/

/**
 * @addtogroup FIFO Buffer var
 */
 
/*@{*/

/**
 * 环形buf
 * @note none
 */
static uKey_Bitmap_t keyBuf[KEY_BUFFER_SIZE];

/**
 * buf入指针
 * @note none
 */
static uKey_Bitmap_t keyBufInIx = 0;    

/**
 * buf出指针
 * @note none
 */
static uKey_Bitmap_t keyBufOutIx = 0; 

/**
 * buf中按键数量
 * @note none
 */
static uKey_Bitmap_t keyNRead = 0;

/*@}*/

/**
 * @addtogroup simplify list Functions
 */
 
/*@{*/

/**
 * 获取当前list_head链表节点所在的宿主结构项
 *
 * @param ptr 宿主结构体下的osList_t*
 * @param type 宿主类型
 * @param name 宿主结构类型定义中osList_t成员名
 * 
 * @retval 宿主结构体指针
 */
#define list_Entry(ptr, type, name) \
  (type*)((uint8_t*)(ptr) - ((uint32_t) &((type *)0)->name))

	
/**
 * 遍历链表中的所有list_head节点
 *
 * @param pos 位置
 * @param head 节点头
 * 
 * @retval none
 */
#define list_ForEach(pos, head) \
	for (pos = (head)->next; pos != (head); pos = pos->next)
  
    
/**
 * 初始化节点
 *
 * @param head 节点 
 * 
 * @retval none
 */	
static __inline void list_HeadInit(struct __list_t* head) {
	head->next = head;
	head->previous = head;
}


/**
 * 将newNode添加到list节点尾部
 *
 * @param list 原节点 
 * @param newNode 新节点 
 * 
 * @retval none
 */	
static __inline void list_AddTail(struct __list_t* list, struct __list_t* newNode) {
  list->previous->next = newNode;
  newNode->previous = list->previous;

  list->previous = newNode;
  newNode->next = list;
}


/**
 * 将newNode添加到list节点头部
 *
 * @param list 原节点 
 * @param newNode 新节点 
 * 
 * @retval none
 */	
static __inline void list_Add(struct __list_t* list, struct __list_t* newNode) {
  list->next->previous = newNode;
  newNode->next = list->next;

  list->next = newNode;
  newNode->previous = list;
}

/*@}*/

/**
 * @addtogroup Key Functions
 */
 
/*@{*/

/**
 * 读取一次GPIO状态
 *
 * @param none
 *
 * @return uint8_t key扫描结果
 */
static __inline uKey_Bitmap_t GPIORead(void) {
  uKey_Bitmap_t keyScanCode = 0;
  uKeyDef_t *obj;
  struct __list_t *index;

#if USE_SHIFT_KEY == 1
  keyScanCode |= ((((GPIO_TypeDef *)key_ShiftObj.port)->IDR) & key_ShiftObj.gpio) ? 
    (key_ShiftObj.level ? key_ShiftObj.value : 0) : 
    (key_ShiftObj.level ? 0 : key_ShiftObj.value);
#endif

  list_ForEach(index, &keysList) {
    obj = list_Entry(index, uKeyDef_t, list);
    
    if (obj->type & KEY_TYPE_MULTI) {
      if (keyScanCode == obj->value2) {
        keyScanCode = obj->value;
      }
    }
    else {
      keyScanCode |= ((((GPIO_TypeDef *)obj->port)->IDR) & obj->gpio) ? 
        (obj->level ? obj->value : 0) : 
        (obj->level ? 0 : obj->value);
    }
  }  

  return keyScanCode;
}


/**
 * 读取缓冲区中按键信息
 *
 * @param none
 *
 * @return uKey_Bitmap_t key
 */
static uKey_Bitmap_t keyBufferOut(void) {
	uKey_Bitmap_t code = 0;
	
	if (keyNRead > 0) {     
		/*-1表示取走一个按键值*/
		keyNRead--;
		
		/*从buf中取走一个扫描码*/
		code = keyBuf[keyBufOutIx];
		keyBufOutIx++;
		
		/*buf满则指针循环回到起点*/
		if (keyBufOutIx >= KEY_BUFFER_SIZE) {
			keyBufOutIx = 0;
		}
		
		/*返回按键扫描值*/
		return (code); 
	}
	else {
		/*没有按键,则返回0*/
		return 0;
	}
}


/**
 * 写入一个按键信息到缓冲区
 *
 * @param none
 *
 * @return none
 */
static void keyBufferIn(uKey_Bitmap_t code) {    
	/*buf满则放弃最早的一个按键值*/
	if (keyNRead >= KEY_BUFFER_SIZE) {               
		keyBufferOut();                                                 
	}

	keyNRead++;
	
	/*输入按键扫描码*/
	keyBuf[keyBufInIx++] = code;
	
	/*buf满则指针循环回到起点*/
	if (keyBufInIx >= KEY_BUFFER_SIZE) {        
		keyBufInIx = 0;
	}                      
}


/**
 * 周期扫描按键
 *
 * @param none
 *
 * @return none
 */
static void uKey_Scan(void) {
	/*当前按键值扫描值*/
	uKey_Bitmap_t nowScanKey = 0;
	/*当前按键值*/
	uKey_Bitmap_t nowReadKey = 0;
	
	/*按键按下*/
	//uKey_Bitmap_t keyPressDown = 0;
  
	/*按键释放*/
	uKey_Bitmap_t keyRelease = 0;
	
	nowScanKey = GPIORead();
	
	/*电平触发 | 采样保持(即消抖)*/
	nowReadKey = (preScanKey & nowScanKey) | (preReadKey & (preScanKey ^ nowScanKey));

	/*上升沿触发*/
	//KeyPressDown  = NowReadKey & (NowReadKey ^ PreReadKey);  
  
	/*下降沿触发*/
	keyRelease = (preReadKey & (nowReadKey ^ preReadKey));

	/*用电平触发做长按键的有效判据*/
	if (nowReadKey == preReadKey && nowReadKey) {
		keyPressTmr--;
		
		/*长按判断周期到,保存相应长按键值*/
		if (!keyPressTmr) {
			/*长按键-连发模式*/
			if (nowReadKey & ~(key_LongMask)) {
#if USE_SHIFT_KEY == 1
				keyBufferIn(nowReadKey | keyShift);
#else
				keyBufferIn(nowReadKey);
#endif
			}
			/*长按键-反码模式*/
			else if (nowReadKey & (key_LongMask) & ~keyMask) {
#if USE_SHIFT_KEY == 1
				keyBufferIn(~(nowReadKey | keyShift));  
#else
				keyBufferIn(~nowReadKey);
#endif
			}
			/*重置连按周期,准备获取下1个长按键*/
			keyPressTmr = KEY_PRESS_TMR;
      
			keyMask = nowReadKey;
		}
	}
	else {
		/*按键变化,重置按键判断周期*/
		keyPressTmr = KEY_PRESS_DLY;
	}
	
	/*短按键判断*/
	if (keyRelease) {
		if (keyRelease & (~keyMask)) {
			/*shift按键码(边缘触发)*/
#if USE_SHIFT_KEY == 1
			keyShift ^= (keyRelease & (key_ShiftObj.keyValue));
      
			keyBufferIn(keyRelease | keyShift);
#else
			keyBufferIn(keyRelease);
#endif
		}
		else {
			keyMask = 0;
		}
	}
  
  preScanKey = nowScanKey;
  preReadKey = nowReadKey; 
}


/**
 * 获取按键
 *
 * @param none
 *
 * @return uKey_Bitmap_t 按键bitmap
 */
static __inline uKey_Bitmap_t uKey_Get(void) {                         
  return keyBufferOut();
}


/**
 * 判断是否有按键按下
 *
 * @param none
 *
 * @return uint8_t 返回结果
 */
static __inline uint8_t uKey_IsHit(void) {
  return keyNRead > 0 ? 1 : 0;
}

/*@}*/

/**
 * @addtogroup Key Thread def
 */
 
/*@{*/
#if KEY_OS_TYPE == 1
void thread_uKey(const void *arg) {
  for (;;) {
    osDelay(30);

    uKey_Scan();
  
    if (!uKey_IsHit()) {
      continue;
    }

    uKey_Bitmap_t keyGet;
    keyGet = uKey_Get();

    uKeyDef_t *obj = 0;
    struct __list_t *index = 0;
    uint8_t keyPressType = KEY_IS_NONE;
    list_ForEach(index, &keysList) {
      obj = list_Entry(index, uKeyDef_t, list);
      
      keyPressType = uKey_GetPressType(obj, keyGet);
      
      if (!keyPressType) {
        continue;
      }
      
      if (obj->callback) {
        obj->callback(obj, keyPressType);
      }
    }
  }
}
osThreadDef(thread_uKey, osPriorityBelowNormal , 1, 0);
#endif

/*@}*/

/**
 * @addtogroup uKey user functions
 */
 
/*@{*/

/**
 * 按键模块启动
 *
 * @param none
 *
 * @return none
 */
void uKey_Startup(void) {
#if USE_SHIFT_KEY == 1
  /*检查shift是否初始化*/
  if (!key_ShiftObj.port) {
    for (;;);
  }
#endif
  
  list_HeadInit(&keysList);
  
#if KEY_OS_TYPE == 1
  osThreadCreate(osThread(thread_uKey), NULL);
#endif
}


#if USE_SHIFT_KEY == 1
/**
 * shift按键初始化
 *
 * @param port
 * @param gpio 
 *
 * @return none
 */
void uKey_ShiftKeySet(uint32_t port, uint32_t gpio, uint8_t level) {
  key_ShiftObj.port = port;
  key_ShiftObj.gpio = gpio;
  key_ShiftObj.level = level;
}
#endif


/**
 * 按键对象插入
 *
 * @param obj 按键对象指针
 *
 * @return none
 */
void uKey_ObjInsert(uKeyDef_t *obj) {
  list_HeadInit(&obj->list);
  
  if (obj->type & KEY_TYPE_LONG) {
    key_LongMask |= obj->value;
    obj->valueLong = ~obj->value;
  }
  
  if (obj->type & KEY_TYPE_MULTI) {
    list_AddTail(&keysList, &obj->list);
  }
  else {
    list_Add(&keysList, &obj->list);
  }
}


/**
 * 判断按键是否输出类型(短/长按/shift)
 *
 * @param obj 按键对象指针
 *
 * @return 类型
 */
uint8_t uKey_GetPressType(uKeyDef_t *obj, uKey_Bitmap_t value) {
#if USE_SHIFT_KEY == 1
  /*短按*/
  if (value == obj->value) {
    return KEY_IS_SP;
  }
  /*长按*/
  else if (value == obj->valueLong)) {
    return KEY_IS_LP;
  }
  /*shift + 短按*/
  else if (value == (obj->vaule | key_ShiftObj.vaule)) {
    return KEY_IS_SSP;
  }
  /*shift + 长按 */
  else if (value == (~(obj->vaule | key_ShiftObj.vaule))) {
    return KEY_IS_SLP;
  }
  /*无类型*/
  else {
    return KEY_IS_NONE;
  }
#else
  /*短按*/
  if (value == obj->value) {
    return KEY_IS_SP;
  }
  /*长按*/
  else if (value == obj->valueLong) {
    return KEY_IS_LP;
  }
  /*无类型*/
  else {
    return KEY_IS_NONE;
  }
#endif
}

/*@}*/
