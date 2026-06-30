#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/io.h>
#include "platform_demo.h"

static const struct of_device_id demo_of_match[] = {
    { .compatible = "myvendor,platform-demo" },
    {}
};
MODULE_DEVICE_TABLE(of, demo_of_match);

static int demo_probe(struct platform_device *pdev)
{
    struct resource *res;
    void __iomem *base;
    int irq;

    /* 取記憶體資源 */
    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (!res) {
        dev_err(&pdev->dev, "no memory resource\n");
        return -ENODEV;
    }

    /* 映射到虛擬位址 */
    base = devm_ioremap_resource(&pdev->dev, res);
    if (IS_ERR(base))
        return PTR_ERR(base);

    /* 取中斷號碼 */
    irq = platform_get_irq(pdev, 0);
    if (irq < 0)
        return irq;

    dev_info(&pdev->dev, "probe: base=%p, irq=%d\n", base, irq);
    return 0;
}

static int demo_remove(struct platform_device *pdev)
{
    dev_info(&pdev->dev, "remove: bye\n");
    return 0;
}

static struct platform_driver demo_driver = {
    .probe  = demo_probe,
    .remove = demo_remove,
    .driver = {
        .name           = "platform-demo",
        .of_match_table = demo_of_match,
    },
};

module_platform_driver(demo_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Edward");
MODULE_DESCRIPTION("Minimal platform driver demo");
