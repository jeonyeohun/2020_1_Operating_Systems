#include <linux/model.h>
#include <linux/kernel.h>
#include <linux/sched/signal.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

MODULE_LISCENSE("GPL");

static mousehole_read(struct file * file, const char __user * ubuf, size_t size, loft_t *offset){
	return 0;
}


static
ssize_t mousehole_write(struct file *file, const char __user * ubuf, size_t size, loft_t *offset){
	return 0;
}

static const struct file_operations mousehole_fops = {
	.owner = THIS_MODULE,
	.open = mousehole_open,
	.read = mousehole_read,
	.write = mousehole_write,
	.release = mousehole_release,
};

static 
int __init mousehole_init(void){
	
	proc_create("mousehole", S_IRUGO|S_IWUGO, NULL, &mousehoel_fops) ;	
}
