# MPU6050 Kernel Module

## Mô tả
Kernel module đọc dữ liệu raw từ cảm biến MPU6050 qua I²C trên Raspberry Pi, xuất ra thiết bị character `/dev/mpu6050`.

## Yêu cầu
- **Raspberry Pi OS**: Đã bật I²C.
- **Gói phần mềm**:
  - `build-essential`
  - `raspberrypi-kernel-headers`
  - `i2c-tools`

### Cài đặt gói phần mềm:
```bash
sudo apt update
sudo apt install -y build-essential raspberrypi-kernel-headers i2c-tools
