#include "ChargePolicy.h"
#include "UPowerBatteryInterface.h"
#include "WriteSysfs.h"

#include <QCoreApplication>
#include <QDebug>
#include <QProcess>
#include <QStringList>

#include <cstdio>
#include <iostream>
#include <string>

namespace {

constexpr const char* behaviorPath = "/sys/class/power_supply/BAT0/charge_behaviour";
constexpr const char* serviceName = "BatteryService.service";

void printUsage() {
    std::cout
        << "Usage: batteryctl [COMMAND]\n"
        << "\n"
        << "Commands:\n"
        << "  daemon           Run the battery policy daemon (default)\n"
        << "  monitor          Run the daemon in the foreground\n"
        << "  once             Evaluate and apply the policy once\n"
        << "  status           Show battery, charge behavior, and service status\n"
        << "  auto             Set charge behavior to auto\n"
        << "  inhibit          Set charge behavior to inhibit-charge\n"
        << "  service status   Show the systemd service status\n"
        << "  service restart  Restart the systemd service\n"
        << "  service start    Start the systemd service\n"
        << "  service stop     Stop the systemd service\n"
        << "  help             Show this help text\n";
}

int runSystemctl(const QStringList& arguments) {
    return QProcess::execute("systemctl", arguments);
}

std::string readFile(const char* path) {
    FILE* file = std::fopen(path, "r");
    if (!file) {
        return {};
    }

    char buffer[256];
    std::string content;
    while (std::fgets(buffer, sizeof(buffer), file)) {
        content += buffer;
    }

    std::fclose(file);
    while (!content.empty() && (content.back() == '\n' || content.back() == ' ')) {
        content.pop_back();
    }
    return content;
}

int setChargeBehavior(const std::string& behavior) {
    ChargeBehaviorHandler chargeHandler{behaviorPath};
    return chargeHandler.writeChargeBehavior(behavior) ? 0 : 1;
}

int applyPolicyOnce() {
    UPowerBatteryInterface batteryMonitor;
    ChargePolicy chargePolicy;
    const double level = batteryMonitor.getBatteryLevel();
    const bool isCharging = batteryMonitor.isBatteryCharging();
    const ChargePolicy::Behavior behavior = chargePolicy.evaluate(level, isCharging);

    qInfo() << "Applying policy once for battery level:" << level
            << "charging:" << isCharging
            << "behavior:" << ChargePolicy::toSysfsValue(behavior);

    return setChargeBehavior(ChargePolicy::toSysfsValue(behavior));
}

int printStatus() {
    UPowerBatteryInterface batteryMonitor;

    std::cout << "Battery level: " << batteryMonitor.getBatteryLevel() << "%\n";
    std::cout << "Battery charging: " << (batteryMonitor.isBatteryCharging() ? "yes" : "no")
              << "\n";
    std::cout << "Kernel status: " << readFile("/sys/class/power_supply/BAT0/status") << "\n";
    std::cout << "Charge behavior: " << readFile(behaviorPath) << "\n";
    std::cout << "\n" << std::flush;
    return runSystemctl({"status", "--no-pager", serviceName});
}

int runDaemon(QCoreApplication& app) {
    double lastLevel{-1.0};

    UPowerBatteryInterface batteryMonitor;
    ChargeBehaviorHandler chargeHandler{behaviorPath};
    ChargePolicy chargePolicy;
    std::string lastChargeBehavior;

    auto controlChargeBehavior{[&chargeHandler, &lastChargeBehavior](
                                   ChargePolicy::Behavior behavior) {
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

    QObject::connect(&batteryMonitor, &UPowerBatteryInterface::batteryLevelChanged,
                     [&](double currentLevel) { updateLogic(currentLevel, false); });

    QObject::connect(&batteryMonitor, &UPowerBatteryInterface::batteryChargingChanged,
                     [&](bool) { updateLogic(batteryMonitor.getBatteryLevel(), true); });

    qInfo() << "Performing initial state check...";
    updateLogic(batteryMonitor.getBatteryLevel(), true);

    qInfo() << "Monitoring battery and charger status...";
    return app.exec();
}

} // namespace

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    const QStringList args = app.arguments();
    const QString command = args.value(1, "daemon");

    if (command == "daemon" || command == "monitor") {
        return runDaemon(app);
    }

    if (command == "once") {
        return applyPolicyOnce();
    }

    if (command == "status") {
        return printStatus();
    }

    if (command == "auto") {
        return setChargeBehavior("auto");
    }

    if (command == "inhibit") {
        return setChargeBehavior("inhibit-charge");
    }

    if (command == "service") {
        const QString action = args.value(2);
        if (action == "status" || action == "restart" || action == "start" || action == "stop") {
            QStringList systemctlArgs{action};
            if (action == "status") {
                systemctlArgs.append("--no-pager");
            }
            systemctlArgs.append(serviceName);
            return runSystemctl(systemctlArgs);
        }
    }

    if (command == "help" || command == "--help" || command == "-h") {
        printUsage();
        return 0;
    }

    printUsage();
    return 2;
}
