#ifndef DBUSINTERFACE_H
#define DBUSINTERFACE_H

#include <QObject>
#include <QtDBus>

class UPowerBatteryInterface : public QObject {
    Q_OBJECT

  public:
    explicit UPowerBatteryInterface(QObject* parent = nullptr);
    double getBatteryLevel();

  signals:
    void batteryLevelChanged(double level);
    void chargerStatusChanged(bool isOnline);
  public slots:
    bool isChargerOnline();
  private slots:
    void onPropertiesChanged(const QString& interfaceName, const QVariantMap& changedProperties,
                             const QStringList& invalidatedProperties);

  private:
    QDBusInterface* m_interface;
};

#endif // DBUSINTERFACE_H