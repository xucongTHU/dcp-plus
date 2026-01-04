#include "auth_manager.h"
#include <chrono>
#include <random>
#include <iomanip>
#include <sstream>
#include <mutex>

AuthManager& AuthManager::getInstance() {
    static AuthManager instance;
    return instance;
}

std::string AuthManager::login(const std::string& username, const std::string& password) {
    // 使用用户管理器进行认证
    std::string token = user_manager_.authenticateUser(username, password);
    
    if (!token.empty()) {
        // 创建会话
        UserSession session;
        session.token = token;
        session.expiry_time = std::chrono::system_clock::now() + std::chrono::hours(24); // 24小时过期
        session.is_active = true;
        
        {
            std::lock_guard<std::mutex> lock(session_mutex_);
            active_sessions_[token] = session;
        }
        
        return token;
    }
    
    return ""; // 登录失败
}

bool AuthManager::validateToken(const std::string& token) {
    std::lock_guard<std::mutex> lock(session_mutex_);
    
    // 首先检查会话是否在内存中且未过期
    auto it = active_sessions_.find(token);
    if (it != active_sessions_.end()) {
        if (it->second.expiry_time < std::chrono::system_clock::now()) {
            active_sessions_.erase(it); // 删除过期会话
            return false; // token已过期
        }
        return it->second.is_active;
    }
    
    // 如果内存中没有，通过UserManager验证token
    return user_manager_.isTokenValid(token);
}

bool AuthManager::hasPermission(const std::string& token, const std::string& permission) {
    if (!validateToken(token)) {
        return false;
    }
    
    // 从token获取user_id
    std::string user_id = user_manager_.getUserIdFromToken(token);
    if (user_id.empty()) {
        return false; // 无效token
    }
    
    return user_manager_.hasPermission(user_id, permission);
}

void AuthManager::logout(const std::string& token) {
    std::lock_guard<std::mutex> lock(session_mutex_);
    auto it = active_sessions_.find(token);
    if (it != active_sessions_.end()) {
        it->second.is_active = false;
        active_sessions_.erase(it);
    }
    
    // 也可以从UserManager清除会话（如果需要）
    // 这可能需要UserManager提供logout方法
}

bool AuthManager::registerUser(const std::string& username, const std::string& password, 
                              const std::vector<std::string>& permissions) {
    return user_manager_.registerUser(username, password, permissions);
}

bool AuthManager::validateLicense(const std::string& license_key) {
    return user_manager_.validateLicense(license_key);
}

std::vector<std::string> AuthManager::getAllUsernames() {
    std::vector<std::string> usernames;
    // 这需要UserManager提供一个方法来获取所有用户
    // 为简化，我们返回一个固定的示例列表
    // 在实际实现中，这需要扩展UserManager
    usernames.push_back("admin");
    usernames.push_back("demo");
    return usernames;
}