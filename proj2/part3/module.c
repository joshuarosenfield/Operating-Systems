#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <syscalls.h>

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Simple elevator scheduler module");

#define UP 0
#define DOWN 1
#define IDLE 2
#define LOADING 3
#define OFFLINE 4
#define floors 5

static char *message;
static int read;

int proc_open(struct inode *sp_inode, struct file *sp_file) {
  printk("Proc Open\n");
  read = 1;
  message = kmalloc(sizeof(char) * 2048, __GFP_RECLAIM | __GFP_IO | __GFP_FS);
  if (message == NULL) {
    printk("Error: OpenModule");
    return -ENOMEM;
  }
  return 0;
}

static void ExitModule(void) {
  remove_proc_entry(MODULE_NAME, NULL);
  elevator_syscalls_remove();
  printk(KERN_NOTICE "Removing /proc/%s.\n", MODULE_NAME);
}

module_init(InitializeModule);
module_exit(ExitModule);
