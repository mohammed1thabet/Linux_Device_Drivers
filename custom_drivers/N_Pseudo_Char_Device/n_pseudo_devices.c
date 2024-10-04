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
        char* buffer;
        size_t size;
        const char* ID;
        /*firs bit for read permition and second bit for write permition*/
        /* example: 0b11 means RW permition                             */
        int permition;
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
            .buffer     = device1_mem,
            .size       = DEV1_MEM_SIZE,
            .ID         = "1024_BYTE_RONLY_MEM",
            .permition  = 0b01
        },
        [1] = 
        {
            .buffer     = device2_mem,
            .size       = DEV2_MEM_SIZE,
            .ID         = "1000_BYTE_RW_MEM",
            .permition  = 0b11
        },
        [2] = 
        {
            .buffer     = device3_mem,
            .size       = DEV3_MEM_SIZE,
            .ID         = "512_BYTE_WONLY_MEM",
            .permition  = 0b10
        },
        [3] = 
        {
            .buffer     = device4_mem,
            .size       = DEV4_MEM_SIZE,
            .ID         = "512_BYTE_RW_MEM",
            .permition  = 0b11
        }
    }
};

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
    
    return 0;
}

ssize_t pseudo_write (struct file *filePtr, const char __user *buffer, size_t count, loff_t *f_pos)
{
	
    return 0;
}

loff_t pseudo_llseek (struct file *filePtr, loff_t offset, int whence)
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