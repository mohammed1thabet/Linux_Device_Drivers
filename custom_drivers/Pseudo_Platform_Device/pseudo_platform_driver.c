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
    int err;
    
    struct dev_priv_data *new_dev_data;
    struct pseudo_platform_data *new_plf_data;

    new_plf_data = (struct pseudo_platform_data*)dev_get_platdata(&plf_dev->dev);

    if(new_plf_data == NULL)
    {
        pr_info("%s:invalid platform data\n",__func__);
        return -EINVAL;
    }

    new_dev_data = devm_kzalloc(&plf_dev->dev, sizeof(*new_dev_data), GFP_KERNEL);
    if(new_dev_data == NULL)
    {
        pr_info("%s:cannot allocate driver private data\n",__func__);
        return -ENOMEM;
    }

    memcpy((void*)&new_dev_data->plf_data, (void*)new_plf_data, sizeof(*new_plf_data));

    pr_info("%s device platform data: serial_number:%s\nsize:%ld\npermission:%x",__func__, new_dev_data->plf_data.serial_number, new_dev_data->plf_data.size, new_dev_data->plf_data.permission);

    /*allocate memory for device mem bvuffer*/
    new_dev_data->data_buffer = devm_kzalloc(&plf_dev->dev, new_dev_data->plf_data.size, GFP_KERNEL);
    if(new_dev_data->data_buffer == NULL)
    {
        pr_info("%s:cannot allocate device memory buffer\n",__func__);
        return -ENOMEM;
    }

    /*initalize device number feild*/
    new_dev_data->dev_num = drv_data.dev_num_base + plf_dev->id;

    /*intialize cdev struct*/
    cdev_init(&new_dev_data->dev_cdev, &pseudo_fops);
        
    /*cdev_init clears all cdev struct elements, so you need to intialize owner after cdev_init*/
    new_dev_data->dev_cdev.owner = THIS_MODULE;

    /*register the device in VFS*/
    err = cdev_add(&new_dev_data->dev_cdev, new_dev_data->dev_num, 1);
    if(err <0)
    {
        pr_err("cdev registration failed\n");
        return err;
    }

    /*create device files*/
    new_dev_data->dev_ptr = device_create(drv_data.dev_class, NULL, new_dev_data->dev_num, NULL, "pseudo_char_dev:%d",plf_dev->id);
    
    if(IS_ERR(new_dev_data->dev_ptr))
    {
        pr_err("device file creation failed\n");
        err = PTR_ERR(new_dev_data->dev_ptr);
        cdev_del(&new_dev_data->dev_cdev);
        return err;
    }

    /*save driver data in platform device struct*/
    dev_set_drvdata(&plf_dev->dev, new_dev_data);

    drv_data.devices_count++;
    pr_info("%s:device is detected\n",__func__);

    return 0;

}

int pseudo_plf_remove(struct platform_device* plf_dev)
{
    struct dev_priv_data *rm_dev_data = dev_get_drvdata(&plf_dev->dev);

    /*destroy device file*/
    device_destroy(drv_data.dev_class, rm_dev_data->dev_num);
    
    /*delete cdev*/
    cdev_del(&rm_dev_data->dev_cdev);

    drv_data.devices_count--;

    pr_info("%s:device removed\n",__func__);
    
    return 0;
}

int pseudo_open (struct inode *inode_ptr, struct file *file_ptr)
{
    int err;
    int minor_num;
    struct dev_priv_data *dev_data;

    pr_info("pseudo_open method called:\n");

    minor_num = MINOR(inode_ptr->i_rdev);
    pr_info("minor number:%d:\n", minor_num);
    
    /*extract pointer to device data using cdev*/
    dev_data = container_of(inode_ptr->i_cdev, struct dev_priv_data, dev_cdev);

    /*update file private data pointer with the device data pointer*/
    file_ptr->private_data = dev_data;

    /*check if the requested permission compatable with device permission*/
    err = check_file_permission(dev_data->plf_data.permission, file_ptr->f_mode);
    
    if(err == 0)
    {
        pr_info("file opened successfully\n");
    }
    else
    {
        pr_info("file open failed\n");
    }
	return err;
}
int pseudo_release (struct inode *inode_ptr, struct file *file_ptr)
{
    pr_info("pseudo_release method called:\n");
	return 0;
}

ssize_t pseudo_read (struct file *file_ptr, char __user *buffer, size_t count, loff_t *f_pos)
{
    struct dev_priv_data *data_ptr = (struct dev_priv_data *)file_ptr->private_data;
    size_t size = data_ptr->plf_data.size;

    pr_info("pseudo_read method called, count:%zu, file position:%lld\n", count, *f_pos);

    /*if the count exeeds the memory size truncate the count*/
    if((count + *f_pos) > size)
        count = size - *f_pos;
    
    /*copy data, note that this code is not thread safe*/
    if(copy_to_user(buffer, data_ptr->data_buffer+(*f_pos), count) > 0)
        return -EFAULT;

    /*update file position*/
    *f_pos = *f_pos + count;

    pr_info("number of bytes have been read%zu, file position:%lld\n", count, *f_pos);
	return count;
}

ssize_t pseudo_write (struct file *file_ptr, const char __user *buffer, size_t count, loff_t *f_pos)
{
	struct dev_priv_data *data_ptr = (struct dev_priv_data *)file_ptr->private_data;
    size_t size = data_ptr->plf_data.size;

	pr_info("pseudo_write method called, count:%zu, file position:%lld\n", count, *f_pos);

    if(*f_pos == size)
    {
        /*EOF*/
        pr_info("no space left\n");
        return -ENOMEM;
    }
    
    /*if the count exeeds the memory size truncate the count*/
    if((count + *f_pos) > size)
        count = size - *f_pos;
    
    /*copy data, note that this code is not thread safe*/
    if(copy_from_user(data_ptr->data_buffer+(*f_pos), buffer, count) > 0)
        return -EFAULT;

    /*update file position*/
    *f_pos = *f_pos + count;
        
    pr_info("number of bytes have been written%zu, file position:%lld\n", count, *f_pos);
	return count;
}

loff_t pseudo_llseek (struct file *file_ptr, loff_t offset, int whence)
{
	struct dev_priv_data *data_ptr = (struct dev_priv_data *)file_ptr->private_data;
    size_t size = data_ptr->plf_data.size;
    loff_t new_pos;

    pr_info("pseudo_llseek method called, current f_pos:%lld\n", file_ptr->f_pos);
    
    switch (whence)
    {
        case SEEK_SET:
            /*return error if the file position will go beyond file memory or if it will be <0*/
            if((offset > size) || (offset < 0))
                return -EINVAL;

            file_ptr->f_pos = offset;
        break;

        case SEEK_CUR:
            new_pos = file_ptr->f_pos +offset;
            
            /*return error if the file position will go beyond file memory or if it will be <0*/
            if( (new_pos > size) || (new_pos < 0) )
                return -EINVAL;

            file_ptr->f_pos = new_pos;
        break;
        
        case SEEK_END:
            new_pos = size + offset;

            /*return error if the file position will go beyond file memory or if it will be <0*/
            if( (new_pos > size) || (new_pos < 0) )
                return -EINVAL;

            file_ptr->f_pos = new_pos;
        break;
        
        default:
            return -EINVAL;
        break;
    }
    pr_info("new f_pos:%lld\n", file_ptr->f_pos);
	return file_ptr->f_pos;
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