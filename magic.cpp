#include "magic.h"

Meow& Meow::operator<<(StdEndl manip) {
    manip(cout);
    luck->unlock();
    return *this;
}
Meow::Meow(mutex& lock){
    luck = &lock;
};

cliInfo::cliInfo (int i,int b, int s, int sfd) :cliInfo(){
    index = i;
    balance = b;
    stage = s;
    sockfd = sfd;
};
cliInfo::cliInfo (int i,int b, int s, int sfd, sockaddr_in so):cliInfo() {
    index = i;
    balance = b;
    stage = s;
    sockfd = sfd;
    sock = so;
    ip = inet_ntoa(so.sin_addr);
};
cliInfo::cliInfo (int i, int sfd,sockaddr_in so) :cliInfo(){
    index = i;
    sockfd = sfd;
    sock = so;
    ip = inet_ntoa(so.sin_addr);
};
cliInfo srvInfo::getCli(int i){
    return cliInfos[i];
}
cliInfo::cliInfo(const cliInfo& cli):ip(cli.ip),openPort(cli.openPort),name(cli.name){
    index = cli.index;
     balance = cli.balance;
     stage = cli.stage;
     sockfd = cli.sockfd;
    sock = cli.sock;
    online = cli.online;
}
bool srvInfo::setCli(int i,cliInfo& cli) {
    rlock.lock();
    cliInfos[i] = cli;
    rlock.unlock();
    return true;
}
string srvInfo::getList(cliInfo& curCli) {
    string srvRes,tmp;
    srvRes += to_string(curCli.balance) + "\n";
    onlineNum = 0;
    for(int i=0;i<registerNum;++i) {
        auto cli = getCli(i);
        if(cli.online){
            onlineNum++;
            tmp += cli.name+"#"+cli.ip+"#"+cli.openPort+"\n";
        }
    }
    srvRes += to_string(onlineNum) + "\n"+tmp;
    return srvRes;
};
int srvInfo::regist(cliInfo& curCli){
    rlock.lock();
    cliInfos[registerNum++] = curCli;
    rlock.unlock();
    return registerNum - 1;
}

bool recvline(int& sockfd, ssize_t & rlen,char* buf, string& res) {
    thread_local static string remainBuf;
    int pos;
    res.clear();
    // mout <<"remain: " << remainbuf << "qwe" << endl;
    if((pos=remainBuf.find("\n"))==string::npos){
        res += remainBuf;   
        remainBuf.clear();
    }else{
        res += remainBuf.substr(0,pos);
        remainBuf = remainBuf.substr(pos+1);
        return true;
    }
    while( (rlen = recv(sockfd, buf, 1024,0))>0){
        char* pos;
        buf[rlen] = '\0';
        // mout << "len: " << rlen << endl;
        if((pos = strchr(buf,'\n'))==nullptr){
            res.append(buf);
            // mout << "buf: " << buf << "qwe" << endl;
            // mout << "res: " << res << "qwe" << endl;
        }else{
            *pos = '\0';
            remainBuf.append(pos+1);
            res.append(buf);
            // mout << "buf: " << buf << "qwe" << endl;
            // mout << "res: " << res << "qwe" << endl;
            break;
        }         
    }
    if(rlen == 0 ){
        // perror("Recv error");
        return false;
    }
    return true;

}

bool ssend(int& sockfd, string& str){
    send(sockfd, str.c_str(),str.length(),0);
    return true;
}
bool ssend(int& sockfd, const char * str){
    send(sockfd, str, strlen(str),0);
    return true;
}
bool isNumber(const string& s) {
    const char *cs = s.c_str();
    for(int i=0;i<s.length();++i) {
        if(cs[i]>'9'||cs[i]<'0')
            return false;
    }
    return true;
}

Mout mout;