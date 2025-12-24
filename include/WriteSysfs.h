#ifndef WRITESYSFS_H
#define WRITESYSFS_H

#include <string>
#include <string_view>

/**
 * @brief Handles reading from and writing to the /sys/class/power_supply/BAT0/charge_behavior file.
 */
class ChargeBehaviorHandler {
  private:
    std::string m_behaviorPath;

    /**
     * @brief Checks if the sysfs file is writable upon instantiation.
     */
    void checkPermissions();

  public:
    ChargeBehaviorHandler(std::string_view behaviorPath);

    /**
     * @brief Writes a new value to the charge_behavior sysfs file.
     * @param value The string value to write.
     * @return True if the write operation was successful, false otherwise.
     */
    bool writeChargeBehavior(const std::string& value);
};

#endif // WRITESYSFS_H
