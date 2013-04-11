#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include "main.h"

/* Char devices files names. */
#define CALC_FIRST "calc_first"
#define CALC_SECOND "calc_second"
#define CALC_OPERATOR "calc_operator"
#define CALC_RESULT "calc_result"

/* Char devices maximum file size. */
#define FILE_MAX_SIZE 16

/* Char devices files names. */
static char names[][16] = {
        CALC_FIRST,
        CALC_SECOND,
        CALC_OPERATOR,
        CALC_RESULT
};

/* Device opened counter. */
static int device_opened = 0;

/* Char devices files buffers. */
static char** devices_buffer;

/* User message. */
static char message[32];

/* Device operations struct. */
static struct file_operations fops = {
        .owner = THIS_MODULE,
        .read = device_read,
        .write = device_write,
        .open = device_open,
        .release = device_release
};

/* Devices numbers. */
static dev_t numbers[4];

/* Global var for the character device struct */
static struct cdev* c_dev;

/* Global var for the device class */
static struct class** classes;

static int device_open(struct inode *inode, struct file *file)
{
        if (device_opened)
                return -EBUSY;

        device_opened++;
        try_module_get(THIS_MODULE);

        return 0;
}

static int device_release(struct inode *inode, struct file *file)
{
        device_opened--;

        module_put(THIS_MODULE);

        return 0;
}

static ssize_t device_read( struct file *filp, char *buffer, size_t length, loff_t * offset)
{
        static int fin = 0;
        long a, b, result = 0;
        char name[32], op = 0, *end;
        int written = 0;
        int i = 0;

        if (fin) {
                fin = 0;
                return 0;
        }

        strcpy(name, filp->f_dentry->d_name.name);

        if (strcmp(name, CALC_FIRST) == 0) {
                sprintf(message, "\n%s\n", devices_buffer[0]);
        } else if (strcmp(name, CALC_SECOND) == 0) {
                sprintf(message, "\n%s\n", devices_buffer[1]);
        } else if (strcmp(name, CALC_OPERATOR) == 0) {
                sprintf(message, "\n%s\n", devices_buffer[2]);
        } else {
                a = simple_strtol(devices_buffer[0], &end, 10);
                if (a == 0 && (*end == devices_buffer[0][0])) {
                        sprintf(message, "\n%s\n", "First operand is not integer.");
                } else {
                        b = simple_strtol(devices_buffer[1], &end, 10);
                        if (b == 0 && (*end == devices_buffer[1][0])) {
                                sprintf(message, "\n%s\n", "Second operand is not integer.");
                        } else {
                                op = devices_buffer[2][0];
                                switch (op) {
                                        case '+': result = a + b; break;
                                        case '-': result = a - b; break;
                                        case '*': result = a * b; break;
                                        case '/':
                                                if (b == 0) {
                                                        sprintf(message, "\n%s\n",
                                                                "Division by zero!");
                                                        written = 1;
                                                        break;
                                                }
                                                result = a / b;
                                                break;
                                        default:
                                                sprintf(message, "\nUnknown operand: %c\n", op);
                                                written = 1;
                                                break;
                                }
                        }
                        if (!written) {
                                sprintf(message, "\n%ld %c %ld = %ld\n", a, op, b, result);
                        }
                }
        }

        for(i = 0; i < length && message[i]; i++) {
                put_user(message[i], buffer + i);
        }

        fin = 1;

        return i;
}

static ssize_t device_write(struct file *filp, const char *buff, size_t len, loff_t * off)
{
        static int fin = 0;
        char name[32];
        int buf_size = 0;
        int index = 0;

        if (fin) {
                fin = 0;
                return 0;
        }

        strcpy(name, filp->f_dentry->d_name.name);

        if (strcmp(name, CALC_FIRST) == 0) {
                index = 0;
        } else if (strcmp(name, CALC_SECOND) == 0) {
                index = 1;
        } else if (strcmp(name, CALC_OPERATOR) == 0) {
                index = 2;
        } else {
                index = 3;
        }

        if (len >= FILE_MAX_SIZE) {
                buf_size = FILE_MAX_SIZE - 1;
        } else {
                buf_size = len;
        }

        if (index != 3) {
                if (copy_from_user(devices_buffer[index], buff, buf_size)) {
                                printk(KERN_ERR "Failed to write to /dev/%s\n", names[index]);
                                return -EFAULT;
                }
                devices_buffer[index][buf_size + 1] = '\0';
        }

        fin = 1;

        return buf_size;
}

/* Module init function */
static int __init calc_init(void)
{
        int i = 0;

        printk(KERN_INFO "Calc driver was loaded.\n");

        classes = (struct class**) kmalloc(sizeof(struct class*) * 4, GFP_KERNEL);
        c_dev = (struct cdev*) kmalloc(sizeof(struct cdev) * 4, GFP_KERNEL);
        devices_buffer = (char**) kmalloc(sizeof(char*) * 4, GFP_KERNEL);

        for (i = 0; i < 4; i++) {
                devices_buffer[i] = (char*) kmalloc(sizeof(char) * FILE_MAX_SIZE, GFP_KERNEL);
                devices_buffer[i][0] = '\0';
        }

        for (i = 0; i < 4; i++) {
                if (alloc_chrdev_region(&numbers[i], 0, 1, names[i]) < 0) {
                        return -1;
                }

                if ((classes[i] = class_create(THIS_MODULE, names[i])) == NULL) {
                        unregister_chrdev_region(numbers[i], 1);
                        return -1;
                }

                if (device_create(classes[i], NULL, numbers[i], NULL, names[i]) == NULL) {
                        class_destroy(classes[i]);
                        unregister_chrdev_region(numbers[i], 1);
                        return -1;
                }

                cdev_init(&c_dev[i], &fops);

                if (cdev_add(&c_dev[i], numbers[i], 1) == -1) {
                        device_destroy(classes[i], numbers[i]);
                        class_destroy(classes[i]);
                        unregister_chrdev_region(numbers[i], 1);
                        return -1;
                }
        }

        printk(KERN_INFO "Calc driver devices were created.\n");
        return 0;
}

/* Module exit function */
static void __exit calc_exit(void)
{
        int i;

        for (i = 0; i < 4; i++) {
                cdev_del(&c_dev[i]);
                device_destroy(classes[i], numbers[i]);
                class_destroy(classes[i]);
                unregister_chrdev_region(numbers[i], 1);
                kfree(devices_buffer[i]);
        }

        kfree(devices_buffer);

        printk(KERN_INFO "Calc driver devices were removed.\n");
        printk(KERN_INFO "Calc driver was unloaded.\n");
}

MODULE_AUTHOR("Roma & Vanya");
MODULE_DESCRIPTION("Kernel mode calculator");
MODULE_LICENSE("GPL");

module_init(calc_init); /* Register module entry point */
module_exit(calc_exit); /* Register module cleaning up */