/*comp3438_buzzer
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/fs.h>
#include <linux/types.h>
#include <linux/moduleparam.h>
#include <linux/ioctl.h>
#include <linux/cdev.h>

#include <asm/uaccess.h>

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/fb.h>
#include <linux/backlight.h>
#include <linux/err.h>
#include <linux/pwm.h>
#include <linux/slab.h>
#include <linux/miscdevice.h>
#include <linux/delay.h>

#include <mach/gpio.h>
#include <mach/regs-gpio.h>
#include <plat/gpio-cfg.h>
#include <plat/regs-timer.h>
#include <linux/clk.h>


#define DEVICE_NAME				"buzzer"
#define N_D         1                   /*Number of Devices*/
#define S_N         1                   /*The start minor number*/

#define GPD0_CON_ADDR	0xE02000A0 
#define GPD0_CON_DATA	0Xe02000A4

#define TCFG0	0xE2500000 //Timer Configuration Register (TCFG0)
#define TCFG1	0xE2500004 //Timer Configuration Register (TCFG1)
#define TCNTB0  0xE250000C //Timer0 Counter Register (TCNTB0)
#define TCMPB0  0xE2500010 //Timer0 Compare Register (TCMPB0)
#define TCON	0xE2500008 //Timer Control Register (TCON)

static unsigned long tcfg0; //Timer Configuration Register (TCFG0)
static unsigned long tcfg1; //Timer Configuration Register (TCFG1)
static unsigned long tcntb0; //Timer0 Counter Register (TCNTB0)
static unsigned long tcmpb0; //Timer0 Compare Register (TCMPB0)
static unsigned long tcon; //Timer Control Register (TCON)

static void *ZL_GPD0_CON_ADDR;
static void *ZL_GPD0_CON_DATA;

static void *ZL_TCFG0; //Timer Configuration Register (TCFG0)
static void *ZL_TCFG1; //Timer Configuration Register (TCFG1)
static void *ZL_TCNTB0; //Timer0 Counter Register (TCNTB0)
static void *ZL_TCMPB0; //Timer0 Compare Register (TCMPB0)
static void *ZL_TCON; //Timer Control Register (TCON)

//static struct semaphore lock;

static int          major;
static dev_t        devno;
static struct cdev  dev_buzzer;

unsigned long pclk;
struct clk *clk_p;


static void buzzer_start(unsigned long cycle_period) //start the buzzer  
{  
    unsigned long sound_frequency;
    unsigned long data;
    clk_p = clk_get(NULL, "pclk");
    pclk = clk_get_rate(clk_p);
	
    //Change GPD0CON to TOUT_0
    data = readl(ZL_GPD0_CON_ADDR);
    data = data&~0xF;
    data = data|0x02; //0010 = TOUT_0
    writel(data, ZL_GPD0_CON_ADDR);
	
    data = readl(ZL_GPD0_CON_DATA); //  
    data = data|0x01; // set data as high   
    writel(data, ZL_GPD0_CON_DATA);
	
    //Prescaler0 should be set up as 65
    tcfg0 = readl(ZL_TCFG0);//
    tcfg0 &= ~0xFF;
    tcfg0 |= 0x41;//0x41 = 65 HEX TO DEC
    writel(tcfg0, ZL_TCFG0);
	
    //Divider MUX0 should be set up as “0100”
    tcfg1 = readl(ZL_TCFG1); //
    tcfg1 &= ~0xF;
    tcfg1 |= 0x4;//0100 = 0x4 BIN TO HEX
    writel(tcfg1, ZL_TCFG1);
    
    sound_frequency=pclk/(0x41+1)/0x10;//Timer input clock frequency = PCLK/({prescaler value+1})/{divider value}
    sound_frequency=sound_frequency/cycle_period; //(1/F)/(1/PRE/DIV Freq)
	
    //Timer 0 Count Buffer should be set up as 1/F /(1/ PRE/DIV Freq)
    tcntb0 = readl(ZL_TCNTB0);
    tcntb0 &= ~0xFFFFFFFF;
    tcntb0 |= sound_frequency;  	// (1/F)/(1/PRE/DIV Freq)
    writel(tcntb0, ZL_TCNTB0);
	
    //Timer 0 Compare Register should be set up as TCNTB0/2
    tcmpb0 = readl(ZL_TCMPB0);
    tcmpb0 &= ~0xFFFFFFFF;
    tcmpb0 = tcntb0/2;//TCNTB0/2
    writel(tcmpb0, ZL_TCMPB0);
	
    //Last Register Setting
    tcon = readl(ZL_TCON);
    tcon &= ~0x1F;//clear last 5 bits for editing
    tcon &= ~0x10;//dead zone generator [disable] = use 0001 0000 to change it back to 0 (no.4 bit)
    tcon |= 0x8;//auto-reload [ON] 0x8 = 1000 to change it to 1 (no.3 bit)
    tcon &= ~0x4;//inverter [OFF] = use 0100 to change it back to 0 (no.2 bit) 
    tcon &= ~0x2;//manual update [OFF] = use 0010 to change it back to 0 (no.1 bit)
    tcon |= 0x1;//timer [ON] = 0001 (no.0 bit)
    writel(tcon, ZL_TCON);
}
  
  
void buzzer_stop(void) //stop the buzzer   
{  
    unsigned int data;  
    //configure the GPIO work as ¡®output¡¯ mode, set the last four bits of the register as 0001
    data = readl(ZL_GPD0_CON_ADDR);
    data = data&(~0x01<<1);   
   	data = data&(~0x01<<2);   
    data = data&(~0x01<<3); 	
    data = data|0x01; //  
    writel(data, ZL_GPD0_CON_ADDR); 
    //end line
    data = readl(ZL_GPD0_CON_DATA);  
    data&=~0x1; // set data as low
    writel(data, ZL_GPD0_CON_DATA);
}  

static int zili_demo_char_buzzer_open(struct inode *inode, struct file *file) {
	//map the IO physical memory address to virtual address
	ZL_GPD0_CON_ADDR = ioremap(GPD0_CON_ADDR, 0x00000004);
	ZL_GPD0_CON_DATA = ioremap(GPD0_CON_DATA, 0x00000004);
	
	//New register IO remap
	ZL_TCFG0 = ioremap(TCFG0, 0x00000004);
	ZL_TCFG1 = ioremap(TCFG1, 0x00000004);
	ZL_TCNTB0 = ioremap(TCNTB0, 0x00000004);
	ZL_TCMPB0 = ioremap(TCMPB0, 0x00000004);
	ZL_TCON = ioremap(TCON, 0x00000004);
	
	buzzer_stop();
	printk("Device " DEVICE_NAME " open.\n");
	return 0;
}

static int zili_demo_char_buzzer_close(struct inode *inode, struct file *file) {
	buzzer_stop();
	return 0;
}

static ssize_t zili_demo_char_buzzer_write(struct file *fp, const char *buf, size_t count, loff_t *position){

	unsigned long buzzer_status;
	int  ret;
	ret = copy_from_user(&buzzer_status, buf, sizeof(buzzer_status) );
	if(ret)
	{
		printk("Fail to copy data from the user space to the kernel space!\n");
	}
	if( buzzer_status > 0 )
	{
		buzzer_start(buzzer_status);
	}
	else
	{
		buzzer_stop();
	}
	return sizeof(buzzer_status);
}

static struct file_operations zili_mini210_pwm_ops = {

	.owner			= THIS_MODULE,
	.open			= zili_demo_char_buzzer_open,
	.release		= zili_demo_char_buzzer_close, 
	.write			= zili_demo_char_buzzer_write,
};

static int __init zili_demo_char_buzzer_dev_init(void) {
	int ret;
	/*Register a major number*/
	ret = alloc_chrdev_region(&devno, S_N, N_D, DEVICE_NAME);
	if(ret < 0){
		printk("Device " DEVICE_NAME " cannot get major number.\n");
		return ret;
	}	
	
	major = MAJOR(devno);
	printk("Device " DEVICE_NAME " initialized (Major Number -- %d).\n", major);	

	/*Register a char device*/
	cdev_init(&dev_buzzer, &zili_mini210_pwm_ops);
	dev_buzzer.owner = THIS_MODULE;
	dev_buzzer.ops   = &zili_mini210_pwm_ops;
	ret = cdev_add(&dev_buzzer, devno, N_D);
	if(ret)
	{
		printk("Device " DEVICE_NAME " register fail.\n");
		return ret;
	}
	return 0;	
	
}

static void __exit zili_demo_char_buzzer_dev_exit(void) {
	buzzer_stop();
	cdev_del(&dev_buzzer);
	unregister_chrdev_region(devno, N_D);
	printk("Device " DEVICE_NAME " unloaded.\n");
}

module_init(zili_demo_char_buzzer_dev_init);
module_exit(zili_demo_char_buzzer_dev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("FriendlyARM Inc.");
MODULE_DESCRIPTION("S5PV210 Buzzer Driver");
