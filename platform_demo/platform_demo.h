#ifndef PLATFORM_DEMO_H
#define PLATFORM_DEMO_H

#define DEMO_MEM_START	0x10000000
#define DEMO_MEM_SIZE	0x1000
#define DEMO_IRQ	80

struct demo_data {
    int irq;
    void __iomem *base;
};

#endif
