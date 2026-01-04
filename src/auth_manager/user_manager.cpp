#include "user_manager.h"
#include <chrono>
#include <random>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <cctype>

UserManager& UserManager::getInstance() {
    static UserManager instance;
    
    // 初始化默认用户
    if (instance.users_.empty()) {
        // 创建默认用户
        User default_user;
        default_user.id = "admin001";
        default_user.username = "admin";
        default_user.password_hash = instance.hashPassword("admin123");
        default_user.permissions = {"data_collection", "advanced_analytics", "system_admin"};
        default_user.is_active = true;
        default_user.license_key = "EDGE_INSIGHT-LICENSE-KEY-1226";
        
        // 设置许可证过期日期
        auto now = std::chrono::system_clock::now();
        auto expiry = now + std::chrono::hours(24 * 365); // 365天
        std::time_t expiry_time_t = std::chrono::system_clock::to_time_t(expiry);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&expiry_time_t), "%Y-%m-%d");
        default_user.expiry_date = ss.str();
        
        instance.users_[default_user.id] = default_user;
    }
    
    return instance;
}

bool UserManager::registerUser(const std::string& username, const std::string& password, 
                              const std::vector<std::string>& permissions) {
    std::lock_guard<std::mutex> lock(user_mutex_);
    
    // 检查用户名是否已存在
    for (const auto& user : users_) {
        if (user.second.username == username) {
            return false; // 用户名已存在
        }
    }
    
    // 创建新用户
    User user;
    user.id = generateToken().substr(0, 8); // 使用token生成器创建用户ID
    user.username = username;
    user.password_hash = hashPassword(password);
    user.permissions = permissions;
    user.is_active = true;
    user.license_key = generateLicenseKey();
    
    // 设置许可证过期日期（例如30天后）
    auto now = std::chrono::system_clock::now();
    auto expiry = now + std::chrono::hours(24 * 30); // 30天
    std::time_t expiry_time_t = std::chrono::system_clock::to_time_t(expiry);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&expiry_time_t), "%Y-%m-%d");
    user.expiry_date = ss.str();
    
    users_[user.id] = user;
    return true;
}

std::string UserManager::authenticateUser(const std::string& username, const std::string& password) {
    std::lock_guard<std::mutex> lock(user_mutex_);

    // 查找用户
    for (const auto& user_pair : users_) {
        const User& user = user_pair.second;
        if (user.username == username && user.password_hash == hashPassword(password) && user.is_active) {
            // 验证许可证是否过期
            // if (isLicenseExpired(user.id)) {
            //     std::cout << "============================4\n";
            //     std::cout << "User license has expired." << std::endl;
            //     return ""; // 许可证已过期
            // }
            
            // 生成会话token
            std::string token = generateToken();
            active_sessions_[token] = user.id;
            
            // 设置token过期时间
            token_expiry_[token] = std::chrono::system_clock::now() + std::chrono::hours(24); // 24小时
            
            return token;
        }
    }
    
    // std::cout << "Authentication failed: invalid username or password." << std::endl;
    return ""; // 认证失败
}

bool UserManager::validateLicense(const std::string& license_key) {
    std::lock_guard<std::mutex> lock(user_mutex_);
    
    for (const auto& user_pair : users_) {
        if (user_pair.second.license_key == license_key) {
            return !isLicenseExpired(user_pair.first);
        }
    }
    
    return false;
}

bool UserManager::hasPermission(const std::string& user_id, const std::string& permission) {
    std::lock_guard<std::mutex> lock(user_mutex_);
    
    auto it = users_.find(user_id);
    if (it == users_.end()) {
        return false;
    }
    
    const User& user = it->second;
    return std::find(user.permissions.begin(), user.permissions.end(), permission) != user.permissions.end();
}

std::shared_ptr<User> UserManager::getUser(const std::string& user_id) {
    std::lock_guard<std::mutex> lock(user_mutex_);
    
    auto it = users_.find(user_id);
    if (it != users_.end()) {
        return std::make_shared<User>(it->second);
    }
    
    return nullptr;
}

bool UserManager::isLicenseExpired(const std::string& user_id) {
    std::lock_guard<std::mutex> lock(user_mutex_);
    
    auto it = users_.find(user_id);
    if (it == users_.end()) {
        return true; // 用户不存在，视为过期
    }
    
    const User& user = it->second;
    
    // 解析过期日期
    int year, month, day;
    char dash1, dash2;
    std::stringstream ss(user.expiry_date);
    ss >> year >> dash1 >> month >> dash2 >> day;
    
    // 创建过期时间点
    std::tm tm = {};
    tm.tm_year = year - 1900;
    tm.tm_mon = month - 1;
    tm.tm_mday = day;
    std::time_t expiry_time_t = std::mktime(&tm);
    std::chrono::system_clock::time_point expiry = 
        std::chrono::system_clock::from_time_t(expiry_time_t);
    
    // 检查是否已过期
    bool expired = std::chrono::system_clock::now() > expiry;
    if (expired) {
        std::cout << "License expired for user: " << user.username << std::endl;
    }
    return expired;
}

std::string UserManager::getUserIdFromToken(const std::string& token) {
    std::lock_guard<std::mutex> lock(user_mutex_);
    
    // 检查token是否过期
    auto expiry_it = token_expiry_.find(token);
    if (expiry_it != token_expiry_.end()) {
        if (expiry_it->second < std::chrono::system_clock::now()) {
            // Token已过期，清理相关记录
            active_sessions_.erase(token);
            token_expiry_.erase(expiry_it);
            std::cout << "Token expired: " << token << std::endl;
            return ""; // Token过期
        }
    } else {
        // std::cout << "Token not found in system: " << token << std::endl;
        return ""; // Token不存在
    }
    
    auto it = active_sessions_.find(token);
    if (it != active_sessions_.end()) {
        return it->second;
    }
    
    return ""; // Token不存在
}

bool UserManager::isTokenValid(const std::string& token) {
    return !getUserIdFromToken(token).empty();
}

std::vector<std::string> UserManager::getAllUsernames() {
    std::lock_guard<std::mutex> lock(user_mutex_);
    std::vector<std::string> usernames;
    
    for (const auto& user_pair : users_) {
        usernames.push_back(user_pair.second.username);
    }
    
    return usernames;
}

std::string UserManager::hashPassword(const std::string& password) {
    // 这里应该使用安全的哈希算法，如bcrypt或SHA-256
    // 为演示目的，我们使用简单的哈希
    // 在实际应用中，请使用适当的密码哈希算法
    std::string hash = password; // 实际应用中应使用安全哈希
    std::transform(hash.begin(), hash.end(), hash.begin(), ::toupper);
    return hash + "_HASHED";
}

std::string UserManager::generateToken() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    
    std::stringstream token_ss;
    for (int i = 0; i < 32; ++i) {
        token_ss << std::hex << dis(gen);
    }

    return token_ss.str();
}

std::string UserManager::generateLicenseKey() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    
    std::stringstream key_ss;
    for (int i = 0; i < 16; ++i) {
        if (i > 0 && i % 4 == 0) {
            key_ss << "-";
        }
        key_ss << std::hex << std::uppercase << dis(gen);
    }
    
    return key_ss.str();
}