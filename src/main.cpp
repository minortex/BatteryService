#include "ChargePolicy.h"
#include "UPowerBatteryInterface.h"
#include "WriteSysfs.h"

#include <QCoreApplication>
#include <QDebug>
#include <QProcess>
#include <QStringList>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace {

constexpr const char* behaviorPath = "/sys/class/power_supply/BAT0/charge_behaviour";
constexpr const char* batteryStatusPath = "/sys/class/power_supply/BAT0/status";
constexpr const char* overrideDir = "/run/batteryctl";
constexpr const char* overridePath = "/run/batteryctl/manual-override";
constexpr const char* serviceName = "BatteryService.service";

void printUsage() {
    std::cout
        << "Usage: batteryctl [COMMAND]\n"
        << "\n"
        << "Commands:\n"
        << "  daemon           Run the battery policy daemon (default)\n"
        << "  monitor          Run the daemon in the foreground\n"
        << "  once             Evaluate and apply the policy once\n"
        << "  status           Show concise battery and daemon status\n"
        << "  auto             Set auto for the current charge session\n"
        << "  inhibit          Set inhibit-charge for the current charge session\n"
        << "  service status   Show the full systemd service status\n"
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

bool writeFile(const char* path, const std::string& content) {
    std::ofstream file(path, std::ios::trunc);
    if (!file.is_open()) {
        return false;
    }

    file << content;
    return !file.fail();
}

std::string activeChargeBehavior(const std::string& chargeBehavior) {
    const std::size_t start = chargeBehavior.find('[');
    const std::size_t end = chargeBehavior.find(']', start);
    if (start == std::string::npos || end == std::string::npos || end <= start + 1) {
        return chargeBehavior;
    }

    return chargeBehavior.substr(start + 1, end - start - 1);
}

ChargePolicy::Behavior behaviorFromSysfsValue(const std::string& value) {
    return value == ChargePolicy::toSysfsValue(ChargePolicy::Behavior::InhibitCharge)
               ? ChargePolicy::Behavior::InhibitCharge
               : ChargePolicy::Behavior::Auto;
}

ChargePolicy currentChargePolicy() {
    return ChargePolicy{behaviorFromSysfsValue(activeChargeBehavior(readFile(behaviorPath)))};
}

bool isChargeSessionActive(bool isBatteryCharging, bool isExternalPowerOnline) {
    return isBatteryCharging || isExternalPowerOnline;
}

std::string manualOverrideBehavior() {
    return readFile(overridePath);
}

bool setManualOverride(const std::string& behavior) {
    std::filesystem::create_directories(overrideDir);
    return writeFile(overridePath, behavior);
}

void clearManualOverride() {
    std::error_code error;
    std::filesystem::remove(overridePath, error);
}

int setChargeBehavior(const std::string& behavior) {
    ChargeBehaviorHandler chargeHandler{behaviorPath};
    return chargeHandler.writeChargeBehavior(behavior) ? 0 : 1;
}

int setManualChargeBehavior(const std::string& behavior) {
    UPowerBatteryInterface batteryMonitor;
    if (!isChargeSessionActive(batteryMonitor.isBatteryCharging(),
                               batteryMonitor.isExternalPowerOnline())) {
        qWarning() << "Manual charge behavior is only meaningful during an active charge session";
        return 2;
    }

    if (!setManualOverride(behavior)) {
        qWarning() << "Failed to write manual override state to" << overridePath;
        return 1;
    }

    const int result = setChargeBehavior(behavior);
    if (result != 0) {
        clearManualOverride();
        return result;
    }

    qInfo() << "Manual override active for this charge session:" << behavior.c_str();
    return 0;
}

int applyPolicyOnce() {
    UPowerBatteryInterface batteryMonitor;
    ChargePolicy chargePolicy = currentChargePolicy();
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
    const int serviceActive = runSystemctl({"is-active", "--quiet", serviceName});
    const std::string kernelStatus = readFile(batteryStatusPath);
    const std::string chargeBehavior = readFile(behaviorPath);
    const std::string manualOverride = manualOverrideBehavior();
    const bool batteryCharging = batteryMonitor.isBatteryCharging();
    const bool externalPowerOnline = batteryMonitor.isExternalPowerOnline();

    std::cout << "Service:       " << (serviceActive == 0 ? "active" : "inactive") << "\n";
    std::cout << "Battery:       " << batteryMonitor.getBatteryLevel() << "%, " << kernelStatus
              << "\n";
    std::cout << "External power:" << (externalPowerOnline ? " yes" : " no") << "\n";
    std::cout << "Battery charge:" << (batteryCharging ? " yes" : " no")
              << "\n";
    std::cout << "Behavior:      " << activeChargeBehavior(chargeBehavior) << "\n";
    std::cout << "Kernel values: " << chargeBehavior << "\n";
    if (!manualOverride.empty()) {
        std::cout << "Manual:        " << manualOverride << " for current charge session\n";
    }
    return 0;
}

int runDaemon(QCoreApplication& app) {
    double lastLevel{-1.0};

    UPowerBatteryInterface batteryMonitor;
    ChargeBehaviorHandler chargeHandler{behaviorPath};
    ChargePolicy chargePolicy = currentChargePolicy();
    bool manualOverrideLogged{false};

    auto controlChargeBehavior{[&chargeHandler](ChargePolicy::Behavior behavior) {
        const std::string targetBehavior = ChargePolicy::toSysfsValue(behavior);
        const std::string currentBehavior = activeChargeBehavior(readFile(behaviorPath));
        if (targetBehavior == currentBehavior) {
            return;
        }

        qInfo() << "Setting charge behavior to" << targetBehavior.c_str();
        chargeHandler.writeChargeBehavior(targetBehavior);
    }};

    auto updateLogic = [&](double currentLevel, bool force) {
        if (!force && currentLevel == lastLevel) {
            return;
        }

        qInfo() << "Checking state for battery level: " << currentLevel << "%";
        lastLevel = currentLevel;

        const bool isCharging = batteryMonitor.isBatteryCharging();
        const bool externalPowerOnline = batteryMonitor.isExternalPowerOnline();
        if (!manualOverrideBehavior().empty()) {
            if (isChargeSessionActive(isCharging, externalPowerOnline)) {
                if (!manualOverrideLogged) {
                    qInfo() << "Manual override active; automatic policy paused";
                    manualOverrideLogged = true;
                }
                return;
            }

            if (!manualOverrideLogged) {
                qInfo() << "Manual override pending until the next charge session";
                manualOverrideLogged = true;
            }
            return;
        }

        controlChargeBehavior(chargePolicy.evaluate(currentLevel, isCharging));
    };

    QObject::connect(&batteryMonitor, &UPowerBatteryInterface::batteryLevelChanged,
                     [&](double currentLevel) { updateLogic(currentLevel, false); });

    QObject::connect(&batteryMonitor, &UPowerBatteryInterface::batteryChargingChanged,
                     [&](bool) { updateLogic(batteryMonitor.getBatteryLevel(), true); });

    QObject::connect(&batteryMonitor, &UPowerBatteryInterface::externalPowerOnlineChanged,
                     [&](bool isOnline) {
        if (isOnline && !manualOverrideBehavior().empty()) {
            clearManualOverride();
            manualOverrideLogged = false;
            qInfo() << "Manual override cleared for new charge session";
            updateLogic(batteryMonitor.getBatteryLevel(), true);
        }
    });

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
        return setManualChargeBehavior("auto");
    }

    if (command == "inhibit") {
        return setManualChargeBehavior("inhibit-charge");
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
