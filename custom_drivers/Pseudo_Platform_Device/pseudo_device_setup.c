#include <linux/module.h>
#include <linux/platform_device.h>
#include "platform.h"
 

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
        .platform_data = &pseudo_plf_data[0]
    }

};
struct platform_device pseudo_plf_dev1 = 
{
    .name = "pseudo-char-dev",
    .id   = 1,
    .dev = 
    {
        .platform_data = &pseudo_plf_data[1]
    }

};

static int __init pseudo_init(void)
{
    platform_device_register(&pseudo_plf_dev0);
    platform_device_register(&pseudo_plf_dev1);
    return 0;
}

static void __exit pseudo_deinit(void)
{
    platform_device_unregister(&pseudo_plf_dev0);
    platform_device_unregister(&pseudo_plf_dev1);
}

/*module registration*/
module_init(pseudo_init);
module_exit(pseudo_deinit);

/*module description*/
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mohammed Thabet");
MODULE_DESCRIPTION("pseudo platform char device driver");
MODULE_INFO(HW, "x86 machine");