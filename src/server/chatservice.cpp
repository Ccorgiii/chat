#include "chatservice.hpp"
#include "public.hpp"
#include <muduo/base/Logging.h>
#include <vector>
using namespace std;
using namespace muduo;

// 业务聊天服务器 注册消息以及对应的回调函数
ChatService::ChatService()
{
    // 登陆
    _msgHandlerMap[EnMsgType::LOGIN_MSG] = std::bind(&ChatService::login, this, _1, _2, _3);
    // 注册
    _msgHandlerMap[EnMsgType::REG_MSG] = std::bind(&ChatService::reg, this, _1, _2, _3);
    // 一对一
    _msgHandlerMap[EnMsgType::ONE_CHAT_MSG] = std::bind(&ChatService::oneChat, this, _1, _2, _3);
    // 添加好友
    _msgHandlerMap[EnMsgType::ADD_FRIEND_MSG] = std::bind(&ChatService::addFriend, this, _1, _2, _3);
    // 登出
    _msgHandlerMap[EnMsgType::LOGINOUT_MSG] = std::bind(&ChatService::loginout, this, _1, _2, _3);
    // 建组
    _msgHandlerMap[EnMsgType::CREATE_GROUP_MSG] = std::bind(&ChatService::createGroup, this, _1, _2, _3);
    // 添加组
    _msgHandlerMap[EnMsgType::ADD_GROUP_MSG] = std::bind(&ChatService::addGroup, this, _1, _2, _3);
    // 群聊
    _msgHandlerMap[EnMsgType::GROUP_CHAT_MSG] = std::bind(&ChatService::groupChat, this, _1, _2, _3);
    //
    _msgHandlerMap[EnMsgType::ERR_MSG]= std::bind(&ChatService::groupChat, this, _1, _2, _3);
    // 连接redis服务器
    if (_redis.connect()) {
        // 设置上报信息的回调函数
        _redis.init_notify_handler(std::bind(&ChatService::handleRedisSubscribeMessage, this, _1, _2));
    }
}

ChatService* ChatService::instance()
{
    static ChatService service;
    return &service;
}

// 登陆业务 id passwaord
void ChatService::login(const TcpConnectionPtr& conn, json& js, Timestamp time)
{
    LOG_INFO << "do login service!!";

    int id = js["id"].get<int>();
    string pwd = js["password"];

    User user = _userModel.query(id);
    if (user.getId() == id && user.getPwd() == pwd) {
        if (user.getState() == "online") {
            // 用户已经登录，不允许重复登录
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 1;
            response["errmsg"] = "该账号已经登录，不允许重复登录，请重新输入账号";
            conn->send(response.dump());
        } else {
            // 登录成功，记录用户连接信息
            {
                lock_guard<mutex> lock(_connMutex);
                _userConnectionMap.insert({id, conn});
            }

            // id用户登陆后，向redis订阅channel(id)
            _redis.subscribe(id);

            // 更新用户状态信息  state offline -> online
            user.setState("online");
            _userModel.updateState(user);

            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 0;
            response["id"] = user.getId();
            response["name"] = user.getName();

            // 查询该用户是否具有离线消息
            vector<string> vec = _offlineMsgModel.query(id);
            if (!vec.empty()) {
                response["offlinemsg"] = vec;
                // 读取该用户离线消息后，把离线消息删除
                _offlineMsgModel.remove(id);
            }

            // 查询该用户的好友信息并返回
            vector<User> userVec = _friendModel.query(id);
            if (!userVec.empty())
            {
                vector<string> vec2;
                for (User &user : userVec)
                {
                    json js;
                    js["id"] = user.getId();
                    js["name"] = user.getName();
                    js["state"] = user.getState();
                    vec2.push_back(js.dump());
                }
                response["friends"] = vec2;
            }

            conn->send(response.dump());
        } 

    } else {
        // 用户不存在，登录失败
        json response;
        response["msgid"] = LOGIN_MSG_ACK;
        response["errno"] = 1;
        response["errmsg"] = "用户名不存在";
        conn->send(response.dump());
    }
}

// 处理注册业务 name password
void ChatService::reg(const TcpConnectionPtr& conn, json& js, Timestamp time)
{
    LOG_INFO << "do register service!!";
    string name = js["name"];
    string pwd = js["password"];

    User user;
    user.setName(name);
    user.setPwd(pwd);
    bool state = _userModel.insert(user);
    if (state) {
        // 注冊成功
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 0;
        response["id"] = user.getId();
        conn->send(response.dump());
    } else {
        // 注冊失敗
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 1;
        response["id"] = user.getId();
        conn->send(response.dump());
    }
}

// 服务器异常，业务重置方法
void ChatService::reset()
{
    // 把online状态用户，设置成offline
    _userModel.resetState();
}

MsgHandler ChatService::getHandler(int msgid)
{
    // 记录错误日志，msgid沒有对应的handler
    auto it = _msgHandlerMap.find(msgid);
    if (it == _msgHandlerMap.end()) {
        return [=](const TcpConnectionPtr& conn, json& js, Timestamp time) {
            LOG_ERROR << "msgid: " << msgid << " can not find handler! ";
        };
    } else {
        return _msgHandlerMap[msgid];
    }
}

void ChatService::clientCloseException(const TcpConnectionPtr& conn)
{
    User user;
    {
        lock_guard<mutex> lock(_connMutex);
        for (auto it = _userConnectionMap.begin(); it != _userConnectionMap.end();) {
            if (it->second == conn) {
                // 从map表删除用户的连接信息
                user.setId(it->first);
                it = _userConnectionMap.erase(it);
                break;
            } else {
                it++;
            }
        }
    }

    _redis.unsubscribe(user.getId());

    // 更新用户的状态未offline
    if (user.getId() != -1) {
        user.setState("offline");
        _userModel.updateState(user);
    }
}

// 处理注销业务
void ChatService::loginout(const TcpConnectionPtr& conn, json& js, Timestamp time)
{
    int userid = js["id"].get<int>();
    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnectionMap.find(userid);
        if (it != _userConnectionMap.end()) {
            _userConnectionMap.erase(it);
        }
    }

    // 用户注销，相当于下线，在redis中取消订阅通道
    _redis.unsubscribe(userid);

    User user;
    user.setId(userid);
    user.setPwd("");
    user.setName("");
    user.setState("offline");
    // 更新用户状态信息
    _userModel.updateState(user);
}

// 一对一通知
void ChatService::oneChat(const TcpConnectionPtr& conn, json& js, Timestamp time)
{
    int toId = js["toid"].get<int>();
    int from_id = js["id"].get<int>();
    
    //查询该用户的好友信息并返回
    bool isf = _friendModel.queryisfriend(from_id,toId);
    if(isf){

        // 在当前主机上查询是否有该用户
        {
            lock_guard<mutex> lock(_connMutex);
            auto it = _userConnectionMap.find(toId);
            if (it != _userConnectionMap.end())
            {
                // 这里有一点就是，在我们发送消息的过程中
                // 对端conn可能刚好下线，所以conn要考虑它的线程安全，
                // 操作它需要加锁

                // toId在线，转发消息
                // 服务器主动推送消息给toid用户
                it->second->send(js.dump());
                return;
            }
        }

        // 通过redis注册后的，查询是否在其他主机上有用户
        // 查询toid是否在线 在线说明用户在另一台设备上，通过redis发布到对应消息队列上
        User user = _userModel.query(toId);
        if (user.getState() == "online") {

            _redis.publish(toId, js.dump());
            return;
        }

        // toId不在线，存储离线消息
        _offlineMsgModel.insert(toId, js.dump());
    }
    else{
        // 非好友 回送客户端消息
        {
            json jss;
            jss["msgid"] = EnMsgType::ERR_MSG;
            jss["id"] = from_id;
            jss["name"] = js["name"].get<string>();
            jss["toid"] = from_id;
            jss["msg"] = "NO Friend!";
            jss["time"] = js["time"].get<string>();
            //string buffer = jss.dump();
            lock_guard<mutex> lock(_connMutex);
            auto it = _userConnectionMap.find(from_id);
            if (it != _userConnectionMap.end())
            {
                // toId在线，转发消息
                // 服务器主动推送消息给toid用户
                it->second->send(jss.dump());
                return;
            }

        }
    }

}

// 添加好友业务
void ChatService::addFriend(const TcpConnectionPtr& conn, json& js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int friendid = js["friendid"].get<int>();

    _friendModel.insert(userid, friendid);
}

// 创建群组业务
void ChatService::createGroup(const TcpConnectionPtr& conn, json& js, Timestamp time)
{
    int userid = js["id"].get<int>();
    string name = js["groupname"];
    string desc = js["groupdesc"];

    // 存储新创建的群组信息
    Group group(-1, name, desc);
    if (_groupModel.createGroup(group)) {
        // 存储群组创建人信息
        _groupModel.addGroup(userid, group.getId(), "creator");
    }
}

// 网络层 业务层 数据操作层 数据库层

// 加入群组业务
void ChatService::addGroup(const TcpConnectionPtr& conn, json& js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    _groupModel.addGroup(userid, groupid, "normal");
}

// 群组聊天业务
void ChatService::groupChat(const TcpConnectionPtr& conn, json& js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    vector<int> useridVec = _groupModel.queryGroupUsers(userid, groupid);
    {
        lock_guard<mutex> lock(_connMutex);
        for (int id : useridVec) {
            auto it = _userConnectionMap.find(id);
            if (it != _userConnectionMap.end()) {
                // 转发群消息
                it->second->send(js.dump());
            } else {
                User user = _userModel.query(id);
                if (user.getState() == "online") {
                    _redis.publish(id, js.dump());
                } else {
                    // 存储离线消息
                    _offlineMsgModel.insert(id, js.dump());
                }
            }
        }
    }
}


void ChatService::errMsg(const TcpConnectionPtr& conn, json& js, Timestamp time){

}

// 从redis消息队列中获取订阅消息
void ChatService::handleRedisSubscribeMessage(int userid, string msg)
{
    lock_guard<mutex> lock(_connMutex);
    auto it = _userConnectionMap.find(userid);
    if (it != _userConnectionMap.end()) {
        it->second->send(msg);
        return;
    }

    // 存储该用户离线消息
    _offlineMsgModel.insert(userid, msg);
}


