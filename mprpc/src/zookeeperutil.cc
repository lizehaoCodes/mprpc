#include "zookeeperutil.h"
#include "mprpcapplication.h"
#include <semaphore.h>
#include <iostream>

// 全局的watcher观察器   zkserver给zkclient的通知
void global_watcher(zhandle_t *zh, int type,
					int state, const char *path, void *watcherCtx)
{
	if (type == ZOO_SESSION_EVENT) // 回调的消息类型是和会话相关的消息类型
	{
		// zkclient和zkserver连接成功
		if (state == ZOO_CONNECTED_STATE)
		{
			// 从句柄上获取信号量
			sem_t *sem = (sem_t *)zoo_get_context(zh);
			// 向ZkClient::Start发布信号量
			sem_post(sem);
		}
	}
}

ZkClient::ZkClient() : m_zhandle(nullptr)
{
}

ZkClient::~ZkClient()
{
	if (m_zhandle != nullptr)
	{
		zookeeper_close(m_zhandle); // 关闭句柄，释放资源  MySQL_Conn
	}
}

// 异步连接zkserver
void ZkClient::Start()
{
	std::string host = MprpcApplication::GetInstance().GetConfig().Load("zookeeperip");
	std::string port = MprpcApplication::GetInstance().GetConfig().Load("zookeeperport");
	std::string connstr = host + ":" + port;

	/*
	zookeeper_mt：多线程版本
	zookeeper的API客户端程序提供了三个线程：
	1.API调用线程
	2.网络I/O线程  pthread_create  poll
	3.watcher回调线程（zkserver给zkclient通知消息） pthread_create
	*/
	m_zhandle = zookeeper_init(connstr.c_str(), global_watcher, 30000, nullptr, nullptr, 0); // 连接信息 + watcher回调 + 超时时间30s
	if (nullptr == m_zhandle)
	{
		std::cout << "zookeeper_init error!" << std::endl;
		exit(EXIT_FAILURE);
	}

	// 初始化信号量为零，等待信号量被发送过来
	sem_t sem;
	sem_init(&sem, 0, 0);

	// 给句柄绑定信号量
	zoo_set_context(m_zhandle, &sem);

	// 信号量阻塞，当信号量被 global_watcher 通知到这里，则接受回调成功，即连接成功，开始运行（异步连接过程）
	// A 等待 B运行成功，通知A运行
	sem_wait(&sem);
	std::cout << "zookeeper_init success!" << std::endl;
}

void ZkClient::Create(const char *path, const char *data, int datalen, int state)
{
	char path_buffer[128];
	int bufferlen = sizeof(path_buffer);
	int flag;
	// 先判断path表示的znode节点是否存在，如果存在，就不再重复创建了
	flag = zoo_exists(m_zhandle, path, 0, nullptr);
	// 表示path的znode节点不存在
	if (ZNONODE == flag)
	{
		// 创建指定path的znode节点
		flag = zoo_create(m_zhandle, path, data, datalen,
						  &ZOO_OPEN_ACL_UNSAFE, state, path_buffer, bufferlen);
		if (flag == ZOK)
		{
			std::cout << "znode create success... path:" << path << std::endl;
		}
		else
		{
			std::cout << "flag:" << flag << std::endl;
			std::cout << "znode create error... path:" << path << std::endl;
			exit(EXIT_FAILURE);
		}
	}
}

// 根据指定的path，获取znode节点的值
std::string ZkClient::GetData(const char *path)
{
	char buffer[64];
	int bufferlen = sizeof(buffer);
	int flag = zoo_get(m_zhandle, path, 0, buffer, &bufferlen, nullptr);
	if (flag != ZOK)
	{
		std::cout << "get znode error... path:" << path << std::endl;
		return "";
	}
	else
	{
		return buffer;
	}
}