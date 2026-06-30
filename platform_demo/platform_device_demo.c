#include <linux/module.h>
#include <linux/platform_device.h>
#include "platform_demo.h"

static struct resource demo_resources[] = {
    [0] = {
        .start = DEMO_MEM_START,
        .end   = DEMO_MEM_START + DEMO_MEM_SIZE - 1,
        .flags = IORESOURCE_MEM,
    },
    [1] = {
        .start = DEMO_IRQ,
        .end   = DEMO_IRQ,
        .flags = IORESOURCE_IRQ,
    },
};

static struct platform_device demo_device = {
    .name          = "platform-demo",
    .id            = -1,
    .num_resources = ARRAY_SIZE(demo_resources),
    .resource      = demo_resources,
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
