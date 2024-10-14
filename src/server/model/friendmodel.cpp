#include "friendmodel.hpp"
#include "db.h"
#include <string.h>
#include <iostream>
// 添加好友关系
void FriendModel::insert(int userid, int friendid)
{
    // 1.查询后不进行释放mysql_free_result(res);可能会报错
    // 2.mysql中已经有的数据再insert也会报错
    // 通过mysql_error(_conn);来获取具体的报错信息

    char sql[1024] = {0};

    MySQL mysql;

    //sprintf(sql, "SELECT name, state FROM user WHERE id = %d",friendid);
    //string sqll = "SELECT name, state FROM user WHERE id = " + to_string(friendid);
    sprintf(sql, "SELECT count(*) FROM user where id = %d",friendid);
    
    bool isvalue = false;
    if (mysql.connect()) {
        MYSQL_RES* res = mysql.query(sql);
        if (res != nullptr) {
                MYSQL_ROW row;
                if((row = mysql_fetch_row(res)) != nullptr)
                {
                    isvalue = true;
                }
                mysql_free_result(res);
        }
    }
    // 存在
    if(isvalue){
         int num_rows = 0;
        memset(sql,0,sizeof(sql));
        sprintf(sql, "SELECT userid,friendid FROM friend where userid = %d and friendid = %d",userid, friendid);
        MYSQL_RES* res = mysql.query(sql);
        if (res != nullptr) {
                MYSQL_ROW row;
                if((row = mysql_fetch_row(res)) != nullptr)
                {
                    num_rows = 1;
                }
                mysql_free_result(res);
        }
       
        if(num_rows == 0){
            
            memset(sql,0,sizeof(sql));
            sprintf(sql, "insert into friend values(%d, %d)",
            userid, friendid);
            if(mysql.isconnect()){
                mysql.update(sql);
            }
            else{

                if (mysql.connect()){
                    mysql.update(sql);
                }

            }

            memset(sql,0,sizeof(sql));
            sprintf(sql, "insert into friend values(%d, %d)",
            friendid, userid);
            
            //MySQL mysql;
            mysql.update(sql);
        }
    }
    else{
        // 不存在
    }

    //sprintf(sql, "select count(*) from user where id = %d",friendid);

    // // 1.组装sql語句
    // char sql[1024] = {0};
    // sprintf(sql, "insert into friend values(%d, %d)",
    //  userid, friendid);
    
    // //MySQL mysql;
    // if (mysql.connect()) {
    //     mysql.update(sql);
    // }
}


// 返回用户好友列表
vector<User> FriendModel::query(int userid)
{
    // 1.组装sql語句
    char sql[1024] = {0};

    // 多表联合查询，省去查数据库的次数
    //a.id, a.name, a.state 指定3列
    //inner join Friend b 使用inner join从friend表联合并取名称为b
    //on 后续联合条件
    //where 增加过滤条件
    sprintf(sql, "select a.id, a.name, a.state from user a inner join friend b on b.friendid = a.id where b.userid = %d",
     userid);
    
    vector<User> vec;

    MySQL mysql;
    if (mysql.connect()) {
       MYSQL_RES* res = mysql.query(sql);
       if (res != nullptr) {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                User user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setState(row[2]);
                vec.push_back(user);
            }
            mysql_free_result(res);
       }
    }

    return vec; 
}

bool FriendModel::queryisfriend(int user_id,int friend_id){

    //组装SQL语句
    char sql[1024] = {0};
    MySQL mysql;
    bool num_rows = false;
    sprintf(sql, "SELECT userid,friendid FROM friend where userid = %d and friendid = %d",user_id, friend_id);
    if (mysql.connect()) {
        MYSQL_RES* res = mysql.query(sql);
       
        if (res != nullptr) {
            MYSQL_ROW row;
            if((row = mysql_fetch_row(res)) != nullptr)
            {
                num_rows = true;
            }
            mysql_free_result(res);
        }
    }
    

    return num_rows;
}