#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include <linux/time.h>
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Simple module featuring proc read");

#define ENTRY_NAME "my_xtime"
#define ENTRY_SIZE 80
#define PERMS 0644
#define PARENT NULL
static struct file_operations fops;
static struct timespec xtime;
static char *message;
static int read_p;
static int old_time_sec, new_time_sec;
static long old_time_ns, new_time_ns;
static long dif;
int my_xtime_open(struct inode *sp_inode, struct file *sp_file) {
	printk(KERN_INFO "proc called open\n");
	
	read_p = 1;
	message = kmalloc(sizeof(char) * ENTRY_SIZE, __GFP_RECLAIM | __GFP_IO | __GFP_FS);
	if (message == NULL) {
		printk(KERN_WARNING "my_xtime_open");
		return -ENOMEM;
	}
	xtime = current_kernel_time();
	new_time_sec = xtime.tv_sec;
	new_time_ns = xtime.tv_nsec;
	if(old_time_sec == -1)
		sprintf(message,"current time:%d.%ld\n", new_time_sec, new_time_ns);
	else{
		dif = new_time_ns - old_time_ns;
		if(dif < 0){
			++old_time_sec;
			dif = 1000000000 + dif;
		}
		sprintf(message,"current time:%d.%ld\n time: %d.%ld\n", 
				new_time_sec, new_time_ns, new_time_sec - old_time_sec, dif);
	}
	old_time_sec = new_time_sec;
	old_time_ns = new_time_ns;
	return 0;
}

ssize_t my_xtime_read(struct file *sp_file, char __user *buf, size_t size, loff_t *offset) {
	int len = strlen(message);
	
	read_p = !read_p;
	if (read_p)
		return 0;
		
	printk(KERN_INFO "proc called read\n");
	copy_to_user(buf, message, len);
	return len;
}

int my_xtime_release(struct inode *sp_inode, struct file *sp_file) {
	printk(KERN_NOTICE "proc called release\n");
	kfree(message);
	return 0;
}

static int time_init(void) {
	printk(KERN_NOTICE "/proc/%s create\n",ENTRY_NAME);
	fops.open = my_xtime_open;
	fops.read = my_xtime_read;
	fops.release = my_xtime_release;
	old_time_sec = -1;
	new_time_sec = -1;
	old_time_ns = -1;
	new_time_ns = -1;

	if (!proc_create(ENTRY_NAME, PERMS, NULL, &fops)) {
		printk(KERN_WARNING "proc create\n");
		remove_proc_entry(ENTRY_NAME, NULL);
		return -ENOMEM;
	}
	
	return 0;
}
module_init(time_init);

static void time_exit(void) {
	remove_proc_entry(ENTRY_NAME, NULL);
	printk(KERN_NOTICE "Removing /proc/%s\n", ENTRY_NAME);
}
module_exit(time_exit);
