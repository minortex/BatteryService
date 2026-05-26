#ifndef DBUSINTERFACE_H
#define DBUSINTERFACE_H

#include <QObject>
#include <QtDBus>

class UPowerBatteryInterface : public QObject {
    Q_OBJECT

  public:
    explicit UPowerBatteryInterface(QObject* parent = nullptr);
    double getBatteryLevel();
    bool isBatteryCharging();
    bool isExternalPowerOnline();

  signals:
    void batteryLevelChanged(double level);
    void batteryChargingChanged(bool isCharging);
    void externalPowerOnlineChanged(bool isOnline);

  private slots:
    void onPropertiesChanged(const QString& interfaceName, const QVariantMap& changedProperties,
                             const QStringList& invalidatedProperties);

  private:
    QDBusInterface* m_interface{nullptr};
    bool m_hasLastBatteryChargingState{false};
    bool m_lastBatteryChargingState{false};
    bool m_hasLastExternalPowerState{false};
    bool m_lastExternalPowerState{false};
};

#endif // DBUSINTERFACE_H
