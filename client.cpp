#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <cstring>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <unistd.h>
using namespace std;

struct cliInfo {
    int sockfd;
    int balance;
    int online;
    int stage = 0;
    int port;
    sockaddr_in sock;
    string name;
};

class onlineInfo {
public:
    static int total;
    string name;
    sockaddr_in sock;
    int sockfd;
};
int onlineInfo::total=0;
bool make_connect(int sockfd, struct sockaddr_in& sock);
bool isNumber(const string& s) {
    const char *cs = s.c_str();
    for(int i=0;i<s.length();++i) {
        if(cs[i]>'9'||cs[i]<'0')
            return false;
    }
    return true;
}
bool recvline(int& sockfd, ssize_t & rlen,char* buf, string& res) {
    thread_local static string remainBuf;
    int pos;
    res.clear();
    // mout <<"remain: " << remainbuf << "qwe" << endl;
    #ifdef _DRECV_
    cout << "Recvline by " << sockfd << endl;
    cout << "Remained: " << remainBuf << "##" << endl;
    #endif
    if((pos=remainBuf.find("\n"))==string::npos){
        res += remainBuf;   
        remainBuf.clear();
    }else{
        res += remainBuf.substr(0,pos);
        remainBuf = remainBuf.substr(pos+1);
        #ifdef _DRECV_
        cout << "Recv1: " << res << "##" << endl;
        #endif
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
    #ifdef _DRECV_
    cout << "Recv2: " << res << "##" << endl;
    #endif
    return true;

}


void ssend(int& sockfd, string& cliIn){
    cliIn.append("\n");
    send(sockfd, cliIn.c_str(),cliIn.length(),0);
}

void trader(cliInfo& cli, RSA *publicRSA, RSA *privateRSA, string prompt) {

    int rsa_len = RSA_size(publicRSA);
    
    cliInfo srv;
    char c;
    inet_pton(AF_INET, "127.0.0.1", &(srv.sock.sin_addr)); 
    srv.sock.sin_family = AF_INET;
    srv.sock.sin_port = htons(cli.port);
    if ((srv.sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("create socket");
        cout << "sock failed" << endl;
        exit(1);
    }
    int TRUE=1;
    if (setsockopt(srv.sockfd, SOL_SOCKET, SO_REUSEADDR, &TRUE, sizeof(int))!=0) {
        perror("set reuseaddr");
                cout << "reuse failed" << endl;
        exit(1);
    }

    if(bind(srv.sockfd, (struct sockaddr*)&srv.sock, sizeof(srv.sock)) < 0) {
        perror("Bind fail");
                cout << "bind failed" << endl;
        exit(1);
    }

    if(listen(srv.sockfd, 10) < 0) {
        perror("listen");
        cout << "listen failed" << endl;
        exit(1);
    }   
    int strfd, payerfd = -1, tradefd = -1; // stranger, payer
    struct sockaddr_in newSock;
    socklen_t newSockl = sizeof(newSock);
    char srvBuf[0x50];
    string srvRes;
    ssize_t rlen;
    std::cout << "start listening" << endl;
    cout << prompt << std::flush;
    while( (strfd = tradefd) > 0 || (strfd = accept(srv.sockfd,(struct sockaddr*) &newSock, &newSockl ))) {
    
        cout << "Connect from: " << inet_ntoa(newSock.sin_addr) << endl;
        recvline(strfd, rlen, srvBuf, srvRes);
        cout << "Tradeing Recv: " << srvRes << "#" <<endl;
        if(srvRes == "key?"){
            ifstream fpub("publickey.pem");
            stringstream buffer;
            buffer << fpub.rdbuf();
            string msg(buffer.str()); 
            std::cout << "PUB Key" << msg << endl;
            ssend(strfd,msg);
            if(strfd != tradefd)
                close(strfd);
        }else{
            int deli;
            string msg;
            if((deli=srvRes.find("#")) != string::npos && (deli=srvRes.find("#",deli+1)) !=string::npos) {
                payerfd = strfd;
                cout << "Process trade: " << payerfd << ":" << srvRes <<  endl;
                int enclen = atoi(srvRes.substr(deli+1).c_str()),enclen2;
                unsigned char encMsg[0x100]; 
                cout << "Recv enc1 ...." << endl;
                recv(payerfd, encMsg, enclen,0);
                cout << "REcved enc1 sucess" << endl;
                pid_t pid;
                pid = fork();
                if(pid ==0){
                    const unsigned char * src = (const unsigned char *) encMsg;
                    unsigned char * enc = (unsigned char *)malloc(0x200);
                    if( (enclen2 = RSA_public_encrypt(rsa_len-11, src, enc, publicRSA, RSA_PKCS1_PADDING)) < 0) {
                        std::cout << "enc error" << endl;
                    }
                    msg = "Trade#"+srvRes.substr(0,deli+1) + to_string(enclen2);
                    std::cout << "Trading: " << msg << "qqqqq" << msg.length() << endl;
                    tradefd = socket(AF_INET,SOCK_STREAM, 0);
                    if(!make_connect(tradefd, cli.sock)){
                        cout << "Error connect to Server, abort transaction" << endl;
                    }else{
                        ssend(tradefd, msg);
                        // send(tradefd, msg.c_str(),msg.length(),0);
                        cout << "enclen2" << endl;
                        send(tradefd, enc, enclen2, 0);
                        // ssend(tradefd, msg);
                        cout << "Wait Auth" << endl;
                        while(!recvline(tradefd, rlen, srvBuf, srvRes));
                        cout << "Payee Recieved from Server: " << srvRes << endl;
                        while(!recvline(tradefd, rlen, srvBuf, srvRes));
                        cout << "Payee Recieved from Server: " << srvRes << endl;
                        close(tradefd);
                        ssend(payerfd, srvRes);
                        close(payerfd);
                        tradefd = strfd = -1;
                    }
                    exit(0);
                }
            }
        }
        cout << prompt << std::flush;
    }
    cout << "Close listening" << endl;
    return;
}

bool make_trade(struct cliInfo& cli, string& cliIn, int deli1, int deli2, RSA* privateRSA, int rsa_len, onlineInfo onInfo[]) {
    string  smoney = cliIn.substr(deli2+1),
            payee = cliIn.substr(deli1+1,deli2-deli1-1),
            msg= cli.name+"#"+smoney+"#"+payee,
            srvRes;
    ssize_t rlen;
    char srvBuf[1024];
    cout << "Payee: " << payee << ", Money: " << smoney << endl;
    cout << "Trade msg to Encrypted: " << msg << endl;
    if(!isNumber(smoney.c_str())) {
        cout << "Money not a number" << endl;
        return false;
    }
    for(int i=0;i < onInfo[0].total;++i) {
        if(onInfo[i].name == payee) {
            onInfo[i].sockfd = socket(AF_INET,SOCK_STREAM, 0);
            if(!make_connect(onInfo[i].sockfd, onInfo[i].sock)) {
                cout << "Error connect to payee" << endl;
                return false;
            }
            cout << "connect payee success" << endl;
            const unsigned char * src = (const unsigned char *) msg.c_str();
            unsigned char * enc = (unsigned char *)malloc(rsa_len);
            int enclen;
            if((enclen = RSA_public_encrypt(rsa_len-11, src, enc, privateRSA, RSA_PKCS1_PADDING)) < 0) {
                cout << "enc error" << endl;
            }
            string authmsg = cli.name+"#"+payee+"#"+to_string(enclen);
            cout << "Auth msg to sendL " << authmsg << endl;
            ssend(onInfo[i].sockfd, authmsg);
            send(onInfo[i].sockfd, enc, enclen, 0);
            while(!recvline(onInfo[i].sockfd, rlen, srvBuf, srvRes));
            cout << "Payee response: " << srvRes << endl;
            close(onInfo[i].sockfd);
            return true;
        }
        cout << "onInfo " << i << " " << onInfo[i].name << endl;
    }
    cout << "Payee Nonexist" <<  endl;
    return false;
}

bool getlist(struct cliInfo& cli,string& srvRes, ssize_t rlen, char *buf, onlineInfo* onInfo) {
    string LIST = "List";
    std::cout << "Getlist" << endl;
    ssend(cli.sockfd, LIST);
    recvline(cli.sockfd, rlen, buf, srvRes);
    if(srvRes.find("220 AUTH_FAIL") == 0) {
        return 0;                 
    }
    if( isNumber(srvRes) && (cli.balance = atoi(srvRes.c_str()))){
        std::cout << "Account balance: " << cli.balance << endl;   
    }else{
        std::cout << srvRes << endl;
        return false;
    }
    recvline(cli.sockfd, rlen, buf, srvRes);
    std::cout << "number of accounts online: " << srvRes << endl;
    if(isNumber(srvRes) && (onInfo[0].total = atoi(srvRes.c_str()))) {
        for(int i=0;i<onInfo[0].total;++i) {
            recvline(cli.sockfd, rlen, buf, srvRes);
            int deli1=srvRes.find("#"),deli2 = srvRes.find("#", deli1+1),iport;
            string  name = srvRes.substr(0,deli1),
                    host = srvRes.substr(deli1+1,deli2-deli1-1),
                    port = srvRes.substr(deli2+1);
            std::cout  << srvRes << endl;
            std::cout << "Parse: " << name << ',' << host << ',' << port << endl;   
            onInfo[i].name = name;
            onInfo[i].sock.sin_family=AF_INET;
            inet_pton(AF_INET, host.c_str(), &(onInfo[i].sock.sin_addr));
            if( isNumber(port) && ( iport = atoi(port.c_str()) ) && iport < 65536 && iport > 1024 ){
                onInfo[i].sock.sin_port = htons(iport);
            }else {
                cout << "Invalid port detected: " << onInfo[i].name << "port: "<< port <<"##"<< endl;
                onInfo[i].sock.sin_port = htons(0);
            }
        }
    }     
    return true;
}

int main(int argc, char **argv) {
    struct sockaddr_in srv;
    struct cliInfo cli;
    onlineInfo* onInfo = new onlineInfo[20];  
    char c;
    string cliIn,srvRes,prompt="$ ";
    ssize_t rlen;
    char srvBuf[1024];
    size_t inbuf_used=0, inbuf_remain=1024;
    size_t pos;
    thread* tthread;
    int deli1, deli2;
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

    // RSA init

    FILE *pub, *pri;
    RSA *publicRSA = nullptr, *privateRSA = nullptr; 
    if((pub = fopen("publickey.pem","r")) == NULL) {
       std::cout << "pub Error" << endl;
        exit(-1);
    }
    if((pri = fopen("privatekey.pem","r")) == NULL) {
       std::cout << "pri Error" << endl;
        exit(-1);
    }
    OpenSSL_add_all_algorithms();
    if((privateRSA = PEM_read_RSAPrivateKey(pri, NULL,NULL,NULL)) == NULL) {
       std::cout << "Read pri error" << endl;
    }
    if((publicRSA = PEM_read_RSA_PUBKEY(pub,NULL,NULL,NULL)) == NULL) {
       std::cout << "REad pub erro " << endl;
    }
    fclose(pri), fclose(pub);
    std::cout << "pub: " << publicRSA << endl;
    std::cout << "pri: " << privateRSA << endl;


    Connect:

    cli.sockfd = socket(AF_INET,SOCK_STREAM, 0);
    cli.sock.sin_family=AF_INET;
    if(!make_connect(cli.sockfd, cli.sock)) {
        cout << "connect to server fail" << endl;
        exit(-1);
    }
    recvline(cli.sockfd, rlen, srvBuf, srvRes);
    std::cout << srvRes << endl;
    
    GetCommand:
    std::cout << prompt;
    cliIn.clear();
    if((getline(cin,cliIn)).eof()) {
        cout << "EOF!" << endl;
        goto End;
    }else if(cliIn==""){
        goto GetCommand;
    }
    //std::cout << "Typein: " << cliIn.c_str() << "#Len: " <<cliIn.length()<< endl;
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
        // Register
        ssend(cli.sockfd, cliIn);
        recvline(cli.sockfd, rlen, srvBuf, srvRes);
       std::cout << srvRes << endl;
    }else if((pos=cliIn.find("#")) > 0 && pos != string::npos){
        // Login
        cli.name = cliIn.substr(0,pos);
        if(!isNumber(cliIn.substr(pos+1))) {
           std::cout << "Port not a number" << endl;
        }else{
            cli.port = atoi(cliIn.substr(pos+1).c_str());
            ssend(cli.sockfd, cliIn);
            if(getlist(cli,srvRes,rlen,srvBuf,onInfo) == false){
                std::cout << "220 AUTH_FAIL" << endl;    
                cli.name.clear(); 
            }else{
                cout << "login success, port: " << cli.port << endl; 
                cli.stage = 1;
                prompt = cli.name + prompt;
                tthread = new thread(&trader,std::ref(cli),publicRSA,privateRSA,prompt);
            }
        }
        cout << "COninuted Run" << endl;
    }else {
       std::cout << "Invalid Options" << endl;
       std::cout << "Options:\n1)Register: REGISTER#<UserAccountName><CRLF>\n2)Login: <UserAccount>#<portNum><CRLF>" << endl;
    }
    goto GetCommand;

    Logined:
    if(cliIn.compare("List")==0){
        getlist(cli,srvRes,rlen,srvBuf,onInfo);
    }else if(cliIn.compare("Exit")==0){
        delete tthread;
        ssend(cli.sockfd, cliIn);
        recvline(cli.sockfd, rlen, srvBuf, srvRes);
        std::cout << srvRes << endl;
        goto End;
    }else if(cliIn.compare("Logout")==0){
        delete tthread;
        ssend(cli.sockfd, cliIn);
        recvline(cli.sockfd, rlen, srvBuf, srvRes);
        close(cli.sockfd);
        cli.name.clear();
        cli.stage = 0;
        prompt = "$ ";
        goto Connect;
    }else if(cliIn.find("Give") == 0 && (deli1=cliIn.find(" ")) == 4 && (deli2=cliIn.find(" ",deli1+1)) != string::npos){
        // Give [name] 1000
        getlist(cli,srvRes,rlen,srvBuf,onInfo);
        make_trade(cli, cliIn,deli1,deli2,privateRSA, RSA_size(publicRSA),onInfo);
    }else{
       std::cout << "Invalid Options" << endl;
       std::cout << "Options:\n1)List: List<CRLF>\n2)Exit: Exit<CRLF>\n3)Logout: Logout" << endl;
    }
    goto GetCommand;
    End:

    std::cout << "Close socket" << endl;
    tthread->join();
    delete tthread;
    close(cli.sockfd);
}

bool make_connect(int sockfd, struct sockaddr_in& sock){
    char tmp[INET_ADDRSTRLEN];
    cout << "Make connect: " << inet_ntop(AF_INET,&(sock.sin_addr),tmp, INET_ADDRSTRLEN) << ntohs(sock.sin_port) << endl;
    if(connect(sockfd, (struct sockaddr*) &(sock), sizeof(sock)) <0) {
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
        return false;
    }
    return true;
}