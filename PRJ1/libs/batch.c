#include <os/kernel.h>
#include <os/batch.h>
#include <os/string.h>


//约定0x51000000开始为字符串存放位置
//每个字符串最长31, 考虑到以'\0'结尾不计入, 实际空间最大32
//最多有8个字符串

char *str_p;

void batch_init(void)
{
    char *clear_p = BATCH_STR_STA;
    memset(clear_p, 0, BATCH_STR_SPA * BATCH_STR_NUM);
    str_p = BATCH_STR_STA;
}

void batch_putstr(const char *src)
{
    int i = 0;
    char *next_p = str_p + BATCH_STR_SPA;
    
    if(str_p < BATCH_STR_STA || str_p >= BATCH_STR_LIM){
        bios_putstr("\n\rWARNING:Batch str_p error!\n\r");
        return;
    }
    for(i = 0; i < BATCH_STR_LEN && src[i]; i++){
        *str_p++ = src[i];
    }
    *(str_p+i) = '\0';// i最大为BATCH_STR_LEN
    str_p = next_p;
}
int batch_load(char *dest)
{
    int num = 0;
    char *load_p = BATCH_STR_STA;

    for(num = 0; num < BATCH_STR_NUM && load_p[0]!='\0'; num++, load_p += BATCH_STR_SPA, dest += BATCH_STR_SPA){
        strncpy(dest, load_p, BATCH_STR_SPA);
    }
    return num;
}


