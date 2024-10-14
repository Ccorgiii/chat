#include "db.h"
#include <muduo/base/Logging.h>
// 数据库配置信息
static string server = "127.0.0.1";
static string user = "root";
static string password = "123456";
static string dbname = "chat";

// 初始化数据库连接
MySQL::MySQL()
{
 _conn = mysql_init(nullptr);
}
// 释放数据库连接资源
MySQL::~MySQL()
{
    if (_conn != nullptr)
        mysql_close(_conn);
}
// 连接数据库
bool MySQL::connect()
{
    MYSQL *p = mysql_real_connect(_conn, server.c_str(), user.c_str(),
    password.c_str(), dbname.c_str(), 3306, nullptr, 0);
    if (p != nullptr)
    {
        // c和c++代码默认编码是ASCII，如果不设置，从MYSQL上拉下来中文会显示乱麻？？？ 设置GBK
        mysql_query(_conn, "set names gbk");
        LOG_INFO << "connect mysql success!";
    } else {
        LOG_INFO << "connect mysql fail!";
    }
    return p;
}
// 更新操作
bool MySQL::update(string sql)
{
    if (mysql_query(_conn, sql.c_str()))
    {
        LOG_INFO << __FILE__ << ":" << __LINE__ << ":"
            << sql << "更新失败!";
        LOG_INFO << "UPDATE 查询失败: " << mysql_error(_conn);
        return false;
    }
    return true;
}
// 查询操作
MYSQL_RES* MySQL::query(string sql)
{
    if (mysql_query(_conn, sql.c_str()))
    {
        LOG_INFO << __FILE__ << ":" << __LINE__ << ":"
            << sql << "查询失败!";
        return nullptr;
    }
    return mysql_use_result(_conn);
}

MYSQL* MySQL::getConnection()
{
    return this->_conn;
}

// 连接还在吗
bool MySQL::isconnect(){
    return !(_conn == nullptr);
}