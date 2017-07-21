# uKey
# 使用范例
```C
uKey_Callback(keyUP_Callback) {
  osSignalSet(main_ThreadID, KEY_UP_SIGNAL);
}
uKeyDef_t key_Up = {
  .type = KEY_TYPE_SHORT,
  .level = KEY_LEVEL_LOW,
  
  .value = 0x02,
  
  .port = (uint32_t)GPIOA,
  .gpio = GPIO_Pin_3,

  .callback = keyUP_Callback
};


uKey_Callback(keyDown_Callback) {
  osSignalSet(main_ThreadID, KEY_DOWN_SIGNAL);
}
uKeyDef_t key_Down = {
  .type = KEY_TYPE_SHORT,
  .level = KEY_LEVEL_LOW,
  
  .value = 0x04,
  
  .port = (uint32_t)GPIOA,
  .gpio = GPIO_Pin_2,

  .callback = keyDown_Callback
};


uKey_Callback(keyMulti_Callback) {
  if (SELF_PRESS_TYPE == KEY_IS_SP) {
    osSignalSet(main_ThreadID, KEY_OK_SHORT_SIGNAL);
  }
  else {
    osSignalSet(main_ThreadID, KEY_OK_LONG_SIGNAL);
  }
}
uKeyDef_t key_Multi = {
  .type = KEY_TYPE_LONG | KEY_TYPE_MULTI,

  .value = 0x08,
  .value2 = 0x06, /*< Up & Down */
  
  .callback = keyMulti_Callback
};


void key_InterfaceInit(void) {
  GPIO_InitTypeDef gpio;
  gpio.GPIO_Mode = GPIO_Mode_IN;
  gpio.GPIO_OType = GPIO_OType_PP;
  gpio.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_3;
  gpio.GPIO_PuPd = GPIO_PuPd_UP;
  gpio.GPIO_Speed = GPIO_Speed_Level_3;
  GPIO_Init(GPIOA, &gpio);
  
  uKey_ObjInsert(&key_Up);
  uKey_ObjInsert(&key_Down);
  uKey_ObjInsert(&key_Multi);
}


int main(void) {
  uKey_Startup();
  key_InterfaceInit();
}
```
