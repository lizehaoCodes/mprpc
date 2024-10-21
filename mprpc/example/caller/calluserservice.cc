#include <iostream>
#include "mprpcapplication.h"
#include "user.pb.h"
#include "mprpcchannel.h"

int main(int argc, char **argv)
{
    // 1、初始化rpc框架(仅1次)
    MprpcApplication::Init(argc, argv);

    // 2、创建stub代理对象，调用远程rpc方法（1.做rpc方法调用的数据序列/反序列化  2.网络数据的收发）
    fixbug::UserServiceRpc_Stub stub(new MprpcChannel());

    // 3、设置调用rpc方法的请求参数
    fixbug::LoginRequest request;
    request.set_name("zhang san");
    request.set_pwd("123456");

    // 4、设置rpc远程处理后的响应
    fixbug::LoginResponse response;

    // 5、通过stub代理对象，调用远程rpc方法（底层由MprpcChannel::callmethod发起rpc调用）以同步阻塞的方式
    MprpcController controller;
    stub.Login(&controller, &request, &response, nullptr);

    // 6、读取rpc成功调用后的结果
    if (controller.Failed())
    {
        std::cout << controller.ErrorText() << std::endl;
    }
    else
    {
        if (0 == response.result().errcode())
        {
            std::cout << "rpc login response success:" << response.sucess() << std::endl;
        }
        else
        {
            std::cout << "rpc login response error : " << response.result().errmsg() << std::endl;
        }
    }

    // 演示2：远程调用rpc方法Register
    fixbug::RegisterRequest req;
    req.set_id(2000);
    req.set_name("mprpc");
    req.set_pwd("666666");
    fixbug::RegisterResponse rsp;

    // 通过stub代理对象，调用远程rpc方法（底层由MprpcChannel::callmethod发起rpc调用）以同步阻塞的方式
    stub.Register(nullptr, &req, &rsp, nullptr);

    // 读取rpc成功调用后的结果
    if (0 == rsp.result().errcode())
    {
        std::cout << "rpc register response success:" << rsp.sucess() << std::endl;
    }
    else
    {
        std::cout << "rpc register response error : " << rsp.result().errmsg() << std::endl;
    }

    return 0;
}