#ifndef _DEVICE_H_
#define _DEVICE_H_

#include <QObject>
#include <QList>
#include <QVariant>
#include <QBluetoothDeviceDiscoveryAgent>

class Device : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariant deviceList READ getDeviceList NOTIFY deviceListChanged)
    Q_PROPERTY(QString scanAction READ getScanAction WRITE setScanAction NOTIFY scanActionChanged)
    Q_PROPERTY(bool displayBusyIndicator READ getDisplayBusyIndicator WRITE setDisplayBusyIndicator NOTIFY displayBusyIndicatorChanged)

//    Q_PROPERTY(int hr READ hr NOTIFY statsChanged)

public:
    Device();
    QVariant getDeviceList(void);
    Q_INVOKABLE void scan(void);
    Q_INVOKABLE void stop(void);
    QString getScanAction(void);
    void setScanAction(const QString& str);
    bool getDisplayBusyIndicator(void);
    void setDisplayBusyIndicator(bool state);
    Q_INVOKABLE void connectDevice(const QString &address);
    Q_INVOKABLE void disconnectDevice(const QString &address);
//    Q_INVOKABLE void connectSevice(const QString &uuid);
//    Q_INVOKABLE void disconnectSevice(const QString &uuid);
    void updateHeartRateValue(const QLowEnergyCharacteristic &c,
                              const QByteArray &value);
    void updateBtnValue(const QLowEnergyCharacteristic &c,
                              const QByteArray &value);
    void confirmedDescriptorWrite(const QLowEnergyDescriptor &d,
                              const QByteArray &value);
    void confirmedBtnDescriptorWrite(const QLowEnergyDescriptor &d,
                              const QByteArray &value);
    void writeBtnCharacteristic(const QByteArray &value);
    void confirmedCharacteristicWrite(const QLowEnergyCharacteristic &c,
                              const QByteArray &value);
    std::string getHumData();
    std::string removeString(std::string m_string, std::string oldStr, std::string newStr){
        size_t pos = 0;

        while (pos += newStr.length()) {
            pos = m_string.find(oldStr, pos);
            if (pos == std::string::npos) {
                break;
            }

            m_string.replace(pos, oldStr.length(), newStr);
        }

        return m_string;
    }

private:
    std::string humdata;
    DeviceInfo currentDevice;
    QLowEnergyService *m_service = nullptr;
    QLowEnergyService *n_service = nullptr;
    QLowEnergyDescriptor m_notificationDesc;
    QLowEnergyDescriptor n_notificationDesc;
    QLowEnergyController *controller = nullptr;
    QList<QLowEnergyController*> listController;
    QList<QObject*> devices;
    QList<QObject*> services;
    QList<QObject*> mConnectDevices;
    QBluetoothDeviceDiscoveryAgent* discoveryAgent;
    QString scanAction;
    bool displayBusyIndicator;
    //DeviceHandler *m_deviceHandler;
    bool m_foundHeartRateService;
    QString heartRateService = "{4fafc201-1fb5-459e-8fcc-c5c9c331914b}";
    QString heartRateCharecteristic = "{ca73b3ba-39f6-4ab3-91ae-186dc9577d99}";
    
    bool m_foundBtnService;
    QString btnService = "{22b90e85-4df1-4a4e-9716-c25245707547}";
    QString btnCharecteristic = "{f78ebbff-c8b7-4107-93de-889a6a06d408}";
     //QLowEnergyController
    void serviceDiscovered(const QBluetoothUuid &);
    void serviceScanDone();

    //QLowEnergyService
    void serviceStateChanged(QLowEnergyService::ServiceState s);
    void btnServiceStateChanged(QLowEnergyService::ServiceState s);

signals:
    void deviceListChanged();
    void scanActionChanged();
    void displayBusyIndicatorChanged();
    void measuringChanged(const QString &value);
    void requestWrite(QString value);

public slots:
    void addDevice(const QBluetoothDeviceInfo &info);
    void deviceScanError(QBluetoothDeviceDiscoveryAgent::Error);
    void deviceScanFinished(void);


};

#endif /* _DEVICE_H_ */
