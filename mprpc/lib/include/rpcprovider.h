#pragma once
#include "google/protobuf/service.h"
#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/InetAddress.h>
#include <muduo/net/TcpConnection.h>
#include <string>
#include <functional>
#include <google/protobuf/descriptor.h>
#include <unordered_map>

//---------提供rpc远程调用服务---------
// 服务对象:callee
// 作用:1. 网络上处理rpc请求的收发 (muduo库:)
//      2. protobuf:数据的序列\反序列化
class RpcProvider
{
public:
    // 提供rpc本地节点上，*service的所有信息
    void NotifyService(google::protobuf::Service *service); // 基类指针
    // 向远端提供相应的rpc服务
    void Run();

private:
    muduo::net::EventLoop m_eventLoop;

    // *service信息
    struct ServiceInfo
    {
        // *service对象
        google::protobuf::Service *m_service;
        // *service对象的方法描述
        std::unordered_map<std::string, const google::protobuf::MethodDescriptor *> m_methodMap;
    };
    // *service的所有信息
    std::unordered_map<std::string, ServiceInfo> m_serviceMap;

    // 连接回调
    void OnConnection(const muduo::net::TcpConnectionPtr &);
    // 处理远端发出的rpc读写事件回调
    void OnMessage(const muduo::net::TcpConnectionPtr &, muduo::net::Buffer *, muduo::Timestamp);

    // 本地rpc节点的处理回调（序列化rpc响应和网络发送）
    void SendRpcResponse(const muduo::net::TcpConnectionPtr &, google::protobuf::Message *);
};