#include "UPowerBatteryInterface.h"
#include "WriteSysfs.h"
#include <QCoreApplication>
#include <QDebug>
#include <qdebug.h>
#include <qlogging.h>

int main(int argc, char* argv[]) {
    constexpr double threshHold{80.0};
    bool inhibitChargeSet{false}; // Track if inhibit-charge has been set

    QCoreApplication a(argc, argv);

    UPowerBatteryInterface batteryMonitor;
    ChargeBehaviorHandler chargeHandler{"/sys/class/power_supply/BAT0/charge_behaviour"};

    auto controlChargeBehavior{[&chargeHandler](bool action) {
        if (action) {
            qInfo() << "Setting charge behavior to inhibit-charge";
            chargeHandler.writeChargeBehavior("inhibit-charge");
        } else {
            qInfo() << "Setting charge behavior to auto";
            chargeHandler.writeChargeBehavior("auto");
        }
    }};

    auto isLevelSufficient{[](double level) -> bool {
        qInfo() << "Current battery level: " << level << "%";
        return level >= threshHold;
    }};

    // --- Connections ---

    // Triggered when battery level changes
    QObject::connect(
        &batteryMonitor, &UPowerBatteryInterface::batteryLevelChanged, [&](double level) {
            if (batteryMonitor.isChargerOnline() && isLevelSufficient(level) && !inhibitChargeSet) {
                controlChargeBehavior(true);
                inhibitChargeSet = true;
            }
        });

    // Triggered when charger is plugged/unplugged
    QObject::connect(
        &batteryMonitor, &UPowerBatteryInterface::chargerStatusChanged, [&](bool online) {
            if (online) {
                if (isLevelSufficient(batteryMonitor.getBatteryLevel())) {
                    controlChargeBehavior(true);
                    inhibitChargeSet = true;
                } else {
                    qInfo() << "Battery level below threshold. Setting charge behavior to auto.";
                    controlChargeBehavior(false);
                    inhibitChargeSet = false; // Reset the flag
                }
            }
        });

    // --- Initial Check ---
    bool shouldInhibitCharge =
        batteryMonitor.isChargerOnline() && isLevelSufficient(batteryMonitor.getBatteryLevel());

    controlChargeBehavior(shouldInhibitCharge);
    inhibitChargeSet = shouldInhibitCharge;

    qInfo() << "Monitoring battery level changes...";
    return a.exec();
}