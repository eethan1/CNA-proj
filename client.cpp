#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <cstring>
#include <string>
#include <vector>
#include <iostream>
using namespace std;

struct cliInfo {
    int port=-1;
    int sockfd;
    int balance;
    int online;
    int stage = 0;
    sockaddr_in sock;
    string name;
};

bool isNumber(const string& s) {
    const char *cs = s.c_str();
    for(int i=0;i<s.length();++i) {
        if(cs[i]>'9'||cs[i]<'0')
            return false;
    }
    return true;
}
void recvline(int& sockfd, ssize_t & rlen,char* buf, string& res) {
    static string remainBuf;
    int pos;
    res.clear();
    // cout <<"remain: " << remainbuf << "qwe" << endl;
    if((pos=remainBuf.find("\n"))==string::npos){
        res += remainBuf;   
        remainBuf.clear();
    }else{
        res += remainBuf.substr(0,pos);
        remainBuf = remainBuf.substr(pos+1);
        return;
    }
    while( (rlen = recv(sockfd, buf, 1024,0))>0){
        char* pos;
        if(rlen < 0 ){
            perror("Recv error");
            abort();
        }
        buf[rlen] = '\0';
        // cout << "len: " << rlen << endl;
        if((pos = strchr(buf,'\n'))==nullptr){
            res.append(buf);
            // cout << "buf: " << buf << "qwe" << endl;
            // cout << "res: " << res << "qwe" << endl;
        }else{
            *pos = '\0';
            remainBuf.append(pos+1);
            res.append(buf);
            // cout << "buf: " << buf << "qwe" << endl;
            // cout << "res: " << res << "qwe" << endl;
            break;
        }         
    }
}

void ssend(int& sockfd, string& cliIn){
    cliIn.append("\n");
    send(sockfd, cliIn.c_str(),cliIn.length(),0);
}
int main(int argc, char **argv) {
    struct sockaddr_in srv;
    struct cliInfo cli;
    char c;
    string cliIn,srvRes;
    char srvBuf[1024];
    size_t inbuf_used=0, inbuf_remain=1024;
    ssize_t rlen;
    size_t pos;
    while((c=getopt(argc, argv, "t:p:"))!=EOF) {
        switch(c){
        case 't':
            inet_pton(AF_INET, optarg, &(cli.sock.sin_addr));    
            break;
        case 'p':    
            cli.sock.sin_port = htons(atoi(optarg));
            break;
        default:
            puts("Format: ./client -t <ip> -p <port>\n");
            exit(-1);
        }
    }

    Connect:

    cli.sockfd = socket(AF_INET,SOCK_STREAM, 0);
    cli.sock.sin_family=AF_INET;

    if(connect(cli.sockfd, (struct sockaddr*) &(cli.sock), sizeof(cli.sock)) <0) {
        switch(errno)
        {
            case 111:
                cerr << "Connection Refused" << endl;
                break;
            case 115:
            case 110:
                cerr << "Timeoute" << endl;
                break;
            default:
                perror("New");
                cerr << errno << endl;
                break;
        }
        exit(-1);
    }else{
        recvline(cli.sockfd, rlen, srvBuf, srvRes);
        cout << srvRes << endl;
    }

    GetCommand:
    cout << cli.name << "$ ";
    cliIn.clear();
    if((cin >> cliIn).eof()) {
        goto End;
    }
    // cout << "Typein: " << cliIn.c_str() << "#Len: " <<cliIn.length()<< endl;
    switch(cli.stage) {
        case 0:
            goto Welcome;
        case 1:
            goto Logined;
        default:
            goto Welcome;
    }

    Welcome:
    if((pos=cliIn.find("REGISTER#")) == 0){
        ssend(cli.sockfd, cliIn);
        recvline(cli.sockfd, rlen, srvBuf, srvRes);
        cout << srvRes << endl;
        // for(int i=0;i<srvRes.length();++i) {
        //     cout << int(srvRes[i]) << ' ';
        // }
    }else if((pos=cliIn.find("#")) > 0 && pos != string::npos){
        cli.name = cliIn.substr(0,pos);
        ssend(cli.sockfd, cliIn);
        recvline(cli.sockfd, rlen, srvBuf, srvRes);
        if(srvRes.find("220 AUTH_FAIL") == 0) {
            cout << srvRes << endl;    
            cli.name.clear();        
        }else{
            cli.stage = 1;
            goto GetList;
        }
    }else {
        cout << "Invalid Options" << endl;
        cout << "Options:\n1)Register: REGISTER#<UserAccountName><CRLF>\n2)Login: <UserAccount>#<portNum><CRLF>" << endl;
    }
    goto GetCommand;

    GetList:
    cout << "Getlist" << endl;
    if( isNumber(srvRes) && (cli.balance = atoi(srvRes.c_str()))){
        cout << "Account balance: " << cli.balance << endl;   
    }else{
        cout << srvRes << endl;
        goto GetCommand;
    }
    recvline(cli.sockfd, rlen, srvBuf, srvRes);
    cout << srvRes << endl;
    #ifdef _REVISED_ 
    if(isNumber(srvRes) && (cli.online = atoi(srvRes.c_str()))) {
        for(int i=0;i<cli.online;++i) {
            recvline(cli.sockfd, rlen, srvBuf, srvRes);
            cout << "number of accounts online: " << srvRes << endl;   
        }
    }
    #else
    if((pos=srvRes.find("number of accounts online: "))!=string::npos){
        string subRes = srvRes.substr(pos+27);
        // cout << "Num: " << subRes << "#" << endl;
        if(isNumber(subRes) && (cli.online = atoi(subRes.c_str()))) {
            for(int i=0;i<cli.online;++i) {
                recvline(cli.sockfd, rlen, srvBuf, srvRes);
                cout << srvRes << endl;   
            }
        }
    }
    #endif

    goto GetCommand;

    Logined:
    if(cliIn.compare("List")==0){
        ssend(cli.sockfd, cliIn);
        recvline(cli.sockfd, rlen, srvBuf, srvRes);
        // cout << srvRes << endl;
        goto GetList;
    }else if(cliIn.compare("Exit")==0){
        ssend(cli.sockfd, cliIn);
        recvline(cli.sockfd, rlen, srvBuf, srvRes);
        cout << srvRes << endl;
        goto End;
    }else if(cliIn.compare("Logout")==0){
        ssend(cli.sockfd, cliIn);
        recvline(cli.sockfd, rlen, srvBuf, srvRes);
        close(cli.sockfd);
        cli.name.clear();
        cli.stage = 0;
        goto Connect;
    }else{
        cout << "Invalid Options" << endl;
        cout << "Options:\n1)List: List<CRLF>\n2)Exit: Exit<CRLF>\n3)Logout: Logout" << endl;
    }
    goto GetCommand;

    End:
    cout << "Close socket" << endl;
    close(cli.sockfd);

}