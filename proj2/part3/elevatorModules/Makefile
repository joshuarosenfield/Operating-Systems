obj-m := elevator.o

KERNELDIR ?= /lib/modules/`uname -r`/build/
PWD := `pwd`

default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

clean:
	rm -f *.ko *.o Module* *mod*
