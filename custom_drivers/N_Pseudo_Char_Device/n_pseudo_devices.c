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


#define DEV1_MEM_SIZE           1024
#define DEV2_MEM_SIZE           1024
#define DEV3_MEM_SIZE           512
#define DEV4_MEM_SIZE           512

#define MINOR_NUM_START_NUMBER  0
#define NUMBER_OF_DEVICES       4

/*devices pseudo memory*/
static char device1_mem[DEV1_MEM_SIZE];
static char device2_mem[DEV2_MEM_SIZE];
static char device3_mem[DEV3_MEM_SIZE];
static char device4_mem[DEV4_MEM_SIZE];

/*device private data*/
struct dev_priv_data
{
        char* data_buffer;
        size_t size;
        const char* ID;
        /*firs bit for read permission and second bit for write permission*/
        /* example: 0b11 means RW permission                             */
        int permission;
        struct cdev dev_cdev;
        struct device *dev_ptr;
};

/*driver data, data used by the driver to access all 4 devices*/
struct drv_priv_data
{
    /*number of devices*/
    int dev_count;
    /*device number*/
    dev_t dev_num;

    struct class *dev_class;
    
    struct dev_priv_data devs_data[NUMBER_OF_DEVICES];
};

struct drv_priv_data drv_data = {
    .dev_count = NUMBER_OF_DEVICES,
    .devs_data =
    {
        [0] = 
        {
            .data_buffer = device1_mem,
            .size        = DEV1_MEM_SIZE,
            .ID          = "1024_BYTE_RONLY_MEM",
            .permission  = 0b01
        },
        [1] = 
        {
            .data_buffer = device2_mem,
            .size        = DEV2_MEM_SIZE,
            .ID          = "1000_BYTE_RW_MEM ",
            .permission  = 0b11
        },
        [2] = 
        {
            .data_buffer = device3_mem,
            .size        = DEV3_MEM_SIZE,
            .ID          = "512_BYTE_WONLY_MEM",
            .permission  = 0b10
        },
        [3] = 
        {
            .data_buffer = device4_mem,
            .size        = DEV4_MEM_SIZE,
            .ID          = "512_BYTE_RW_MEM",
            .permission  = 0b11
        }
    }
};
/*file operations*/
loff_t pseudo_llseek (struct file *file_ptr, loff_t offset, int whence);
ssize_t pseudo_read (struct file *file_ptr, char __user *buffer, size_t count, loff_t *f_pos);
ssize_t pseudo_write (struct file *file_ptr, const char __user *buffer, size_t count, loff_t *f_pos);\
int pseudo_open (struct inode *inode_ptr, struct file *file_ptr);
int pseudo_release (struct inode *inode_ptr, struct file *file_ptr);


int check_file_permission (void);

/*file_operations struct*/
struct file_operations pseudo_fops = {
    .open       = pseudo_open,
    .release    = pseudo_release,
    .read       = pseudo_read,
    .write      = pseudo_write,
    .llseek     = pseudo_llseek,
    .owner      = THIS_MODULE
};

/*code section*/
static int __init pseudo_init(void)
{
    int err;
    int itr;
    /*allocate device number*/
    err = alloc_chrdev_region(&drv_data.dev_num, MINOR_NUM_START_NUMBER, NUMBER_OF_DEVICES, "n pseudo memory devices");
    if(err <0)
    {
        pr_err("chrdev alloc failed\n");
        goto alloc_fail;
    }
    pr_info("start module intialization \n");

    /*create device class*/
    drv_data.dev_class = class_create("n_pseudo_char_class");

    /*if class creation failed, it doesnt return null, it returns pointer to error code*/
    if(IS_ERR(drv_data.dev_class))
    {
        pr_err("class creation failed\n");
        err = PTR_ERR(drv_data.dev_class);
        goto unreg_dev;
    }
    
    for(itr=0; itr<NUMBER_OF_DEVICES; itr++)
    {
        pr_info("major:%d,minor:%d\n", MAJOR(drv_data.dev_num+itr), MINOR(drv_data.dev_num+itr));

        /*intialize cdev struct*/
        cdev_init(&drv_data.devs_data[itr].dev_cdev, &pseudo_fops);
        
        /*cdev_init clears all cdev struct elements, so you need to intialize owner after cdev_init*/
        drv_data.devs_data[itr].dev_cdev.owner = THIS_MODULE;

        /*register the device in VFS*/
        err = cdev_add(&drv_data.devs_data[itr].dev_cdev, drv_data.dev_num+itr, 1);
        if(err <0)
        {
            pr_err("device registration failed\n");
            goto cdev_del;
        }
    }
    for(int itr=0; itr<NUMBER_OF_DEVICES; itr++)
    {
        /*create device files*/
        drv_data.devs_data[itr].dev_ptr = device_create(drv_data.dev_class, NULL, drv_data.dev_num+itr, NULL, "pseudo_char_dev:%d",itr);
        
        if(IS_ERR(drv_data.devs_data[itr].dev_ptr))
        {
            pr_err("device creation failed\n");
            err = PTR_ERR(drv_data.devs_data[itr].dev_ptr);
            goto dev_del;
        }

    }

    
    pr_info("module intialization done without errors\n");
    
    return err;

dev_del:
    for(;itr>=0;itr--)
        device_destroy(drv_data.dev_class, drv_data.dev_num+itr);
    
    /*reset the iterator to be used in cdev_del*/
    itr = NUMBER_OF_DEVICES-1;

cdev_del:
    for(;itr>=0;itr--)
        cdev_del(&drv_data.devs_data[itr].dev_cdev);

    class_destroy(drv_data.dev_class);

unreg_dev:
    /*dealloc device number*/
    unregister_chrdev_region(drv_data.dev_num, NUMBER_OF_DEVICES);

alloc_fail:
    pr_info("module intialization failed\n");
    return err;

}

static void __exit pseudo_deinit(void)
{
	pr_info("unload pseudo char driver\n");

    for(int itr=0;itr<NUMBER_OF_DEVICES;itr++)
    {
        device_destroy(drv_data.dev_class, drv_data.dev_num+itr);
        cdev_del(&drv_data.devs_data[itr].dev_cdev);
    }
    class_destroy(drv_data.dev_class);
    
    /*dealloc device number*/
    unregister_chrdev_region(drv_data.dev_num, NUMBER_OF_DEVICES);

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

    err = check_file_permission();
    
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
    size_t size = data_ptr->size;

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
    size_t size = size;

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
    size_t size = data_ptr->size;
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

int check_file_permission()
{
    
    return 0;
}

/*registration section*/
module_init(pseudo_init);
module_exit(pseudo_deinit);

/*registration description*/

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mohammed Thabet");
MODULE_DESCRIPTION("simple pseudo driver to read a multiple pseudo memory devices");
MODULE_INFO(HW, "x86 machine");