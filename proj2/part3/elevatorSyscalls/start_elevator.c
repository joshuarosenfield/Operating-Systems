#include <linux/linkage.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/syscalls.h>

/* System call stub */
long (*STUB_start_elevator)(void) = NULL;
EXPORT_SYMBOL(STUB_start_elevator);


/* System call wrapper */
SYSCALL_DEFINE0(start_elevator){
        printk(KERN_NOTICE "inside SYSCALL_DEFINE0 block. %s\n", 
         __FUNCTION__);
        if (STUB_start_elevator != NULL)
                return STUB_start_elevator();
        else
                return -ENOSYS;
}

