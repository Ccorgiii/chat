#include "chatserver.hpp"
#include "json.hpp"
#include "chatservice.hpp"

#include <string>
#include <functional>
using namespace std;
using namespace placeholders;
using json = nlohmann::json;

ChatServer::ChatServer(EventLoop* loop,
            const InetAddress& listenAddr,
            const string& nameArg)
        : _server(loop, listenAddr, nameArg), _loop(loop)
{
    // 绑定连接回调
    _server.setConnectionCallback(std::bind(&ChatServer::onConnection, this, _1));
    // 绑定消息回调
    _server.setMessageCallback(std::bind(&ChatServer::onMessage, this, _1, _2, _3));
    // 1主3子
    _server.setThreadNum(4);
}

void ChatServer::start()
{
    _server.start();
}

void ChatServer::onConnection(const TcpConnectionPtr& conn)
{
    // 用戶断开连接
    if (!conn->connected()) {
        ChatService::instance()->clientCloseException(conn);
        conn->shutdown();
    }
}

void ChatServer::onMessage(const TcpConnectionPtr& conn,
                            Buffer* buffer,
                            Timestamp time)
{
    // muduo使用read、write两个指针将缓存区分为3个区域(已读、未读、可写)
    string buf = buffer->retrieveAllAsString();
    // 数据的反序列化
    json js = json::parse(buf);
    // 通过js["msgid"]获取-》业务handler-》conn js time
    // 达到的目的， 完全解耦网络模块的代码和业务模块的代码  
    auto msgHandler = ChatService::instance()->getHandler(js["msgid"].get<int>());
    // map中存储的各业务回调函数
    msgHandler(conn, js, time);
}