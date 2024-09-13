/*header section*/
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>

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

/*code section*/
static int __init pseudo_init(void)
{
    int err =-1;

    /*allocate device number*/
    err = alloc_chrdev_region(&dev_num, MINOR_NUM_START_NUMBER, MINOR_NUMBER_COUNT, "pseudo memory");
    
    pseudo_fops.owner = THIS_MODULE;
    /*intialize cdev struct*/
    cdev_init(&pseudo_cdev, &pseudo_fops);
    
    //cdev_init clears all cdev struct elements, so you need to intialize owner after cdev_init
    pseudo_cdev.owner = THIS_MODULE;

    /*register the device in VFS*/
    cdev_add(&pseudo_cdev, dev_num, MINOR_NUMBER_COUNT);
    return err;
}

static void __exit pseudo_deinit(void)
{
	pr_info("deinit pseudo device\n");
}

/*registration section*/
module_init(pseudo_init);
module_exit(pseudo_deinit);

/*registration description*/

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mohammed Thabet");
MODULE_DESCRIPTION("simple pseudo driver to read a pseudo memory");
MODULE_INFO(board, "Beagle Bone Black");