#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/moduleparam.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Edward");
MODULE_DESCRIPTION("Hello Kernel Module with module_param");

/*
 * module_param(name, type, perm)
 *   - name：變數名
 *   - type：int / charp(字串) / bool / short / long ...
 *   - perm：/sys/module/hello_param/parameters/ 下的檔案權限
 *           0 = 不在 sysfs 出現；0644 = root 可讀寫、其他人可讀
 *
 * 載入時覆寫：  insmod hello_param.ko param_value=100 name="World"
 * 不給就用這裡的預設值。
 */

static int param_value = 42;
module_param(param_value, int, 0644);
MODULE_PARM_DESC(param_value, "An integer parameter (default 42)");

static char *name = "Kernel";
module_param(name, charp, 0644);
MODULE_PARM_DESC(name, "A string parameter (default \"Kernel\")");

/* 陣列參數：count 會被填入實際傳入的元素個數 */
static int arr[4];
static int arr_count;
module_param_array(arr, int, &arr_count, 0644);
MODULE_PARM_DESC(arr, "An int array, e.g. arr=1,2,3");

static int __init hello_param_init(void)
{
    int i;

    printk(KERN_INFO "hello_param: loaded. name=%s, param_value=%d\n",
           name, param_value);

    for (i = 0; i < arr_count; i++)
        printk(KERN_INFO "hello_param: arr[%d] = %d\n", i, arr[i]);

    return 0;
}

static void __exit hello_param_exit(void)
{
    printk(KERN_INFO "hello_param: unloaded. final param_value=%d\n",
           param_value);
}

module_init(hello_param_init);
module_exit(hello_param_exit);
