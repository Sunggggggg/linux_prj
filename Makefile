obj-m += driver_buzzer.o
obj-m += driver_gpio.o
obj-m += driver_fnd.o
KDIR = ~/workspace/kernel
CCC = gcc

RESULT = dbg
SRC = $(RESULT).c

all :
	make -C $(KDIR) M=$(PWD) modules 
	$(CCC) -o $(RESULT) $(SRC) -lrt

clean:
	make -C $(KDIR) M=$(PWD) clean 
	rm -f $(RESULT)