//
//  masstree.h
//

#ifndef YCSB_C_MASSTREE_DB_H_
#define YCSB_C_MASSTREE_DB_H_

#include "core/db.h"

#include <iostream>
#include <string>
#include <unordered_map>
#include "core/properties.h"

#include "mtbenchclient.hh"

using std::cout;
using std::endl;

namespace ycsbc {

class MasstreeDB : public DB {
 public:
  // TODO: use init and close
  MasstreeDB(utils::Properties &props);
  ~MasstreeDB();

  /*
  MasstreeDB() {
    int childno = 1;
    int ret = setup_child(c, childno);
    if (ret < 0) {
      throw "MasstreeDB: setup_child failed";
    }
    client.c_ = &c;
  }

  ~MasstreeDB() {
    teardown_child(c);
  }
  */

  void Init();
  
  void Close();

  int Read(const std::string &table, const std::string &key,
           const std::vector<std::string> *fields,
           std::vector<KVPair> &result);

  int Scan(const std::string &table, const std::string &key,
           int len, const std::vector<std::string> *fields,
           std::vector<std::vector<KVPair>> &result) {
    throw "Scan: function not implemented!";
  }

  int Update(const std::string &table, const std::string &key,
             std::vector<KVPair> &values);

  int Insert(const std::string &table, const std::string &key,
             std::vector<KVPair> &values) {
    return Update(table, key, values);
  }

  int Delete(const std::string &table, const std::string &key);

 private:
  std::unordered_map<pid_t, kvbench_client *> clients;
};

} // ycsbc

#endif // YCSB_C_REDIS_DB_H_

