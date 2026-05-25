#include "ChargePolicy.h"

#include <iostream>
#include <string_view>

namespace {

int failures = 0;

const char* behaviorName(ChargePolicy::Behavior behavior) {
    return behavior == ChargePolicy::Behavior::InhibitCharge ? "inhibit-charge" : "auto";
}

void expectBehavior(std::string_view name, ChargePolicy::Behavior actual,
                    ChargePolicy::Behavior expected) {
    if (actual == expected) {
        return;
    }

    ++failures;
    std::cerr << name << ": expected " << behaviorName(expected) << ", got "
              << behaviorName(actual) << '\n';
}

void expectString(std::string_view name, std::string_view actual, std::string_view expected) {
    if (actual == expected) {
        return;
    }

    ++failures;
    std::cerr << name << ": expected " << expected << ", got " << actual << '\n';
}

void chargingAt80Inhibits() {
    ChargePolicy policy;

    expectBehavior("charging below 80 starts auto", policy.evaluate(79.0, true),
                   ChargePolicy::Behavior::Auto);
    expectBehavior("charging at 80 inhibits", policy.evaluate(80.0, true),
                   ChargePolicy::Behavior::InhibitCharge);
}

void chargingAbove80KeepsInhibited() {
    ChargePolicy policy;

    expectBehavior("charging at 80 inhibits", policy.evaluate(80.0, true),
                   ChargePolicy::Behavior::InhibitCharge);
    expectBehavior("charging above 80 stays inhibited", policy.evaluate(85.0, true),
                   ChargePolicy::Behavior::InhibitCharge);
}

void inhibitedStateUsesResumeThreshold() {
    ChargePolicy policy;

    expectBehavior("charging at 80 inhibits", policy.evaluate(80.0, true),
                   ChargePolicy::Behavior::InhibitCharge);
    expectBehavior("79 keeps inhibited until resume threshold", policy.evaluate(79.0, false),
                   ChargePolicy::Behavior::InhibitCharge);
    expectBehavior("76 keeps inhibited until resume threshold", policy.evaluate(76.0, false),
                   ChargePolicy::Behavior::InhibitCharge);
    expectBehavior("75 resumes auto", policy.evaluate(75.0, false), ChargePolicy::Behavior::Auto);
}

void startupStateDependsOnBatteryChargingState() {
    {
        ChargePolicy policy;
        expectBehavior("startup charging above 80 inhibits", policy.evaluate(81.0, true),
                       ChargePolicy::Behavior::InhibitCharge);
    }

    {
        ChargePolicy policy;
        expectBehavior("startup charging below 80 starts auto", policy.evaluate(79.0, true),
                       ChargePolicy::Behavior::Auto);
    }

    {
        ChargePolicy policy;
        expectBehavior("startup not charging above 80 starts auto", policy.evaluate(81.0, false),
                       ChargePolicy::Behavior::Auto);
    }
}

void sysfsValuesMatchKernelInterface() {
    expectString("auto sysfs value", ChargePolicy::toSysfsValue(ChargePolicy::Behavior::Auto),
                 "auto");
    expectString("inhibit sysfs value",
                 ChargePolicy::toSysfsValue(ChargePolicy::Behavior::InhibitCharge),
                 "inhibit-charge");
}

} // namespace

int main() {
    chargingAt80Inhibits();
    chargingAbove80KeepsInhibited();
    inhibitedStateUsesResumeThreshold();
    startupStateDependsOnBatteryChargingState();
    sysfsValuesMatchKernelInterface();

    if (failures != 0) {
        std::cerr << failures << " ChargePolicy test(s) failed\n";
        return 1;
    }

    return 0;
}
