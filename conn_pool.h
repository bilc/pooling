#ifndef _CONN_POOL_H
#define _CONN_POOL_H

#include <map>
#include <vector>
#include <string>
#include <pthread.h>

typedef void Connection ;

//create and connect
typedef Connection* (*CreateConnFun)(const std::string& host, int port);
//close and free
typedef void (*CloseConnFun)(Connection*);

class Host;
class ConnPool {
    public:
        ConnPool(CreateConnFun f1, CloseConnFun f2, int defaultConns);
        ~ConnPool();

        Connection *GetConn();
        void ReleaseConn(Connection *c, bool isRight);

        void AddHost(const std::string& host, int port, int maxConn=0);
        void DelHost(const std::string& host, int port);

    private:
        Host *connHost(Connection *);
        void addConnInuse(Connection *c, Host *h) ;
        void delConnInuse(Connection *c) ;

        uint64_t count_;
        int defaultConns_;
        std::vector<Host *> hosts_;
        pthread_mutex_t mutex_;

        std::map<void *, Host *> inUse_; //connection in use
        pthread_mutex_t inUseMutex_;

        CreateConnFun createConn_;
        CloseConnFun closeConn_;
};

#endif
