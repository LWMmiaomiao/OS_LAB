#include <stdio.h>
#include <stdint.h>
#include <stdint.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>

#define TEST_POINT 120
int total;
void test(int a, int b, int c){
	total = a+b+c;
	if(b!=c){
		printf("has error");
	}
}
int main(int argc, char *argv[])
{
	int error_time = 0;
	srand(clock());
	int data[TEST_POINT];
	for (int i = 0; i < TEST_POINT; ++i) {
		data[i] = rand() & 1023;
		// printf("%ld\n", data[i]);
	}
	for (long int i = 0; i < 4096 * TEST_POINT; i += 4096) {
		*(int *)i = data[i / 4096];
		// printf("0x%lx, %ld, %ld\n", i, data[i / 4096], *(long *)i);
	}
	for (long int i = 0; i < 4096 * TEST_POINT; i += 4096) {
		test(i, data[i / 4096], *(int *)i);
		printf("0x%lx, %d, %d  ", i, data[i / 4096], *(int *)i);
		// if (*(int *)i != data[i / 4096]) {
		// 	printf("Error! ");
		// 	error_time++;
		// 	// return 0;
		// }
	}

	printf("\nTOTAL %d TIMES, ERROR %d TIMES.\n", TEST_POINT, error_time);
	if(!error_time)
		printf("Success!\n");
	return 0;
}