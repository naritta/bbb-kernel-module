
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h> 
#include <asm/uaccess.h> 
#include <asm/io.h>

#define PROCNAME "driver/gpio"

#define MAX_LEN 4
#define GPIO0_BASE		(0x44E07000)
#define GPIO1_BASE		(0x4804C000)
#define GPIO2_BASE		(0x481AC000)
#define GPIO3_BASE		(0x481AE000)

#define OUTPUT_ENABLE 0x134
#define DATA_IN 0x138
#define DATA_OUT 0x13C
#define SET_DATA_OUT 0x194
#define CLEAR_DATA_OUT 0x190

#define USER_BUTTON_GPIO_NUMBER 73
#define LED0_GPIO_NUMBER 46
#define LED1_GPIO_NUMBER 47

#define ON (1)
#define OFF (0)

#define IN (0)
#define OUT (1)
#define CLEAR (2)
#define SET (3)
#define OUT_ENABLE (4)

static long unsigned int GPIO_BASE[]={
	GPIO0_BASE,GPIO1_BASE,GPIO2_BASE,GPIO3_BASE
};
static unsigned char gpio_buf[MAX_LEN];

static long unsigned int * get_base_address(int gpio_number,int operation)
{
	long unsigned int p;
	switch(operation){
	case IN:
		p = GPIO_BASE[gpio_number>>5]+DATA_IN;
		break;
	case OUT:
		p = GPIO_BASE[gpio_number>>5]+DATA_OUT;
		break;
	case SET:
		p = GPIO_BASE[gpio_number>>5]+SET_DATA_OUT;
		break;
	case CLEAR:
		p = GPIO_BASE[gpio_number>>5]+CLEAR_DATA_OUT;
		break;
	case OUT_ENABLE:
		p = GPIO_BASE[gpio_number>>5]+OUTPUT_ENABLE;
		break;
	default:
		p = GPIO_BASE[gpio_number>>5]+DATA_IN;
		break;
	}
	return ioremap(p,SZ_8K);
}

static int get_mask_bit(int gpio_number)
{
	return (1<<(gpio_number&0x1f));
}

static void set_led(int onoff)
{
	long unsigned int mask = get_mask_bit(LED0_GPIO_NUMBER);
	long unsigned int v;
	v = __raw_readl(get_base_address(LED0_GPIO_NUMBER,OUT_ENABLE));
	__raw_writel(v&(~mask), get_base_address(LED0_GPIO_NUMBER,OUT_ENABLE));
	if(onoff==ON){
		__raw_writel(mask, get_base_address(LED0_GPIO_NUMBER,SET));
	}else{
		__raw_writel(mask, get_base_address(LED0_GPIO_NUMBER,CLEAR));
	}
}

static int user_button(void)
{
	long unsigned int v=__raw_readl(get_base_address(USER_BUTTON_GPIO_NUMBER,IN));
	if(v&get_mask_bit(USER_BUTTON_GPIO_NUMBER)){
		return 1;
	}
	return 0;
}

static int proc_write( struct file *filp, const char *buf, unsigned long len, void *data )
{
	if( len >= MAX_LEN ){
		return -ENOSPC;
	}
	if( copy_from_user( gpio_buf, buf, len ) ) return -EFAULT;
	if(gpio_buf[0]=='0'){
		set_led(OFF);
	}else if(gpio_buf[0]=='1'){
		set_led(ON);
	}else{
		printk( KERN_INFO "value error\n" );
	}
	return len;
}
static int proc_read( char *buf, char **start, off_t offset, int count, int *eof, void *data )
{
	unsigned long len = 0;

	if( offset > 0 ){
		*eof = 1;
		return 0;
	}

	if(user_button()==0){
		len = sprintf( buf, "0");
	}else{
		len = sprintf( buf,"1");
	}
	*eof = 1;
	return len;
}

static int __init gpio_init( void )
{
	struct proc_dir_entry* entry;
	entry = create_proc_entry( PROCNAME, 0666, NULL );
	if( entry ){
		entry->write_proc  = proc_write;
		entry->read_proc  = proc_read;
	}
	else{
		printk( KERN_ERR "create_proc_entry failed\n" );
		return -EBUSY;
	}
	printk( KERN_INFO "gpio is loaded\n" );
	return 0;
}
static void __exit gpio_exit( void )
{
	remove_proc_entry( PROCNAME, NULL );
	printk( KERN_INFO "gpio is removed\n" );
}

module_init( gpio_init );
module_exit( gpio_exit );

MODULE_DESCRIPTION("gpio");
MODULE_LICENSE( "GPL2" );
