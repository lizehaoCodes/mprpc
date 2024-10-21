#include "mprpcconfig.h"
#include <iostream>
#include <string>

// 读取配置文件(rpcServerIp = 127.0.0.1\n)
void MprpcConfig::LoadConfigFile(const char *config_file)
{
    FILE *pf = fopen(config_file, "r");
    if (nullptr == pf)
    {
        std::cout << config_file << " is note exist!" << std::endl;
        exit(EXIT_FAILURE);
    }
    while (!feof(pf))
    {
        char buf[512] = {0};
        fgets(buf, 512, pf);

        // 1.去掉字符串前后多余的空格
        std::string read_buf(buf);
        Trim(read_buf);
        // 2.判断#的注释和空格
        if (read_buf[0] == '#' || read_buf.empty())
        {
            continue;
        }
        // 3.获取配置项(rpcServerIp = 127.0.0.1\n)
        int idx = read_buf.find('=');
        if (idx == -1)
        {
            continue;
        }
        std::string key;
        std::string value;
        // 3.1 截取键(rpcserverip)
        key = read_buf.substr(0, idx);
        Trim(key);
        // 3.2 找到字符串末尾\n
        int endidx = read_buf.find('\n', idx);
        // 3.3 截取值(127.0.0.1)
        value = read_buf.substr(idx + 1, endidx - idx - 1);
        Trim(value); // 去除值前后空格

        m_configMap.insert({key, value});
    }
    fclose(pf);
}

// 返回配置信息（value:127.0.0.1）
std::string MprpcConfig::Load(const std::string &key)
{
    auto it = m_configMap.find(key);
    if (it == m_configMap.end())
    {
        return "";
    }
    return it->second;
}

// 去掉字符串前后的空格
void MprpcConfig::Trim(std::string &src_buf)
{
    int idx = src_buf.find_first_not_of(' ');
    if (idx != -1)
    {
        // 字符串前面有空格
        src_buf = src_buf.substr(idx, src_buf.size() - idx);
    }
    // 去掉字符串后面多余的空格
    idx = src_buf.find_last_not_of(' ');
    if (idx != -1)
    {
        // 字符串后面有空格
        src_buf = src_buf.substr(0, idx + 1);
    }
}