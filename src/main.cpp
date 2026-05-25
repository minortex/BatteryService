#include "ChargePolicy.h"
#include "UPowerBatteryInterface.h"
#include "WriteSysfs.h"
#include <QCoreApplication>
#include <QDebug>
#include <qdebug.h>
#include <qlogging.h>
#include <string>

int main(int argc, char* argv[]) {
        double lastLevel{-1.0}; // To store the last known battery level
    
        QCoreApplication a(argc, argv);
    
        UPowerBatteryInterface batteryMonitor;
        ChargeBehaviorHandler chargeHandler{"/sys/class/power_supply/BAT0/charge_behaviour"};
        ChargePolicy chargePolicy;
        std::string lastChargeBehavior;
    
        auto controlChargeBehavior{[&chargeHandler, &lastChargeBehavior](ChargePolicy::Behavior behavior) {
            const std::string targetBehavior = ChargePolicy::toSysfsValue(behavior);
            if (targetBehavior == lastChargeBehavior) {
                return;
            }

            qInfo() << "Setting charge behavior to" << targetBehavior.c_str();
            if (chargeHandler.writeChargeBehavior(targetBehavior)) {
                lastChargeBehavior = targetBehavior;
            }
        }};
    
        auto updateLogic = [&](double currentLevel, bool force) {
            if (!force && currentLevel == lastLevel) {
                return;
            }
    
            qInfo() << "Checking state for battery level: " << currentLevel << "%";
            lastLevel = currentLevel;
    
            controlChargeBehavior(
                chargePolicy.evaluate(currentLevel, batteryMonitor.isBatteryCharging()));
        };
    
        // --- Connections ---
        QObject::connect(
            &batteryMonitor, &UPowerBatteryInterface::batteryLevelChanged, [&](double currentLevel) {
                updateLogic(currentLevel, false);
            });
    
        QObject::connect(
            &batteryMonitor, &UPowerBatteryInterface::batteryChargingChanged, [&](bool) {
                updateLogic(batteryMonitor.getBatteryLevel(), true);
            });
    
        // --- Initial Check ---
        qInfo() << "Performing initial state check...";
        updateLogic(batteryMonitor.getBatteryLevel(), true);
    
        qInfo() << "Monitoring battery and charger status...";
        return a.exec();
    }
    
