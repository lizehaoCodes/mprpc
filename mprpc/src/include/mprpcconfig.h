#pragma once
#include <unordered_map>
#include <string>

// -----mprpc框架的配置文件类-----
// 作用：读取配置信息(rpcserverip=127.0.0.1)
class MprpcConfig
{
public:
    // 读取配置文件(rpcServerIp = 127.0.0.1\n)
    void LoadConfigFile(const char *config_file);
    // 返回配置信息(value:127.0.0.1)
    std::string Load(const std::string &key);

private:
    // 存储配置信息(rpcServerIp = 127.0.0.1\n)
    std::unordered_map<std::string, std::string> m_configMap;
    // 去掉字符串前后的空格
    void Trim(std::string &src_buf);
};