#include <linux/module.h>
#include <linux/platform_device.h>
#include "platform_demo.h"

static int demo_probe(struct platform_device *pdev)
{
	dev_info(&pdev->dev, "probe: hello from platform driver\n");
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
