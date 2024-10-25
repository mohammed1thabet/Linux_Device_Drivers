/*************************************************************/
/*N psuedo devices driver                                    */
/*interface with 4 pseudo memory devices using one driver    */
/*************************************************************/

/*header section*/
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/uaccess.h>
#include <linux/platform_device.h>
#include "platform.h"

/*file operations*/
loff_t pseudo_llseek (struct file *file_ptr, loff_t offset, int whence);
ssize_t pseudo_read (struct file *file_ptr, char __user *buffer, size_t count, loff_t *f_pos);
ssize_t pseudo_write (struct file *file_ptr, const char __user *buffer, size_t count, loff_t *f_pos);\
int pseudo_open (struct inode *inode_ptr, struct file *file_ptr);
int pseudo_release (struct inode *inode_ptr, struct file *file_ptr);


int pseudo_plf_probe(struct platform_device *plf_dev);
int pseudo_plf_remove(struct platform_device *plf_dev);

int check_file_permission(int device_permission, fmode_t mode);


/*device private data*/
struct dev_priv_data
{
        char*  data_buffer;
        struct pseudo_platform_data plf_data;
        dev_t  dev_num;
        struct cdev dev_cdev;
        struct device *dev_ptr;
};

/*driver private data*/
struct drv_priv_data
{
    int devices_count;
    dev_t dev_num_base;
    struct class *dev_class;
    struct dev_priv_data *devs_data;
};

struct drv_priv_data drv_data;

/*file_operations struct*/
struct file_operations pseudo_fops = {
    .open       = pseudo_open,
    .release    = pseudo_release,
    .read       = pseudo_read,
    .write      = pseudo_write,
    .llseek     = pseudo_llseek,
    .owner      = THIS_MODULE
};

struct platform_driver pseudo_plf_drv ={
    .probe  = pseudo_plf_probe,
    .remove = pseudo_plf_remove,
    .driver =
    {
        .name = "pseudo-char-dev"
    }
};

/*code section*/
static int __init pseudo_plf_drv_init(void)
{
    int err;
    /*intitalize devices count to zero*/
    drv_data.devices_count =0;

    pr_info("%s:start module intialization \n", __func__);
    
    /*allocate device number*/
    err = alloc_chrdev_region(&drv_data.dev_num_base, 0, MAX_NUMBER_OF_DEVICES, "pseudo memory platform devs");
    if(err <0)
    {
        pr_err("%s:chrdev alloc failed\n", __func__);
        goto alloc_fail;
    }
    
    /*create device class*/
    drv_data.dev_class = class_create("pseudo_plf_dev_class");

    /*if class creation failed, it doesnt return null, it returns pointer to error code*/
    if(IS_ERR(drv_data.dev_class))
    {
        pr_err("%s:class creation failed\n", __func__);
        err = PTR_ERR(drv_data.dev_class);
        goto unreg_dev;
    }

    /*register the platform driver*/
    platform_driver_register(&pseudo_plf_drv);

    pr_info("%s:plf drv module loaded successfully\n",__func__);
    return err;

unreg_dev:
    /*dealloc device number*/
    unregister_chrdev_region(drv_data.dev_num_base, MAX_NUMBER_OF_DEVICES);

alloc_fail:
    pr_info("%s:module intialization failed\n", __func__);
    return err;

}

static void __exit pseudo_plf_drv_deinit(void)
{
    /*unregister the platform driver*/
    platform_driver_unregister(&pseudo_plf_drv);
    
    /*distroy driver class*/
    class_destroy(drv_data.dev_class);
    
    /*dealloc device number*/
    unregister_chrdev_region(drv_data.dev_num_base, MAX_NUMBER_OF_DEVICES);

    pr_info("%s:plf drv module unloaded\n",__func__);
}


int pseudo_plf_probe(struct platform_device* plf_dev)
{
    

    return 0;

}

int pseudo_plf_remove(struct platform_device* plf_dev)
{
    
    return 0;
}

int pseudo_open (struct inode *inode_ptr, struct file *file_ptr)
{
    
	return 0;
}
int pseudo_release (struct inode *inode_ptr, struct file *file_ptr)
{
	return 0;
}

ssize_t pseudo_read (struct file *file_ptr, char __user *buffer, size_t count, loff_t *f_pos)
{
    
	return 0;
}

ssize_t pseudo_write (struct file *file_ptr, const char __user *buffer, size_t count, loff_t *f_pos)
{
	return 0;
}

loff_t pseudo_llseek (struct file *file_ptr, loff_t offset, int whence)
{
	return 0;
}

int check_file_permission(int device_permission, fmode_t request_mode)
{
    if(device_permission == RW_PERMISSION)
        return 0;
    
    /*if the device is read only, check if open request mode is read only*/
    if(device_permission == RONLY_PERMISSION)
    {
        if((request_mode & FMODE_READ) && !(request_mode & FMODE_WRITE))
            return 0;
    }

    /*if the device is write only, check if open request mode is write only*/
    if(device_permission == WONLY_PERMISSION)
    {
        if((request_mode & FMODE_WRITE) && !(request_mode & FMODE_READ))
            return 0;
    }

    return -EPERM;
}

/*registration section*/
module_init(pseudo_plf_drv_init);
module_exit(pseudo_plf_drv_deinit);

/*registration description*/

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mohammed Thabet");
MODULE_DESCRIPTION("pseudo platform char device driver");
MODULE_INFO(HW, "x86 machine");