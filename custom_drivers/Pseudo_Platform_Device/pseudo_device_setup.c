/**************************************************************/
/*psuedo platform device driver                               */
/*interface with 2 pseudo memory devices using platform driver*/
/**************************************************************/

/********file includes********/

#include <linux/module.h>
#include <linux/platform_device.h>
#include "platform.h"
 
/********functions decleartions*******/

/*device release function*/
void pseudo_dev_release(struct device*);


/********data types definition********/

struct pseudo_platform_data pseudo_plf_data[PLF_DEV_COUNT] = 
{
    [0] = 
    {
        .size           = DEV0_MEM_SIZE,
        .serial_number  = "PLFDEV0000",
        .permission     = RONLY_PERMISSION
    },
    [1] = 
    {
        .size           = DEV1_MEM_SIZE,
        .serial_number  = "PLFDEV0001",
        .permission     = RW_PERMISSION
    }
};

struct platform_device pseudo_plf_dev0 = 
{
    .name = "pseudo-char-dev",
    .id   = 0,
    .dev = 
    {
        .platform_data = &pseudo_plf_data[0],
        .release       = pseudo_dev_release
    }

};
struct platform_device pseudo_plf_dev1 = 
{
    .name = "pseudo-char-dev",
    .id   = 1,
    .dev = 
    {
        .platform_data = &pseudo_plf_data[1],
        .release       = pseudo_dev_release
    }

};

/********functions implementation*******/

static int __init pseudo_plf_dev_init(void)
{
    platform_device_register(&pseudo_plf_dev0);
    platform_device_register(&pseudo_plf_dev1);
    pr_info("%s:plf setup module loaded successfully\n",__func__);
    return 0;
}

static void __exit pseudo_plf_dev_deinit(void)
{
    /*if you have too many devices, you can use  platform_add_devices(struct platform_device **devs, int num)*/
    platform_device_unregister(&pseudo_plf_dev0);
    platform_device_unregister(&pseudo_plf_dev1);
    pr_info("%s:plf setup module unloaded\n",__func__);
}

void pseudo_dev_release(struct device* dev)
{
    pr_info("%s:platform device released\n",__func__);
}

/*module registration*/
module_init(pseudo_plf_dev_init);
module_exit(pseudo_plf_dev_deinit);

/*module description*/
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mohammed Thabet");
MODULE_DESCRIPTION("pseudo platform char device driver");
MODULE_INFO(HW, "x86 machine");