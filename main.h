#include <linux/fs.h>

static int device_open(struct inode *inode, struct file *file);
static int device_release(struct inode *inode, struct file *file);
static ssize_t device_read(	struct file *filp, char *buffer, size_t length, loff_t * offset);
static ssize_t device_write(struct file *filp, const char *buff, size_t len, loff_t * off);
