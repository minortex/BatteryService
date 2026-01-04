#include "UPowerBatteryInterface.h"
#include "WriteSysfs.h"
#include <QCoreApplication>
#include <QDebug>
#include <qdebug.h>
#include <qlogging.h>

int main(int argc, char* argv[]) {
    constexpr double threshHold{80.0};
        double lastLevel{-1.0}; // To store the last known battery level
    
        QCoreApplication a(argc, argv);
    
        UPowerBatteryInterface batteryMonitor;
        ChargeBehaviorHandler chargeHandler{"/sys/class/power_supply/BAT0/charge_behaviour"};
    
        auto controlChargeBehavior{[&chargeHandler](bool shouldInhibit) {
            if (shouldInhibit) {
                qInfo() << "Setting charge behavior to inhibit-charge";
                chargeHandler.writeChargeBehavior("inhibit-charge");
            } else {
                qInfo() << "Setting charge behavior to auto";
                chargeHandler.writeChargeBehavior("auto");
            }
        }};
    
        auto updateLogic = [&](double currentLevel) {
            if (currentLevel == lastLevel) {
                qInfo() << "Battery level " << currentLevel << "% is unchanged. No action taken.";
                return;
            }
    
            qInfo() << "Checking state for battery level: " << currentLevel << "%";
            lastLevel = currentLevel;
    
            bool shouldInhibit = batteryMonitor.isChargerOnline() && (currentLevel >= threshHold);
            controlChargeBehavior(shouldInhibit);
        };
    
        // --- Connections ---
        QObject::connect(
            &batteryMonitor, &UPowerBatteryInterface::batteryLevelChanged, updateLogic);
    
        QObject::connect(
            &batteryMonitor, &UPowerBatteryInterface::chargerStatusChanged, [&](bool) {
                updateLogic(batteryMonitor.getBatteryLevel());
            });
    
        // --- Initial Check ---
        qInfo() << "Performing initial state check...";
        updateLogic(batteryMonitor.getBatteryLevel());
    
        qInfo() << "Monitoring battery and charger status...";
        return a.exec();
    }
    