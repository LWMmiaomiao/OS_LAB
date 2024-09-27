#ifndef __INCLUDE_BATCH_H__
#define __INCLUDE_BATCH_H__

#include <type.h>



//约定0x51000000开始为字符串存放位置
//每个字符串最长31, 考虑到以'\0'结尾不计入, 实际空间最大32
//最多有8个字符串
#define BATCH_STR_STA 0x50600000
#define BATCH_STR_LEN 31
#define BATCH_STR_SPA 32
#define BATCH_STR_NUM 8 //如果要超过十需要改输出逻辑
#define BATCH_STR_LIM (BATCH_STR_STA + BATCH_STR_SPA * BATCH_STR_NUM)

//#define BATCH_ALIGN(p) ((p) % 32 ? (p) - (p) % 32 + 32 : (p)) 



void batch_init(void);
void batch_putstr(const char *src);
int batch_load(char *dest);

#endif