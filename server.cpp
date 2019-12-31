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
#include <iostream>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <thread>
#include <memory>
#include "magic.h"
using namespace std;

void slave(int newfd, struct sockaddr_in* newSock, srvInfo* srv);
condition_variable cv;
mutex cvlock;
class Master{
public:
    typedef void (*func)(int, struct sockaddr_in*, srvInfo*);
    typedef struct Task{
        int fd=-1;
        struct sockaddr_in sock;
    } Task;
    Master(func fun, int num, srvInfo* ptr){
        slaveNum = maxNum = num;
        srvptr = ptr;
        f = fun;
        pools.resize(num);
        assigned.resize(num);
        for(int i=0;i<num;++i){
            assigned[i] = nullptr;
            pools[i] =  nullptr;
        }
    }
    ~Master() {
        while(!tasks.empty())
            tasks.pop();
        for(int i=0;i<maxNum;++i) {
            pools[i] = nullptr;
            assigned[i] = nullptr;
        }
    }
    int run(){
        for (int i=0;i<maxNum;++i){
            mout << "Create thread " << i << endl;
            pools[i] = std::make_shared<thread>(&Master::worker,this,i);
        }
        return 0;
    }
    void joinall(){
        for(auto& t:pools) {
            (*t).join();
        }
        return;
    }
    bool addTask(int fd, struct sockaddr_in& sock) {
        qlock.lock();
        shared_ptr<Task> atask(new Task{.fd=fd,.sock=sock});
        tasks.push(atask);
        qlock.unlock();
        cout << "Add success" << endl;
        cv.notify_all();
        return true;
    }
    
private:
    shared_ptr<Task> getTask() {
        qlock.lock();
        shared_ptr<Task> task = nullptr;
        if(!tasks.empty()) {
            task = tasks.front();
            tasks.pop();
        }
        qlock.unlock();
        return task;
    };
    void peeknotify() {
        qlock.lock();
        if(!tasks.empty()) {
            cv.notify_all();
        };
        qlock.unlock();
        return;
    }
    int worker(int id) {
        while (true) {
            assigned[id] = getTask();
            if (assigned[id] != nullptr) {
                mout << "Thread " << id << " process " << inet_ntoa(assigned[id]->sock.sin_addr) << "\nfd: " << assigned[id]->fd << endl;
                f(assigned[id]->fd, &(assigned[id]->sock), srvptr);
                peeknotify();
                assigned[id] = nullptr;
            }else{
                unique_lock<mutex> mLock(cvlock);
                cv.wait(mLock);
            }
        }
        mout << "Worker " << id << " dead" << endl;
        return 0;
    };
    func f;
    srvInfo* srvptr;
    int maxNum;
    int slaveNum;
    mutex qlock;
    queue< shared_ptr<Task> > tasks;
    vector< shared_ptr<thread> > pools;
    vector< shared_ptr<Task> > assigned;
};

int main(int argc, char **argv) {
    srvInfo srv;
    char c;

    srv.sock.sin_addr.s_addr = inet_addr("0.0.0.0");
    srv.sock.sin_family = AF_INET;
    while((c=getopt(argc, argv, "t:p:"))!=EOF) {
        switch(c){
        case 't':
            inet_pton(AF_INET, optarg, &(srv.sock.sin_addr)); 
            break;
        case 'p':    
            srv.sock.sin_port = htons(atoi(optarg));
            break;
        default:
            puts("Format: ./client -t <ip> -p <port>\n");
            exit(-1);
        }
    }

    if ((srv.sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("create socket");
        exit(1);
    }
    int TRUE=1;
    if (setsockopt(srv.sockfd, SOL_SOCKET, SO_REUSEADDR, &TRUE, sizeof(int))!=0) {
        perror("set reuseaddr");
        exit(1);
    }

    if(bind(srv.sockfd, (struct sockaddr*)&srv.sock, sizeof(srv.sock)) < 0) {
        perror("Bind fail");
        exit(1);
    }

    if(listen(srv.sockfd, 10) < 0) {
        perror("listen");
        exit(1);
    }
    int newfd = -1;
    struct sockaddr_in newSock;
    socklen_t newSockl = sizeof(newSock);
    Master master(&slave, 2, &srv);
    master.run();
    while(newfd = accept(srv.sockfd, (struct sockaddr*) &newSock, &newSockl)){
        if (newfd < 0) {
            perror("Accept fail");
            exit(1);
        }
        mout << "Connect from: " << inet_ntoa(newSock.sin_addr) << endl;
        // slave(newfd, &newSock, &srv);
        master.addTask(newfd, newSock);
    }
    master.joinall();
    // End:
    // mout << "Close socket" << endl;
    // close(srv.sockfd);
}

void slave(int newfd, struct sockaddr_in* newSockptr, srvInfo* srvptr) {
    auto& srv = *srvptr; 
    auto& newSock = *newSockptr;
    mout << "Hi" << endl;
    string cliIn;
    char cliBuf[1024];
    ssize_t inlen;
    cliInfo curCli(-1,newfd,newSock);
    size_t pos;
    int flag;
    string srvRes;
    
    mout << "Connect from: " << curCli.ip << endl;
    ssend(curCli.sockfd,"Accepted\n");
    GetCommand:
    mout << "Regist: " << srv.registerNum << endl;
    flag = 1120;
    if(recvline(curCli.sockfd, inlen, cliBuf, cliIn) == false) {
        flag = 7122;
        goto Exit;
    }
    mout << curCli.sock.sin_addr.s_addr << " Input: " << cliIn << "#Len" << cliIn.length() << endl;
    // mout << "Typein: " << cliIn.c_str() << "#Len: " <<cliIn.length()<< endl;
    switch(curCli.stage) {
        case 0:
            goto Welcome;
        case 1:
            goto Logined;
        default:
            goto Welcome;
    }

    Welcome:
    if((pos=cliIn.find("REGISTER#")) == 0 && cliIn.length() > 9){
        string tmpName = cliIn.substr(9);
        flag = 100;

        for(int i=0;i < srv.registerNum;++i) {
            auto cli = srv.getCli(i);
            if(cli.name == tmpName) {
                mout << "Depulicate Username: " << tmpName << endl;
                flag = 210;
                break;
            }
        }
        if(flag == 100) {
            mout << "Register Success: " << tmpName << endl;
            cliInfo newCli(srv.registerNum-1,10000,0,-1);
            newCli.name = tmpName;
            srv.regist(newCli);
        }
    }else if((pos=cliIn.find("#")) > 0 && pos != string::npos){
        string tmpName = cliIn.substr(0,pos);
        string port = cliIn.substr(pos+1).c_str();
        flag = 220;
        for(int i=0;i<srv.registerNum;++i) {
            auto cli = srv.getCli(i);
            if(cli.name == tmpName) {
                cli.online = 1;
                cli.openPort = atoi(port.c_str());
                cli.sock = curCli.sock;
                cli.sockfd = curCli.sockfd;
                cli.stage = 1;
                cli.openPort = port;
                cli.ip = curCli.ip;
                srv.setCli(i,cli);
                curCli = cli;
                flag = 90;
                mout << curCli.name << " login from " << curCli.sock.sin_addr.s_addr << endl;
                break;
            }
        }
        if(flag == 220) {
            mout << curCli.name << " login failed from" << curCli.sock.sin_addr.s_addr << endl;
        }
    }else if(cliIn.compare("Exit")==0) {  
        flag = 7122;
        goto Exit;
    }else {
        flag = 210;
    }
    goto ProcessFlag;


    ProcessFlag:
    switch(flag){
        case 100:
            ssend(curCli.sockfd,"100 OK\n");
            flag = 1120;
            break;
        case 90:
            flag = 1204;
            break;
        case 210:
            ssend(curCli.sockfd,"210 FAIL\n");
            flag = 1120;
            break;
        case 220:
            ssend(curCli.sockfd,"220 AUTH_FAIL\n");
            flag = 1120;
            break;
        case 300:
            ssend(curCli.sockfd,"Bye\n");
            close(curCli.sockfd);
            return;
            break;
        case 1204:
            // send list
            mout << "User: " << curCli.name <<  "Getlist" << endl;
            srvRes = srv.getList(curCli);
            ssend(curCli.sockfd,srvRes);
            flag = 1120;
            break;
        case 7122:
            // mout << "Something go wrong" << endl;
             close(curCli.sockfd);
            return;
            break;
        case 1120:
            // pass
            goto GetCommand;
            break;
    }
    srvRes.clear();
    goto ProcessFlag;

    Logined:
    if(cliIn.compare("List")==0){
        flag = 1204;    
    }else if(cliIn.compare("Exit")==0){
        flag = 300;
        Exit:
        for(int i=0;i<srv.registerNum;++i) {
            auto cli = srv.getCli(i);
            if(cli.name == curCli.name) {
                curCli.online = 0;
                curCli.stage = 0;
                srv.setCli(i, curCli);
                break;
            }
        }
        mout << "User: " << curCli.name <<  " Exit" << endl;
    }else {
        flag = 210;
    }
    goto ProcessFlag;
}
