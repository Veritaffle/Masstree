//
//  masstree_db.cc
//  YCSB-C
//

#include "masstree_db.h"
#include "mtbenchclient.hh"

#include <cstring>
#include <sys/unistd.h>
#include <vector>

using namespace std;

namespace ycsbc {

MasstreeDB::MasstreeDB(utils::Properties &props) {
  // int num_threads = stoi(props["threadcount"]);
  // clients = new (kvbench_client *)[num_threads];
}

MasstreeDB::~MasstreeDB() {

}

void MasstreeDB::Init() {
  clients.emplace(gettid(), new kvbench_client(gettid() % 48));
}

void MasstreeDB::Close() {
  clients.erase(gettid());
}

int MasstreeDB::Read(const string &table, const string &key,
         const vector<string> *fields,
         vector<KVPair> &result) {
  // TODO: for now, don't use columns (fields).
  
  // cout << "Read: " << key << endl;

  // if (fields) {
  //   throw "Read: field reads not implemented!";
  // }
  // else {
  kvbench_client &client = *(clients[gettid()]);
  char val[512];
  int ret = client.get(Str(key), val, sizeof(val));
  if (ret < 0) {
    //  Entry not found.
    return DB::kOK;
  }
  result.push_back(make_pair(
    key,
    std::string(val)
  ));
  return DB::kOK;
  


  /*
  if (fields) {
    int argc = fields->size() + 2;
    const char *argv[argc];
    size_t argvlen[argc];
    int i = 0;
    argv[i] = "HMGET"; argvlen[i] = strlen(argv[i]);
    argv[++i] = key.c_str(); argvlen[i] = key.length();
    for (const string &f : *fields) {
      argv[++i] = f.data(); argvlen[i] = f.size();
    }
    assert(i == argc - 1);
    redisReply *reply = (redisReply *)redisCommandArgv(
        redis_.context(), argc, argv, argvlen);
    if (!reply) return DB::kOK;
    assert(reply->type == REDIS_REPLY_ARRAY);
    assert(fields->size() == reply->elements);
    for (size_t i = 0; i < reply->elements; ++i) {
      const char *value = reply->element[i]->str;
      result.push_back(make_pair(fields->at(i), string(value ? value : "")));
    }
    freeReplyObject(reply);
  } else {
    redisReply *reply = (redisReply *)redisCommand(redis_.context(),
        "HGETALL %s", key.c_str());
    if (!reply) return DB::kOK;
    assert(reply->type == REDIS_REPLY_ARRAY);
    for (size_t i = 0; i < reply->elements / 2; ++i) {
      result.push_back(make_pair(
          string(reply->element[2 * i]->str),
          string(reply->element[2 * i + 1]->str)));
    }
    freeReplyObject(reply);
  }
  return DB::kOK;
  */
}

int MasstreeDB::Update(const string &table, const string &key,
           vector<KVPair> &values) {
  // cout << "Update: function not implemented!" << endl;

  // cout << "Update: " << key << " " << values[0].second << endl;

  kvbench_client &client = *(clients[gettid()]);
  // char val[512];
  int ret = client.put(Str(key), Str(values[0].second));
  
  return DB::kOK;

  // string cmd("HMSET");
  // size_t len = cmd.length() + key.length() + 1;
  // for (KVPair &p : values) {
  //   len += p.first.length() + p.second.length() + 2;
  // }
  // cmd.reserve(len);

  // cmd.append(" ").append(key);
  // for (KVPair &p : values) {
  //   assert(p.first.find(' ') == string::npos);
  //   cmd.append(" ").append(p.first);
  //   assert(p.second.find(' ') == string::npos);
  //   cmd.append(" ").append(p.second);
  // }
  // assert(cmd.length() == len);
  // redis_.Command(cmd);
  // return DB::kOK;
}

int MasstreeDB::Delete(const std::string &table, const std::string &key) {
  cout << "Delete: function not implemented!" << endl;
  exit(1);
}

} // namespace ycsbc
