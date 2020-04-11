#include <linux/syscalls.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/kallsyms.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/cred.h>
#include <asm/unistd.h>
#include <linux/uidgid.h>
#include <linux/string.h>

MODULE_LICENSE("GPL");

char filepath[128] = { 0x0, } ; // indicate filename, given by user program
char usrid[128] = { 0x0, } ; // indicate filename, given by user program
void ** sctable ;
int command = 0;

asmlinkage int (*orig_sys_open)(const char __user * filename, int flags, umode_t mode) ; 

asmlinkage int openhook_sys_open(const char __user * filename, int flags, umode_t mode)
{
	char fname[256] ; // kernel memory space
	if (command == 2){
		printk("open-block is running....") ;
		copy_from_user(fname, filename, 256) ; // bring filename which is trying to be openned now from user level to kernel level

		if (filepath[0] != 0x0 && strstr(fname, filepath) != NULL) {
			if(simple_strtol(usrid, NULL, 10) == (current->cred->uid.val)){	
				printk("uid %d tries to access the file %s", current->cred->uid.val, fname);	
				return -1; // return open failure
			}	
		}	
	}
	
	return orig_sys_open(filename, flags, mode) ; // open file normally
}


static 
int openhook_proc_open(struct inode *inode, struct file *file) {
	return 0 ;
}

static 
int openhook_proc_release(struct inode *inode, struct file *file) {
	return 0 ;
}

static
ssize_t openhook_proc_read(struct file *file, char __user *ubuf, size_t size, loff_t *offset) 
{
	char buf[256] ;
	ssize_t toread ;


	toread = strlen(buf) >= *offset + size ? size : strlen(buf) - *offset ;

	if (copy_to_user(ubuf, buf + *offset, toread))
		return -EFAULT ;	

	*offset = *offset + toread ;

	return toread ;
}

static 
ssize_t openhook_proc_write(struct file *file, const char __user *ubuf, size_t size, loff_t *offset) 
{
	char buf[256] ;

	if (*offset != 0 || size > 128)
		return -EFAULT ;

	if (copy_from_user(buf, ubuf, size))
		return -EFAULT ;

	sscanf(buf,"%d %d %s", &command, filepath, usrid) ;
	printk("%d %d %s\n", command, filepath, usrid);
	
	*offset = strlen(buf) ;

	return *offset ;
}

static const struct file_operations openhook_fops = {
	.owner = 	THIS_MODULE,
	.open = 	openhook_proc_open,
	.read = 	openhook_proc_read,
	.write = 	openhook_proc_write,
	.llseek = 	seq_lseek,
	.release = 	openhook_proc_release,
} ;

static 
int __init openhook_init(void) {
	unsigned int level ; 
	pte_t * pte ;

	proc_create("openhook", S_IRUGO | S_IWUGO, NULL, &openhook_fops) ;

	sctable = (void *) kallsyms_lookup_name("sys_call_table") ; // bring system call handler table

	orig_sys_open = sctable[__NR_open] ; // the index of system call routine given by linux kernel(/include/linux/syscalls.h)

	pte = lookup_address((unsigned long) sctable, &level) ;
	/*sctable is read only so we need to change the authorization temporarily*/
	if (pte->pte &~ _PAGE_RW) 
		pte->pte |= _PAGE_RW ;	

	sctable[__NR_open] = openhook_sys_open ; // change system call routine by defined function.

	return 0;
}

static 
void __exit openhook_exit(void) { // restore original system call handler
	unsigned int level ;
	pte_t * pte ;
	remove_proc_entry("openhook", NULL) ;

	sctable[__NR_open] = orig_sys_open ;
	pte = lookup_address((unsigned long) sctable, &level) ;
	pte->pte = pte->pte &~ _PAGE_RW ;
}

module_init(openhook_init);
module_exit(openhook_exit);
