#include "UPowerBatteryInterface.h"

#include <QDebug>
#include <QVariant>

const QString UPOWER_SERVICE = "org.freedesktop.UPower";
const QString UPOWER_PATH = "/org/freedesktop/UPower/devices/battery_BAT0";
const QString UPOWER_INTERFACE = "org.freedesktop.UPower.Device";
const QString DBUS_PROPERTIES_INTERFACE = "org.freedesktop.DBus.Properties";

UPowerBatteryInterface::UPowerBatteryInterface(QObject* parent) : QObject(parent) {
    QDBusConnection bus = QDBusConnection::systemBus();
    if (!bus.isConnected()) {
        qWarning() << "Cannot connect to the system bus";
        return;
    }

    m_interface =
        new QDBusInterface(UPOWER_SERVICE, UPOWER_PATH, DBUS_PROPERTIES_INTERFACE, bus, this);
    if (!m_interface->isValid()) {
        qWarning() << "Failed to create D-Bus interface:" << m_interface->lastError().message();
        return;
    }

    bus.connect(UPOWER_SERVICE, UPOWER_PATH, DBUS_PROPERTIES_INTERFACE, "PropertiesChanged", this,
                SLOT(onPropertiesChanged(QString, QVariantMap, QStringList)));
}

void UPowerBatteryInterface::onPropertiesChanged(const QString& interfaceName,
                                                 const QVariantMap& changedProperties,
                                                 const QStringList& invalidatedProperties) {
    Q_UNUSED(invalidatedProperties);

    if (interfaceName != UPOWER_INTERFACE) {
        return;
    }

    if (changedProperties.contains("Percentage")) {
        double percentage = changedProperties["Percentage"].toDouble();
        qInfo() << "Battery level changed:" << percentage;
        emit batteryLevelChanged(percentage);
    }

    if (changedProperties.contains("State")) {
        // State 1 typically means charging, 2 means discharging
        bool isOnline = (changedProperties["State"].toUInt() == 1);
        qInfo() << "Charger status changed (based on State):" << isOnline;
        emit chargerStatusChanged(isOnline);
    }
}

double UPowerBatteryInterface::getBatteryLevel() {
    if (!m_interface->isValid()) {
        return -1;
    }

    QDBusMessage reply = m_interface->call("Get", UPOWER_INTERFACE, "Percentage");

    if (reply.type() == QDBusMessage::ReplyMessage && reply.arguments().count() > 0) {
        QVariant firstArg = reply.arguments().at(0);
        if (firstArg.canConvert<QDBusVariant>()) {
            QDBusVariant dbusVariant = firstArg.value<QDBusVariant>();
            return dbusVariant.variant().toDouble();
        }
    }

    qWarning() << "Failed to get battery level:" << reply.errorMessage();
    return -1;
}

bool UPowerBatteryInterface::isChargerOnline() {
    if (!m_interface->isValid()) {
        return false;
    }

    QDBusMessage reply = m_interface->call("Get", UPOWER_INTERFACE, "State");

    if (reply.type() == QDBusMessage::ReplyMessage && reply.arguments().count() > 0) {
        QVariant firstArg = reply.arguments().at(0);
        if (firstArg.canConvert<QDBusVariant>()) {
            QDBusVariant dbusVariant = firstArg.value<QDBusVariant>();
            // State 1 typically means charging, 2 means discharging
            return dbusVariant.variant().toUInt() == 1;
        }
    }

    qWarning() << "Failed to get charger status (State):" << reply.errorMessage();
    return false;
}