#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>

MODULE_LICENSE("GPL");

static 
char hello_name[128] = { 0x0, } ; // static array. part of module

static 
int hello_open(struct inode *inode, struct file *file) { // defined by struct
	return 0 ; // success
}

static 
int hello_release(struct inode *inode, struct file *file) {
	return 0 ;
}

static
ssize_t hello_read(struct file *file, char __user *ubuf/*character buffer in user space*/, size_t size/*MAX size of buffer*/, loff_t *offset/*cursor point*/) 
{
	char buf[256] ;
	ssize_t toread ;

	sprintf(buf/*direction*/, "Hello %s from kernel!\n", hello_name) ;

	if (strlen(buf) >= *offset + size/*get offset size and data size to see if it is possible to put data*/) {
		toread = size ;
	}
	else { /*if the length of data is overflowed, cut of the data and read only a certain amount*/
		toread = strlen(buf) - *offset ;
	}

	if (copy_to_user(ubuf, buf + *offset, toread))
		return -EFAULT ;	

	*offset = *offset + toread ; /*after read data, increase the offset*/

	return toread ;
}

static 
ssize_t hello_write(struct file *file, const char __user *ubuf/*data is in here*/, size_t size, loff_t *offset) 
{
	char buf[128] ;

	if (*offset != 0 || size > 128)
		return -EFAULT ;

	if (copy_from_user(buf, ubuf, size)) /*copy ubuf data to buf*/
		return -EFAULT ;

	sscanf(buf,"%128s", hello_name) ; /*read data from buffer and give it to hello_name*/
	*offset = strlen(buf) ;

	return *offset ;
}

static const struct file_operations hello_fops = {
	.owner = 	THIS_MODULE,
	.open = 	hello_open, //function pointer
	.read = 	hello_read,
	.write = 	hello_write,
	.llseek = 	seq_lseek, // file cursor
	.release = 	hello_release,
} ;

static 
int __init hello_init(void) {
	// create file in proc. name hellokernelworld. using hello fops
	// resides in kernel space and hellokenelworld is put as agent to use api function
	proc_create("hellokernelworld", S_IRUGO | S_IWUGO, NULL, &hello_fops) ; // create virtual file with name hellpkernelworld
	return 0;
}

static 
void __exit hello_exit(void) {
	remove_proc_entry("hellokernelworld", NULL) ;
}

module_init(hello_init);
module_exit(hello_exit);
