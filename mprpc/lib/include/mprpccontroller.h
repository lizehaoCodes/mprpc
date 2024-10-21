#pragma once
#include <google/protobuf/service.h>
#include <string>

// 作用：记录rpc远程调用中，成功/失败的状态信息
class MprpcController : public google::protobuf::RpcController
{
public:
    MprpcController();
    // 重置成员变量
    void Reset();
    // 判断调用成功与否
    bool Failed() const;
    // 返回错误信息
    std::string ErrorText() const;
    // 设置错误状态及错误信息
    void SetFailed(const std::string &reason);

    // 目前未实现具体的功能
    void StartCancel();
    bool IsCanceled() const;
    void NotifyOnCancel(google::protobuf::Closure *callback);

private:
    bool m_failed;         // 错误状态
    std::string m_errText; // 错误信息
};