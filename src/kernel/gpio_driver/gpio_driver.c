#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/gpio/consumer.h>

#define DEVICE_NAME "gpio_driver"
#define CLASS_NAME  "gpio_class"

/*
 From your /sys/kernel/debug/gpio output:
 GPIO24 = gpio-536
*/
#define KERNEL_GPIO_NUM 536

static int majorNumber;
static struct class *gpioClass  = NULL;
static struct device *gpioDevice = NULL;
static struct gpio_desc *led_gpio;

/* Write from user space */
static ssize_t dev_write(struct file *filep,
                         const char *buffer,
                         size_t len,
                         loff_t *offset)
{
    char value;

    if (copy_from_user(&value, buffer, 1))
        return -EFAULT;

    if (value == '1') {
        gpiod_set_value(led_gpio, 1);
        pr_info("GPIO ON\n");
    } 
    else if (value == '0') {
        gpiod_set_value(led_gpio, 0);
        pr_info("GPIO OFF\n");
    }

    return len;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .write = dev_write,
};

/* Module Init */
static int __init gpio_driver_init(void)
{
    pr_info("GPIO Driver Init (FINAL HYBRID gpiod)\n");

    majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
    if (majorNumber < 0) {
        pr_err("Failed to register major number\n");
        return majorNumber;
    }

    gpioClass = class_create(CLASS_NAME);
    if (IS_ERR(gpioClass)) {
        unregister_chrdev(majorNumber, DEVICE_NAME);
        return PTR_ERR(gpioClass);
    }

    gpioDevice = device_create(gpioClass, NULL,
                               MKDEV(majorNumber, 0),
                               NULL, DEVICE_NAME);
    if (IS_ERR(gpioDevice)) {
        class_destroy(gpioClass);
        unregister_chrdev(majorNumber, DEVICE_NAME);
        return PTR_ERR(gpioDevice);
    }

    /* ✅ Convert kernel GPIO number to descriptor */
    led_gpio = gpio_to_desc(KERNEL_GPIO_NUM);
    if (!led_gpio) {
        pr_err("gpio_to_desc() failed\n");
        device_destroy(gpioClass, MKDEV(majorNumber, 0));
        class_destroy(gpioClass);
        unregister_chrdev(majorNumber, DEVICE_NAME);
        return -EINVAL;
    }

    gpiod_direction_output(led_gpio, 0);

    pr_info("GPIO driver loaded successfully (FINAL HYBRID gpiod)\n");
    return 0;
}

/* Module Exit */
static void __exit gpio_driver_exit(void)
{
    gpiod_set_value(led_gpio, 0);

    device_destroy(gpioClass, MKDEV(majorNumber, 0));
    class_destroy(gpioClass);
    unregister_chrdev(majorNumber, DEVICE_NAME);

    pr_info("GPIO driver removed\n");
}

module_init(gpio_driver_init);
module_exit(gpio_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Subramanyam Upadrasta");
MODULE_DESCRIPTION("Final Hybrid gpiod GPIO Character Driver (Linux 6.12+)");
