#ifndef __FUNC_CMD_COMMON_H__
#define __FUNC_CMD_COMMON_H__

bool fm_func_set(void *priv, u8 *data, u16 len);
u32 fm_func_get(void *priv, u8 *buf, u16 buf_size, u32 mask);

bool rtc_func_set(void *priv, u8 *data, u16 len);
u32 rtc_func_get(void *priv, u8 *buf, u16 buf_size, u32 mask);

bool music_func_set(void *priv, u8 *data, u16 len);
u32 music_func_get(void *priv, u8 *buf, u16 buf_size, u32 mask);

bool bt_func_set(void *priv, u8 *data, u16 len);
u32 bt_func_get(void *priv, u8 *buf, u16 buf_size, u32 mask);

bool linein_func_set(void *priv, u8 *data, u16 len);
u32 linein_func_get(void *priv, u8 *buf, u16 buf_size, u32 mask);
#endif
