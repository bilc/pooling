#include <thrift/transport/TSocket.h>  
#include <thrift/transport/TBufferTransports.h>  
#include <thrift/protocol/TBinaryProtocol.h>  
#include "../thrift_pool.h"
#include "gen-cpp/Demo.h"
  
using namespace apache::thrift;  
using namespace apache::thrift::protocol;  
using namespace apache::thrift::transport;  
  
using boost::shared_ptr;  

int main() {
    ThriftPool<TBufferedTransport, TBinaryProtocol, DemoClient> service("http://10.94.97.161:4080/v1/catalog/service/thrift", 10);
    bool ret = service.Run();
    if (!ret) {
        printf("service run false\n");
        return -1;
    }
    DemoClient *c = service.GetClient();
    if (c) {
        int res = c->Get(1);//
        printf("ret %d\n", res);
    } else {
        printf("NULL\n");
    }
    service.ReleaseClient(c, true);
}
