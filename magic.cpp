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
bool srvInfo::isOnline(string& name){
    for(int i=0;i<registerNum;++i) {
        if(cliInfos[i].online) return true;
    }
    return false;
};
cliInfo srvInfo::getInfo(string& name){
    for(int i=0;i<registerNum;++i) {
        if(cliInfos[i].name==name)
        return cliInfos[i];
    }
    cout << "Error getInfo " << endl;
    exit(-1);
};

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

string srvInfo::checker(string& spayer, string& spayee, unsigned char* encMsg) {
    string spubkey,payRes;
    struct sockaddr_in sock;
    int payfd;
    ssize_t rlen;
    char payBuf[1024];
    sock.sin_family = AF_INET;

    RSA* publicRSA;
    BIO* bo;
   

    BIO_free(bo);
    RSA_free(publicRSA);

    if(isOnline(spayer) && isOnline(spayee)) {
        cliInfo payer = getInfo(spayer), payee = getInfo(spayee);
        inet_pton(AF_INET, payee.ip.c_str() , &(sock.sin_addr)); 
        sock.sin_port = htons(atoi(payee.openPort.c_str()));  
        if ((payfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            perror("create socket");
            exit(1);
        }
        if(!make_connect(payfd, sock)) {
            mout << "connect to payee fail" << endl;
            exit(-1);
        }
        ssend(payfd,"key?");
        mout << "Try get key: " << endl;
        char buf[0x200];
        recv(payfd, buf, 272,0);
        spubkey.copy(buf, 272);
        cout << spubkey;

        // for(int i=0;i<6;++i) {
        //     recvline(payfd, rlen, payBuf, payRes);
        //     mout << "line " << i << ": " << payRes << endl;
        //     spubkey += payRes + "\n";
        // }
        close(payfd);
        mout << "Publickey from payee: " << payee.name << ": \n" << spubkey << "##"<<endl; 
        BIO_write(bo, spubkey.c_str(),spubkey.length());
        PEM_read_bio_RSA_PUBKEY(bo, &publicRSA, 0, 0 );
        int rsa_len = RSA_size(publicRSA);
        const unsigned char * enc = (const unsigned char *)encMsg;
        unsigned char * dec = (unsigned char *)malloc(0x100);
    
        if(RSA_private_decrypt(rsa_len, (unsigned char*) encMsg,dec,publicRSA, RSA_PKCS1_PADDING) < 0){
            mout << "payee dec error" << endl;
            return "";
        }
        BIO_free(bo);
        RSA_free(publicRSA);
        spubkey.clear();
        inet_pton(AF_INET, payer.ip.c_str() , &(sock.sin_addr)); 
        sock.sin_port = htons(atoi(payer.openPort.c_str()));  
        if ((payfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            perror("create socket");
            exit(1);
        }
        if(!make_connect(payfd, sock)) {
            mout << "connect to payee fail" << endl;
            exit(-1);
        }
        ssend(payfd,"key?");
        recv(payfd, buf, 272,0);
        spubkey.copy(buf, 272);
        cout << spubkey;
        close(payfd);
        mout << "Publickey from payer: " << payer.name << ": \n" << spubkey << "##"<<endl; 
        BIO_write(bo, spubkey.c_str(),spubkey.length());
        PEM_read_bio_RSA_PUBKEY(bo, &publicRSA, 0, 0 );
        const unsigned char * enc2 = (const unsigned char *)dec;
        unsigned char * dec2 = (unsigned char *)malloc(0x100);

        if(RSA_private_decrypt(rsa_len, enc2,dec2,publicRSA, RSA_PKCS1_PADDING) < 0){
            mout << "payer dec error" << endl;
            return "";
        }
        BIO_free(bo);
        RSA_free(publicRSA);

        string plain((char*)dec2); 
        int deli1 = plain.find("#"),
            deli2 = plain.find("#",deli1+1);
            
        if( deli1 != string::npos &&
            deli2 != string::npos && 
            plain.substr(0,deli1) == payer.name && 
            plain.substr(deli2+1) == payee.name && 
            isNumber(plain.substr(deli1+1,deli2-deli1-1))) {
            return plain.substr(deli1+1,deli2-deli1-1);
        }
    }
    return "";
    
}
bool srvInfo::trailer(string& spayer, string& smoney, string& spayee) {
    cliInfo payer = getInfo(spayer), payee = getInfo(spayee);
    int money = atoi(smoney.c_str());
    if(payer.balance - money >= 0) {
        payer.balance -= money;
        payee.balance += money;
        if(setCli(payer.index,payer) && setCli(payee.index,payee))
            return true;
    }
    return false;
}
bool make_connect(int sockfd, struct sockaddr_in& sock){
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
Mout mout;