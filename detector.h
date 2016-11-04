#ifndef SERVICE_DISCOVERY_H
#define SERVICE_DISCOVERY_H
#include "conn_pool.h"

/*
 * detect the service's changes
 * you can use consul, etcd, zookeeper and so on.
 *
 * */
class Detector {
    public:
        Detector(ConnPool *p):pool_(p) {}
        //run with thread. cannot block the program.
        virtual bool Run() = 0;
    protected:
        ConnPool *pool_;
};

#endif
