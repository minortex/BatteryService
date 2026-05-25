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

  signals:
    void batteryLevelChanged(double level);
    void batteryChargingChanged(bool isCharging);

  private slots:
    void onPropertiesChanged(const QString& interfaceName, const QVariantMap& changedProperties,
                             const QStringList& invalidatedProperties);

  private:
    QDBusInterface* m_interface{nullptr};
    bool m_hasLastBatteryState{false};
    uint m_lastBatteryState{0};
};

#endif // DBUSINTERFACE_H
