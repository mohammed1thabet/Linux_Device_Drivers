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

/*file_operations struct*/
struct file_operations pseudo_fops;
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

/*registration section*/
module_init(pseudo_init);
module_exit(pseudo_deinit);

/*registration description*/

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mohammed Thabet");
MODULE_DESCRIPTION("simple pseudo driver to read a pseudo memory");
MODULE_INFO(board, "Beagle Bone Black");