#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include "platform_demo.h"



static int demo_probe(struct platform_device *pdev)
{
    struct resource *res;

    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (!res) {
        dev_err(&pdev->dev, "no memory resource\n");
        return -ENODEV;
    }

    dev_info(&pdev->dev, "probe: mem start=0x%llx size=0x%llx\n",
             (u64)res->start, (u64)resource_size(res));
    return 0;
}


static int demo_remove(struct platform_device *pdev)
{
	dev_info(&pdev->dev, "remove: bye\n");
	return 0;
}

static struct platform_driver demo_driver = {
	.probe	=  demo_probe,
	.remove	=  demo_remove,
	.driver =  {
		.name = "platform-demo",
	},
}; 


module_platform_driver(demo_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Edward");
MODULE_DESCRIPTION("Minimal platform driver demo");
