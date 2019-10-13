#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/syscall.h>
#define __NR_START_ELEVATOR 436
#define __NR_ISSUE_REQUEST 437
#define __NR_STOP_ELEVATOR 438

int start_elevator(){
	return syscall(__NR_START_ELEVATOR);
}

int issue_request(int passenger_type, int start_floor, int destination_floor){
	return syscall(__NR_ISSUE_REQUEST, passenger_type, start_floor, destination_floor);
}

int stop_elevator(void){
        return syscall(__NR_STOP_ELEVATOR);
}

int main(){

	long ret;

	ret  = start_elevator();

	if (ret < 0)
		perror("system call error");
	else
		printf("Function successful. returned %ld\n", ret);

	printf("Returned value: %ld\n", ret);


        ret  = issue_request(1,1,1);

        if (ret < 0)
                perror("system call error");
        else
                printf("Function successful. returned %ld\n", ret);

        printf("Returned value: %ld\n", ret);


	ret  = stop_elevator();

        if (ret < 0)
                perror("system call error");
        else
                printf("Function successful. returned %ld\n", ret);

        printf("Returned value: %ld\n", ret);

	return 0;
}
