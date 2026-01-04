#ifndef AUTH_MANAGER_H
#define AUTH_MANAGER_H

#include <string>
#include <memory>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <mutex>
#include "user_manager.h"

struct UserSession {
    std::string user_id;
    std::string token;
    std::chrono::system_clock::time_point expiry_time;
    bool is_active;
};

class AuthManager {
public:
    static AuthManager& getInstance();
    
    // 用户登录
    std::string login(const std::string& username, const std::string& password);
    
    // 验证用户token
    bool validateToken(const std::string& token);
    
    // 检查用户权限
    bool hasPermission(const std::string& token, const std::string& permission);
    
    // 用户登出
    void logout(const std::string& token);
    
    // 注册新用户
    bool registerUser(const std::string& username, const std::string& password, 
                     const std::vector<std::string>& permissions = {"data_collection"});
    
    // 验证许可证
    bool validateLicense(const std::string& license_key);
    
    // 获取所有可用的用户（用于演示目的）
    std::vector<std::string> getAllUsernames();

private:
    AuthManager() = default;
    UserManager& user_manager_ = UserManager::getInstance();
    std::unordered_map<std::string, UserSession> active_sessions_;
    std::mutex session_mutex_;
};

#endif // AUTH_MANAGER_H