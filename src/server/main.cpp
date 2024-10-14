#include "chatserver.hpp"
#include <iostream>
#include <signal.h>
#include "chatservice.hpp"
using namespace std;

    // 调试步骤 gdb ChatServer
    // 打断点 break chatservice.cpp:20
    // r 运行直接运行到断点处 
    // n 一步步调试
    // {"msgid":1,"id":1,"password":"123456"}
    // {"msgid":1,"id":4,"password":"000000"}
    // {"msgid":5,"id":1,"to":4,"msg":"hello im zhangsan"}
    // {"msgid":5,"id":4,"to":1}
    // {"msgid":6,"id":1,"friendid":4}
    // UPDATE USER SET state = 'offline' where id = 1;

// ctrl+c异常退出服务器，触发resetHandler方法重置表各用户的state信息为offline
// 至于数据结构map中保存的conn，在服务器异常退出进程终止后
//，自然这些连接也就没了，所以不用手动消除

void resetHandler(int)
{
    ChatService::instance()->reset();
    exit(0);
}

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        cerr << "command invalid! example: ./ChatClient 127.0.0.1 6000" << endl;
        exit(-1);
    }

    char *ip = argv[1];
    uint16_t port = atoi(argv[2]);

    // 处理服务器异常中断导致没法执行onConnection方法
    // 通过注册SIGINT的信号处理函数，捕获SIGINT信号，执行handler
    // sigint 2 对应ctrl+c的退出操作
    signal(SIGINT, resetHandler);

    EventLoop loop;
    InetAddress addr(ip, port);
    ChatServer server(&loop, addr, "ChatServer");

    server.start();
    loop.loop();
    return 0;
}
