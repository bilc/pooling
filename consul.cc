#include <curl/curl.h>
#include <unistd.h>
#include <iostream>

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "consul.h"



using std::pair;
using std::set;
using std::string;

using namespace std;
using namespace rapidjson;


size_t write_data(void *ptr, size_t size, size_t nmemb, void *userdata) {
    char *pch = (char *)ptr;
    int len = size*nmemb;
    std::string *s = (std::string *)userdata;
    for (int i = 0; i < len; i++){
        s->push_back(pch[i]);
    }
    return len;
}

std::string getHttpBody(const std::string &url, const string &header){
    CURL *curl;
    curl_global_init(CURL_GLOBAL_ALL);
    curl=curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

    std::string res;
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &res);

    if (header.size()) {
        struct curl_slist *headers=NULL; /* init to NULL is important */
        headers = curl_slist_append(headers, header.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    }

    curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    return res;
}

set<pair<string, int> > extractIpPort(string &body) {
    std::set<pair<string,int> > s;

    Document d;
    d.Parse(body.c_str());
    if (d.IsArray()){
        for (SizeType i = 0; i < d.Size(); i++) // rapidjson uses SizeType instead of size_t.
            if (d[i]["ServiceAddress"].IsString() && d[i]["ServicePort"].IsInt()) {
                s.insert(make_pair(d[i]["ServiceAddress"].GetString(), 
                            d[i]["ServicePort"].GetInt()));
            }
    }
    return s;
}

std::set<IpPortPair> getIpPorts(const std::string &url, 
        const string &header){
    std::string body = getHttpBody(url, header);
    set<pair<string,int> > s = extractIpPort(body);
    return s;
}

ConsulService::ConsulService(const std::string &url, int interval, ConnPool *p) 
    :url_(url), interval_(interval), Detector(p){
    getIpPorts_ = getIpPorts;
}

set<pair<string,int> > minusSet(set<pair<string,int> >& s1, set<pair<string,int> >& s2) {
    set<pair<string,int> > s;
    for (set<pair<string,int> >::iterator i = s1.begin(); i != s1.end(); ++i){
        if (s2.find(*i) == s2.end()) {
            s.insert(*i);
        }
    }
    return s;
}

void *threadRun(void *s) {
    ConsulService *t = (ConsulService *)s;
    while (1) {
        t->DetectChanges();
        if (t->interval_ > 0) sleep(t->interval_);
    }
    return NULL;
}

bool ConsulService::DetectChanges() {
    string header;
    if (oldIpPorts_.size() == 0) {
        header = "X-Consul-Index: 30m";
    }
    set<pair<string,int> > s = getIpPorts_(url_, header);
    if (s.size() == 0) {
        return false;
    }
    
    set<pair<string,int> > add = minusSet(s, oldIpPorts_);
    set<pair<string,int> > del = minusSet(oldIpPorts_, s);
    
    for (set<pair<string,int> >::iterator i = add.begin(); i != add.end(); ++i){
        pool_->AddHost(i->first, i->second);
    }
    for (set<pair<string,int> >::iterator i = del.begin(); i != del.end(); ++i){
        pool_->DelHost(i->first, i->second);
    }
    oldIpPorts_ = s;
    return true;
}

bool ConsulService::Run() {
    bool res = this->DetectChanges();
    if (res == false) {
        return res;
    }
    pthread_create(&thread_, NULL, threadRun, this);
    return true;
}

