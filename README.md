pwm_buzzer

Three files = buzzer-driver.c + button-driver.c + application.c
Three files are placed in the directory: /tiny6410/linux-3.0.8/drivers/char

1.	Open the Makefile file and add the following lines into the Makefile file
Add the following command:
obj-$(CONFIG_BUTTON_DRIVER) += button-driver.o
obj-$(CONFIG_BUZZER_DRIVER) += buzzer-driver.o

2.	Open the Kconfig file and add the following lines into the Kconfig file
Add the following command:
config BUTTON_DRIVER
	tristate "BUTTON DRIVER"
	depends on CPU_S5PV210
	
config BUZZER_DRIVER
	tristate "BUZZER DRIVER"
	depends on CPU_S5PV210

3.	Cd to /tiny6410/linux-3.0.8 in the terminal and use (make menuconfig) and (make)

4.	Dynamically load the drivers into the kernel:
Command:
insmod buzzer-driver.ko
insmod button-driver.ko

5.	Create a special file for our pseudo device and connect this file with the major number:
Command:
mknod /dev/pwm_buzzer c 250 1 
mknod /dev/lab5_1 c 249 1

6.	Compile the c program via the cross-platform compiler:
Assumption: Three files are placed in the directory: /tiny6410/linux-3.0.8/drivers/char
Command:
arm-linux-gcc -o application application.c

7.	Executing application (./application)

Check the arm board for result
