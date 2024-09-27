#include <batch.h>
#include <kernel.h>

int main(void){
    char strarr[BATCH_STR_NUM][BATCH_STR_SPA];
    int num;
    num = batch_load(&strarr[0][0]);
    int dedup[BATCH_STR_NUM] = {0};

    // bios_putstr("TEST POINT4!!!!!!!\n\r\n\r");
    // bios_putchar('0'+num);
    // for(int k = 0; k < num; k++){
    //     bios_putstr(strarr[k]);
    // }

    for (int i = 0; i < num - 1; i++){
        if(!strncmp(strarr[i], strarr[i+1], BATCH_STR_SPA)){
            dedup[i] = 1;
        }
    }

    batch_init();
    for(int i = 0; i < num; i++){
        if(!dedup[i]){
            batch_putstr(strarr[i]);
            // bios_putchar('a');
            // bios_putstr(strarr[i]);
        }
    }
    return 0;
}