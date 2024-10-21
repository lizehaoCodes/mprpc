#aux_source_directory(. SRC_LIST) 增加新文件会报错
set(SRC_LIST 
    mprpcapplication.cc 
    mprpcconfig.cc 
    rpcheader.pb.cc 
    rpcprovider.cc 
    mprpcchannel.cc
    mprpccontroller.cc
    logger.cc
    zookeeperutil.cc)
    
#生成库文件mprpc{静态库(STATIC)：libmprpc.a， 动态库(SHARED)：libmprpc.so}
add_library(mprpc ${SRC_LIST})
#链接所需要的库(muduo库顺序不能乱)
target_link_libraries(mprpc muduo_net muduo_base pthread zookeeper_mt)