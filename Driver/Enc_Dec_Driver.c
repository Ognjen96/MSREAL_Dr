#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>	
#include <linux/types.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/kdev_t.h>
#include <linux/uaccess.h>
#include <linux/of.h>

#include <linux/mm.h>//za memorijsko mapiranje
#include <linux/io.h>//iowrite ioread
#include <linux/slab.h>//kmalloc kfree
#include <linux/platform_device.h>//platform driver
#include <linux/of.h>//of match table
#include <linux/ioport.h>//ioremap

MODULE_LICENSE("Dual BSD/GPL");

dev_t my_dev_id;
static struct class *my_class;
static struct device *my_device;
static struct cdev *my_cdev;


struct ED_info {
	unsigned long mem_start;
	unsigned long mem_end;
	void __iomem *base_addr;
};

static struct ED_info *ip = NULL;//Enc_dec
static struct ED_info *bp1 = NULL;//bram0
static struct ED_info *bp2 = NULL;//bram1

#define BUFF_SIZE 30
#define number_of_minors 3
#define DRIVER_NAME "ED_driver"
#define DEVICE_NAME "ED"

int position = 0;
int endRead = 0;
int counter = 0;
int k=0;

static int ED_probe(struct platform_device *pdev);
static int ED_remove(struct platform_device *pdev);
int ED_open(struct inode *pinode, struct file *pfile);
int ED_close(struct inode *pinode, struct file *pfile);
static ssize_t ED_read(struct file *pfile, char __user *buffer, size_t length, loff_t *offset);
static ssize_t ED_write(struct file *pfile, const char __user *buffer, size_t length, loff_t *offset);
int ED_mmap (struct file *f, struct vm_area_struct *vma_s);

static int __init ED_init(void);
static void __exit ED_exit(void);

struct file_operations my_fops =
{
	.owner = THIS_MODULE,
	.open = ED_open,
	.read = ED_read,
	.write = ED_write,
	.release = ED_close,
	.mmap = ED_mmap,
};

static struct of_device_id ED_of_match[] = {

	{ .compatible = "bram0", },
	{ .compatible = "bram1", },
	{ .compatible = "Enc_dec" },
	{ /* end of list */ }
};

MODULE_DEVICE_TABLE(of, ED_of_match);

static struct platform_driver ED_driver = {

	.driver = {
		.name = DRIVER_NAME,
		.owner = THIS_MODULE,
		.of_match_table = ED_of_match,
		},

	.probe = ED_probe,
	.remove = ED_remove,
};


static int ED_probe (struct platform_device *pdev)
{
	struct resource *r_mem;
	int rc = 0;
	r_mem=platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if(!r_mem){
			printk(KERN_ALERT "Failed to get resource\n");
			return -ENODEV;
	}
	
	switch(counter){

	case 0://za bram0
		
		bp1 = (struct ED_info *) kmalloc(sizeof(struct ED_info), GFP_KERNEL);
		if(!bp1){
			printk(KERN_ALERT "Could not allocate memory\n");
			return -ENOMEM;
		}
		
		bp1->mem_start= r_mem->start;
		bp1->mem_end = r_mem->end;
		printk(KERN_INFO "start address: %x \t end address: %x", r_mem->start, r_mem->end);

		if(!request_mem_region(bp1->mem_start, bp1->mem_end - bp1->mem_start+1, DRIVER_NAME)){
			printk(KERN_ALERT "Could not lock memory region at %p\n", (void *) bp1->mem_start);
			rc = -EBUSY;
			goto error3;
		}
		
		bp1->base_addr = ioremap (bp1->mem_start, bp1->mem_end - bp1->mem_start+1);

		if(!bp1->base_addr){
			printk(KERN_ALERT "Could not allocate memory\n");
			rc = -EIO;
			goto error4;
		}

		counter ++;
		printk(KERN_WARNING "bram0 registered\n");
		return 0;//Sve sljaka

		error4:
			release_mem_region(bp1->mem_start, bp1->mem_end - bp1->mem_start+1);
		error3:
			return rc;

	case 1://za bram1

		bp2 = (struct ED_info *) kmalloc(sizeof(struct ED_info), GFP_KERNEL);
		if(!bp2){
			printk(KERN_ALERT "Could not allocate memory\n");
			return -ENOMEM;
		}
		
		bp2->mem_start= r_mem->start;
		bp2->mem_end = r_mem->end;
		printk(KERN_INFO "start address: %x \t end address: %x", r_mem->start, r_mem->end);

		if(!request_mem_region(bp2->mem_start, bp2->mem_end - bp2->mem_start+1, DRIVER_NAME)){
			printk(KERN_ALERT "Could not lock memory region at %p\n", (void *) bp2->mem_start);
			rc = -EBUSY;
			goto error6;
		}
		
		bp2->base_addr = ioremap (bp2->mem_start, bp2->mem_end - bp2->mem_start+1);

		if(!bp2->base_addr){
			printk(KERN_ALERT "Could not allocate memory\n");
			rc = -EIO;
			goto error5;
		}

		counter ++;
		printk(KERN_WARNING "bram1 registered\n");
		return 0;//Sve sljaka

		error5:
			release_mem_region(bp2->mem_start, bp2->mem_end - bp2->mem_start+1);
		error6:
			return rc;

	case 2://sad malo i za ip xD

		ip = (struct ED_info *) kmalloc(sizeof(struct ED_info), GFP_KERNEL);
		if(!ip){
			printk(KERN_ALERT "Could not allocate memory\n");
			return -ENOMEM;
		}
		
		ip->mem_start= r_mem->start;
		ip->mem_end = r_mem->end;
		printk(KERN_INFO "start address: %x \t end address: %x", r_mem->start, r_mem->end);

		if(!request_mem_region(ip->mem_start, ip->mem_end - ip->mem_start+1, DRIVER_NAME)){
			printk(KERN_ALERT "Could not lock memory region at %p\n", (void *) ip->mem_start);
			rc = -EBUSY;
			goto error1;
		}
		
		ip->base_addr = ioremap (ip->mem_start, ip->mem_end - ip->mem_start+1);

		if(!ip->base_addr){
			printk(KERN_ALERT "Could not allocate memory\n");
			rc = -EIO;
			goto error2;
		}

		printk(KERN_WARNING "IP registered\n");
		return 0;//Sve sljaka

		error2:
			release_mem_region(ip->mem_start, ip->mem_end - ip->mem_start+1);
		error1:
			return rc;
	}
	return 0;
}

static int ED_remove (struct platform_device *pdev)
{
		
	switch(counter){

	case 0://za bram0
		printk(KERN_WARNING "bram0_remove: platform driver removing\n");
		iowrite32(0,bp1->base_addr);
		iounmap(bp1->base_addr);
		release_mem_region(bp1->mem_start,(bp1->mem_end - bp1->mem_start+1));
		kfree(bp1);
		printk(KERN_INFO"bram0_remove: bram0 removed\n");

		break;

	case 1://za bram1
		
		printk(KERN_WARNING "bram1_remove: platform driver removing\n");
		iowrite32(0,bp2->base_addr);
		iounmap(bp2->base_addr);
		release_mem_region(bp2->mem_start,(bp2->mem_end - bp2->mem_start+1));
		kfree(bp2);
		printk(KERN_INFO"bram1_remove: bram1 removed\n");
		counter--;

	break;

	case 2://za ip
		
		printk(KERN_WARNING "ip_remove: platform driver removing\n");
		iowrite32(0,ip->base_addr);
		iounmap(ip->base_addr);
		release_mem_region(ip->mem_start,(ip->mem_end - ip->mem_start+1));
		kfree(ip);
		printk(KERN_INFO"ip_remove: ip removed\n");
		counter--;

	break;

	}
	return 0;
}

int ED_open(struct inode *pinode, struct file *pfile) 
{
		printk(KERN_INFO "Succesfully opened file\n");
		return 0;
}

int ED_close(struct inode *pinode, struct file *pfile) 
{
		printk(KERN_INFO "Succesfully closed file\n");
		return 0;
}

ssize_t ED_read(struct file *pfile, char __user *buffer, size_t length, loff_t *offset) 
{
	int ret, pos=0;
	char buff[BUFF_SIZE];
	int len, value;
	int minor = MINOR(pfile->f_inode->i_rdev);
	if (endRead == 1)
	{
		endRead=0;
		return 0;
	}
	switch(minor){
	
		case 0://bram0
		
			pos = position + k*4;
			value = ioread32(bp1->base_addr + pos);
			len = scnprintf(buff, BUFF_SIZE, "%d\n", value);
			*offset += len;
			ret = copy_to_user(buffer, buff, len);
			if(ret){
				return -EFAULT;
			}
			k++;
			if(k == 2048)
			{
				endRead=1;
				k = 0;
			}
			break;

		case 1://bram1
		
			pos = position + k*4;
			value = ioread32(bp2->base_addr + pos);
			len = scnprintf(buff, BUFF_SIZE, "%d\n", value);
			*offset += len;
			ret = copy_to_user(buffer, buff, len);
			if(ret){
				return -EFAULT;
			}
			k++;
			if(k == 2048)
			{
				endRead=1;
				k = 0;
			}
			break;

		case 2://ip
		
			value = ioread32(ip->base_addr+k*4);
			len = scnprintf(buff, BUFF_SIZE, "%d\n", value);
			*offset += len;
			ret=copy_to_user(buffer, buff, len);
			if(ret)
			{
				return -EFAULT;
			}
			k++;
			if(k == 9)
			{
				endRead = 1;
				k = 0;
			}

			break;
	
		default:
			printk(KERN_INFO"Something went wrong\n");
	}

	return len;
	

}


ssize_t ED_write(struct file *pfile, const char __user *buffer, size_t length, loff_t *offset)
{
	char buff[BUFF_SIZE];
	int minor = MINOR(pfile->f_inode->i_rdev);
	int ret = 0, i = 0, pos = 0;
	unsigned int xpos=0;
	unsigned int value=0;
	int vrednost = 0;
	char string[40];
	int case_vr = 0;
	char start1[] = "start", e_key1[]="e_key", reset1[]="reset", ready1[]="ready", public_key1[]="public_key", private_key1[]="private_key", start_enc1[]="start_enc", start_dec1[]="start_dec", txt_length1[]="txt_length";
//	int start, reset, ready, start_enc, start_dec, txt_length, public_key, private_key, e_key;
	ret = copy_from_user(buff, buffer, length);
	
	
	if(ret){
		printk("Copy from user failed \n");
		return -EFAULT;
	}
	buff[length] = '\0';

	switch(minor){
		
		case 0://bram0

			sscanf(buff, "(%d); %d", &xpos, &value);

			if (xpos > 2047)
			{
				printk(KERN_WARNING"bram0: position exceeded maximum value \n");
			}
			else
			{
				position = xpos*4;
				pos = position +i*4;
				iowrite32(value, bp1->base_addr+pos);
			}

		break;

		case 1://bram1

			sscanf(buff, "(%d); %d", &xpos, &value);

			if (xpos > 2047)
			{
				printk(KERN_WARNING"bram1: position exceeded maximum value \n");
			}
			else
			{
				position = xpos*4;
				pos = position +i*4;
				iowrite32(value, bp2->base_addr+pos);
			}

		break;

		case 2://enc_dec
		
			
				
			sscanf(buff, "%s = %d", string, &vrednost);
			
			if(strcmp(string, e_key1)==0){
				case_vr = 1;
			}else if(strcmp(string, private_key1)==0){
				case_vr = 2;
			}else if(strcmp(string, public_key1)==0){
				case_vr = 3;
			}else if(strcmp(string, txt_length1)==0){
				case_vr = 4;
			}else if(strcmp(string, start_enc1)==0){
				case_vr = 5;
			}else if(strcmp(string, start_dec1)==0){
				case_vr = 6;
			}else if(strcmp(string, start1)==0){
				case_vr = 7;
			}else if(strcmp(string, reset1)==0){
				case_vr = 8;
			}else if(strcmp(string, ready1)==0){
				case_vr = 9;
			}else{
				printk(KERN_ALERT"Ne postoji registar");
			}

		
			switch(case_vr){


				case 1:
				
							iowrite32(vrednost, ip->base_addr);
							printk(KERN_WARNING"Vrednost e_key signala je: %d\n", vrednost);
						
				break;

				case 3:
					
			
							iowrite32(vrednost, ip->base_addr+8);
							printk(KERN_WARNING"Vrednost public_key signala je: %d\n", vrednost);
					
				break;

			
				case 2:
			
		
							iowrite32(vrednost, ip->base_addr+4);
							printk(KERN_WARNING"Vrednost private_key signala je: %d\n", vrednost);
					
				break;

				case 4:
						
						if(vrednost>2047){
							printk(KERN_WARNING"IP:Duzina teksta prevelika\n");
						}else{
							iowrite32(vrednost, ip->base_addr+12);
							printk(KERN_WARNING"Vrednost txt_length signala je: %d\n", vrednost);
					
				}break;


				case 5:
		
						if(vrednost!=0 && vrednost!=1){
							printk(KERN_WARNING"IP:start_enc mora biti 1 ili 0\n");
						}else{
							iowrite32(vrednost, ip->base_addr+16);
							printk(KERN_WARNING"Vrednost start_enc signala je: %d\n", vrednost);
					
				}break;
	

				case 6:

						if(vrednost!=0 && vrednost!=1){
							printk(KERN_WARNING"IP: start_dec mora biti 1 ili 0\n");
						}else{
							iowrite32(vrednost, ip->base_addr+20);
							printk(KERN_WARNING"Vrednost start_dec signala je: %d\n", vrednost);
					
				}break;
				

				case 9:
						if(vrednost!=0 && vrednost!=1){
							printk(KERN_WARNING"IP:ready mora biti 1 ili 0\n");
						}else{
							iowrite32(vrednost, ip->base_addr+32);
							printk(KERN_WARNING"Vrednost ready signala je: %d\n", vrednost);
					
				}break;


				case 8: 

	
						if(vrednost!=0 && vrednost!=1){
							printk(KERN_WARNING"IP:reset mora biti 1 ili 0\n");
						}else{
							iowrite32(vrednost, ip->base_addr+28);
							printk(KERN_WARNING"Vrednost reset signala je: %d\n", vrednost);
					
				}break;


				case 7:
			
		
						if(vrednost!=0 && vrednost!=1){
							printk(KERN_WARNING"IP:start mora biti 1 ili 0\n");
						}else{
							iowrite32(vrednost, ip->base_addr+24);
							printk(KERN_WARNING"Vrednost start signala je: %d\n", vrednost);
					
				}break;
			}
		break;

		default:
			printk(KERN_INFO"Something went wrong\n");
	}

	return length;
	
				
}  

int ED_mmap(struct file *f, struct vm_area_struct *vma_s){
	
	int ret = 0;
	int minor = MINOR(f->f_inode->i_rdev);
	unsigned long vsize;
	unsigned long psize;
	switch(minor){
	
	case 0:
	
		vsize = vma_s->vm_end - vma_s->vm_start; //velicina addr prostora koji zahteva aplikacija
		psize = bp1->mem_end - bp1->mem_start+1; //velicina addr prostora koji zauzima jezgro
		vma_s->vm_page_prot = pgprot_noncached(vma_s-> vm_page_prot);
		printk(KERN_INFO "bram0:buffer is being memory mapped\n");

		if(vsize>psize)
		{
			printk(KERN_ERR"bram0: Trying to mmap more space than it's allocated, mmap failed\n");
			return -EIO;
		}
		ret = vm_iomap_memory(vma_s, bp1->mem_start, vsize);
		if(ret)
		{
			printk(KERN_ERR"bram0:memory maped failed\n");
			return ret;
		}
		printk(KERN_INFO"MMAP is a success for bram0\n");

		break;


	case 1:
	
		vsize = vma_s->vm_end - vma_s->vm_start; //velicina addr prostora koji zahteva aplikacija
		psize = bp2->mem_end - bp2->mem_start+1; //velicina addr prostora koji zauzima jezgro
		vma_s->vm_page_prot = pgprot_noncached(vma_s-> vm_page_prot);
		printk(KERN_INFO "bram0:buffer is being memory mapped\n");

		if(vsize>psize)
		{
			printk(KERN_ERR"bram1: Trying to mmap more space than it's allocated, mmap failed\n");
			return -EIO;
		}
		ret = vm_iomap_memory(vma_s, bp2->mem_start, vsize);
		if(ret)
		{
			printk(KERN_ERR"bram1:memory maped failed\n");
			return ret;
		}
		printk(KERN_INFO"MMAP is a success for bram1\n");

		break;


	case 2:
	
		vsize = vma_s->vm_end - vma_s->vm_start; //velicina addr prostora koji zahteva aplikacija
		psize = ip->mem_end - ip->mem_start+1; //velicina addr prostora koji zauzima jezgro
		vma_s->vm_page_prot = pgprot_noncached(vma_s-> vm_page_prot);
		printk(KERN_INFO "IP:buffer is being memory mapped\n");

		if(vsize>psize)
		{
			printk(KERN_ERR"IP: Trying to mmap more space than it's allocated, mmap failed\n");
			return -EIO;
		}
		ret = vm_iomap_memory(vma_s, ip->mem_start, vsize);
		if(ret)
		{
			printk(KERN_ERR"IP:memory maped failed\n");
			return ret;
		}
		printk(KERN_INFO"MMAP is a success form ip\n");

		break;
		
		default:
			printk(KERN_INFO"something went wrong\n");
		
		}
	return 0;
}
	

static int __init ED_init(void)
{
   int ret = 0;

   ret = alloc_chrdev_region(&my_dev_id, 0, number_of_minors, "storage");
   if (ret){
      printk(KERN_ERR "failed to register char device\n");
      return ret;
   }
   printk(KERN_INFO "char device region allocated\n");

   my_class = class_create(THIS_MODULE, "ED_class1");
   if (my_class == NULL){
      printk(KERN_ERR "failed to create class\n");
      goto fail_0;
   }
   printk(KERN_INFO "class created\n");
   
   my_device = device_create(my_class, NULL,MKDEV(MAJOR(my_dev_id),0), NULL, "bram0");
   if (my_device == NULL){
      printk(KERN_ERR "failed to create device\n");
      goto fail_1;
   }
   printk(KERN_INFO "device created\n");

   my_device = device_create(my_class, NULL, MKDEV(MAJOR(my_dev_id),1), NULL,"bram1");
   if (my_device == NULL){
       printk(KERN_ERR "failed to create device\n");
       goto fail_1;
   }
       printk(KERN_INFO "device created\n");

   my_device = device_create(my_class, NULL, MKDEV(MAJOR(my_dev_id),2), NULL, "Enc_dec");
   if (my_device == NULL){
	printk(KERN_ERR "failed to create device\n");
	goto fail_1;
	}


	printk(KERN_INFO "device created\n");
 
	my_cdev = cdev_alloc();	
	my_cdev->ops = &my_fops;
	my_cdev->owner = THIS_MODULE;
	ret = cdev_add(my_cdev, my_dev_id, number_of_minors);
	if (ret)
	{
      printk(KERN_ERR "failed to add cdev\n");
		goto fail_2;
	}
   printk(KERN_INFO "cdev added\n");
   printk(KERN_INFO "Hello world\n");


   return platform_driver_register(&ED_driver);

   fail_2:
	device_destroy(my_class, my_dev_id);
   fail_1:
      class_destroy(my_class);
   fail_0:
      unregister_chrdev_region(my_dev_id, 1);
   return -1;

}
static void __exit ED_exit(void)
{

   printk(KERN_ALERT "ED_exit: rmmod called\n");  
   platform_driver_unregister(&ED_driver);
   printk(KERN_INFO"ED_exit: platform_driver_unregister_done\n");
   cdev_del(my_cdev);
   printk(KERN_ALERT "ED_exit: cdev_del done\n");
   device_destroy(my_class, MKDEV(MAJOR(my_dev_id),0));
   printk(KERN_INFO"ED_exit: device 0 destroyed\n");
   device_destroy(my_class, MKDEV(MAJOR(my_dev_id),1));
   printk(KERN_INFO"ED_exit: device 1 destroyed\n");
   device_destroy(my_class, MKDEV(MAJOR(my_dev_id),2));
   printk(KERN_INFO"ED_exit: device 2 destroyed\n");
   class_destroy(my_class);
   printk(KERN_INFO"ED_exit: class destroyed\n");
   unregister_chrdev_region(my_dev_id,number_of_minors);
   printk(KERN_INFO "Goodbye, cruel world\n");
}


module_init(ED_init);
module_exit(ED_exit);
