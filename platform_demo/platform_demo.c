#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include "platform_demo.h"



static int demo_probe(struct platform_device *pdev)
{
	struct resource *res;
	void __iomem *base;

	res		= platform_get_resource(pdev, IORESOURCE_MEM, 0);
	base	= devm_ioremap_resource(&pdev->dev, res);
	if(IS_ERR(base))
		return PTR_ERR(base);

	dev_info(&pdev->dev, "probe: base=%p\n", base);
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
