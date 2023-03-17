#ifndef _DEVICE_INFO_H_
#define _DEVICE_INFO_H_

#include <QObject>
#include <qbluetoothdeviceinfo.h>
#include <qbluetoothaddress.h>

class DeviceInfo : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name READ getName NOTIFY deviceChanged)
    Q_PROPERTY(QString address READ getAddress NOTIFY deviceChanged)
public:
    DeviceInfo();
    DeviceInfo(const QBluetoothDeviceInfo &aDevice);
    QString getName(void) const;
    QString getAddress() const;
    QBluetoothDeviceInfo getDevice();
    void setDevice(const QBluetoothDeviceInfo &dev);

private:
    QBluetoothDeviceInfo mDevice;


signals:
    void deviceChanged();
};

#endif
