#ifndef USER_MANAGER_H
#define USER_MANAGER_H

#include <string>
#include <memory>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <iostream>

struct User {
    std::string id;
    std::string username;
    std::string password_hash;  // 密码哈希
    std::vector<std::string> permissions;
    bool is_active;
    std::string license_key;
    std::string expiry_date;  // 许可证过期日期
};

class UserManager {
public:
    static UserManager& getInstance();
    
    // 注册新用户
    bool registerUser(const std::string& username, const std::string& password, 
                     const std::vector<std::string>& permissions = {"data_collection"});
    
    // 验证用户凭据
    std::string authenticateUser(const std::string& username, const std::string& password);
    
    // 验证许可证密钥
    bool validateLicense(const std::string& license_key);
    
    // 检查用户权限
    bool hasPermission(const std::string& user_id, const std::string& permission);
    
    // 获取用户信息
    std::shared_ptr<User> getUser(const std::string& user_id);
    
    // 检查许可证是否过期
    bool isLicenseExpired(const std::string& user_id);
    
    // 从token获取用户ID（新增）
    std::string getUserIdFromToken(const std::string& token);
    
    // 验证token是否有效（新增）
    bool isTokenValid(const std::string& token);
    
    // 获取所有用户名（新增）
    std::vector<std::string> getAllUsernames();

private:
    UserManager() = default;
    std::unordered_map<std::string, User> users_;
    std::unordered_map<std::string, std::string> active_sessions_; // token -> user_id
    std::unordered_map<std::string, std::chrono::system_clock::time_point> token_expiry_; // token -> expiry time
    std::mutex user_mutex_;
    
    // 内部辅助函数
    std::string hashPassword(const std::string& password);
    std::string generateToken();
    std::string generateLicenseKey();
};

#endif // USER_MANAGER_H