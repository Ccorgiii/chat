#ifndef USER_H
#define USER_H

#include <string>
using namespace std;
// USER表的数据映射类 orm
class User 
{
public:
    User(int id = -1, string name = "", string pwd = "", string state = "offline")
    : id(id), name(name), password(pwd), state(state)  
    {
        
    }

    void setId(int id) {this->id = id;}
    void setName(string name) {this->name = name;}
    void setPwd(string pwd) {this->password = pwd;}
    void setState(string state) {this->state = state;}

    int getId() {return this->id;}
    string getName() {return this->name;}
    string getPwd() {return this->password;}
    string getState() {return this->state;}
private:
    int id;
    string name;
    string password;
    string state;
};

#endif