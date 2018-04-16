#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/poll.h>
#include <linux/irq.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <linux/interrupt.h>
#include <asm/uaccess.h>
#include <mach/hardware.h>
#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/miscdevice.h>

#include <mach/map.h>
#include <mach/gpio.h>
#include <mach/regs-clock.h>
#include <mach/regs-gpio.h>

#define DEVICE_NAME "comp309_char_buttons"
//Assignment requires KEY1 to KEY4 only
#define KEY1        S5PV210_GPH2(0)
#define KEY2        S5PV210_GPH2(1)
#define KEY3        S5PV210_GPH2(2)
#define KEY4        S5PV210_GPH2(3)
#define N_D         1                      /*Number of Devices*/
#define S_N         1                      /*The start minor number*/


static volatile char key_values[] = {'1'};
static DECLARE_WAIT_QUEUE_HEAD(button_waitq);

static volatile int ev_press = 0;

static int          major;
static dev_t        devno;
static struct cdev  dev_lab6;

static irqreturn_t zili_demo_char_button_interrupt(int irq, void *dev_id)
{
	unsigned current_value;
	int key_irq;
	
	key_irq = gpio_to_irq(KEY1);
	if(key_irq == irq)
	{
		/*The current value of the KEY1*/
		current_value = gpio_get_value(KEY1); 
		printk("[KERNEL] KEY1: %d\n", current_value);	
		key_values[0] = '1';
		
		goto finish;
	}
	
	key_irq = gpio_to_irq(KEY2);
	if(key_irq == irq)
	{
		/*The current value of the KEY1*/
		current_value = gpio_get_value(KEY2); 

		printk("[KERNEL] KEY2: %d\n", current_value);	
		key_values[0] = '2';		
		goto finish;
	}

	key_irq = gpio_to_irq(KEY3);
	if(key_irq == irq)
	{
		/*The current value of the KEY1*/
		current_value = gpio_get_value(KEY3); 
		printk("[KERNEL] KEY3: %d\n", current_value);	
		key_values[0] = '3';
		
		goto finish;
	}

	key_irq = gpio_to_irq(KEY4);
	if(key_irq == irq)
	{
		/*The current value of the KEY1*/
		current_value = gpio_get_value(KEY4); 
		printk("[KERNEL] KEY4: %d\n", current_value);	
		key_values[0] = '4';
		
		goto finish;
	}
finish:
	ev_press = 1;
	wake_up(&button_waitq);
	
	return IRQ_HANDLED;
}

static int zili_demo_char_button_open(struct inode *inode, struct file *file)
{
	int irq;
	int ret = 0;
	
	/*Register IRQ for 8 keys*/
	irq = gpio_to_irq(KEY1);
	ret = request_irq(irq, zili_demo_char_button_interrupt, IRQ_TYPE_EDGE_BOTH, "KEY1", NULL);
	if(ret != 0)
	{
		printk("Fail to Register IRQ for KEY1\n");
	}

	irq = gpio_to_irq(KEY2);
	ret = request_irq(irq, zili_demo_char_button_interrupt, IRQ_TYPE_EDGE_BOTH, "KEY2", NULL);
	if(ret != 0)
	{
		printk("Fail to Register IRQ for KEY2\n");
	}

	irq = gpio_to_irq(KEY3);
	ret = request_irq(irq, zili_demo_char_button_interrupt, IRQ_TYPE_EDGE_BOTH, "KEY3", NULL);
	if(ret != 0)
	{
		printk("Fail to Register IRQ for KEY3\n");
	}
	
	irq = gpio_to_irq(KEY4);
	ret = request_irq(irq, zili_demo_char_button_interrupt, IRQ_TYPE_EDGE_BOTH, "KEY4", NULL);
	if(ret != 0)
	{
		printk("Fail to Register IRQ for KEY4\n");
	}
		
	ev_press = 1;
	printk("Device " DEVICE_NAME " open\n");
	return 0;
}

static int zili_demo_char_button_release(struct inode *inode, struct file *file)
{
	int irq;

	/*Release the IRQ resource*/
	irq = gpio_to_irq(KEY1);
	free_irq(irq, NULL);

	irq = gpio_to_irq(KEY2);
	free_irq(irq, NULL);

	irq = gpio_to_irq(KEY3);
	free_irq(irq, NULL);

	irq = gpio_to_irq(KEY4);
	free_irq(irq, NULL);

	printk("Device " DEVICE_NAME " release.\n");
	return 0;
}

static int zili_demo_char_button_read(struct file *filp, char __user *buff,
		size_t count, loff_t *offp)
{
	unsigned long ret;

	if (!ev_press) {
		if (filp->f_flags & O_NONBLOCK)
			return -EAGAIN;
		else
			wait_event(button_waitq, ev_press);
	}

	ev_press = 0;

	ret = copy_to_user((void *)buff, (const void *)(&key_values), min(sizeof(key_values), count));

	return ret ? -EFAULT : min(sizeof(key_values), count);
}

static struct file_operations zili_demo_fops = {
	owner   : THIS_MODULE,
	open    : zili_demo_char_button_open,
	read    : zili_demo_char_button_read,
	release : zili_demo_char_button_release,
};

static int __init zili_demo_char_button_init(void)
{
	int ret;

	/*Register a major number*/
	ret = alloc_chrdev_region(&devno, S_N, N_D, DEVICE_NAME);
	if(ret < 0)
	{
		printk("Device " DEVICE_NAME " cannot get major number.\n");
		return ret;
	}
	
	major = MAJOR(devno);
	printk("Device " DEVICE_NAME " initialized (Major Number -- %d).\n", major);

	/*Register a char device*/
	cdev_init(&dev_lab6, &zili_demo_fops);
	dev_lab6.owner = THIS_MODULE;
	dev_lab6.ops   = &zili_demo_fops;
	ret = cdev_add(&dev_lab6, devno, N_D);
	if(ret)
	{
		printk("Device " DEVICE_NAME " register fail.\n");
		return ret;
	}
	return 0;
}

static void __exit zili_demo_char_button_exit(void)
{
	cdev_del(&dev_lab6);
	unregister_chrdev_region(devno, N_D);
	printk("Device " DEVICE_NAME " unloaded.\n");
}

module_init(zili_demo_char_button_init);
module_exit(zili_demo_char_button_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Dr. Zili Shao <cszlshao@comp.polyu.edu.hk>");
MODULE_DESCRIPTION("Char Driver Development: BUTTON Detection. Course: HK POLYU COMP 309");
