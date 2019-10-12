#include <linux/linkage.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/syscalls.h>

/* System call stub */
long (*STUB_issue_request)(int,int,int) = NULL;
EXPORT_SYMBOL(STUB_issue_request);


/* System call wrapper */
SYSCALL_DEFINE3(issue_request, int, passenger_type, int, start_floor, int, destination_floor){
	printk(KERN_NOTICE "inside SYSCALL_DEFINE3 block. %s: inputs passenger_type-> %d, start_floor-> %d, destination_floor-> %d\n",
	 __FUNCTION__, passenger_type, start_floor, destination_floor);
	if (STUB_issue_request != NULL)
		return STUB_issue_request(passenger_type, start_floor, destination_floor);
	else
		return -ENOSYS;
}
