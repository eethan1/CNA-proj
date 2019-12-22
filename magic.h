#ifndef _MaGic__
#define _MaGic__
#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <cstring>
#include <string>
#include <vector>
#include <stack>
#include <iostream>
#include <mutex>
#include <memory>

using namespace std;
class Meow{
public:
    Meow(mutex& lock);
    typedef std::basic_ostream<char, std::char_traits<char> > CoutType;
    typedef CoutType& (*StdEndl)(CoutType&);

    template <typename T>
    Meow& operator<<(const T& s) {
        cout << s;
        return *this;
    };
    Meow& operator<<(StdEndl manip);
private:
    mutex* luck;
};

class Mout{
public:
    Mout():meow(luck){
    };
    Meow meow;
    template <typename T>
    Meow& operator<<(const T& s) {
        luck.lock();
        return (meow << s);
    };
    mutex luck;
};
class cliInfo {
public:
    cliInfo() {
        online=0;
        stage=0;
        sockfd=-1;
        ip="";
        openPort="";
        name="";
    };
    cliInfo(const cliInfo&);
    cliInfo (int i,int b, int s, int sfd);
    cliInfo (int i,int b, int s, int sfd, sockaddr_in so);
    cliInfo (int i, int sfd,sockaddr_in so);
    int index;
    int balance;
    int stage ;
    int sockfd ;
    sockaddr_in sock;
    string ip ;
    string openPort ;
    string name ;
    bool online ;
};

class srvInfo {

public:
    srvInfo() {
        cout << "JIZ" << endl;
        onlineNum = 0;
        registerNum = 0;
        cout << registerNum << endl;
    };
    int onlineNum;
    int registerNum;
    int sockfd;
    sockaddr_in sock;
    cliInfo cliInfos[5]; // used to be vector<cliInfo>
    int regist(cliInfo& curCli);
    string getList(cliInfo& curCli);
    cliInfo getCli(int i);
    bool setCli(int i,cliInfo&);
    mutex rlock;
};

bool isNumber(const string& s);
bool recvline(int& sockfd, ssize_t & rlen,char* buf, string& res);
bool ssend(int& sockfd, string& str);
bool ssend(int& sockfd, const char * str);
void slave(const int newfd, const struct sockaddr_in newSock, srvInfo& srv);
extern Mout mout;
#endif