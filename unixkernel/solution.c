#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/errno.h>
#include <linux/stat.h>
#include <linux/printk.h>
#include <linux/kdev_t.h>
#include <linux/uaccess.h>

MODULE_AUTHOR("Vladimir Petrigo");
MODULE_LICENSE("Dual BSD/GPL");

#define TOPDIR_NAME "my_kobject"
#define ARR_SIZE 5
#define PREDEF_MAJOR 240
#define PREDEF_MINOR 0
#define DEV_NAME "solution_node"
#define PRINTBUF_SIZE 128

struct solution_dev {
	struct kobj_attribute sd_file;
	unsigned read_ops_counter;
	unsigned write_ops_counter;
};

struct solution_char_dev {
	struct cdev s_cdev;
	dev_t id;
	ssize_t access_counter;
	ssize_t write_bytes;
};

#define to_solution_cdev(x) container_of((x), struct solution_char_dev, s_cdev)

static int a = 0;
module_param(a, int, S_IRUGO);
static int b = 0;
module_param(b, int, S_IRUGO);
static int c[ARR_SIZE];
module_param_array(c, int, NULL, S_IRUGO);

static struct kobject *top_dir = NULL;

ssize_t sol_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	int sum = a + b;
	pr_debug("solution: call show\n");

	for (int i = 0; i < ARRAY_SIZE(c); ++i)
		sum += c[i];

	return scnprintf(buf, PAGE_SIZE, "%d\n", sum);
}

ssize_t sol_store(struct kobject *kobj, struct kobj_attribute *attr,
		const char *buf, size_t count)
{
	struct solution_dev *sdev_p = container_of(attr, struct solution_dev, sd_file);

	++sdev_p->write_ops_counter;
	pr_debug("solution: call store\n");
	return 0;
}

ssize_t solution_read(struct file *filp, char __user *to, size_t size, loff_t *pos)
{
	char buf[PRINTBUF_SIZE];
	struct solution_char_dev *sdev_p = to_solution_cdev(filp->private_data);
	int retval = 0;

	if (*pos != 0)
		goto end;

	pr_debug("solution: read from file\n");
	pr_debug("solution: access %zu, write bytes %zu\n", sdev_p->access_counter,
			sdev_p->write_bytes);
	retval = scnprintf(buf, PRINTBUF_SIZE, "%zu %zu\n", sdev_p->access_counter, 
			sdev_p->write_bytes);

	if (copy_to_user(to, buf, retval) != 0)
		retval = -EFAULT;
	
	*pos += retval;
end:
	return retval;
}

ssize_t solution_write(struct file *filp, const char __user *from, size_t size, loff_t *pos)
{
	struct solution_char_dev *sdev_p = to_solution_cdev(filp->private_data);	

	sdev_p->write_bytes += size;

	return size;
}

int solution_release(struct inode *inode, struct file *filp)
{
	return 0;
}

int solution_open(struct inode *inode, struct file *filp)
{
	struct solution_char_dev *sdev_p = container_of(inode->i_cdev, struct solution_char_dev, s_cdev); 

	pr_debug("solution: open file\n");
	filp->private_data = sdev_p;
	++sdev_p->access_counter;

	return 0;
}

static struct solution_dev sdev = {
	.read_ops_counter = 0,
	.write_ops_counter = 0,
	.sd_file = __ATTR(my_sys, S_IRUGO | S_IWUSR, sol_show, sol_store)
};

static struct solution_char_dev sdev_char = {
	.id = MKDEV(PREDEF_MAJOR, PREDEF_MINOR),
	.write_bytes = 0,
	.access_counter = 0
};

static struct file_operations solution_fops = {
	.owner = THIS_MODULE,
	.read = solution_read,
	.write = solution_write,
	.open = solution_open,
	.release = solution_release
};

static int __init solution_init(void)
{
	int retval = 0;

	top_dir = kobject_create_and_add(TOPDIR_NAME, kernel_kobj);

	if (!top_dir) {
		retval = -ENOMEM;
		goto fail;
	}

	retval = sysfs_create_file(top_dir, &sdev.sd_file.attr);

	if (retval != 0) {
		kobject_put(top_dir);
		goto fail;
	}

	retval = register_chrdev_region(sdev_char.id, 1, DEV_NAME);
	
	if (retval != 0)
		goto fail;

	pr_debug("solution: init cdev\n");
	cdev_init(&sdev_char.s_cdev, &solution_fops);
	sdev_char.s_cdev.owner = THIS_MODULE;
	retval = cdev_add(&sdev_char.s_cdev, sdev_char.id, 1);

	if (retval != 0)
		goto fail;

fail:
	return retval;
}

static void __exit solution_exit(void)
{
	kobject_put(top_dir);
	cdev_del(&sdev_char.s_cdev);
	unregister_chrdev_region(sdev_char.id, 1);
}

module_init(solution_init);
module_exit(solution_exit);