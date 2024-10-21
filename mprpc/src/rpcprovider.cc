#include "rpcprovider.h"
#include "mprpcapplication.h"
#include "rpcheader.pb.h"
#include "logger.h"
#include "zookeeperutil.h"

// 提供rpc本地节点上，*service的所有信息
void RpcProvider::NotifyService(google::protobuf::Service *service)
{
    ServiceInfo service_info;

    // 1、获取*service的抽象描述
    const google::protobuf::ServiceDescriptor *pserviceDesc = service->GetDescriptor();

    // 2、获取*service的服务对象
    std::string service_name = pserviceDesc->name();

    // 3、获取*service对象的方法数量
    int methodCnt = pserviceDesc->method_count();

    LOG_INFO("service_name:%s", service_name.c_str());

    // 4、获取*service对象的所有方法
    for (int i = 0; i < methodCnt; ++i)
    {
        const google::protobuf::MethodDescriptor *pmethodDesc = pserviceDesc->method(i);
        std::string method_name = pmethodDesc->name();
        service_info.m_methodMap.insert({method_name, pmethodDesc});

        LOG_INFO("method_name:%s", method_name.c_str());
    }
    service_info.m_service = service;

    // 5、存储*service的所有信息
    m_serviceMap.insert({service_name, service_info});
}

// 向远端提供相应的rpc服务
void RpcProvider::Run()
{
    // 1、读取配置文件*server（ip + port）
    std::string ip = MprpcApplication::GetInstance().GetConfig().Load("rpcserverip");
    uint16_t port = atoi(MprpcApplication::GetInstance().GetConfig().Load("rpcserverport").c_str());
    muduo::net::InetAddress address(ip, port);

    // 2、创建TcpServer对象
    muduo::net::TcpServer server(&m_eventLoop, address, "RpcProvider");

    // 3、绑定网络连接、读写事件的回调
    server.setConnectionCallback(std::bind(&RpcProvider::OnConnection, this, std::placeholders::_1));
    server.setMessageCallback(std::bind(&RpcProvider::OnMessage, this, std::placeholders::_1,
                                        std::placeholders::_2, std::placeholders::_3));
    // 4、设置muduo库的线程数
    server.setThreadNum(4);

    // 把当前rpc节点上要发布的服务全部注册到zk上面，让rpc client可以从zk上发现服务
    // （zkclient 网络I/O线程 每 1/3 * session timeout（10s) 时间向zkserver发送ping消息，来保持和zkserver的通信畅通）  zkserver超时时间：session timeout (30s)
    // service_name为永久性节点    method_name为临时性节点
    // 1、zkclient启动连接zkserver
    ZkClient zkCli;
    zkCli.Start();

    // 2、在zk上创建节点
    for (auto &sp : m_serviceMap)
    {
        // service_name（UserServiceRpc）
        std::string service_path = "/" + sp.first;
        zkCli.Create(service_path.c_str(), nullptr, 0);
        for (auto &mp : sp.second.m_methodMap)
        {
            std::string method_path = service_path + "/" + mp.first;
            char method_path_data[128] = {0};
            sprintf(method_path_data, "%s:%d", ip.c_str(), port);
            // zknode:  /service_name/method_name(/UserServiceRpc/Login 存储当前这个rpc服务节点主机的ip和port)
            zkCli.Create(method_path.c_str(), method_path_data, strlen(method_path_data), ZOO_EPHEMERAL); // ZOO_EPHEMERAL表示znode是一个临时性节点
        }
    }

    // 5、启动rpc远程调用服务
    std::cout << "RpcProvider start service at ip:" << ip << " port:" << port << std::endl;
    server.start();
    m_eventLoop.loop();
}

// 处理网络连接回调
void RpcProvider::OnConnection(const muduo::net::TcpConnectionPtr &conn)
{
    if (!conn->connected())
    {
        // 和rpc client的连接断开了
        conn->shutdown();
    }
}

/*
RpcProvider和RpcConsumer，协定rpc通信用的protobuf的数据类型，进行数据头的序列化和反序列化
数据头：16UserServiceLogin?zhangsan123456
数据头格式:   header_size(int)    +    header_str     +      args_str
                          (service_name  method_name  args_size)
*/
// 处理远端发出的rpc读写事件回调
void RpcProvider::OnMessage(const muduo::net::TcpConnectionPtr &conn,
                            muduo::net::Buffer *buffer,
                            muduo::Timestamp)
{
    // 1、读取数据头(header_size + header_str + args_str)
    std::string recv_buf = buffer->retrieveAllAsString();

    // 2、读取header_size
    uint32_t header_size = 0;
    recv_buf.copy((char *)&header_size, 4, 0);

    // 3、读取header_str
    std::string rpc_header_str = recv_buf.substr(4, header_size);
    mprpc::RpcHeader rpcHeader;
    std::string service_name;
    std::string method_name;
    uint32_t args_size;
    if (rpcHeader.ParseFromString(rpc_header_str)) // 数据头反序列化
    {
        service_name = rpcHeader.service_name();
        method_name = rpcHeader.method_name();
        args_size = rpcHeader.args_size();
    }
    else
    {
        std::cout << "rpc_header_str:" << rpc_header_str << " parse error!" << std::endl;
        return;
    }
    // 4、根据args_size，读取args_str
    std::string args_str = recv_buf.substr(4 + header_size, args_size);

    // 5、打印
    std::cout << "============================================" << std::endl;
    std::cout << "header_size: " << header_size << std::endl;
    std::cout << "rpc_header_str: " << rpc_header_str << std::endl;
    std::cout << "service_name: " << service_name << std::endl;
    std::cout << "method_name: " << method_name << std::endl;
    std::cout << "args_str: " << args_str << std::endl;
    std::cout << "============================================" << std::endl;

    // 6、获取远端调用的service对象
    auto it = m_serviceMap.find(service_name);
    if (it == m_serviceMap.end())
    {
        std::cout << service_name << " is not exist!" << std::endl;
        return;
    }
    google::protobuf::Service *service = it->second.m_service;

    // 7、获取远端调用的service对象方法
    auto mit = it->second.m_methodMap.find(method_name);
    if (mit == it->second.m_methodMap.end())
    {
        std::cout << service_name << ":" << method_name << " is not exist!" << std::endl;
        return;
    }
    const google::protobuf::MethodDescriptor *method = mit->second;

    // 8、获取远端调用service对象方法参数(request参数 + response参数)
    google::protobuf::Message *request = service->GetRequestPrototype(method).New();
    if (!request->ParseFromString(args_str))
    {
        std::cout << "request parse error, content:" << args_str << std::endl;
        return;
    }
    google::protobuf::Message *response = service->GetResponsePrototype(method).New();

    // 9、给CallMethod方法调用，绑定1个Closure的处理回调(RpcProvider::SendRpcResponse)
    google::protobuf::Closure *done = google::protobuf::NewCallback<RpcProvider,
                                                                    const muduo::net::TcpConnectionPtr &,
                                                                    google::protobuf::Message *>(this,
                                                                                                 &RpcProvider::SendRpcResponse,
                                                                                                 conn, response);

    // 10、根据远端rpc请求，调用本地rpc节点上的方法
    service->CallMethod(method, nullptr, request, response, done); // new UserService().Login(controller, request, response, done)
}

// 本地rpc节点的处理回调（序列化rpc响应和网络发送）
void RpcProvider::SendRpcResponse(const muduo::net::TcpConnectionPtr &conn, google::protobuf::Message *response)
{
    std::string response_str;
    if (response->SerializeToString(&response_str))
    {
        // 序列化成功后，通过网络，把rpc方法执行后的结果，发送给rpc的调用方
        conn->send(response_str);
    }
    else
    {
        std::cout << "serialize response_str error!" << std::endl;
    }
    // 模拟http的短链接服务，由rpcprovider主动断开连接，节省资源
    conn->shutdown();
}