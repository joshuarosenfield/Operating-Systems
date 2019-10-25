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
int calc_weight(void);		//calculators the elvators weight

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

static struct file_operations fops;
static char *message;
static int read_p;

static int active;
static int state; //0: OFFLINE, 1: IDLE, 2: LOADING, 3: UP, 4: DOWN
static int i;
static int currents;
static int should_stop;		//signal from stop elevator call
static int passengersServiced;	//total serviced
static int current_floor;	//current floor
static int next_floor;		//next floor
static int number_passengers;
static int elevator_weight;
static int weight;
static int temp_int;		//temperary variable

struct list_head elevator;
struct list_head floors[NUMFLOORS];

struct mutex waitingPassenger; 	//for going through floors
struct mutex insideElevator;	//for going through the elevator

struct task_struct* elevator_thread; //threaad

struct person{
	struct list_head list;
	int type;
	int startFloor;
	int destinationFloor;
};

int scheduler(void *data){
	//struct thread_parameter *parm = data;
	while(!kthread_should_stop()){
		if(state == OFFLINE){
			break;
		}
		else if(state == IDLE){	//IDLE is bottom
			state = UP;	//headed up
			if(!should_stop && should_load()){
				state = LOADING;
			}
			else{
				next_floor = current_floor + 1;
				state = UP;
			}
		}
		else if(state == LOADING){
		//do loading stuff
			unload_elevator();
			ssleep(1);
			if(!should_stop && should_load()){
				load_elevator();
				ssleep(1);
			}
		}
		else if(state == UP){
		//do up stuff
		}
		else if(state == DOWN){
		//TODO::finish last case
		//do down stuff
		if(should_stop && (current_floor == 1) && (calc_weight() == 0)){	//last case
			state = OFFLINE;
		}
		}

		/*
		if((!should_stop) && (state == IDLE))
			printk("running");
		else
			printk("stopped");
		ssleep(2);
		*/
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
        	return 0;
	}
	else
		return -ENOMEM;

	return 1;
}

extern long (*STUB_issue_request)(int, int, int);
long issue_request(int passenger_type, int start_floor, int destination_floor){
	//TODO::add if parameters are outobounds and make return 1
	printk(KERN_NOTICE "%s: pass->%d, start_floor->%d, destination->%d\n", __FUNCTION__,passenger_type,start_floor,destination_floor);
	if(start_floor == destination_floor)
	        passengersServiced++;
	else{
		struct person *newPerson;
		newPerson = kmalloc(sizeof(struct person), __GFP_RECLAIM | __GFP_IO | __GFP_FS);
		newPerson->type = passenger_type;
    		newPerson->startFloor = start_floor;
    		newPerson->destinationFloor = destination_floor;
		mutex_lock_interruptible(&waitingPassenger);
    		list_add_tail(&newPerson->list, &floors[start_floor - 1]);
    		mutex_unlock(&waitingPassenger);
		printFloors();
	}
	return 1;
}

extern long (*STUB_stop_elevator)(void);
long stop_elevator(void){
	printk(KERN_INFO "stopping elevator\n");
    if (should_stop)
        return 1;
    else
	should_stop = 1;
    return 0;
}

int elevator_open(struct inode *sp_inode, struct file *sp_file) {
	printk(KERN_INFO "proc called open\n");
	read_p = 1;
	message = kmalloc(sizeof(char) * 1024, __GFP_RECLAIM | __GFP_IO | __GFP_FS);
	if (message == NULL) {
		printk(KERN_WARNING "elevator_open");
		return -ENOMEM;
	}
	return 0;
}

ssize_t elevator_read(struct file *sp_file, char __user *buf, size_t size, loff_t *offset) {
	//TODO::change movement state to string
	sprintf(message, "Movement State: %d\nCurrent Floor: %d\nNext Floor: %d\nLoad Passengers: %d\nLoad Weight: %d\nPassengers Serviced: %d", state, current_floor, next_floor, number_passengers, elevator_weight, passengersServiced);

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

	elevator_thread = kthread_run(scheduler, NULL, "elevator thread init");

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
	kthread_stop(elevator_thread);

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
	struct list_head * position;
	struct person * thisPerson;
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

        mutex_unlock(&insideElevator);

	mutex_unlock(&waitingPassenger);
}


void unload_elevator(void){

        mutex_lock_interruptible(&waitingPassenger);

        mutex_lock_interruptible(&insideElevator);

        mutex_unlock(&insideElevator);

        mutex_unlock(&waitingPassenger);
}



int should_load(void){
	mutex_lock_interruptible(&waitingPassenger);

	mutex_unlock(&waitingPassenger);
	return 0;
}

int should_unload(void){
	mutex_lock_interruptible(&waitingPassenger);

        mutex_unlock(&waitingPassenger);
	return 0;
}

int calc_weight(void){
//TODO::CHILD WEIGHT
	weight = 0;
	temp_int = 0;
	struct list_head *temp;
	struct person * thisPerson; //each person in the elevator
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
