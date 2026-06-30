#include <linux/module.h>
#include <linux/platform_device.h>

static void demo_device_release(struct device *dev)
{
	/*靜態分配，不需要釋放*/
}

static struct platform_device demo_device = {
	.name = "platform-demo",
	.id   = -1,
	.dev  = {
		.release = demo_device_release,
	},
};

static int __init demo_device_init(void)
{
	return platform_device_register(&demo_device);
}

static void __exit demo_device_exit(void)
{
	platform_device_unregister(&demo_device);
}

module_init(demo_device_init);
module_exit(demo_device_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Edward");
MODULE_DESCRIPTION("Fake platform device for demo");


