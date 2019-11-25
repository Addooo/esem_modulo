#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#define DRIVER_AUTHOR "Marco Adobati"
#define DRIVER_DESC "Esempio"
#define DEVICE_NAME "esempdevice"
#define BUF_LEN 10

//Tutte le volte che verrà aperto scriverà quanti processi lo hanno aperto attualmente.
//Una volta caricato il modulo, creo il file esm in /dev con sudo mknod esm c MAJOR MINOR
//e facendo cat /dev/esm verrà messo a video quanto scritto  nella funzione device_open

static int Major;
static int paral_cont = 0; //conteggio parallelo per contare il num di processi che utilizzano il modulo
static int Device_Open = 0; //Device aperto oppure no
static char msg[BUF_LEN];
static char *msg_Ptr;

static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char *, size_t, loff_t *);

//funzione implementate per il device
static struct file_operations fops = {
	.read = device_read,
	.open = device_open,
	.release = device_release
};


int start(void)
{
	printk(KERN_INFO "Inizio Modulo\n");
	//registro il device
	Major = register_chrdev(0, DEVICE_NAME, &fops); //0 così assegnato da solo
	if(Major < 0){
		printk(KERN_ALERT "Major number non assegnato, val ritornato: %d\n", Major);
		return Major;
	}
	
	printk(KERN_INFO "Assegnato il numero %d\n", Major);

	return 0;
}


void end(void)
{
	unregister_chrdev(Major, DEVICE_NAME);//rimuovo
	printk(KERN_INFO "Fine Modulo\n");
}


module_init(start);
module_exit(end);

MODULE_LICENSE("GPL");
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);


//chiamata quando un processo prova ad aprire il device file
static int device_open(struct inode *inode, struct file *file)
{
	if (Device_Open)
		return -EBUSY;

	Device_Open++;
	msg_Ptr = msg;
	try_module_get(THIS_MODULE);//aumenta il count del numero di processi che usano il modulo
	sprintf(msg, "Ciao! %d\n", ++paral_cont);
	return 0;
}

//quando viene rilasciato
static int device_release(struct inode *inode, struct file *file)
{
	Device_Open--;
	
	module_put(THIS_MODULE);//decrementano il count del numero di processi che usano il modulo
	paral_cont--;
	return 0;
}

//legge dal device file
static ssize_t device_read(struct file *filp, char* buffer, size_t length,loff_t * offset) {
		      
	int bytes_read = 0;
	     	       
	if (*msg_Ptr == 0)
		return 0;
	
	while (length && *msg_Ptr) {
		put_user(*(msg_Ptr++), buffer++);//per mettere byte nel segmento di memoria dello user
		length--;
		bytes_read++;
	}

	return bytes_read;
}
