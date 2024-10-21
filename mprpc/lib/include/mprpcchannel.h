#pragma once

#include <google/protobuf/service.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>

// rpc方法调用方：通过stub代理对象,调用远程的rpc方法，（1.做rpc方法调用的数据序列/反序列化  2.网络数据的收发）
// 自定义MprpcChannel,重写RpcChannel::CallMethod抽象方法,通过CallMethod方法调用远程rpc节点上的对应业务函数
class MprpcChannel : public google::protobuf::RpcChannel
{
public:
    void CallMethod(const google::protobuf::MethodDescriptor *method,
                    google::protobuf::RpcController *controller,
                    const google::protobuf::Message *request,
                    google::protobuf::Message *response,
                    google::protobuf::Closure *done);
};