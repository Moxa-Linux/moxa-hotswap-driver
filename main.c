/*
 * This is Moxa hotswap driver
 * Date		Author		Comment
 * 03-01-2011	Wade Liang	Creat it
 * 10-03-2013	Jared Wu	Porting to V2616A
 * 12-11-2018	Elvis Yao	Porting to V2406C
 * 01-29-2021	Elvis Yao	Porting to kernel version > 4.15
 */

#include <linux/delay.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/miscdevice.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/types.h>	/*define u8 ...*/
#include <linux/pci.h>
#include <linux/workqueue.h>
#include <linux/timer.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/spinlock.h>
#include <linux/version.h>
#include <linux/dmi.h>
#include <linux/printk.h>
#include "mxhtsp_ioctl.h"
#include "moxa_hotswap.h"
#include "moxa_superio.h"

#if (LINUX_VERSION_CODE > KERNEL_VERSION(4, 15, 0))
#include <linux/uaccess.h>
#endif

MODULE_AUTHOR("ElvisCW.Yao@moxa.com");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("MOXA V2406C hotswap driver");

static unsigned long paddr; /* physical memory address */
static void *vaddr; /* virtual memory address */
static struct timer_list timer;
static disk_info d1, d2;
struct dmi_read_state {
	char *buf;
	loff_t pos;
	size_t count;
	int entry_length;
};

static size_t dmi_entry_length(const struct dmi_header *dh)
{
	const char *p = (const char *)dh;
	p += dh->length;
	while (p[0] || p[1])
		p++;
	return 2 + p - (const char *)dh;
}

static void get_dmi_sysconf(const struct dmi_header *dh, void *private)
{
	struct dmi_read_state *state = (struct dmi_read_state*)private;
	if (dh->type == DMI_ENTRY_SYSCONF) {
		size_t entry_length = dmi_entry_length(dh);
		memory_read_from_buffer(state->buf, state->count, &state->pos, dh, entry_length);
		state->entry_length = entry_length;
	}
}

const char *get_dmi_sysconf_name(void)
{
	char buf[BUFF_SZ];
	static char m_name_buf[NAME_LEN];
	int i = 0, j = 0;
	struct dmi_read_state state = {
		.buf = buf,
		.pos = 0,
		.count = BUFF_SZ,
		0,
	};

	memset(&buf, 0, BUFF_SZ);
	memset(&m_name_buf, 0, NAME_LEN);

	dmi_walk(get_dmi_sysconf, (void*)&state);
	for (i = NAME_START; i < NAME_END; i++) {
		if (buf[i] != ' ') {
			m_name_buf[j] = buf[i];
			j++;
		}
	}

	return m_name_buf;
}

static int hotswap_led_control(int led_num, int on)
{
	int led_bit;
	unsigned char pled_status;

	if (((led_num != 1) && (led_num != 2)) || ((on != 1) && (on != 0)))
		return -1;

	if (led_num == 1) {
		led_bit = BIT_DISK1_LED;
	} else {
		led_bit = BIT_DISK2_LED;
	}
	if (on == 1) {  /* change led status to 1, because high activate */
		pled_status = moxainb(LED_BTN_ADDR);
		pled_status &= ~led_bit;
		moxaoutb(pled_status, LED_BTN_ADDR);
	} else if (on == 0) {
		pled_status = moxainb(LED_BTN_ADDR);
		pled_status |= led_bit;
		moxaoutb(pled_status, LED_BTN_ADDR);
	}

	return 0;
}

static int hotswap_open(struct inode *inode, struct file *file)
{
	pr_debug("\n");
	return 0;
}

static int hotswap_release(struct inode *inode, struct file *file)
{
	pr_debug("\n");
	return 0;
}

static long hotswap_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	void __user *addr = (void __user *)arg;
	int err = 0;
	mxhtsp_param p;

	if (_IOC_TYPE(cmd) != IOCTL_MAGIC)
		return -ENOTTY;
	if (_IOC_NR(cmd) > IOCTL_MAXNR)
		return -ENOTTY;


/* The 5.0 kernel dropped the type argument to access_ok() */
/* [PATCH] Remove 'type' argument from access_ok() function */
/* https://lkml.org/lkml/2019/1/4/418 */
#if (LINUX_VERSION_CODE > KERNEL_VERSION(5, 0, 0))
	if (_IOC_DIR(cmd) & _IOC_READ) {
		err = !access_ok((void __user *)arg, _IOC_SIZE(cmd));
	} else if (_IOC_DIR(cmd) & _IOC_WRITE) {
		err = !access_ok((void __user *)arg, _IOC_SIZE(cmd));
	}
#else
	if (_IOC_DIR(cmd) & _IOC_READ) {
		err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
	} else if (_IOC_DIR(cmd) & _IOC_WRITE) {
		err = !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
	}
#endif

	if (err)
		return -EFAULT;

	switch (cmd) {
		case IOCTL_GET_DISK_STATUS:
			if (copy_from_user(&p, addr, sizeof(mxhtsp_param)))
				goto err;
			if (p.disk_num == 1)
				p.val = d1.busy;
			else if (p.disk_num == 2)
				p.val = d2.busy;
			else
				return -EINVAL;
			if (copy_to_user(addr, &p, sizeof(mxhtsp_param)))
				goto err;
			break;

		case IOCTL_SET_DISK_LED:
			if (copy_from_user(&p, addr, sizeof(mxhtsp_param)))
				goto err;
			return hotswap_led_control(p.disk_num, p.val);
			break;

		case IOCTL_GET_DISK_BTN:
			if (copy_from_user(&p, addr, sizeof(mxhtsp_param)))
				goto err;
			if (p.disk_num == 1) {
				p.val = moxainb(LED_BTN_ADDR);
				/* if BIT_DISK1_BTN is 0 (button pressed), set p.val to 1 to indicated the button is pressed */
				p.val = ((~p.val) & BIT_DISK1_BTN) ? 1 : 0 ;
			} else if (p.disk_num == 2) {
				p.val = moxainb(LED_BTN_ADDR);
				p.val = ((~p.val) & BIT_DISK2_BTN) ? 1 : 0 ;
			} else
				return -EINVAL;
			if (copy_to_user(addr, &p, sizeof(mxhtsp_param)))
				goto err;
			break;

		case IOCTL_CHECK_DISK_PLUGGED:
			if (copy_from_user(&p, addr, sizeof(mxhtsp_param))) {
				goto err;
			}
			if (p.disk_num == 1) {
				p.val = (ioread32(vaddr + SATA_P1SSTS) == 0x4) ? 0 : 1;/* 0x4 for un-plug */
				if (p.val == 1)
					p.val = (ioread32(vaddr + SATA_P1SSTS) == 0x00) ? 0 : 1;/* TODO: BIOS disable hotswap */
			} else if (p.disk_num == 2) {
				p.val = (ioread32(vaddr + SATA_P2SSTS) == 0x4) ? 0 : 1;/* 0x4 for un-plug */
				if (p.val == 1)
					p.val = (ioread32(vaddr + SATA_P2SSTS) == 0x00) ? 0 : 1;/* TODO: BIOS disable hotswap */
			} else
				return -EINVAL;
			if (copy_to_user(addr, &p, sizeof(mxhtsp_param)))
				goto err;
			break;

		default:
			pr_err("ioctl error\n");
			return -EINVAL;
	}
	return 0;

err:
	pr_err("copy data error\n");
	return -1;
}

ssize_t hotswap_write(struct file *file, const char __user *buf, size_t count,
		loff_t *pos)
{
	char param[4];
	int led_num;
	int on;

	if (count != 4)
		return -1;

	if (copy_from_user(param, buf, count))
		return -1;

	pr_debug("count=%zu, param[3]=%x\n", count, param[3]);
	param[3] = '\0'; /* change \n to \0 */
	sscanf(param, "%d %d", &led_num, &on);

	if (hotswap_led_control(led_num, on))
		return -1;

	return count;
}

static struct file_operations hotswap_fops = {
	.owner		=	THIS_MODULE,
	.unlocked_ioctl =	hotswap_ioctl,
	.open		=	hotswap_open,
	.write		=	hotswap_write,
	.release	=	hotswap_release,
};

static struct miscdevice hotswap_dev = {
	.minor = MOXA_HOTSWAP_MINOR,
	.name = DEVICE_NAME,
	.fops = &hotswap_fops,
};

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 15, 0))
static void hotswap_check_disk(unsigned long arg)
#else
static void hotswap_check_disk(struct timer_list *t)
#endif
{
	unsigned long now;

	now = ioread32(vaddr+SATA_P1SSTS);

	if (now != d1.status) {
		d1.status = now;
		d1.busy = 1;
		d1.idle_cnt = 0;
	} else if (d1.idle_cnt++ > 10) {
		d1.busy = 0;
		d1.idle_cnt = 0;
	}

	now = ioread32(vaddr+SATA_P2SSTS);

	if (now != d2.status) {
		d2.status = now;
		d2.busy = 1;
		d2.idle_cnt = 0;
	} else if (d2.idle_cnt++ > 10) {
		d2.busy = 0;
		d2.idle_cnt = 0;
	}

	mod_timer(&timer, jiffies+100*HZ/1000);

	pr_debug("disk 0x128=%x, 0x1A8=%x, 0x228=%x, 0x2A8=%x\n",
		ioread32(vaddr+0x128), ioread32(vaddr+0x1A8), ioread32(vaddr+0x228), ioread32(vaddr+0x2A8));
}

static int __init hotswap_init_module(void)
{
	struct pci_dev *dev;
	unsigned int device_id = KBL_DEVICE_ID;
	const char *cpu_model;

	pr_info("moxa_hotswap: initializing MOXA hotswap module\n");

	/* get base address for ACHI controller according to CPU model */
	/* to get CPU model name from DMI type 12 option 1 */
	cpu_model = get_dmi_sysconf_name();

	if (!strcmp(cpu_model, WHL_CPU_NAME)) {
		device_id = WHL_DEVICE_ID;
	}

	pr_info("moxa_hotswap: use device id 0x%x for CPU model %s\n", device_id, cpu_model);

	dev = pci_get_device(PCI_VENDOR_ID_INTEL, device_id, NULL);
	if (!dev) {
		pr_err("can't find pci device id 0x%x\n", device_id);
		return -1;
	}
	paddr = pci_resource_start(dev, 5);
	pr_debug("paddr=%lu\n", paddr);

#ifdef DEBUG
	unsigned long flag = pci_resource_flags(dev, 5);
	if (flag & IORESOURCE_IO)
		pr_debug("ioresource io\n");
	else if (flag & IORESOURCE_MEM)
		pr_debug("ioresource mem\n");
#endif
	/* register in /proc/iomem, 1024 can count in the same file
	if (!request_mem_region(paddr, 1024, "hotswap")) {
		pr_err("can't reguest mem region");
	}
	*/
	vaddr = ioremap(paddr, 1024);

	if (misc_register(&hotswap_dev)) {
		pr_err("register hotswap driver fail!\n");
		return -1;
	}

	/* initial */
	d1.status = ioread32(vaddr + SATA_P1SSTS);
	d1.idle_cnt = 0;
	d2.status = ioread32(vaddr + SATA_P2SSTS);
	d2.idle_cnt = 0;
	hotswap_led_control(1, 0);
	hotswap_led_control(2, 0);

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 15, 0))
	init_timer(&timer);
	timer.data = 0;
	timer.function = hotswap_check_disk;
#else
	timer_setup(&timer, hotswap_check_disk, 0);
#endif
	timer.expires = jiffies + 100*HZ/1000;
	add_timer(&timer);

	return 0;
}

/*
 * close and cleanup module
 */
static void __exit hotswap_cleanup_module(void)
{
	pr_debug("\n");
	misc_deregister(&hotswap_dev);
	iounmap(vaddr);
	/* release_mem_region(paddr, 1024); */
	del_timer(&timer);
}

module_init(hotswap_init_module);
module_exit(hotswap_cleanup_module);
