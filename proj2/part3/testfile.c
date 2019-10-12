#include<stdio.h>
#include<syscall.h>
#define __NR_START_ELEVATOR 436
#define __NR_ISSUE_REQUEST 437
#define __NR_STOP_ELEVATOR 438
int main(){
	printf("test start: %d\n", syscall(__NR_START_ELEVATOR);
	printf("test add: %d\n", syscall(__NR_ISSUE_REQUEST,1,1,2);
	printf("test stop: %d\n", syscall(__NR_STOP_ELEVATOR);
	return 0;
}
