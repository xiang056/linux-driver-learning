#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/of.h>
#include "platform_demo.h"


static const struct of_device_id demo_of_match []= {
    { .compatible = "myvendor,platform-demo" },
    {}
};

static irqreturn_t demo_irq_handler(int irq, void *dev_id)
{
    pr_info("platform-demo: irq %d fired\n", irq);
    return IRQ_HANDLED;
}

static int demo_probe(struct platform_device *pdev)
{
    struct resource *res;
    struct demo_data *data;
    int irq;
    int ret;

    data = devm_kzalloc(&pdev->dev, sizeof(*data), GFP_KERNEL);

    irq = platform_get_irq(pdev, 0);
    data->irq = irq;
    dev_set_drvdata(&pdev->dev, data);

    if (irq < 0) {
        dev_warn(&pdev->dev, "no valid irq (err=%d), skipping irq test\n", irq);
    } else {
        ret = devm_request_irq(&pdev->dev, irq, demo_irq_handler, 0, "platform-demo", NULL);
        if (ret)
            dev_warn(&pdev->dev, "request_irq(%d) failed (err=%d), skipping irq test\n", irq, ret);
    }

    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (!res) {
        dev_err(&pdev->dev, "no memory resource\n");
        return -ENODEV;
    }

    dev_info(&pdev->dev, "probe: mem start=0x%llx size=0x%llx\n",
             (u64)res->start, (u64)resource_size(res));

    data->base = devm_ioremap_resource(&pdev->dev, res);
    if (IS_ERR(data->base)) {
        dev_err(&pdev->dev, "devm_ioremap_resource failed: %ld\n", PTR_ERR(data->base));
        return PTR_ERR(data->base);
    }
    dev_info(&pdev->dev, "devm_ioremap_resource ok, virt addr=%p\n", data->base);

    return 0;
}


static void demo_remove(struct platform_device *pdev)
{
	struct demo_data *data = dev_get_drvdata(&pdev->dev);
    dev_info(&pdev->dev, "remove: irq was %d\n", data->irq);
    dev_info(&pdev->dev, "remove: bye\n");
}

static struct platform_driver demo_driver = {
	.probe	=  demo_probe,
	.remove	=  demo_remove,
	.driver =  {
		.name = "platform-demo",
        .of_match_table = demo_of_match,
	},
}; 


module_platform_driver(demo_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Edward");
MODULE_DESCRIPTION("Minimal platform driver demo");
MODULE_DEVICE_TABLE(of, demo_of_match);