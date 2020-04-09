#include <linux/model.h>
#include <linux/kernel.h>
#include <linux/sched/signal.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/cred.h>

MODULE_LISCENSE("GPL");

asmlinkage/*prefix for system call routine*/ int (*orig_sys_open/*function pointer*/)(const char __user * filename, int flags, umode_t mode)/*argument type of the function*/ ; 

asmlinkage int mousehole_sys_open(const char __user * filename, int flags, umode_t mode)
{
	char fname[256] ; // kernel memory space
	const struct cred *cred = current_cred();
	
	uid_t uid = cred->uid;

	copy_from_user(fname, filename, 256) ; // bring filename from user level to kernel level
	uid = current_uid();
	printk(uid);

	return orig_sys_open(filename, flags, mode) ; // 
}


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

	unsigned int level;
	pte_t * pte;

	proc_create("mousehole", S_IRUGO|S_IWUGO, NULL, &mousehoel_fops) ;	

	sctable = (void *) kallsyms_lookup_name("sys_call_table") ;

	orig_sys_open = sctable[__NR_open] ;
	
	pte = look_address((unsigned long) sctable, &level);

	if (pte->pte &~ _PAGE_RW)
		pte->pte |=  _PAGE_RW;

	sctable[__NR_open] = mousehole_sys_open ; 
	pte = look_address((unsigned long) sctable, &level);
	pre->pte = pte->pte &~ _PAGE_RW ;
}

static 
void __exit mousehole_exit(void) { // restore original system call handler
	unsigned int level ;
	pte_t * pte ;
	remove_proc_entry("mousehole", NULL) ;

	sctable[__NR_open] = orig_sys_open ;
	pte = lookup_address((unsigned long) sctable, &level) ;
	pte->pte = pte->pte &~ _PAGE_RW ;
}

module_init(mousehole_init);
module_exit(mousehole_exit);