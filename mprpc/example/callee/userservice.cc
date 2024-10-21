#include <iostream>
#include <string>
#include "user.pb.h"
#include "mprpcapplication.h"
#include "rpcprovider.h"

/*
------定义派生类，重写rpc服务基类（UserServiceRpc）中的虚函数，实现特定功能------
角色：RpcProvider的使用者，rpc服务方法的发布方
作用：提供本地业务
*/
/*
通过rpc框架调用本地方法：
caller  ->  Login(LoginRequest)   ->   muduo网络层   ->   callee  -> 调用重写的Login(LoginRequest)   |
caller  <-     muduo网络层    <-   调用重写的Login(LoginResponse)      <-    callee      <-          V
*/
class UserService : public fixbug::UserServiceRpc
{
public:
    // 本地方法
    bool Login(std::string name, std::string pwd)
    {
        std::cout << "doing local service: Login" << std::endl;
        std::cout << "name:" << name << " pwd:" << pwd << std::endl;
        return false;
    }

    bool Register(uint32_t id, std::string name, std::string pwd)
    {
        std::cout << "doing local service: Register" << std::endl;
        std::cout << "id:" << id << "name:" << name << " pwd:" << pwd << std::endl;
        return true;
    }

    // 重写基类方法，将本地方法发布成rpc方法
    void Login(::google::protobuf::RpcController *controller,
               const ::fixbug::LoginRequest *request,
               ::fixbug::LoginResponse *response,
               ::google::protobuf::Closure *done)
    {
        // 1、从LoginRequest获取参数的值
        std::string name = request->name();
        std::string pwd = request->pwd();

        // 2、执行本地Login业务，并获取返回值
        bool login_result = Login(name, pwd);

        // 3、将返回值写入LoginResponse
        fixbug::ResultCode *code = response->mutable_result();
        code->set_errcode(0);
        code->set_errmsg("");
        response->set_sucess(login_result);

        // 4、执行回调, 序列化LoginResponse通过网络模块，发送给rpc服务的调用方(Client)
        done->Run();
    }

    void Register(::google::protobuf::RpcController *controller,
                  const ::fixbug::RegisterRequest *request,
                  ::fixbug::RegisterResponse *response,
                  ::google::protobuf::Closure *done)
    {
        // 1、从RegisterRequest获取参数的值
        uint32_t id = request->id();
        std::string name = request->name();
        std::string pwd = request->pwd();

        // 2、执行本地Register业务，并获取返回值
        bool ret = Register(id, name, pwd);

        // 3、将返回值写入RegisterResponse
        response->mutable_result()->set_errcode(0);
        response->mutable_result()->set_errmsg("");
        response->set_sucess(ret);

        // 4、执行回调, 序列化RegisterResponse序列化给网络模块，发送给rpc服务的调用方(Client)
        done->Run();
    }
};

int main(int argc, char **argv)
{
    // 1、初始化rpc框架
    MprpcApplication::Init(argc, argv);

    // 2、RpcProvider(rpc服务提供者）：把本地Service服务，发布成远程rpc服务
    RpcProvider provider;
    provider.NotifyService(new UserService());

    // 3、启动一个rpc服务发布节点，进程进入阻塞状态，等待远端的rpc调用请求
    provider.Run();

    return 0;
}