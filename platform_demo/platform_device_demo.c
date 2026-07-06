#include <linux/module.h>
#include <linux/platform_device.h>
#include "platform_demo.h"

static void demo_device_release(struct device *dev)
{
	/*靜態分配，不需要釋放*/
}

static struct resource demo_resources[] = {
	[0] = {
		.start = DEMO_MEM_START,	//0x10000000
		.end   = DEMO_MEM_START + DEMO_MEM_SIZE - 1,	//0x10000fff
		.flags = IORESOURCE_MEM,	//記憶體資源
	},
};
static struct platform_device demo_device = {
	.name = "platform-demo",
	.id   = -1,
	.dev  = {
		.release = demo_device_release,
	},
	.num_resources = ARRAY_SIZE(demo_resources),
	.resource 	   = demo_resources,
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


