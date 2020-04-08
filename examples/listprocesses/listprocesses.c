#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

MODULE_LICENSE("GPL");


char * text = 0x0 ;
  
static 
int listps_open(struct inode *inode, struct file *file) { //open list of processes in text file.

// text data generation
	char buf[256] ;

	struct task_struct * t ; // to iterate list of task struct, it is like a PCB. single task is in task struct
	int idx = 0 ;

	text = kmalloc(2048, GFP_KERNEL) ; // allocation in kilobyte.
	text[0] = 0x0 ;

	for_each_process(t) { // iterate processes with given pointer.
		sprintf(buf, "%s : %d\n", t->comm, t->pid) ; // comm: command text, pid: pid
		
		printk(KERN_INFO "%s", buf) ; // we can check this in system log

		idx += strlen(buf) ;
		if (idx < 2048) // we have limited size of text
			strcat(text, buf) ; // concatenate text with new entry of process
		else
			break ; // exception
	}
	return 0 ;
}

static 
int listps_release(struct inode *inode, struct file *file) {
	return 0 ;
}

static
ssize_t listps_read(struct file *file, char __user *ubuf, size_t size, loff_t *offset) 
{// bring data with given size 
	ssize_t toread ;
	if (strlen(text) >= *offset + size) {
		toread = size ;
	}
	else {
		toread = strlen(text) - *offset ;
	}

	if (copy_to_user(ubuf, text + *offset, toread)) // copy kernel data to user space buffer
		return -EFAULT ;	

	*offset = *offset + toread ;

	return toread ;
}

static 
ssize_t listps_write(struct file *file, const char __user *ubuf, size_t size, loff_t *offset) 
{
	return 0 ;
}

static const struct file_operations listps_fops = {
	.owner = 	THIS_MODULE,
	.open = 	listps_open,
	.read = 	listps_read,
	.write = 	listps_write,
	.llseek = 	seq_lseek,
	.release = 	listps_release,
} ;

static 
int __init listps_init(void) {
	proc_create("listprocesses", S_IRUGO | S_IWUGO, NULL, &listps_fops) ;
	return 0;
}

static 
void __exit listps_exit(void) {
	remove_proc_entry("listprocesses", NULL) ;
}

module_init(listps_init);
module_exit(listps_exit);
