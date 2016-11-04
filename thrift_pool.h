#include <thrift/transport/TSocket.h>
#include <boost/shared_ptr.hpp>
#include "conn_pool.h"
#include "consul.h"

template<class Transport_, class Proto_, class Client_>
Connection* CreateConn(const std::string &host, int port) {
    boost::shared_ptr<apache::thrift::transport::TTransport> socket(
            new apache::thrift::transport::TSocket(host.c_str(), port));
    boost::shared_ptr<apache::thrift::transport::TTransport> transport(new Transport_(socket));
    boost::shared_ptr<apache::thrift::protocol::TProtocol> protocol(new Proto_(transport));

    try {
        Client_ *c = new Client_(protocol);
        transport->open();
        return c;
    } catch (apache::thrift::TException& tx) {
        return NULL;
    } 
    return NULL;
}

template<class Client_>
void CloseConn(Connection* c) {
    Client_* cli = (Client_ *)c;
    cli->getOutputProtocol()->getTransport()->close(); 
    delete cli;
}

template<class Transport_, class Proto_, class Client_>
class ThriftPool {
    public:
        ThriftPool(const std::string &url, int maxOpen) {
            pool_ = new ConnPool(CreateConn<Transport_, Proto_, Client_>, CloseConn<Client_>, maxOpen);
            detector_ = new ConsulService(url, 10, pool_);
        }

        bool Run(){
            return detector_->Run();
        }

        Client_ *GetClient() {
            Connection *c = pool_->GetConn();           
            return (Client_*)c;
        }

        void ReleaseClient(Client_ *c, bool isHealthy) {
            pool_->ReleaseConn(c, isHealthy);
        }

    private:
        ConnPool *pool_;
        Detector *detector_;
};

