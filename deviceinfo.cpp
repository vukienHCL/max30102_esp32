#include "deviceinfo.h"

DeviceInfo::DeviceInfo()
{

}

DeviceInfo::DeviceInfo(const QBluetoothDeviceInfo &aDevice)
{
    mDevice = aDevice;
}

QString DeviceInfo::getName(void) const
{
    return mDevice.name();
}

QString DeviceInfo::getAddress() const
{
    return mDevice.address().toString();
}

QBluetoothDeviceInfo DeviceInfo::getDevice()
{
    return mDevice;
}

void DeviceInfo::setDevice(const QBluetoothDeviceInfo &dev)
{
    mDevice = QBluetoothDeviceInfo(dev);
    Q_EMIT deviceChanged();
}
