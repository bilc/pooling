
curl -X PUT    http://10.94.97.161:4080/v1/catalog/register -d \
'{
"Datacenter":"dc1",
"Node":"foobar",
"Address":"127.0.0.1",
"Service":{
"ID":"thrift1",
"Service":"thrift",
"Tags":[
"master",
"v1"
],
"Address":"127.0.0.1",
"TaggedAddresses":{
"wan":"127.0.0.1"
},
"Port":8000
},
"Check":{
"Node":"foobar",
"CheckID":"service:thrift1",
"Name":"Redishealthcheck",
"Notes":"Scriptbasedhealthcheck",
"Status":"passing",
"ServiceID":"thrift1"
}
}'
