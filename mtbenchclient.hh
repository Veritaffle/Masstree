
#ifndef KVBC_HH
#define KVBC_HH 1

// using lcdf::Json;
// #include "str.hh"

// using lcdf::String;

#include "mtclient.hh"




//  Synchronous only.
//  Designed for use with YCSB.
struct kvbench_client {
    //  TODO: constructor should take info necessary to set up conn.
    kvbench_client(int id);

    ~kvbench_client();

    //  get (non-column)
    int get(const Str &key, char *val, int max);
    
    //  put (non-column)
    int put(const Str &key, const Str &val);
    
    //  remove
    bool remove(const Str &key);

    //  getrange (non-column)
    //  TODO
    


    int s;      //  socket
    int udp;    // 1 -> udp, 0 -> tcp
    KVConn *conn;

    unsigned seq_ = 0;
};

#endif