#include "mprpcchannel.h"
#include <string>
#include "rpcheader.pb.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include "mprpcapplication.h"
#include "mprpccontroller.h"
#include "zookeeperutil.h"

/*
数据头:16UserServiceLogin?zhangsan123456
数据头格式:   header_size(int)    +    header_str     +      args_str
                          (service_name  method_name  args_size)
*/
// ----------通过User-stub代理对象，进行rpc远程方法调用-----------
// 服务对象: caller
// 作用: 1.序列化rpc调用请求参数
//       2.通过网络发送/接受数据
void MprpcChannel::CallMethod(const google::protobuf::MethodDescriptor *method,
                              google::protobuf::RpcController *controller,
                              const google::protobuf::Message *request,
                              google::protobuf::Message *response,
                              google::protobuf::Closure *done)
{
    // --------------------步骤一：序列化rpc请求参数-------------------
    const google::protobuf::ServiceDescriptor *sd = method->service();

    // 1、读取要调用的远端service_name
    std::string service_name = sd->name();

    // 2、读取要调用的远端method_name
    std::string method_name = method->name();

    // 3、读取要调用的远端args_size、args_str
    uint32_t args_size = 0;
    std::string args_str;
    if (request->SerializeToString(&args_str))
    {
        args_size = args_str.size();
    }
    else
    {
        controller->SetFailed("serialize request error!");
        return;
    }

    // 4、设置向远端发送rpc请求的数据头
    mprpc::RpcHeader rpcHeader;
    rpcHeader.set_service_name(service_name);
    rpcHeader.set_method_name(method_name);
    rpcHeader.set_args_size(args_size);

    // 5、读取header_size、rpc_header_str
    uint32_t header_size = 0;
    std::string rpc_header_str;
    if (rpcHeader.SerializeToString(&rpc_header_str))
    {
        header_size = rpc_header_str.size();
    }
    else
    {
        controller->SetFailed("serialize rpc header error!");
        return;
    }

    // 6、组织待发送的数据头
    std::string send_rpc_str;
    send_rpc_str.insert(0, std::string((char *)&header_size, 4)); // 插入header_size(在字符串前插入4个字节(int))
    send_rpc_str += rpc_header_str;                               // 插入header_str
    send_rpc_str += args_str;                                     // 插入args_str

    // 7、输出数据头信息
    std::cout << "============================================" << std::endl;
    std::cout << "header_size: " << header_size << std::endl;
    std::cout << "rpc_header_str: " << rpc_header_str << std::endl;
    std::cout << "service_name: " << service_name << std::endl;
    std::cout << "method_name: " << method_name << std::endl;
    std::cout << "args_str: " << args_str << std::endl;
    std::cout << "============================================" << std::endl;

    // ------------------步骤二：通过网络发送/接受数据-----------------
    // 1、socket
    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == clientfd)
    {
        char errtxt[512] = {0};
        sprintf(errtxt, "create socket error! errno:%d", errno);
        controller->SetFailed(errtxt);
        return;
    }

    // 读取配置文件rpcserver的信息
    // std::string ip = MprpcApplication::GetInstance().GetConfig().Load("rpcserverip");
    // uint16_t port = atoi(MprpcApplication::GetInstance().GetConfig().Load("rpcserverport").c_str());
    // rpc调用方想调用service_name的method_name服务，需要查询zk上该服务所在的host信息
    // 1、zkclient启动连接zkserver
    ZkClient zkCli;
    zkCli.Start();

    // 2、组装zk节点路径，调用远程zkserver端的方法： /UserServiceRpc/Login
    std::string method_path = "/" + service_name + "/" + method_name;

    // 3、通过zk节点路径，获取远程zkserver端地址：127.0.0.1:8000
    std::string host_data = zkCli.GetData(method_path.c_str());
    if (host_data == "")
    {
        controller->SetFailed(method_path + " is not exist!");
        return;
    }
    int idx = host_data.find(":");
    if (idx == -1)
    {
        controller->SetFailed(method_path + " address is invalid!");
        return;
    }
    std::string ip = host_data.substr(0, idx);
    uint16_t port = atoi(host_data.substr(idx + 1, host_data.size() - idx).c_str());

    // 2、bind
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip.c_str());

    // 3、connect（连接rpc服务节点）
    if (-1 == connect(clientfd, (struct sockaddr *)&server_addr, sizeof(server_addr)))
    {
        close(clientfd);
        char errtxt[512] = {0};
        sprintf(errtxt, "connect error! errno:%d", errno);
        controller->SetFailed(errtxt);
        return;
    }

    // 4、send（发送rpc请求）
    if (-1 == send(clientfd, send_rpc_str.c_str(), send_rpc_str.size(), 0))
    {
        close(clientfd);
        char errtxt[512] = {0};
        sprintf(errtxt, "send error! errno:%d", errno);
        controller->SetFailed(errtxt);
        return;
    }

    // 5、recv（接受rpc请求的响应值）
    char recv_buf[1024] = {0};
    int recv_size = 0;
    if (-1 == (recv_size = recv(clientfd, recv_buf, 1024, 0)))
    {
        close(clientfd);
        char errtxt[512] = {0};
        sprintf(errtxt, "recv error! errno:%d", errno);
        controller->SetFailed(errtxt);
        return;
    }

    // 6、反序列化rpc远程调用的响应数据
    /*
        bug，出现问题，recv_buf中遇到\0后面的数据就存不下来了，导致反序列化失败，用数组接受
        std::string response_str(recv_buf, 0, recv_size);
        if (!response->ParseFromString(response_str))
    */
    // if (!response->ParseFromArray(recv_buf, recv_size))
    // {
    //     close(clientfd);
    //     char errtxt[512] = {0};
    //     sprintf(errtxt, "parse error! response_str:%s", recv_buf);
    //     controller->SetFailed(errtxt);
    //     return;
    // }
    if (!response->ParseFromArray(recv_buf, recv_size))
    {
        close(clientfd);
        char errtxt[512] = {0};
        snprintf(errtxt, sizeof(errtxt), "parse error! response_str:%.*s", recv_size, recv_buf);
        controller->SetFailed(errtxt);
        return;
    }

    close(clientfd);
}
