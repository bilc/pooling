#ifndef CONF_SERVICE_H
#define CONF_SERVICE_H

#include <set>
#include <utility>
#include "detector.h"

typedef std::pair<std::string, int> IpPortPair;
typedef std::set<IpPortPair> (*funGetIpPort)(const std::string &url, const std::string &header);

class ConsulService: public Detector {
    public:
        ConsulService(const std::string &url, int interval, ConnPool *p);        
        ~ConsulService() {
            pthread_cancel(thread_);
        }
        bool Run();
        bool DetectChanges();
        
    private:
        std::string url_;
        int interval_;
        std::set<std::pair<std::string, int> > oldIpPorts_;
        
        funGetIpPort getIpPorts_;
        pthread_t thread_;
        friend  void *threadRun(void *s) ;
};

#endif
