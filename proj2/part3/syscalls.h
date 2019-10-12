#ifndef __ELEVATOR_SYSCALLS_H
#define __ELEVATOR_SYSCALLS_H

void elevator_syscalls_create(void);
void elevator_syscalls_remove(void);
int runElevator(void *data);
int moveElevator(int floor);
void loadPassenger(int floor);
void unloadPassengers(void);

#endif
