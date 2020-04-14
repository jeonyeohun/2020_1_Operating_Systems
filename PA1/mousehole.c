#include <linux/syscalls.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/kallsyms.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/cred.h>
#include <linux/sched/signal.h>
#include <asm/unistd.h>
#include <linux/uidgid.h>
#include <linux/string.h>

MODULE_LICENSE("GPL");

char filepath[128] = { 0x0, } ; // indicate filename, given by user program
int uid;
void ** sctable ;
int command = 0;

asmlinkage int (*orig_sys_open)(const char __user * filename, int flags, umode_t mode) ; 
asmlinkage long  (*orig_sys_kill)(pid_t pid, int sig);

asmlinkage long  mousehole_sys_kill (pid_t pid, int sig){
	struct task_struct * t ;
	/* Do this only when /proc has '3' in command varialbe */
	if(command == 3){	
		/* traverse all process  */
		for_each_process(t){
			// if the selected process is currently working process and the owner is same as given uid, the process should not be killed */
			if(t->pid == pid && t->cred->uid.val == uid){ 
				printk("The process %d(pid) owned by %d(uid) is protected. mousehole deny the kill.", pid, uid);
				return -1; // return kill failure
			}
		}
	}
	return orig_sys_kill(pid, sig);
}

asmlinkage int mousehole_sys_open(const char __user * filename, int flags, umode_t mode)
{
	char fname[256] ; // kernel memory space
	/* Do this only when /proc has '2' in command varialbe */
	if (command == 2){
		/* bring filename which is trying to be openned now from user level to kernel level */
		copy_from_user(fname, filename, 256) ; 
		if (filepath[0] != 0x0 && strstr(fname, filepath) != NULL) {
			if(uid == (current->cred->uid.val)){	
				printk("uid %d tries to access the file %s. mousehole deny the access.", current->cred->uid.val, filename);	
				return -1; // return open failure
			}	
		}	
	}
	
	return orig_sys_open(filename, flags, mode) ; 
}


static 
int mousehole_proc_open(struct inode *inode, struct file *file) {
	return 0 ;
}

static 
int mousehole_proc_release(struct inode *inode, struct file *file) {
	return 0 ;
}

static
ssize_t mousehole_proc_read(struct file *file, char __user *ubuf, size_t size, loff_t *offset) 
{
	char buf[256] ;
	ssize_t toread ;

	if (command == 1){
		sprintf(buf, "Mousehole module is now ready for new protection.")
	}
	if (command == 2){
		sprintf(buf, "Mousehole module is currently protecting files contain \"%s\" in the filename from user with uid %d", filepath, uid) ;
	}
	if (command == 3){
		sprintf(buf, "Mousehole module is currently protecting all processes created by user with uid %d from kill system call", uid) ;
	}

	toread = strlen(buf) >= *offset + size ? size : strlen(buf) - *offset ;

	if (copy_to_user(ubuf, buf + *offset, toread))
		return -EFAULT ;	

	*offset = *offset + toread ;

	return toread ;
}

static 
ssize_t mousehole_proc_write(struct file *file, const char __user *ubuf, size_t size, loff_t *offset) 
{
	char buf[256] ;

	if (*offset != 0 || size > 128)
		return -EFAULT ;

	if (copy_from_user(buf, ubuf, size))
		return -EFAULT ;

	/* read written string from user buffer(jerry) */
	if (buf[0] == '1'){
		sscanf(buf, "%d", &command);
	}
	if (buf[0] == '2'){
		sscanf(buf,"%d %d %s", &command, &uid, filepath) ;
	}
	if (buf[0] == '3'){
		sscanf(buf,"%d %d", &command, &uid) ;
	}
	
	*offset = strlen(buf) ;

	return *offset ;
}

static const struct file_operations mousehole_fops = {
	.owner = 	THIS_MODULE,
	.open = 	mousehole_proc_open,
	.read = 	mousehole_proc_read,
	.write = 	mousehole_proc_write,
	.llseek = 	seq_lseek,
	.release = 	mousehole_proc_release,
} ;

static 
int __init mousehole_init(void) {
	unsigned int level ; 
	pte_t * pte ;

	proc_create("mousehole", S_IRUGO | S_IWUGO, NULL, &mousehole_fops) ;

	/* bring system call handler table */
	sctable = (void *) kallsyms_lookup_name("sys_call_table") ; 

	/* save the original system call routine */
	orig_sys_open = sctable[__NR_open] ; 
	orig_sys_kill = sctable[__NR_kill];

	pte = lookup_address((unsigned long) sctable, &level) ;

	/* sctable is read only so we need to change the authorization temporarily */
	if (pte->pte &~ _PAGE_RW) 
		pte->pte |= _PAGE_RW ;	

	/* change system call routine by defined function. */
	sctable[__NR_open] = mousehole_sys_open ; 
	sctable[__NR_kill] = mousehole_sys_kill ;

	return 0;
}

static 
void __exit mousehole_exit(void) { 
	unsigned int level ;
	pte_t * pte ;
	remove_proc_entry("mousehole", NULL) ;

	/* restore all the system call table to original */
	sctable[__NR_open] = orig_sys_open ;
	sctable[__NR_kill] = orig_sys_kill ;
	pte = lookup_address((unsigned long) sctable, &level) ;
	pte->pte = pte->pte &~ _PAGE_RW ;
}

module_init(mousehole_init);
module_exit(mousehole_exit);
