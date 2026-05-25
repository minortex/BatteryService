#ifndef CHARGEPOLICY_H
#define CHARGEPOLICY_H

class ChargePolicy {
  public:
    enum class Behavior {
        Auto,
        InhibitCharge,
    };

    Behavior evaluate(double batteryLevel, bool isBatteryCharging) {
        if (isBatteryCharging && batteryLevel >= inhibitThreshold) {
            m_shouldInhibitCharging = true;
        } else if (batteryLevel <= resumeThreshold) {
            m_shouldInhibitCharging = false;
        }

        return currentBehavior();
    }

    Behavior currentBehavior() const {
        return m_shouldInhibitCharging ? Behavior::InhibitCharge : Behavior::Auto;
    }

    static const char* toSysfsValue(Behavior behavior) {
        return behavior == Behavior::InhibitCharge ? "inhibit-charge" : "auto";
    }

    static constexpr double inhibitThreshold{80.0};
    static constexpr double resumeThreshold{75.0};

  private:
    bool m_shouldInhibitCharging{false};
};

#endif // CHARGEPOLICY_H
