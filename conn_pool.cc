#include <assert.h>
#include "conn_pool.h"

#define DEFAULT_MAX_OPEN_CONN  10
using std::vector;
using std::string;

class Host {
    public:
        Host(string host, int port, int maxConn, CreateConnFun f, CloseConnFun f2);
        ~Host();
        Connection *GetConn();
        void ReleaseConn(Connection *c, bool isRight);

        void SetInvalid();
        bool IsValid();
        bool CanBeRelease();

    private:
        string ip_;
        int port_;
        int maxOpen_;
        int numOpen_;
        int inUse_;
        bool valid_; //true: can use; false: 
        vector<Connection*> freeConns_;
        pthread_mutex_t mutex_;

        CreateConnFun createConn_;
        CloseConnFun closeConn_;
        //ConnPool *pool_;
        friend class ConnPool;
};

Host::Host(string host, int port, int maxConn, CreateConnFun createFun, CloseConnFun closeFun):
    ip_(host), port_(port), maxOpen_(maxConn), 
    numOpen_(0), inUse_(0), valid_(true), createConn_(createFun), closeConn_(closeFun) {
        pthread_mutex_init(&mutex_, NULL);
        if (maxOpen_ <= 0) maxOpen_ = DEFAULT_MAX_OPEN_CONN;
    }

Host::~Host() {
    pthread_mutex_destroy(&mutex_);
}

Connection *Host::GetConn() {
    Connection * conn = NULL;
    if ( 0 == pthread_mutex_trylock(&mutex_)) {
        if (valid_) {
            if (freeConns_.size() == 0 && numOpen_ < maxOpen_) {
                conn = createConn_(ip_, port_);
                if (conn != NULL) {
                    numOpen_++;
                    inUse_++;
                }
            } else if (freeConns_.size()) {
                conn = freeConns_.back();
                freeConns_.pop_back();
                inUse_++;
            }
        }
        pthread_mutex_unlock(&mutex_);
    } 
    //printf("%s:%d %d %d %d\n", ip_.c_str(), port_, numOpen_, inUse_, maxOpen_);
    return conn;
}

void Host::ReleaseConn(Connection *conn, bool isRight) {
    pthread_mutex_lock(&mutex_);
    if (valid_ && isRight) {
        freeConns_.push_back(conn);
        inUse_--;
    } else {
        closeConn_(conn);
        numOpen_--;
        inUse_--;
    }
    pthread_mutex_unlock(&mutex_);
}

void Host::SetInvalid() {
    valid_ = false;
}

bool Host::IsValid() {
    return valid_;
}

bool Host::CanBeRelease() {
    bool ret = false;
    pthread_mutex_lock(&mutex_);
    if (!valid_) {
        if (freeConns_.size() != 0){
            do {
                Connection *conn = freeConns_.back();
                freeConns_.pop_back();
                numOpen_--;
            } while (freeConns_.size());
        }
        if (numOpen_ == 0) {
            ret = true;
        }
    } else {
        ret = false;
    }
    pthread_mutex_unlock(&mutex_);
    return ret;
}

ConnPool::ConnPool(CreateConnFun f1, CloseConnFun f2, int defaultConns):
    count_(0), defaultConns_(defaultConns), createConn_(f1), closeConn_(f2){
        pthread_mutex_init(&mutex_, NULL);
    }

ConnPool::~ConnPool(){
    assert(inUse_.size() == 0);
    for(vector<Host*>::iterator i = hosts_.begin(); i != hosts_.end(); ++i) {
        delete *i;
    }
}

Connection *ConnPool::GetConn() {
    Connection *conn = NULL;
    Host *h = NULL;

    pthread_mutex_lock(&mutex_);
    count_++;
    if (hosts_.size()) {
        int loc = count_ % hosts_.size();
        if (hosts_[loc]->IsValid()) {
            conn = hosts_[loc]->GetConn();
            h = hosts_[loc];
        } else {
            if (hosts_[loc]->CanBeRelease()) {//删去host节点
                Host *release = hosts_[loc];
                hosts_.erase(hosts_.begin()+loc);
                delete release;
            }
        } 
    }
    pthread_mutex_unlock(&mutex_);
    if (conn) {
        addConnInuse(conn, h);
    }
    return conn;
}

void ConnPool::ReleaseConn(Connection *c, bool isRight) {
    if (c) {
        Host *h = connHost(c);
        if (h) {
            h->ReleaseConn(c, isRight);
        }
        delConnInuse(c);
    }
}

void ConnPool::AddHost(const std::string& host, int port, int maxConn) {

    pthread_mutex_lock(&mutex_);
    bool exsit = false;
    for (vector<Host*>::iterator i = hosts_.begin(); i != hosts_.end(); ++i) {
        if ((*i)->ip_ == host && (*i)->port_ == port) {
            exsit = true;
            break;
        }
    }
    if (maxConn == 0) maxConn = defaultConns_;
    if (!exsit) {
        Host *h = new Host(host, port, maxConn, createConn_, closeConn_);
        hosts_.push_back(h);
    }
    pthread_mutex_unlock(&mutex_);
}

void ConnPool::DelHost(const std::string& host, int port) {
    pthread_mutex_lock(&mutex_);
    for (vector<Host *>::iterator it = hosts_.begin(); it!=hosts_.end(); ++it) {
        if ((*it)->ip_ == host && (*it)->port_ == port ) {
            (*it)->SetInvalid();
        }
    }
    pthread_mutex_unlock(&mutex_);
}

Host *ConnPool::connHost(Connection *c){
    Host *h = NULL;
    pthread_mutex_lock(&inUseMutex_);
    std::map<void *, Host *>::iterator i = inUse_.find(c);
    if ( i != inUse_.end()) {
        h = i->second;
    }
    pthread_mutex_unlock(&inUseMutex_);
    return h;
}

void ConnPool::addConnInuse(Connection *c, Host *h) {
    pthread_mutex_lock(&inUseMutex_);
    inUse_[c] = h;
    pthread_mutex_unlock(&inUseMutex_);
}

void ConnPool::delConnInuse(Connection *c) {
    pthread_mutex_lock(&inUseMutex_);
    inUse_.erase(c);
    pthread_mutex_unlock(&inUseMutex_);
}

