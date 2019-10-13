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
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Elevator module");

#define ENTRY_NAME "elevator"
#define ENTRY_SIZE 80
#define PERMS 0644
#define PARENT NULL
static struct file_operations fops;
static char *message;
static int read_p;

static int active;
static int state; //0: OFFLINE, 1: IDLE, 2: LOADING, 3: UP, 4: DOWN
static int floor;

struct list_head elevator;
struct list_head floors;

struct thread_parameter{
	int id;
	int cnt;
	struct task_struct *kthread;
};
struct person{
	int type;
	int dest;
};
struct  thread_parameter thread1;
int scheduler(void *data){
	struct thread_parameter *parm = data;
	static int i = 0;
	while(!kthread_should_stop()){

	}
	printk(KERN_INFO "test: %d", i);
	return 0;
}
void thread_init_parameter(struct thread_parameter *parm){
	static int id = 1;

	parm->id = id++;
	parm->cnt = 0;
	thread1.kthread = kthread_run(scheduler,parm,"thread %d",parm->id);
}

extern long (*STUB_start_elevator)(void);
long start_elevator(void){
	printk(KERN_NOTICE "%s\n", __FUNCTION__);
	/*
	if(active == 0){
		state = 1;
		floor = 1;
		active = 1;
		thread1.id = 1;
		thread1.cnt = 0;
		thread_init_parameter(&thread1);
		return 0;
	}
	if(active == 1){
		return 1;
	}
	return -ENOMEM;
	*/
	return 1;
}

extern long (*STUB_issue_request)(int, int, int);
long issue_request(int passenger_type, int start_floor, int destination_floor){
	printk(KERN_NOTICE "%s: pass->%d, start_floor->%d, destination->%d\n", __FUNCTION__,passenger_type,start_floor,destination_floor);
/*
	person * p = kmalloc(sizeof(person)*1,__GFP_RECLAIM);
	if(p == null)
		return -ENOMEM;
	p->type = passenger_type;
	p->dest = destination_floor;
	*/
	return 1;
}

extern long (*STUB_stop_elevator)(void);
long stop_elevator(void){
	//kthread_stop(thread1.kthread);
	return 1;
}

int elevator_open(struct inode *sp_inode, struct file *sp_file) {
	printk(KERN_INFO "proc called open\n");
	
	read_p = 1;
	message = kmalloc(sizeof(char) * ENTRY_SIZE, __GFP_RECLAIM | __GFP_IO | __GFP_FS);
	if (message == NULL) {
		printk(KERN_WARNING "my_xtime_open");
		return -ENOMEM;
	}
	return 0;
}

ssize_t elevator_read(struct file *sp_file, char __user *buf, size_t size, loff_t *offset) {
	int len = strlen(message);
	
	read_p = !read_p;
	if (read_p)
		return 0;
		
	printk(KERN_INFO "proc called read\n");
	copy_to_user(buf, message, len);
	return len;
}

int elevator_release(struct inode *sp_inode, struct file *sp_file) {
	kfree(message);
	return 0;
}

static int elevator_init(void) {
	printk(KERN_NOTICE "/proc/%s create\n",ENTRY_NAME);
	/*
	fops.open = elevator_open;
	fops.read = elevator_read;
	fops.release = elevator_release;
	active = 0;
	state = 0;
*/
	STUB_start_elevator = &(start_elevator);
        STUB_issue_request = &(issue_request);
        STUB_stop_elevator = &(stop_elevator);
/*
	//start_elevator();
	if (!proc_create(ENTRY_NAME, PERMS, NULL, &fops)) {
		printk(KERN_WARNING "proc create\n");
		remove_proc_entry(ENTRY_NAME, NULL);
		return -ENOMEM;
	}
*/	
	return 0;
}
module_init(elevator_init);

static void elevator_exit(void) {
	//stop_elevator();
	//remove_proc_entry(ENTRY_NAME, NULL);
	STUB_start_elevator = NULL;
        STUB_issue_request = NULL;
        STUB_stop_elevator = NULL;

}
module_exit(elevator_exit);
