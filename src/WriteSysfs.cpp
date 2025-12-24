#include "WriteSysfs.h"
#include <QDebug>
#include <cerrno>
#include <cstring>
#include <fstream>
#include <qlogging.h>

#include <cstdlib> // For exit()

ChargeBehaviorHandler::ChargeBehaviorHandler(std::string_view behaviorPath)
    : m_behaviorPath(behaviorPath) {
    checkPermissions();
}

void ChargeBehaviorHandler::checkPermissions() {
    std::ofstream file(m_behaviorPath, std::ios::app); // Open in append mode to check writability
    if (!file.is_open()) {
        qCritical(
            "Error: Cannot open '%s' for writing: %s. Please run with sufficient privileges (e.g., sudo).",
            m_behaviorPath.c_str(), strerror(errno));
        exit(1);
    }
    // The file is immediately closed as we just wanted to check for write access.
    file.close();
}

bool ChargeBehaviorHandler::writeChargeBehavior(const std::string& value) {
    std::ofstream file(m_behaviorPath, std::ios::trunc);
    if (file.is_open()) {
        file << value;
        file.close();
        if (file.fail()) {
            qWarning() << "Failed to write to" << m_behaviorPath.c_str() << ":" << strerror(errno);
            return false;
        }
        return true;
    } else {
        // This should theoretically not be reached due to the constructor check, but is good
        // practice.
        qWarning() << "Failed to open" << m_behaviorPath.c_str()
                   << "for writing:" << strerror(errno);
        return false;
    }
}