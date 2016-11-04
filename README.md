# pooling

conn_pool.h is general interface of connection pool.   
detector.h is general interface of service discovery, which may be consul, etcd and zookeeper.  

consul.h is implement of detector.  
thrift_pool.h is thrift call implement with consul and conn_pool.  
