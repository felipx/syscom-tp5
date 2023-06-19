/******************************************************************************
* 7segs.c
* Driver que controla un display de 7 segmentos
*******************************************************************************/
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/gpio.h>
#include <linux/err.h>

// Pines conectados al display
#define GPIO_7  (7)
#define GPIO_8  (8)
#define GPIO_9  (9)
#define GPIO_10 (10)
#define GPIO_11 (11)
#define GPIO_12 (12)
#define GPIO_13 (13)

static long int value = 0; 
 
dev_t dev = 0;
static struct class *dev_class;
static struct cdev disp_cdev;
 
static int __init display_driver_init(void);
static void __exit display_driver_exit(void);
 
 
static int disp_open(struct inode *inode, struct file *file);
static int disp_close(struct inode *inode, struct file *file);
static ssize_t disp_read(struct file *filp, char __user *buf, size_t len,loff_t * off);
static ssize_t disp_write(struct file *filp, const char __user *buf, size_t len, loff_t * off);

 
// Estructura file operations  
static struct file_operations fops =
{
    .owner          = THIS_MODULE,
    .read           = disp_read,
    .write          = disp_write,
    .open           = disp_open,
    .release        = disp_close,
};

static int seg_table[16][7] = {{1,1,1,1,1,1,0},  // 0
                               {0,1,1,0,0,0,0},  // 1
                               {1,1,0,1,1,0,1},  // 2
                               {1,1,1,1,0,0,1},  // 3
                               {0,1,1,0,0,1,1},  // 4
                               {1,0,1,1,0,1,1},  // 5
                               {1,0,1,1,1,1,1},  // 6
                               {1,1,1,0,0,0,1},  // 7
                               {1,1,1,1,1,1,1},  // 8
                               {1,1,1,1,0,1,1}}; // 9
/**
 * @brief Función llamada al abrir el archivo asociado al driver
*/
static int disp_open(struct inode *inode, struct file *file)
{
    pr_info("7segs abierto\n");
    return 0;
}


/**
 * @brief Función llamada al cerrar el archivo asociado al driver
*/
static int disp_close(struct inode *inode, struct file *file)
{
    pr_info("7segs cerrado!\n");
    return 0;
}


/**
 * @brief Función llamada al leer el archivo asociado al driver
*/
static ssize_t disp_read(struct file *filp, char __user *buf, size_t len, loff_t *off)
{
    uint8_t gpio_state;
    uint8_t i;
    char kbuf = (char) value;
    
    pr_info("disp_read function : value = %d \n", kbuf);
    // Lee el valor de los gpio
    for (i = 7; i < 14; i++) {
        gpio_state = gpio_get_value(i);
        pr_info("disp_read function : GPIO_%d = %d \n", i, gpio_state);
    }
    

    // Escribe al espacio de usuario
    len = 1;
    kbuf += 0x30;
    if(copy_to_user(buf, &kbuf, len) != 0) {
        pr_err("ERROR: No se pudo enviar valor del display\n");
        return -1;
    }

    return 1;
}


/**
 * @brief Función llamada al escribir el archivo asociado al driver
*/ 
static ssize_t disp_write(struct file *filp, const char __user *buf, size_t len, loff_t *off)
{
    uint8_t rec_buf[10] = {0};
    uint8_t i = 0;
    long int num = 0;
    
    if(copy_from_user(rec_buf, buf, len ) > 0) {
        pr_err("ERROR: No se pudo leer información desde espacio de usuario\n");
        return -1;
    }
    
    pr_info("disp_write : 7segs set = %c\n", rec_buf[0]);
    
    if (kstrtol(rec_buf, 10, &num) != 0) {
        pr_err("disp_writed : Valor invalido. Escribir valor entre 0 y 9\n");
        return -1;
    }

    if (num < 0 || num > 9) {
        pr_err("disp_writed : Valor invalido. Escribir valor entre 0 y 9\n");
        return -1;
    }

    for (i = 0; i < 7; i++)
        gpio_set_value(i+7, seg_table[num][i]);
    
    value = num;

    return len;
}


/**
 * @brief Init
*/
static int __init display_driver_init(void)
{
    uint8_t i;

    if((alloc_chrdev_region(&dev, 0, 1, "7segs")) <0) {
        pr_err("ERROR: No se puedo alocar major number\n");
        goto r_unreg;
    }
    pr_info("Major = %d Minor = %d \n",MAJOR(dev), MINOR(dev));
   
    cdev_init(&disp_cdev, &fops);
   
    if((cdev_add(&disp_cdev, dev, 1)) < 0) {
        pr_err("ERROR: No se puedo agregar dispositivo al sistema\n");
        goto r_del;
    }
   
    if(IS_ERR(dev_class = class_create(THIS_MODULE, "7segs"))) {
        pr_err("ERROR: No se pudo crear struct class\n");
        goto r_class;
    }
   
    if(IS_ERR(device_create(dev_class, NULL, dev, NULL,"7segs"))) {
        pr_err("ERROR: No se pudo crear dispositivo\n");
        goto r_device;
    }
    
    for (i = 7; i < 14; i++) {
        if(gpio_is_valid(i) == false){
            pr_err("GPIO %d is not valid\n", i);
            goto r_device;
        }
    }
    
    if(gpio_request(GPIO_7, "GPIO_7") < 0) {
        pr_err("ERROR: GPIO %d request\n", GPIO_7);
        goto r_gpio7;
    }
    if(gpio_request(GPIO_8, "GPIO_8") < 0) {
        pr_err("ERROR: GPIO %d request\n", GPIO_8);
        goto r_gpio8;
    }
    if(gpio_request(GPIO_9, "GPIO_9") < 0) {
        pr_err("ERROR: GPIO %d request\n", GPIO_9);
        goto r_gpio9;
    }
    if(gpio_request(GPIO_10, "GPIO_10") < 0) {
        pr_err("ERROR: GPIO %d request\n", GPIO_10);
        goto r_gpio10;
    }
    if(gpio_request(GPIO_11, "GPIO_11") < 0) {
        pr_err("ERROR: GPIO %d request\n", GPIO_11);
        goto r_gpio11;
    }
    if(gpio_request(GPIO_12, "GPIO_12") < 0) {
        pr_err("ERROR: GPIO %d request\n", GPIO_12);
        goto r_gpio12;
    }
    if(gpio_request(GPIO_13, "GPIO_13") < 0) {
        pr_err("ERROR: GPIO %d request\n", GPIO_13);
        goto r_gpio13;
    }
    
    gpio_direction_output(GPIO_7, 0);
    gpio_direction_output(GPIO_8, 0);
    gpio_direction_output(GPIO_9, 0);
    gpio_direction_output(GPIO_10, 0);
    gpio_direction_output(GPIO_11, 0);
    gpio_direction_output(GPIO_12, 0);
    gpio_direction_output(GPIO_13, 0);
    
    gpio_export(GPIO_7, false);
    gpio_export(GPIO_8, false);
    gpio_export(GPIO_9, false);
    gpio_export(GPIO_10, false);
    gpio_export(GPIO_11, false);
    gpio_export(GPIO_12, false);
    gpio_export(GPIO_13, false);
    
    pr_info("Driver display 7 segmentos cargado\n");
    return 0;

r_gpio13:
    gpio_free(GPIO_13);
r_gpio12:
    gpio_free(GPIO_12);
r_gpio11:
    gpio_free(GPIO_11);
r_gpio10:
    gpio_free(GPIO_10); 
r_gpio9:
    gpio_free(GPIO_9); 
r_gpio8:
    gpio_free(GPIO_8);  
r_gpio7:
    gpio_free(GPIO_7);
r_device:
    device_destroy(dev_class,dev);
r_class:
    class_destroy(dev_class);
r_del:
    cdev_del(&disp_cdev);
r_unreg:
    unregister_chrdev_region(dev,1);
  
    return -1;
}

/*
** Module exit function
*/ 
static void __exit display_driver_exit(void)
{
    gpio_unexport(GPIO_7);
    gpio_free(GPIO_7);
    gpio_unexport(GPIO_8);
    gpio_free(GPIO_8);
    gpio_unexport(GPIO_9);
    gpio_free(GPIO_9);
    gpio_unexport(GPIO_10);
    gpio_free(GPIO_10);
    gpio_unexport(GPIO_11);
    gpio_free(GPIO_11);
    gpio_unexport(GPIO_12);
    gpio_free(GPIO_12);
    gpio_unexport(GPIO_13);
    gpio_free(GPIO_13);
    device_destroy(dev_class,dev);
    class_destroy(dev_class);
    cdev_del(&disp_cdev);
    unregister_chrdev_region(dev, 1);
    pr_info("Driver display 7 segmentos removido\n");
}
 
module_init(display_driver_init);
module_exit(display_driver_exit);
 
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Soporte Tecnico");
MODULE_DESCRIPTION("Diver para display de 7 segmentos");
MODULE_VERSION("0.1");