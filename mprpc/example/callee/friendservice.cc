#include <iostream>
#include <string>
#include "friend.pb.h"
#include "mprpcapplication.h"
#include "rpcprovider.h"
#include <vector>
#include "logger.h"

class FriendService : public fixbug::FiendServiceRpc
{
public:
    // 本地方法
    std::vector<std::string> GetFriendsList(uint32_t userid)
    {
        std::cout << "do GetFriendsList service! userid:" << userid << std::endl;
        std::vector<std::string> vec;
        vec.push_back("li ze hao");
        vec.push_back("zhang san");
        vec.push_back("li si");
        return vec;
    }

    // 重写基类方法，发布成rpc方法
    void GetFriendsList(::google::protobuf::RpcController *controller,
                        const ::fixbug::GetFriendsListRequest *request,
                        ::fixbug::GetFriendsListResponse *response,
                        ::google::protobuf::Closure *done)
    {
        // 1、从LoginRequest获取参数的值
        uint32_t userid = request->userid();

        // 2、执行本地GetFriendsList业务，获取好友列表
        std::vector<std::string> friendsList = GetFriendsList(userid);

        // 3、将返回值（好友列表）写入GetFriendsListResponse
        response->mutable_result()->set_errcode(0);
        response->mutable_result()->set_errmsg("");
        for (std::string &name : friendsList)
        {
            std::string *p = response->add_friends();
            *p = name;
        }
        // 4、执行回调, 序列化LoginResponse通过网络模块，发送给rpc服务的调用方(Client)
        done->Run();
    }
};

int main(int argc, char **argv)
{
    // 记录日志
    LOG_INFO("first log message!");
    LOG_ERR("%s:%s:%d", __FILE__, __FUNCTION__, __LINE__);

    // 1、初始化rpc框架
    MprpcApplication::Init(argc, argv);

    // 2、RpcProvider(rpc服务提供者）：把本地Service服务，发布成远程rpc服务
    RpcProvider provider;
    provider.NotifyService(new FriendService());

    // 3、启动一个rpc服务发布节点，进程进入阻塞状态，等待远端的rpc调用请求
    provider.Run();

    return 0;
}