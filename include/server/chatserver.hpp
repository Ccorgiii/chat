#ifndef CHATSERVER_H
#define CHATSERVER_H

#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
using namespace muduo;
using namespace muduo::net;

class ChatServer
{
public:
    ChatServer(EventLoop* loop,
            const InetAddress& listenAddr,
            const string& nameArg);
    void start();
private:
    // 上报连接相关信息回调
    void onConnection(const TcpConnectionPtr&);
    // IO相关回调
    void onMessage(const TcpConnectionPtr&,
                            Buffer*,
                            Timestamp);
                            
    TcpServer _server;
    EventLoop *_loop;
};

#endif