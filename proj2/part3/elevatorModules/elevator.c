#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/linkage.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include <linux/time.h>
#include <linux/kthread.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/delay.h> 	//sleep

void load_elevator(void);
void unload_elevator(void);
void printFloors(void);		//prints floors for testing
int should_load(void);		//checks if should load 1 yes 0 no
int should_unload(void); 	//checks if should unload 1 yes 0 no
int check_done(void);		//check if a request exists and elevator is empty
//int calc_weight(void);		//calculators the elvators weight; not used right now

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("elevator module");

#define ENTRY_NAME "elevator"
#define ENTRY_SIZE 80
#define PERMS 0644
#define PARENT NULL

#define OFFLINE 0
#define IDLE 1
#define LOADING 2
#define UP 3
#define DOWN 4

#define NUMFLOORS 10
#define MAXWEIGHT 15*2 	/* *2 for decimal weight*/
#define MAXPASSENGERS 10

static struct file_operations fops;
static char *message;
static char *state_message;
static int read_p;

static int active;
static int state; //0: OFFLINE, 1: IDLE, 2: LOADING, 3: UP, 4: DOWN
static int i;
static int currents;
static int should_stop;		//signal from stop elevator call
static int passengersServiced;	//total serviced
static int current_floor;	//current floor
static int next_floor;
static int number_passengers;
static int elevator_weight;
static int weight;
static int temp_int;		//temperary variable
static int temp_weight;
static int temp_capacity;
static int old_state;

static struct list_head *temp;
static struct list_head *dummy;
static struct list_head *position;
static struct person *thisPerson;
static struct person *newPerson;

struct list_head elevator;
struct list_head floors[NUMFLOORS];

struct mutex waitingPassenger; 	//for going through floors
struct mutex insideElevator;	//for going through the elevator

struct task_struct* elevator_thread; //threaad

struct person{
	struct list_head list;
	int valid;
	int type;
	int startFloor;
	int destinationFloor;
};

int scheduler(void *data){
	//struct thread_parameter *parm = data;
	while(!kthread_should_stop()){
		if(should_load() || should_unload()){ //Load and/or unload on current floor
			//Wonky LOADING implementation; need to edit functions
			//to handle state being LOADING
			old_state = state;
			state = LOADING;
			ssleep(1);
			state = old_state;

                        if(should_load())
                                load_elevator();
                        if(should_unload())
                                unload_elevator();
                }
		if(should_stop && check_done()){
			if(current_floor == 1){
				state = OFFLINE;
				next_floor = 1;
				printk(KERN_NOTICE "Elevator stopped and offline\n");
				kthread_stop(elevator_thread);
			}
			else{
				state = DOWN;
				ssleep(2);
				current_floor -= 1;
				next_floor = current_floor - 1;
			}
		}
		else if(state == IDLE && !check_done()){	//If at bottom and a request arrives, start
			state = UP;
		}
		else if(check_done() && current_floor == 1){ //If no more requests and at bottom, go idle
			state = IDLE;
			ssleep(1);
		}
		else if(check_done()){ //If no more requests, move to bottom
			state = DOWN;
			ssleep(2);
			--current_floor;
		}
		else if(state == UP){ //Go up until the last floor, then turn around
			if(current_floor != 10){
				ssleep(2);
				++current_floor;
			}
			else{
				state = DOWN;
			}
		}
		else if(state == DOWN){ //Go down until the first floor, then turn around
			if(current_floor != 1){
				ssleep(2);
				--current_floor;
			}
			else{
				state = UP;
			}
		}
		//printk(KERN_NOTICE "State: %d, Floor: %d\n",state,current_floor);
	}
	return 0;
}

extern long (*STUB_start_elevator)(void);
long start_elevator(void){
	//TODO::add return 1 if the elevator is already active
	printk(KERN_NOTICE "%s starting elevator function\n", __FUNCTION__);
	if(state == OFFLINE) {
        	printk("starting elevator\n");
   		state = IDLE;
		should_stop = 0;
		//Validate entries made after elevator stopped
		for(temp_int = 0; temp_int < NUMFLOORS; ++temp_int){
			list_for_each_safe(temp, dummy, &floors[temp_int]){
				thisPerson = list_entry(temp, struct person, list);
			       	thisPerson->valid = 1;	
			}
		}
		elevator_thread = kthread_run(scheduler, NULL, "elevator thread init");
        	return 0;
	}
	else
		return -ENOMEM;

	return 1;
}

extern long (*STUB_issue_request)(int, int, int);
long issue_request(int passenger_type, int start_floor, int destination_floor){
	//TODO::add if parameters are outobounds and make return 1
	if(passenger_type < 1 || passenger_type > 4 || start_floor < 1 || start_floor > 10 || destination_floor < 1 || destination_floor > 10)
	       return 1;	
	printk(KERN_NOTICE "%s: pass->%d, start_floor->%d, destination->%d\n", __FUNCTION__,passenger_type,start_floor,destination_floor);
	if(start_floor == destination_floor)
	        passengersServiced++;
	else{
		newPerson = kmalloc(sizeof(struct person), __GFP_RECLAIM | __GFP_IO | __GFP_FS);
		newPerson->type = passenger_type;
    		newPerson->startFloor = start_floor;
    		newPerson->destinationFloor = destination_floor;
		if(should_stop)
			newPerson->valid = 0; //Invalidate new requests while stopping
		else
			newPerson->valid = 1;
		mutex_lock_interruptible(&waitingPassenger);
    		list_add_tail(&newPerson->list, &floors[start_floor - 1]);
    		mutex_unlock(&waitingPassenger);
		//printFloors();
	}
	return 0;
}

extern long (*STUB_stop_elevator)(void);
long stop_elevator(void){
	printk(KERN_INFO "stopping elevator\n");
    if (should_stop){
        return 1;
    }
    else
	should_stop = 1;
    return 0;
}

int elevator_open(struct inode *sp_inode, struct file *sp_file) {
	printk(KERN_INFO "proc called open\n");
	read_p = 1;
	message = kmalloc(sizeof(char) * 1024, __GFP_RECLAIM | __GFP_IO | __GFP_FS);
	state_message = kmalloc(sizeof(char) * 10, __GFP_RECLAIM | __GFP_IO | __GFP_FS);
	if (message == NULL) {
		printk(KERN_WARNING "elevator_open");
		return -ENOMEM;
	}
	return 0;
}

ssize_t elevator_read(struct file *sp_file, char __user *buf, size_t size, loff_t *offset) {

	mutex_lock_interruptible(&insideElevator);

	//String movement state
	if(state == OFFLINE)
		strcpy(state_message, "OFFLINE");
	else if(state == IDLE)
		strcpy(state_message, "IDLE");
	else if(state == LOADING)
		strcpy(state_message, "LOADING");
	else if(state == UP)
		strcpy(state_message, "UP");
	else if(state == DOWN)
		strcpy(state_message, "DOWN");
	else
		strcpy(state_message, "ERROR");

	//for next floor
	if(current_floor == 1)
		next_floor = 2;
	else if(current_floor == 10)
		next_floor = 9;
	else if(state == UP)
		next_floor = current_floor + 1;
	else if(state == DOWN)
		next_floor = current_floor - 1;

	//Handle child weight
	//Currently always outputs as 'decimal'; not sure if that matters or not
	temp_int = elevator_weight % 2;
	if(temp_int == 0)
		temp_int = 0;
	else if(temp_int == 1)
		temp_int = 5;

	sprintf(message, "Movement State: %s\nCurrent Floor: %d\nNext Floor: %d\nLoad Passengers: %d\nLoad Weight: %d.%d\nPassengers Serviced: %d\n", state_message, current_floor, next_floor, number_passengers, elevator_weight/2, temp_int, passengersServiced);

	mutex_unlock(&insideElevator);
	read_p = !read_p;
	if (read_p)
		return 0;

	printk(KERN_INFO "proc called elevator_read\n");
	copy_to_user(buf, message, strlen(message));
	return strlen(message);
}

int elevator_release(struct inode *sp_inode, struct file *sp_file) {
	printk(KERN_NOTICE "elevator_release called.\n");
	kfree(message);
	kfree(state_message);
	return 0;
}

static int elevator_init(void) {
	printk(KERN_NOTICE "/proc/%s create\n",ENTRY_NAME);

	fops.open = elevator_open;
	fops.read = elevator_read;
	fops.release = elevator_release;

	STUB_start_elevator = &(start_elevator);
        STUB_issue_request = &(issue_request);
        STUB_stop_elevator = &(stop_elevator);

	mutex_init(&waitingPassenger);
	mutex_init(&insideElevator);

	active = 0;
        state = OFFLINE;
	currents = 0;
	passengersServiced = 0;
	current_floor = 1;
	next_floor = 1;
	number_passengers = 0;
	elevator_weight = 0;
	weight = 0;
	temp_int = 0;
	should_stop = 0;
	i = 0;

    	while (i < NUMFLOORS) {
        	INIT_LIST_HEAD(&floors[i]);
        	i++;
    	}
	i = 0;

	INIT_LIST_HEAD(&elevator);


	//start_elevator();
	if (!proc_create(ENTRY_NAME, PERMS, NULL, &fops)) {
		printk(KERN_WARNING "proc create\n");
		remove_proc_entry(ENTRY_NAME, NULL);
		return -ENOMEM;
	}

	return 0;
}
module_init(elevator_init);

static void elevator_exit(void) {
	remove_proc_entry(ENTRY_NAME, NULL);
	printk(KERN_NOTICE "Removing /proc/%s.\n", ENTRY_NAME);
	//stop_elevator();
	//kthread_stop(elevator_thread);

	mutex_destroy(&waitingPassenger);
	mutex_destroy(&insideElevator);

	STUB_start_elevator = NULL;
        STUB_issue_request = NULL;
        STUB_stop_elevator = NULL;

}
module_exit(elevator_exit);

void printFloors(){
	currents = 0;
	i = 0;
	printk("passenger floors\n");
	mutex_lock_interruptible(&waitingPassenger);
	while( i < NUMFLOORS){
		printk("floor->%d", i+1);
		list_for_each(position, &floors[i]) {
            		thisPerson = list_entry(position, struct person, list);
            		printk("floor position: %d\ntype: %d\nstart floor: %d\ndestination Floor: %d\n", currents, thisPerson->type, thisPerson->startFloor, thisPerson->destinationFloor);
            		currents = currents + 1;
		}
	i = i + 1;
	}
	mutex_unlock(&waitingPassenger);
}

void load_elevator(void){
	mutex_lock_interruptible(&waitingPassenger);

	mutex_lock_interruptible(&insideElevator);

        list_for_each_safe(temp, dummy, &floors[current_floor-1]){
                thisPerson = list_entry(temp, struct person, list);

                if(thisPerson->destinationFloor - thisPerson->startFloor >= 0)
                        temp_int = UP;
                else
                        temp_int = DOWN;

                temp_weight = 0;
                temp_capacity = 0;
                if(thisPerson->type == 1){
                        temp_weight = 2;
                        temp_capacity = 1;
                }
                else if(thisPerson->type == 2){
                        temp_weight = 1;
                        temp_capacity = 1;
                }
                else if(thisPerson->type == 3){
                        temp_weight = 4;
                        temp_capacity = 2;
                }
                else if(thisPerson->type == 4){
                        temp_weight = 8;
                        temp_capacity = 2;
                }
                //If moving in same direction as elevator and can fit in the elevator
                if(temp_int == state && elevator_weight + temp_weight <= MAXWEIGHT && number_passengers + temp_capacity <= MAXPASSENGERS && thisPerson->valid == 1){
			printk(KERN_NOTICE "Passenger picked up, floor %d\n", current_floor);
                        elevator_weight += temp_weight;
                        number_passengers += temp_capacity;
                        list_move_tail(temp, &elevator);
                }
        }

        mutex_unlock(&insideElevator);

	mutex_unlock(&waitingPassenger);
}


void unload_elevator(void){

        mutex_lock_interruptible(&waitingPassenger);

        mutex_lock_interruptible(&insideElevator);

        list_for_each_safe(temp, dummy, &elevator){
                thisPerson = list_entry(temp, struct person, list);
                if(thisPerson->destinationFloor == current_floor){
                        if(thisPerson->type == 1){
                                elevator_weight -= 2;
                                --number_passengers;
                        }
                        else if(thisPerson->type == 2){
                                --elevator_weight;
                                --number_passengers;
                        }
                        else if(thisPerson->type == 3){
                                elevator_weight -= 4;
                                number_passengers -= 2;
                        }
                        else if(thisPerson->type == 4){
                                elevator_weight -= 8;
                                number_passengers -= 2;
                        }
                        list_del(temp);
                        kfree(thisPerson);
			++passengersServiced;
			printk(KERN_NOTICE "Passenger unloaded, floor %d\n", current_floor);
                }
        }

        mutex_unlock(&insideElevator);

        mutex_unlock(&waitingPassenger);
}



int should_load(void){
	mutex_lock_interruptible(&waitingPassenger);

	temp_int = 0;
	list_for_each(temp,&floors[current_floor-1]){
                thisPerson = list_entry(temp, struct person, list);
        	if(((thisPerson->destinationFloor - thisPerson->startFloor >= 0 && state == UP) ||
		   (thisPerson->startFloor - thisPerson->destinationFloor >= 0 && state == DOWN)) &&
		    thisPerson->valid == 1){
			temp_int = 1;
		}

	}

	mutex_unlock(&waitingPassenger);
	return temp_int;
}

int should_unload(void){
	mutex_lock_interruptible(&insideElevator);

	temp_int = 0;
        list_for_each(temp,&elevator){
                thisPerson = list_entry(temp, struct person, list);
                if(thisPerson->destinationFloor == current_floor){
                        temp_int = 1;
                }

        }

	mutex_unlock(&insideElevator);
	return temp_int;
}
//Check if there are any requests
int check_done(void){
	mutex_lock_interruptible(&waitingPassenger);
	mutex_lock_interruptible(&insideElevator);

	if(!elevator_weight == 0 && !number_passengers == 0){
		mutex_unlock(&insideElevator);
		mutex_unlock(&waitingPassenger);
		return 0;
	}
	for(i = 0; i < NUMFLOORS; ++i){
		list_for_each(temp, &floors[i]){
			thisPerson = list_entry(temp, struct person, list);
			if(thisPerson->valid == 1){
				mutex_unlock(&insideElevator);
				mutex_unlock(&waitingPassenger);
				return 0;
			}
		}
	}
	mutex_unlock(&insideElevator);
	mutex_unlock(&waitingPassenger);
	return 1;
}
/*
int calc_weight(void){
//TODO::CHILD WEIGHT
	weight = 0;
	temp_int = 0;
	mutex_lock_interruptible(&insideElevator);
	list_for_each(temp, &elevator){
		thisPerson = list_entry(temp, struct person, list);
		if(thisPerson->type == 1)
			temp_int = 1;
		else if(thisPerson->type == 2)
			temp_int = 1;
                else if(thisPerson->type == 3)
			temp_int = 2;
                else if(thisPerson->type == 4)
			temp_int = 4;
		weight = weight + temp_int;
		temp_int = 0;
		//list_del(temp);
        }
	mutex_unlock(&insideElevator);
	return weight;
}
*/
