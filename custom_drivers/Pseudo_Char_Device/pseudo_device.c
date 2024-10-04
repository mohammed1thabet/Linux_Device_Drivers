/*header section*/
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/uaccess.h>

#define DEV_MEM_SIZE            512
#define MINOR_NUM_START_NUMBER  0
#define MINOR_NUMBER_COUNT      1

static char device_mem[DEV_MEM_SIZE];

/*device number*/
dev_t dev_num;

/*cdev var*/
struct cdev pseudo_cdev;

/*file operations*/
loff_t pseudo_llseek (struct file *filePtr, loff_t offset, int whence);
ssize_t pseudo_read (struct file *filePtr, char __user *buffer, size_t count, loff_t *f_pos);
ssize_t pseudo_write (struct file *filePtr, const char __user *buffer, size_t count, loff_t *f_pos);\
int pseudo_open (struct inode *inodePtr, struct file *filePtr);
int pseudo_release (struct inode *inodePtr, struct file *filePtr);

/*file_operations struct*/
struct file_operations pseudo_fops = {
    .open       = pseudo_open,
    .release    = pseudo_release,
    .read       = pseudo_read,
    .write      = pseudo_write,
    .llseek     = pseudo_llseek,
    .owner      = THIS_MODULE
};

struct class *pseudo_class;

struct device *pseudo_device;

/*code section*/
static int __init pseudo_init(void)
{
    int err;

    /*allocate device number*/
    err = alloc_chrdev_region(&dev_num, MINOR_NUM_START_NUMBER, MINOR_NUMBER_COUNT, "pseudo memory");
    if(err <0)
    {
        pr_err("chrdev alloc failed\n");
        goto alloc_fail;
    }
    pr_info("start module intialization \n");

    pr_info("major:%d,minor:%d\n", MAJOR(dev_num), MINOR(dev_num));

    /*intialize cdev struct*/
    cdev_init(&pseudo_cdev, &pseudo_fops);
    
    //cdev_init clears all cdev struct elements, so you need to intialize owner after cdev_init
    pseudo_cdev.owner = THIS_MODULE;

    /*register the device in VFS*/
    err = cdev_add(&pseudo_cdev, dev_num, MINOR_NUMBER_COUNT);
    if(err <0)
    {
        pr_err("device registration failed\n");
        goto unreg_dev;
    }
    /*create device class*/
    pseudo_class = class_create("pseudo_char_class");

    /*if class creation failed, it doesnt return null, it returns pointer to error code*/
    if(IS_ERR(pseudo_class))
    {
        pr_err("class creation failed\n");
        err = PTR_ERR(pseudo_class);
        goto cdev_del;
    }

    /*create device file*/
    pseudo_device = device_create(pseudo_class, NULL, dev_num, NULL, "pseudo_char_dev");
    
    if(IS_ERR(pseudo_device))
    {
        pr_err("device creation failed\n");
        err = PTR_ERR(pseudo_device);
        goto class_del;
    }
    
    pr_info("module intialization done without errors\n");
    
    return err;

class_del:
    class_destroy(pseudo_class);

cdev_del:
    cdev_del(&pseudo_cdev);

unreg_dev:
    /*dealloc device number*/
    unregister_chrdev_region(dev_num, 1);

alloc_fail:
    pr_info("module intialization failed\n");
    return err;

}

static void __exit pseudo_deinit(void)
{
	pr_info("unload pseudo char driver\n");

    device_destroy(pseudo_class, dev_num);

    class_destroy(pseudo_class);

    cdev_del(&pseudo_cdev);

    unregister_chrdev_region(dev_num, 1);

}

int pseudo_open (struct inode *inodePtr, struct file *filePtr)
{
    pr_info("pseudo_open method called:\n");
	return 0;
}
int pseudo_release (struct inode *inodePtr, struct file *filePtr)
{
    pr_info("pseudo_release method called:\n");
	return 0;
}

ssize_t pseudo_read (struct file *filePtr, char __user *buffer, size_t count, loff_t *f_pos)
{
    pr_info("pseudo_read method called, count:%zu, file position:%lld\n", count, *f_pos);

    /*if the count exeeds the memory size truncate the count*/
    if((count + *f_pos) > DEV_MEM_SIZE)
        count = DEV_MEM_SIZE - *f_pos;
    
    /*copy data, note that this code is not thread safe*/
    if(copy_to_user(buffer, &device_mem[*f_pos], count) > 0)
        return -EFAULT;

    /*update file position*/
    *f_pos = *f_pos + count;

    pr_info("number of bytes have been read%zu, file position:%lld\n", count, *f_pos);
	return count;
}

ssize_t pseudo_write (struct file *filePtr, const char __user *buffer, size_t count, loff_t *f_pos)
{
	pr_info("pseudo_write method called, count:%zu, file position:%lld\n", count, *f_pos);

    if(*f_pos == DEV_MEM_SIZE)
    {
        /*EOF reached*/
        pr_info("no space left\n");
        return -ENOMEM;
    }
    
    /*if the count exeeds the memory size truncate the count*/
    if((count + *f_pos) > DEV_MEM_SIZE)
        count = DEV_MEM_SIZE - *f_pos;
    
    /*copy data, note that this code is not thread safe*/
    if(copy_from_user(&device_mem[*f_pos], buffer, count) > 0)
        return -EFAULT;

    /*update file position*/
    *f_pos = *f_pos + count;
        
    pr_info("number of bytes have been written%zu, file position:%lld\n", count, *f_pos);
	return count;
}

loff_t pseudo_llseek (struct file *filePtr, loff_t offset, int whence)
{
    pr_info("pseudo_llseek method called, current f_pos:%lld\n", filePtr->f_pos);
    loff_t newPos;
    switch (whence)
    {
        case SEEK_SET:
            /*return error if the file position will go beyond file memory or if it will be <0*/
            if((offset > DEV_MEM_SIZE) || (offset < 0))
                return -EINVAL;

            filePtr->f_pos = offset;
        break;

        case SEEK_CUR:
            newPos = filePtr->f_pos +offset;
            
            /*return error if the file position will go beyond file memory or if it will be <0*/
            if( (newPos > DEV_MEM_SIZE) || (newPos < 0) )
                return -EINVAL;

            filePtr->f_pos = newPos;
        break;
        
        case SEEK_END:
            newPos = DEV_MEM_SIZE + offset;

            /*return error if the file position will go beyond file memory or if it will be <0*/
            if( (newPos > DEV_MEM_SIZE) || (newPos < 0) )
                return -EINVAL;

            filePtr->f_pos = newPos;
        break;
        
        default:
            return -EINVAL;
        break;
    }
    pr_info("new f_pos:%lld\n", filePtr->f_pos);
	return filePtr->f_pos;
}

/*registration section*/
module_init(pseudo_init);
module_exit(pseudo_deinit);

/*registration description*/

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mohammed Thabet");
MODULE_DESCRIPTION("simple pseudo driver to read a pseudo memory");
MODULE_INFO(board, "Beagle Bone Black");