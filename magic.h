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
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/bio.h>
using namespace std;


#define REDIRECT_GETLIST 90
#define DUPLICATE_NAME 210
#define AUTH_FAIL_ 220
#define UNKNOWN_COMMAND 240
#define BYE 300
#define TRADE_COMMIT 500
#define NOT_ENOUGH 501
#define DECRYPT_FAIL 502
#define TRADE_FORMAT_ERROR 503
#define NEXT_COMMAND 1120
#define JIZZ 7122
#define GET_LIST 1204


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
    bool isOnline(string& name);
    cliInfo getInfo(string& name);
    cliInfo getCli(int i);
    bool setCli(int i,cliInfo&);
    string checker(string& spayer, string& spayee, unsigned char * encMsg);
    bool trailer(string& spayer, string& smoney, string& spayee);
    mutex rlock;
};

bool isNumber(const string& s);
bool recvline(int& sockfd, ssize_t & rlen,char* buf, string& res);
bool ssend(int& sockfd, string& str);
bool ssend(int& sockfd, const char * str);
void slave(const int newfd, const struct sockaddr_in newSock, srvInfo& srv);
bool make_connect(int sockfd, struct sockaddr_in& sock);
extern Mout mout;
#endif