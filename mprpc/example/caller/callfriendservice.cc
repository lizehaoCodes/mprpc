#include <iostream>
#include "mprpcapplication.h"
#include "friend.pb.h"

int main(int argc, char **argv)
{
    // 1、初始化rpc框架(仅1次)
    MprpcApplication::Init(argc, argv);

    // 2、创建stub代理对象，调用远程rpc方法（1.做rpc方法调用的数据序列/反序列化  2.网络数据的收发）
    fixbug::FiendServiceRpc_Stub stub(new MprpcChannel());

    // 3、设置调用rpc方法的请求参数
    fixbug::GetFriendsListRequest request;
    request.set_userid(1000);

    // 4、设置rpc远程处理后的响应
    fixbug::GetFriendsListResponse response;

    // 5、通过stub代理对象，调用远程rpc方法（底层由MprpcChannel::callmethod发起rpc调用）以同步阻塞的方式
    MprpcController controller;
    stub.GetFriendsList(&controller, &request, &response, nullptr); // RpcChannel->RpcChannel::callMethod 集中来做所有rpc方法调用的参数序列化和网络发送

    // 6、读取rpc成功调用后的结果
    if (controller.Failed())
    {
        std::cout << controller.ErrorText() << std::endl;
    }
    else
    {
        // 读取好友信息列表
        if (0 == response.result().errcode())
        {
            std::cout << "rpc GetFriendsList response success!" << std::endl;
            int size = response.friends_size();
            for (int i = 0; i < size; ++i)
            {
                std::cout << "index:" << (i + 1) << " name:" << response.friends(i) << std::endl;
            }
        }
        else
        {
            std::cout << "rpc GetFriendsList response error : " << response.result().errmsg() << std::endl;
        }
    }

    return 0;
}