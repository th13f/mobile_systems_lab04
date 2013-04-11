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
}

/* Module exit function */
static void __exit calc_exit(void)
{
}

MODULE_AUTHOR("Roma & Vanya");
MODULE_DESCRIPTION("Kernel mode calculator");
MODULE_LICENSE("GPL");

module_init(calc_init); /* Register module entry point */
module_exit(calc_exit); /* Register module cleaning up */