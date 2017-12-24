#include <linux/module.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <uaccess.h>

#define GLOBALMEM_SIZE 1024
#define GLOBALMEM_MAGIC 'g'
#define MEM_CLEAR _IO(GLOBALMEM_MAGIC, 0)
#define GLOBALMEM_MAJOR 230
#define DEVICE_NUM 10

static int globalmem_major = GLOBALMEM_MAJOR;

struct globalmem_dev {
	struct cdev cdev;
	unsigned char mem[GLOBALMEM_SIZE];
	struct mutex mutex;
};

static const struct file_operations globalmem_fops = {
	.owner = THIS_MODULE,
	.llseek = globalmem_llseek,
	.read = globalmem_read,
	.write = globalmem_write,
	.open = globalmem_open,
	.unlocked_ioctl = globalmem_ioctl,
	.release = globalmem_release,
};

struct globalmem_dev *globalmem_devp;

static ssize_t globalmem_read(struct file *filp, char __user *buf, size_t size, loff_t *ppos)
{
	unsigned long p = *ppos;
	unsigned int count = size;
	int ret = 0;
	struct globalmem_dev *dev = filp->private_data;

	if (p >= GLOBALMEM_SIZE)
		return 0;

	if (count > GLOBALMEM_SIZE - p)
		count = GLOBALMEM_SIZE - p;   //当读的个数大于剩下的时，读剩下的

	mutex_lock(&dev->mutex);

	if (copy_to_user(buf, dev->mem + p, count)) {  //返回未读完的个数
		ret = -EFAULT;
	} else {
		*ppos += count;
		ret = count;
		printk("read %u bytes from %lu\n", count, p);
	}
	mutex_unlock(&dev->mutex);

	return ret;
}

static ssize_t globalmem_write(struct file *filp, const char __user *buf,
					size_t size, loff_t *ppos)
{
	unsigned long p = *ppos;
	unsigned int count = size;
	int ret = 0;
	struct globalmem_dev *dev = filp->private_data;

	if (p >= GLOBALMEM_SIZE)
		return 0;

	mutex_lock(&dev->mutex);

	if (count > GLOBALMEM_SIZE - p)
		count = GLOBALMEM_SIZE - p;   //当写的个数大于剩下的时，写剩下的

	if (copy_from_user(dev->mem + p, buf, count)) {  //返回未读完的个数
		ret = -EFAULT;
	} else {
		*ppos += count;
		ret = count;
		printk("writtem %u bytes from %lu\n", count, p);
	}
	mutex_unlock(&dev->mutex);
	return ret;
}

static long globalmem_ioctl(struct file *filp, unsigned int cmd,
						unsigned long arg)
{
	struct globalmem_dev *dev = filp->private_data;
	switch (cmd) {
	case MEM_CLEAR:
		mutex_lock(&dev->mutex);
		memset(dev->mem, 0, sizeof(dev->mem));
		mutex_unlock(&dev->mutex);
		printk("globalmem is set to zero\n");
		break;

	default:
		return -EINVAL;
	}
	return 0;
}						

static int globalmem_open(struct inode *inode, struct file *filp)
{
//	filp->private_data = globalmem_devp;
	struct globalmem_dev *dev = container_of(inode->i_cdev, struct globalmem_dev, cdev);
	filp->private_data = dev;
	return 0;
}
							
static void globalmem_setup_cdev(struct globalmem_dev *dev, int index)
{
	int err, devno = MKDEV(globalmem_major, index);
	cdev_init(&dev->cdev, &globalmem_fops);
	dev->cdev.owner = THIS_MODULE;
	err = cdev_add(&dev->cdev, devno, 1);
	if (err)
		printk("error %d adding globalmem%d\n", err, index);

}

static int __init globalmem_init(void)
{
	int ret;
	int i;
	dev_t devno = MKDEV(globalmem_major, 0);
	if (globalmem_major)
//		ret = register_chrdev_region(devno, 1, "globalmem");
		ret = register_chrdev_region(devno, DEVICE_NUM, "globalmem");
	else {
//		ret = alloc_chrdev_region(&dev, 0, 1, "globalmem");
		ret = alloc_chrdev_region(&dev, 0, DEVICE_NUM, "globalmem");
		globalmem_major = MAJOR(devno);
	}
	if (ret < 0)
		return ret;
//	globalmem_devp = kzalloc(sizeof(struct globalmem_dev), GFP_KERNEL);
	globalmem_devp = kzalloc(sizeof(struct globalmem_dev) * DEVICE_NUM, GFP_KERNEL);
	if (!globalmem_devp) {
		ret = -ENOMEM;
		goto fail_malloc;
	}

//	globalmem_setup_cdev(globalmem_devp, 0);
	for (i = 0; i < DEVICE_NUM; i++) {
		mutex_init(&(globalmem_devp + i)->mutex);
		globalmem_setup_cdev(globalmem_devp + i, i);
	}
	return 0;
	
fail_malloc:
//	unregister_chrdev_region(devno, 1);
	unregister_chrdev_region(devno, DEVICE_NUM);
	return ret;
}

static void __exit globalmem_exit(void)
{
//	cdev_del(&globalmem_devp->cdev);
	int i;
	for (i = 0; i < DEVICE_NUM; i++) {
		cdev_del(&(globalmem_devp + i)->cdev)
	}
	kfree(globalmem_devp);
//	unregister_chrdev_region(MKDEV(globalmem_major, 0), 1);
	unregister_chrdev_region(MKDEV(globalmem_major, 0), DEVICE_NUM);
}

module_init(globalmem_init);
module_exit(globalmem_exit);
MODULE_LICENSE("GPL");