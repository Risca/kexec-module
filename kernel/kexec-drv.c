#include <linux/module.h>
#include <linux/err.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/reboot.h>
#include <asm/uaccess.h>
#include <linux/kexec.h>
#include <linux/kallsyms.h>
#include <linux/string.h>

int nx_enabled = 0;

static void (*machine_shutdown_ptr)(void);
static void (*kernel_restart_prepare_ptr)(char*);
#ifdef CONFIG_ARM
static void (*soft_restart_ptr)(unsigned long);
static int  (*memblock_is_region_memory_ptr)(phys_addr_t, phys_addr_t);
#endif

void machine_shutdown(void)
{
	machine_shutdown_ptr();
}
void kernel_restart_prepare(char *cmd)
{
	kernel_restart_prepare_ptr(cmd);
}
#ifdef CONFIG_ARM
void soft_restart(unsigned long addr)
{
	soft_restart_ptr(addr);
}
int memblock_is_region_memory(phys_addr_t base, phys_addr_t size)
{
	return memblock_is_region_memory_ptr(base, size);
}
#endif
static long kexecmod_ioctl(struct file *file, unsigned req, unsigned long arg)
{
	struct {
		unsigned long entry;
		unsigned long nr_segs;
		struct kexec_segment *segs;
		unsigned long flags;
	} ap;
	switch (req) {
	case LINUX_REBOOT_CMD_KEXEC - 1:
		if (copy_from_user(&ap, (void*)arg, sizeof ap))
			return -EFAULT;
		return sys_kexec_load(ap.entry, ap.nr_segs, ap.segs, ap.flags);
	case LINUX_REBOOT_CMD_KEXEC:
		return kernel_kexec();
	}
	return -EINVAL;
}

static const struct file_operations fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = kexecmod_ioctl,
};

static void *ksym(const char *name)
{
	// grep machine_shutdown /proc/kallsyms
	if (strcmp(name, "machine_shutdown") == 0) {
		return (void*)0xc010e6c9;
	}
	else if (strcmp(name, "kernel_restart_prepare") == 0) {
		return (void*)0xc0126855;
	}
	else {
		return NULL;
	}
}

static int __init
kexecmod_init(void)
{
	int maj;
	struct class *class;
	struct device *device;

	if (!(machine_shutdown_ptr = ksym("machine_shutdown"))
#ifdef CONFIG_ARM
	    //|| !(machine_shutdown_ptr = ksym("disable_nonboot_cpus"))
	    || !(soft_restart_ptr = ksym("soft_restart"))
	    || !(memblock_is_region_memory_ptr = ksym("memblock_is_region_memory"))
#endif
	    || !(kernel_restart_prepare_ptr = ksym("kernel_restart_prepare")))
		return -ENOENT;

	maj = register_chrdev(0, "kexec", &fops);
	if (maj < 0)
		return maj;
	class = class_create(THIS_MODULE, "kexec");
	if (IS_ERR(class))
		return PTR_ERR(class);
	device = device_create(class, 0, MKDEV(maj, 0), 0, "kexec");
	if (IS_ERR(device))
		return PTR_ERR(device);
	printk(KERN_INFO "kexec module loaded\n");
	return 0;
}

module_init(kexecmod_init)

MODULE_LICENSE("GPL v2");
