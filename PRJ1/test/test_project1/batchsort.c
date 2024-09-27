#include <batch.h>
#include <kernel.h>

int main(void){
    char strarr[BATCH_STR_NUM][BATCH_STR_SPA];
    int num;
    num = batch_load(&strarr[0][0]);

    char temp[BATCH_STR_SPA];// 存储字符排序的交换内容
    for (int i = 0; i < num - 1; i++){
        for (int j = 0; j < num - i - 1; j++){
            if (strncmp(strarr[j], strarr[j+1], BATCH_STR_SPA) > 0) {
                strncpy(temp, strarr[j], BATCH_STR_SPA);
                strncpy(strarr[j], strarr[j+1], BATCH_STR_SPA);
                strncpy(strarr[j+1], temp, BATCH_STR_SPA);
            }
        }
    }

    batch_init();
    for(int i = 0; i < num; i++){
        batch_putstr(strarr[i]);
        // bios_putchar('b');
        // bios_putstr(strarr[i]);
    }

    return 0;
}