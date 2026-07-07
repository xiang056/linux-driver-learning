#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include "platform_demo.h"

static irqreturn_t demo_irq_handler(int irq, void *dev_id)
{
    pr_info("platform-demo: irq %d fired\n", irq);
    return IRQ_HANDLED;
}

static int demo_probe(struct platform_device *pdev)
{
    struct resource *res;
    int irq;
    int ret;
    irq = platform_get_irq(pdev, 0);
    if(irq <0) {
        return irq;
    }
    ret = devm_request_irq(&pdev->dev, irq, demo_irq_handler, 0, "platform-demo", NULL);
    if(ret)
        return ret;
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
