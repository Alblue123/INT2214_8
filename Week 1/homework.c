#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

int homework_init(void) {
    printk(KERN_INFO "Hello from Kernel Module!\n");
    return 0;
}

void homework_exit(void) {
    printk(KERN_INFO "Goodbye from Kernel Module!\n");
}

module_init(homework_init);
module_exit(homework_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("A simple kernel module simulating a system call");
MODULE_AUTHOR("LamHoangHai");
