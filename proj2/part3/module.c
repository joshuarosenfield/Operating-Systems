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

#define MODULE_NAME "elevator"

static void ExitModule(void) {
  remove_proc_entry(MODULE_NAME, NULL);
  elevator_syscalls_remove();
  printk(KERN_NOTICE "Removing /proc/%s.\n", MODULE_NAME);
}

module_init(InitializeModule);
module_exit(ExitModule);
