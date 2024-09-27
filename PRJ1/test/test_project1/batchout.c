#include <batch.h>
#include <kernel.h>

int main(void){
    char strarr[BATCH_STR_NUM][BATCH_STR_SPA];

    int num;
    num = batch_load(&strarr[0][0]);
    for(int i = 0; i < num + 1; i++){
        bios_putstr("line[");
        bios_putchar(i + '1');
        bios_putstr("]: ");

        if(i != num){
            bios_putstr(strarr[i]);
        }
        else{
            bios_putstr("Guo ZiYing\n\r");
        }
    }
    return 0;
}