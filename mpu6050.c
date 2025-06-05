#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/atomic.h>
#include <linux/math64.h>

#define MPU6050_ADDR    0x68
#define GYRO_FS_500     0x01
#define ACCEL_FS_2      0x00
#define _R2D            (180 / 3.14159265)

struct mpu6050_data {
    int16_t acc_x;
    int16_t acc_y;
    int16_t acc_z;
    int16_t gyro_x;
    int16_t gyro_y;
    int16_t gyro_z;
};

static struct i2c_client *mpu6050_client;
static struct miscdevice mpu6050_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name  = "mpu6050",
};

static int mpu6050_write_byte(uint8_t reg, uint8_t data)
{
    return i2c_smbus_write_byte_data(mpu6050_client, reg, data);
}

static int mpu6050_read_bytes(uint8_t reg, uint8_t *buf, size_t len)
{
    return i2c_smbus_read_i2c_block_data(mpu6050_client, reg, len, buf);
}

static int mpu6050_init(void)
{
    int ret;

    ret = mpu6050_write_byte(0x6B, 0x00);
    if (ret < 0)
        return ret;

    ret = mpu6050_write_byte(0x1B, GYRO_FS_500);
    if (ret < 0)
        return ret;

    ret = mpu6050_write_byte(0x1C, ACCEL_FS_2);
    if (ret < 0)
        return ret;

    return 0;
}

static int mpu6050_read_raw(struct mpu6050_data *d)
{
    uint8_t buf[14];
    int ret;

    ret = mpu6050_read_bytes(0x3B, buf, 14);
    if (ret < 0)
        return ret;

    d->acc_x  = (int16_t)((buf[0] << 8)  | buf[1]);
    d->acc_y  = (int16_t)((buf[2] << 8)  | buf[3]);
    d->acc_z  = (int16_t)((buf[4] << 8)  | buf[5]);
    d->gyro_x = (int16_t)((buf[8] << 8)  | buf[9]);
    d->gyro_y = (int16_t)((buf[10] << 8) | buf[11]);
    d->gyro_z = (int16_t)((buf[12] << 8) | buf[13]);

    return 0;
}

static ssize_t mpu6050_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
    struct mpu6050_data d;
    char output[128];
    int len, ret;

    ret = mpu6050_read_raw(&d);
    if (ret < 0)
        return ret;

    len = snprintf(output, sizeof(output),
                   "ACC: X=%6d Y=%6d Z=%6d; GYRO: X=%6d Y=%6d Z=%6d\n",
                   d.acc_x, d.acc_y, d.acc_z,
                   d.gyro_x, d.gyro_y, d.gyro_z);

    if (*ppos > 0 || count < len)
        return 0;

    if (copy_to_user(buf, output, len))
        return -EFAULT;

    *ppos = len;
    return len;
}

static const struct file_operations mpu6050_fops = {
    .owner = THIS_MODULE,
    .read  = mpu6050_read,
};

static int mpu6050_probe(struct i2c_client *client)
{
    int ret;

    mpu6050_client = client;
    ret = mpu6050_init();
    if (ret < 0)
        return ret;

    ret = misc_register(&mpu6050_device);
    if (ret < 0)
        return ret;

    return 0;
}

static void mpu6050_remove(struct i2c_client *client)
{
    misc_deregister(&mpu6050_device);
}

static const struct i2c_device_id mpu6050_id[] = {
    { "mpu6050", 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, mpu6050_id);

static struct i2c_driver mpu6050_driver = {
    .driver = {
        .name  = "mpu6050",
        .owner = THIS_MODULE,
    },
    .probe    = mpu6050_probe,
    .remove   = mpu6050_remove,
    .id_table = mpu6050_id,
};

static int __init mpu6050_init_module(void)
{
    return i2c_add_driver(&mpu6050_driver);
}

static void __exit mpu6050_exit_module(void)
{
    i2c_del_driver(&mpu6050_driver);
}

module_init(mpu6050_init_module);
module_exit(mpu6050_exit_module);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Group 6");
MODULE_DESCRIPTION("MPU6050 I2C Driver for Raspberry Pi");