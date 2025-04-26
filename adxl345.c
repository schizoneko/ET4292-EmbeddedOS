#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/i2c.h>
#include <linux/fs.h>
#include <linux/of.h>
#include <linux/uaccess.h>

#define ADXL345_REG_DATAX0  0x32  // Thanh ghi bắt đầu dữ liệu XYZ

struct adxl345_dev {
    struct i2c_client *client;
    struct miscdevice miscdev;
    char name[12];  // "adxl345X"
};

/* Đọc dữ liệu từ cảm biến */
static ssize_t adxl345_read(struct file *file, char __user *userbuf,
                            size_t count, loff_t *ppos)
{
    struct adxl345_dev *dev;
    s16 x, y, z;
    char outbuf[64];
    int ret;
    u8 buf[6];
    struct i2c_msg msgs[2];
    u8 reg = ADXL345_REG_DATAX0;

    dev = container_of(file->private_data, struct adxl345_dev, miscdev);

    // Gửi địa chỉ thanh ghi bắt đầu đọc
    msgs[0].addr  = dev->client->addr;
    msgs[0].flags = 0;
    msgs[0].len   = 1;
    msgs[0].buf   = &reg;

    // Nhận 6 byte dữ liệu
    msgs[1].addr  = dev->client->addr;
    msgs[1].flags = I2C_M_RD;
    msgs[1].len   = 6;
    msgs[1].buf   = buf;

    ret = i2c_transfer(dev->client->adapter, msgs, 2);
    if (ret < 0)
        return -EIO;

    x = (buf[1] << 8) | buf[0];
    y = (buf[3] << 8) | buf[2];
    z = (buf[5] << 8) | buf[4];

    ret = snprintf(outbuf, sizeof(outbuf), "X:%d Y:%d Z:%d\n", x, y, z);

    if (*ppos > 0)
        return 0;

    if (copy_to_user(userbuf, outbuf, ret))
        return -EFAULT;

    *ppos += ret;
    return ret;
}

static const struct file_operations adxl345_fops = {
    .owner = THIS_MODULE,
    .read = adxl345_read,
};

static int adxl345_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    static int counter = 0;
    struct adxl345_dev *dev;
    int ret;
    u8 setup[2];

    dev = devm_kzalloc(&client->dev, sizeof(*dev), GFP_KERNEL);
    if (!dev)
        return -ENOMEM;

    i2c_set_clientdata(client, dev);
    dev->client = client;

    snprintf(dev->name, sizeof(dev->name), "adxl345%d", counter++);
    dev->miscdev.minor = MISC_DYNAMIC_MINOR;
    dev->miscdev.name  = dev->name;
    dev->miscdev.fops  = &adxl345_fops;

    ret = misc_register(&dev->miscdev);
    if (ret)
        return ret;

    // Cấu hình cảm biến: bật chế độ đo (write 0x08 vào POWER_CTL)
    setup[0] = 0x2D;
    setup[1] = 0x08;
    ret = i2c_master_send(client, setup, 2);
    if (ret < 0)
        dev_err(&client->dev, "Không thể cấu hình cảm biến\n");

    dev_info(&client->dev, "ADXL345 driver đã đăng ký với %s\n", dev->name);
    return 0;
}

static int adxl345_remove(struct i2c_client *client)
{
    struct adxl345_dev *dev = i2c_get_clientdata(client);
    misc_deregister(&dev->miscdev);
    dev_info(&client->dev, "Gỡ driver ADXL345 %s\n", dev->name);
    return 0;
}

static const struct of_device_id adxl345_dt_ids[] = {
    { .compatible = "adi,adxl345" },
    { }
};
MODULE_DEVICE_TABLE(of, adxl345_dt_ids);

static const struct i2c_device_id adxl345_ids[] = {
    { "adxl345", 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, adxl345_ids);

static struct i2c_driver adxl345_driver = {
    .driver = {
        .name = "adxl345_driver",
        .of_match_table = adxl345_dt_ids,
    },
    .probe = adxl345_probe,
    .remove = adxl345_remove,
    .id_table = adxl345_ids,
};

module_i2c_driver(adxl345_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Nhóm Hệ điều hành nhúng");
MODULE_DESCRIPTION("Driver I2C cho cảm biến gia tốc ADXL345 trên Raspberry Pi");
