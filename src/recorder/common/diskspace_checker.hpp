#ifndef DISK_SPACE_CHECKER_HPP
#define DISK_SPACE_CHECKER_HPP

#include <string>
#include <stdexcept>
#include <sys/statvfs.h>

class DiskSpaceChecker {
public:
    DiskSpaceChecker(double thresholdPercent = 80.0): m_thresholdPercent(thresholdPercent) {
        if (thresholdPercent < 0 || thresholdPercent > 100) {
            throw std::invalid_argument("Threshold must be between 0 and 100");
        }
    }

    void setThreshold(double thresholdPercent) {
        if (thresholdPercent < 0 || thresholdPercent > 100) {
            throw std::invalid_argument("Threshold must be between 0 and 100");
        }
        m_thresholdPercent = thresholdPercent;
    }

    double getThreshold() const {
        return m_thresholdPercent;
    }

    bool isOverThreshold(const std::string& path) const {
        double usage = getUsagePercentage(path);
        return usage >= m_thresholdPercent;
    }

    double getUsagePercentage(const std::string& path) const {
        unsigned long long total, free;
        if (!getDiskSpace(path, total, free)) {
            throw std::runtime_error("Failed to get disk space");
        }
        if (total == 0) return 0.0;
        return 100.0 - (static_cast<double>(free) / total * 100.0);
    }

    double estimateUsageAfterWrite(const std::string& path, unsigned long long dataSizeBytes) const {
        unsigned long long total, free;
        if (!getDiskSpace(path, total, free)) {
            throw std::runtime_error("Failed to get disk space");
        }
        if (total == 0) return 0.0;
        if (dataSizeBytes >= free) {
            return 100.0;
        }
        return static_cast<double>(total - free + dataSizeBytes) / total * 100.0;
    }

    bool willExceedThresholdAfterWrite(const std::string& path, unsigned long long dataSizeBytes) const {
        double estimatedUsage = estimateUsageAfterWrite(path, dataSizeBytes);
        return estimatedUsage >= m_thresholdPercent;
    }

    bool getDiskSpace(const std::string& path, unsigned long long& totalBytes, unsigned long long& freeBytes) const {
        struct statvfs vfs;
        if (statvfs(path.c_str(), &vfs) != 0) {
            return false;
        }
        totalBytes = static_cast<unsigned long long>(vfs.f_frsize) * vfs.f_blocks;
        freeBytes = static_cast<unsigned long long>(vfs.f_frsize) * vfs.f_bavail;
        return true;
    }

private:
    double m_thresholdPercent;
};

#endif // DISK_SPACE_CHECKER_HPP

