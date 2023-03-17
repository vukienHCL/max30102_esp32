#include "QDebug"
#include "device.h"
#include "deviceinfo.h"
#include <iostream>
#include <string>

Device::Device()
{
    scanAction = "SCAN";
    displayBusyIndicator = false;

    discoveryAgent = new QBluetoothDeviceDiscoveryAgent();
    discoveryAgent->setLowEnergyDiscoveryTimeout(5000);
    connect(discoveryAgent, &QBluetoothDeviceDiscoveryAgent::deviceDiscovered,
            this, &Device::addDevice);
    connect(discoveryAgent, QOverload<QBluetoothDeviceDiscoveryAgent::Error>::of(&QBluetoothDeviceDiscoveryAgent::error),
            this, &Device::deviceScanError);
    connect(discoveryAgent, &QBluetoothDeviceDiscoveryAgent::finished, this, &Device::deviceScanFinished);
}

QVariant Device::getDeviceList(void)
{
    return QVariant::fromValue(mDevices);
}

void Device::scan()
{
    qDebug() << "Start scan BLE devices";
//    m_deviceHandler->setDevice(nullptr);// add
    qDeleteAll(devices);
    devices.clear();
    emit deviceListChanged();

    discoveryAgent->start(QBluetoothDeviceDiscoveryAgent::LowEnergyMethod);

    if (discoveryAgent->isActive()) {

    }
}

void Device::stop()
{
    discoveryAgent->stop();
}

QString Device::getScanAction()
{
    return scanAction;
}

void Device::setScanAction(const QString &str)
{
    if (str != scanAction)
    {
        scanAction = str;
        emit scanActionChanged();
    }
}

bool Device::getDisplayBusyIndicator()
{
    return displayBusyIndicator;
}

void Device::setDisplayBusyIndicator(bool state)
{
    if (state != displayBusyIndicator)
    {
        displayBusyIndicator = state;
        emit displayBusyIndicatorChanged();
    }
}

void Device::connectDevice(const QString &address)
{
    setDisplayBusyIndicator(true);

    // We need the current device for service discovery.
    for (auto d: qAsConst(devices)) {
        if (auto device = qobject_cast<DeviceInfo *>(d)) {
            if (device->getAddress() == address ) {
                currentDevice.setDevice(device->getDevice());
                break;
            }
        }
    }

    qDebug() << "Attemp to connect " << currentDevice.getName();

    if (!currentDevice.getDevice().isValid()) {
        qWarning() << "Not a valid device";
        return;
    }

    // Disconnect and delete old connection
    if (controller) {
        controller->disconnectFromDevice();
        delete controller ;
        controller  = nullptr;
    }
    // controller = nullptr;

    if (!controller) {
        // Connecting signals and slots for connecting to LE services.
        controller = QLowEnergyController::createCentral(currentDevice.getDevice());
        connect(controller, &QLowEnergyController::connected,
                this, &Device::deviceConnected);
        connect(controller, QOverload<QLowEnergyController::Error>::of(&QLowEnergyController::error),
                this, &Device::errorReceived);
        connect(controller, &QLowEnergyController::disconnected,
                this, &Device::deviceDisconnected);
        //
        connect(controller, &QLowEnergyController::serviceDiscovered,
                this, &Device::serviceDiscovered);
        connect(controller, &QLowEnergyController::discoveryFinished,
                this, &Device::serviceScanDone);

        connect(controller, static_cast<void (QLowEnergyController::*)(QLowEnergyController::Error)>(&QLowEnergyController::error),
                this, [this](QLowEnergyController::Error error) {
            Q_UNUSED(error);
            qDebug() <<"Cannot connect to remote device.";
        });
        connect(controller, &QLowEnergyController::connected, this, [this]() {
            qDebug() << "Controller connected. Search services...";
            controller->discoverServices();
        });
        connect(controller, &QLowEnergyController::disconnected, this, [this]() {
            qDebug() << "LowEnergy controller disconnected";
        });

        // Connect
        controller->connectToDevice();

        listController.append(controller);
    }

    // controller->connectToDevice();
}

void Device::disconnectDevice(const QString &address)
{
    setDisplayBusyIndicator(true);

    // We need the current device for service discovery.
    for (auto d: qAsConst(mConnectDevices)) {
        if (auto device = qobject_cast<DeviceInfo *>(d)) {
            if (device->getAddress() == address ) {
                currentDevice.setDevice(device->getDevice());
                break;
            }
        }
    }

    qDebug() << "Attemp to disconnect from " << currentDevice.getName();

    if (!currentDevice.getDevice().isValid()) {
        qWarning() << "Not a valid device";
        return;
    }

    for (const auto &c : listController)
    {
        if (c->remoteAddress().toString() == address)
        {
            controller = c;
            break;
        }
    }

    controller->disconnectFromDevice();
}


void Device::addDevice(const QBluetoothDeviceInfo &info)
{
    if (info.coreConfigurations() & QBluetoothDeviceInfo::LowEnergyCoreConfiguration)
    {
        qDebug() << "Found name    device: " << info.name();
        qDebug() << "Found address device: " << info.address().toString();
        if (info.name().remove('-') != info.address().toString().remove(':'))
        {
            mDevices.append(new DeviceInfo(info));
            emit deviceListChanged();
        }

    }
}

void Device::deviceScanError(QBluetoothDeviceDiscoveryAgent::Error)
{
    qDebug() << "Error BLE";
}

void Device::deviceScanFinished()
{
//    const QList<QBluetoothDeviceInfo> foundDevices = discoveryAgent->discoveredDevices();
//    for (auto nextDevice : foundDevices)
//    {
//        if (nextDevice.coreConfigurations() & QBluetoothDeviceInfo::LowEnergyCoreConfiguration)
//        {
//            mDevices.append(new DeviceInfo(nextDevice));
//        }
//    }

//    emit deviceListChanged();



    emit deviceListChanged();
}

void Device::errorReceived(QLowEnergyController::Error)
{
    listController.removeOne(controller);
    setDisplayBusyIndicator(false);
    qDebug() << "Connect error " << currentDevice.getName();
}

void Device::deviceDisconnected()
{
    listController.removeOne(controller);
    setDisplayBusyIndicator(false);
}

void Device::serviceDiscovered(const QBluetoothUuid &gatt)
{
    qDebug() << "GATT:" << gatt.toString();
    if (gatt.toString() == heartRateService) {
        qDebug() << "Heart Rate service discovered. Waiting for service scan to be done...";
        m_foundHeartRateService = true;
    }
    else if (gatt.toString() == btnService)
    {
    	qDebug() << "Button service discovered. Waiting for service scan to be done...";
    	m_foundBtnService = true;
    }
    else
    {
    	qDebug() << "not found service ";
    }
}

void Device::serviceScanDone()
{
    qDebug() << "Service scan done.";

    // Delete old service if available
    if (m_service) {
        delete m_service;
        m_service = nullptr;
    }
     if (n_service) {
        delete n_service;
        n_service = nullptr;
    }
//! [Filter HeartRate service 2]
    // If heartRateService found, create new service
    if (m_foundHeartRateService)
        m_service = controller->createServiceObject(QBluetoothUuid(heartRateService), this);
    if (m_foundBtnService)
    	n_service = controller->createServiceObject(QBluetoothUuid(btnService), this);	
    if (m_service) 
    {
        connect(m_service, &QLowEnergyService::stateChanged, this, &Device::serviceStateChanged);
        connect(m_service, &QLowEnergyService::characteristicChanged, this, &Device::updateHeartRateValue);
        connect(m_service, &QLowEnergyService::descriptorWritten, this, &Device::confirmedDescriptorWrite);
        m_service->discoverDetails();
        qDebug() << "Hr Service connect done.";
    } 
    else if(n_service)
    {
    	connect(n_service, &QLowEnergyService::stateChanged, this, &Device::btnServiceStateChanged);
        connect(n_service, &QLowEnergyService::characteristicChanged, this, &Device::updateBtnValue);
        connect(n_service, &QLowEnergyService::descriptorWritten, this, &Device::confirmedBtnDescriptorWrite);
        connect(n_service, &Device::requestWrite,this, &Device::writeBtnCharacteristic);
        connect(n_service, &QLowEnergyService::characteristicWritten, this, &DeviceHandler::confirmedCharacteristicWrite);
        n_service->discoverDetails();
    }
    else 
    {
        setError("Service not found.");
    }
//! [Filter HeartRate service 2]
}

void Device::serviceStateChanged(QLowEnergyService::ServiceState s)
{
    switch (s) {
    case QLowEnergyService::DiscoveringServices:
        qDebug() <<"Discovering services...";
        break;
    case QLowEnergyService::ServiceDiscovered:
    {
        qDebug("Service discovered.");

        const QLowEnergyCharacteristic hrChar = m_service->characteristic(QBluetoothUuid(heartRateCharecteristic));
        if (!hrChar.isValid()) {
            qDebug("HR Data not found.");
            break;
        }
        else
        {

            qDebug("Hr data found");
        }
        m_notificationDesc = hrChar.descriptor(QBluetoothUuid::ClientCharacteristicConfiguration);
        if (m_notificationDesc.isValid())
            m_service->writeDescriptor(m_notificationDesc, QByteArray::fromHex("0100"));

        break;
    }
    default:
        //nothing for now
        break;
    }

//    emit aliveChanged();
}
void Device::btnServiceStateChanged(QLowEnergyService::ServiceState s)
{
    switch (s) {
    case QLowEnergyService::DiscoveringServices:
        qDebug() <<"Discovering services...";
        break;
    case QLowEnergyService::ServiceDiscovered:
    {
        qDebug("Service discovered.");

        const QLowEnergyCharacteristic btnChar = n_service>characteristic(QBluetoothUuid(btnCharecteristic));
        if (!btnChar.isValid()) {
            qDebug("Btn Data not found.");
            break;
        }
        else
        {
            qDebug("Btn data found");
        }

        n_notificationDesc = btnChar.descriptor(QBluetoothUuid::ClientCharacteristicConfiguration);
        if (n_notificationDesc.isValid())
            n_service->writeDescriptor(n_notificationDesc, QByteArray::fromHex("0200"));

        break;
    }
    default:
        //nothing for now
        break;
    }

//    emit aliveChanged();
}
void Device::confirmedDescriptorWrite(const QLowEnergyDescriptor &d, const QByteArray &value)
{
    qDebug("confirmedDescriptorWrite");
}
void Device::confirmedBtnDescriptorWrite(const QLowEnergyDescriptor &d, const QByteArray &value)
{
    qDebug("confirmedBtnDescriptorWrite");
}
void Device::confirmedCharacteristicWrite(const QLowEnergyCharacteristic &c, const QByteArray &value)
{
    qDebug("confirmedCharacteristicWrite");
}
void Device::writeBtnCharacteristic(const QByteArray &value)
{
    qDebug() << "Device::writeBtnCharacteristic: " << value;
    if(n_service && btnCharacteristic.isValid())
    {
        n_service->writeCharacteristic(btnCharacteristic, value, QLowEnergyService::WriteMode);
    }
    //emit requestWrite("I have finished my task);
}
*/
std::string Device::getHumData()
{
    return humdata;
}

void Device::updateHeartRateValue(const QLowEnergyCharacteristic &c,
                          const QByteArray &value)
{
//    sHumanData->heartRate = value.toStdString().c_str();
    if (strcmp(value.toStdString().c_str(), "")){
        humdata = "";
        humdata += value.toStdString().c_str();
        humdata += "__95";
        humdata = removeString(humdata, "___", "__");
    }
//    qDebug() << "updateHeartRateValue: " << value.toStdString().c_str();
//    std::cout << "bytes : " << humdata << '\n';
//    emit measuringChanged(QString(value.toStdString().c_str()));

}
void Device::updateBtnValue(const QLowEnergyCharacteristic &c,
                          const QByteArray &value)
{
    qDebug() << "Button Characteristic UUID: " << c.uuid();
    qDebug() << "updateBtnValue: " << value.toStdString().c_str();
//    std::cout << "bytes : " << btndata << '\n';
//    emit measuringChanged(QString(value.toStdString().c_str()));
}
