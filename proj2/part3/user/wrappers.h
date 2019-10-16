#ifndef __WRAPPERS_H
#define __WRAPPERS_H

#define _GNU_SOURCE
#include <unistd.h>
#include <sys/syscall.h>

#define __NR_START_ELEVATOR 436
#define __NR_ISSUE_REQUEST 437
#define __NR_STOP_ELEVATOR 438

int start_elevator() {
	return syscall(__NR_START_ELEVATOR);
}

int issue_request(int passenger_type, int start_floor, int destination_floor){
        return syscall(__NR_ISSUE_REQUEST, passenger_type, start_floor, destination_floor);
}

int stop_elevator() {
	return syscall(__NR_STOP_ELEVATOR);
}

#endif

