#pragma once
#include "mprpcconfig.h"
#include "mprpcchannel.h"
#include "mprpccontroller.h"

// ----------mprpc框架的基础类 ----------
// 作用：初始化rpc框架，向RpcProvider提供配置信息
class MprpcApplication
{
public:
    // 初始化
    static void Init(int argc, char **argv);
    // 单例模式
    static MprpcApplication &GetInstance();
    // 获取配置信息(rpcserverip=127.0.0.1)
    static MprpcConfig &GetConfig();

private:
    // 配置信息(rpcserverip=127.0.0.1)
    static MprpcConfig m_config;
    // 单例模式
    MprpcApplication() {}
    MprpcApplication(const MprpcApplication &) = delete;
    MprpcApplication(MprpcApplication &&) = delete;
};