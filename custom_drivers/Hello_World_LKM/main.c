/*header section*/
#include <linux/module.h>


/*code section*/
static int __init helloworld_init(void)
{
	pr_info("hi world\n");
    return 0;
}

static void __exit helloworld_deinit(void)
{
	pr_info("good bye world\n");
}

/*registration section*/
module_init(helloworld_init);
module_exit(helloworld_deinit);

/*registration description*/

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mohammed Thabet");
MODULE_DESCRIPTION("simple hellow world LKM");
MODULE_INFO(board, "Beagle Bone Black");