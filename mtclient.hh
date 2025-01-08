/* Masstree
 * Eddie Kohler, Yandong Mao, Robert Morris
 * Copyright (c) 2012-2013 President and Fellows of Harvard College
 * Copyright (c) 2012-2013 Massachusetts Institute of Technology
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, subject to the conditions
 * listed in the Masstree LICENSE file. These conditions include: you must
 * preserve this copyright notice, and you cannot mention the copyright
 * holders in advertising related to the Software without their permission.
 * The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
 * notice is a summary of the Masstree LICENSE file; the license in that file
 * is legally binding.
 */
#ifndef KVC_HH
#define KVC_HH 1
#include "kvproto.hh"
#include "kvrow.hh"
#include "json.hh"
#include "msgpack.hh"
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <string>
#include <queue>
#include <vector>

class KVConn {
  public:
    KVConn(const char *server, int port, int target_core = -1)
        : inbuf_(new char[inbufsz]), inbufpos_(0), inbuflen_(0),
          j_(Json::make_array()) {
        struct hostent *ent = gethostbyname(server);
        always_assert(ent);
        int fd = socket(AF_INET, SOCK_STREAM, 0);
        always_assert(fd > 0);
        fdtoclose_ = fd;
        int yes = 1;
        always_assert(fd >= 0);
        setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &yes, sizeof(yes));

        struct sockaddr_in sin;
        memset(&sin, 0, sizeof(sin));
        sin.sin_family = AF_INET;
        sin.sin_port = htons(port);
        memcpy(&sin.sin_addr.s_addr, ent->h_addr, ent->h_length);
        int r = connect(fd, (const struct sockaddr *)&sin, sizeof(sin));
        if (r) {
            perror("connect");
            exit(EXIT_FAILURE);
        }

        infd_ = fd;
        out_ = new_kvout(fd, 64*1024);
        handshake(target_core);
    }
    KVConn(int fd, bool tcp)
        : inbuf_(new char[inbufsz]), inbufpos_(0), inbuflen_(0), infd_(fd),
          j_(Json::make_array()) {
        out_ = new_kvout(fd, 64*1024);
        fdtoclose_ = -1;
        if (tcp)
            handshake(-1);
    }
    ~KVConn() {
        if (fdtoclose_ >= 0)
            close(fdtoclose_);
        free_kvout(out_);
        delete[] inbuf_;
        for (auto x : oldinbuf_)
            delete[] x;
    }
    void sendgetwhole(Str key, unsigned seq) {
        j_.resize(3);
        j_[0] = seq;
        j_[1] = Cmd_Get;
        j_[2] = String::make_stable(key);
        send();
    }
    void sendgetcol(Str key, int col, unsigned seq) {
        j_.resize(4);
        j_[0] = seq;
        j_[1] = Cmd_Get;
        j_[2] = String::make_stable(key);
        j_[3] = col;
        send();
    }
    void sendget(Str key, const std::vector<unsigned>& f, unsigned seq) {
        j_.resize(4);
        j_[0] = seq;
        j_[1] = Cmd_Get;
        j_[2] = String::make_stable(key);
        j_[3] = Json(f.begin(), f.end());
        send();
    }

    void sendputcol(Str key, int col, Str val, unsigned seq) {
        j_.resize(5);
        j_[0] = seq;
        j_[1] = Cmd_Put;
        j_[2] = String::make_stable(key);
        j_[3] = col;
        j_[4] = String::make_stable(val);
        send();
    }
    void sendputwhole(Str key, Str val, unsigned seq) {
        j_.resize(3);
        j_[0] = seq;
        j_[1] = Cmd_Replace;
        j_[2] = String::make_stable(key);
        j_[3] = String::make_stable(val);
        send();
    }
    void sendremove(Str key, unsigned seq) {
        j_.resize(3);
        j_[0] = seq;
        j_[1] = Cmd_Remove;
        j_[2] = String::make_stable(key);
        send();
    }

    void sendscanwhole(Str firstkey, int numpairs, unsigned seq) {
        j_.resize(4);
        j_[0] = seq;
        j_[1] = Cmd_Scan;
        j_[2] = String::make_stable(firstkey);
        j_[3] = numpairs;
        send();
    }
    void sendscan(Str firstkey, const std::vector<unsigned>& f,
                  int numpairs, unsigned seq) {
        j_.resize(5);
        j_[0] = seq;
        j_[1] = Cmd_Scan;
        j_[2] = String::make_stable(firstkey);
        j_[3] = numpairs;
        j_[4] = Json(f.begin(), f.end());
        send();
    }

    void checkpoint(int childno) {
	always_assert(childno == 0);
        fprintf(stderr, "asking for a checkpoint\n");
        j_.resize(2);
        j_[0] = 0;
        j_[1] = Cmd_Checkpoint;
        send();
        flush();

        printf("sent\n");
        (void) receive();
    }

    void flush() {
        kvflush(out_);
    }

    int check(int tryhard) {
        if (inbufpos_ == inbuflen_ && tryhard)
            hard_check(tryhard);
        return inbuflen_ - inbufpos_;
    }

    const Json& receive() {
        while (!parser_.done() && check(2))
            inbufpos_ += parser_.consume(inbuf_ + inbufpos_,
                                         inbuflen_ - inbufpos_,
                                         String::make_stable(inbuf_, inbufsz));
        if (parser_.success() && parser_.result().is_a())
            parser_.reset();
        else
            parser_.result() = Json();
        return parser_.result();
    }

  private:
    enum { inbufsz = 64 * 1024, inbufrefill = 56 * 1024 };
    char* inbuf_;
    int inbufpos_;
    int inbuflen_;
    std::vector<char*> oldinbuf_;
    int infd_;

    struct kvout *out_;

    Json j_;
    msgpack::streaming_parser parser_;

    int fdtoclose_;
    int partition_;

    void handshake(int target_core) {
        j_.resize(3);
        j_[0] = 0;
        j_[1] = Cmd_Handshake;
        j_[2] = Json::make_object().set("core", target_core)
            .set("maxkeylen", MASSTREE_MAXKEYLEN);
        send();
        kvflush(out_);

        const Json& result = receive();
        if (!result.is_a()
            || result[1] != Cmd_Handshake + 1
            || !result[2]) {
            fprintf(stderr, "Incompatible kvdb protocol\n");
            exit(EXIT_FAILURE);
        }
        partition_ = result[3].as_i();
    }
    inline void send() {
        msgpack::unparse(*out_, j_);
    }
    void hard_check(int tryhard);
};


typedef void (*get_async_cb)(struct child *c, struct async *a,
                             bool has_val, const Str &val);
typedef void (*put_async_cb)(struct child *c, struct async *a,
                             int status);
typedef void (*remove_async_cb)(struct child *c, struct async *a,
                                int status);

struct async {
    int cmd; // Cmd_ constant
    unsigned seq;
    union {
        get_async_cb get_fn;
        put_async_cb put_fn;
        remove_async_cb remove_fn;
    };
    char key[16]; // just first 16 bytes
    char wanted[16]; // just first 16 bytes
    int wantedlen;
    int acked;
};
#define MAXWINDOW 512
unsigned window = MAXWINDOW;

struct child {
    int s;
    int udp; // 1 -> udp, 0 -> tcp
    KVConn *conn;

    struct async a[MAXWINDOW];

    unsigned seq0_;
    unsigned seq1_;
    unsigned long long nsent_;
    int childno;

    inline void check_flush();
};

const char *serverip = "127.0.0.1";
static Json test_param;

void checkasync(struct child *c, int force);

void aget(struct child *, const Str &key, const Str &wanted, get_async_cb fn);
void aget(struct child *c, long ikey, long iwanted, get_async_cb fn);
void aget_col(struct child *c, const Str& key, int col, const Str& wanted,
              get_async_cb fn);
int get(struct child *c, const Str &key, char *val, int max);

void asyncgetcb(struct child *, struct async *a, bool, const Str &val);
void asyncgetcb_int(struct child *, struct async *a, bool, const Str &val);

void aput(struct child *c, const Str &key, const Str &val,
          put_async_cb fn = 0, const Str &wanted = Str());
void aput_col(struct child *c, const Str &key, int col, const Str &val,
              put_async_cb fn = 0, const Str &wanted = Str());
int put(struct child *c, const Str &key, const Str &val);
void asyncputcb(struct child *, struct async *a, int status);

void aremove(struct child *c, const Str &key, remove_async_cb fn);
bool remove(struct child *c, const Str &key);

void udp1(struct child *);
void w1b(struct child *);
void u1(struct child *);
void over1(struct child *);
void over2(struct child *);
void rec1(struct child *);
void rec2(struct child *);
void cpa(struct child *);
void cpb(struct child *);
void cpc(struct child *);
void cpd(struct child *);
void volt1a(struct child *);
void volt1b(struct child *);
void volt2a(struct child *);
void volt2b(struct child *);
void scantest(struct child *);



static int children = 1;
static uint64_t nkeys = 0;
static int prefixLen = 0;
static int keylen = 0;
static uint64_t limit = ~uint64_t(0);
double duration = 10;
double duration2 = 0;
int udpflag = 0;
int quiet = 0;
int first_server_port = 2117;
// Should all child processes connects to the same UDP PORT on server
bool share_server_port = false;
volatile bool timeout[2] = {false, false};
int first_local_port = 0;
const char *input = NULL;
static int rsinit_part = 0;
int kvtest_first_seed = 0;
static int rscale_partsz = 0;
static int getratio = -1;
static int minkeyletter = '0';
static int maxkeyletter = '9';

struct kvtest_client {
    kvtest_client(struct child& c)
        : c_(&c) {
    }
    struct child* child() const {
        return c_;
    }
    int id() const {
        return c_->childno;
    }
    int nthreads() const {
        return ::children;
    }
    char minkeyletter() const {
        return ::minkeyletter;
    }
    char maxkeyletter() const {
        return ::maxkeyletter;
    }
    void register_timeouts(int n) {
        (void) n;
    }
    bool timeout(int which) const {
        return ::timeout[which];
    }
    uint64_t limit() const {
        return ::limit;
    }
    unsigned getratio() const {
        assert(::getratio >= 0);
        return ::getratio;
    }
    uint64_t nkeys() const {
        return ::nkeys;
    }
    int keylen() const {
        return ::keylen;
    }
    int prefixLen() const {
        return ::prefixLen;
    }
    Json param(const String& name, Json default_value = Json()) {
        return test_param.count(name) ? test_param.at(name) : default_value;
    }
    double now() const {
        return ::now();
    }

    void get(long ikey, Str *value) {
        quick_istr key(ikey);
        aget(c_, key.string(),
             Str(reinterpret_cast<const char *>(&value), sizeof(value)),
             asyncgetcb);
    }
    void get(const Str &key, int *ivalue) {
        aget(c_, key,
             Str(reinterpret_cast<const char *>(&ivalue), sizeof(ivalue)),
             asyncgetcb_int);
    }
    bool get_sync(long ikey) {
        char got[512];
        quick_istr key(ikey);
        return ::get(c_, key.string(), got, sizeof(got)) >= 0;
    }
    void get_check(long ikey, long iexpected) {
        aget(c_, ikey, iexpected, 0);
    }
    void get_check(const char *key, const char *val) {
        aget(c_, Str(key), Str(val), 0);
    }
    void get_check(const Str &key, const Str &val) {
        aget(c_, key, val, 0);
    }
    void get_check_key8(long ikey, long iexpected) {
        quick_istr key(ikey, 8), expected(iexpected);
        aget(c_, key.string(), expected.string(), 0);
    }
    void get_check_key10(long ikey, long iexpected) {
        quick_istr key(ikey, 10), expected(iexpected);
        aget(c_, key.string(), expected.string(), 0);
    }
    void many_get_check(int, long [], long []) {
        assert(0);
    }
    void get_col_check(const Str &key, int col, const Str &value) {
        aget_col(c_, key, col, value, 0);
    }
    void get_col_check(long ikey, int col, long ivalue) {
        quick_istr key(ikey), value(ivalue);
        get_col_check(key.string(), col, value.string());
    }
    void get_col_check_key10(long ikey, int col, long ivalue) {
        quick_istr key(ikey, 10), value(ivalue);
        get_col_check(key.string(), col, value.string());
    }
    void get_check_sync(long ikey, long iexpected) {
        char key[512], val[512], got[512];
        sprintf(key, "%010ld", ikey);
        sprintf(val, "%ld", iexpected);
        memset(got, 0, sizeof(got));
        ::get(c_, Str(key), got, sizeof(got));
        if (strcmp(val, got)) {
            fprintf(stderr, "key %s, expected %s, got %s\n", key, val, got);
            always_assert(0);
        }
    }

    void put(const Str &key, const Str &value) {
        aput(c_, key, value);
    }
    void put(const Str &key, const Str &value, int *status) {
        aput(c_, key, value,
             asyncputcb,
             Str(reinterpret_cast<const char *>(&status), sizeof(status)));
    }
    void put(const char *key, const char *value) {
        aput(c_, Str(key), Str(value));
    }
    void put(const Str &key, long ivalue) {
        quick_istr value(ivalue);
        aput(c_, key, value.string());
    }
    void put(long ikey, long ivalue) {
        quick_istr key(ikey), value(ivalue);
        aput(c_, key.string(), value.string());
    }
    void put_key8(long ikey, long ivalue) {
        quick_istr key(ikey, 8), value(ivalue);
        aput(c_, key.string(), value.string());
    }
    void put_key10(long ikey, long ivalue) {
        quick_istr key(ikey, 10), value(ivalue);
        aput(c_, key.string(), value.string());
    }
    void put_col(const Str &key, int col, const Str &value) {
        aput_col(c_, key, col, value);
    }
    void put_col(long ikey, int col, long ivalue) {
        quick_istr key(ikey), value(ivalue);
        put_col(key.string(), col, value.string());
    }
    void put_col_key10(long ikey, int col, long ivalue) {
        quick_istr key(ikey, 10), value(ivalue);
        put_col(key.string(), col, value.string());
    }
    void put_sync(long ikey, long ivalue) {
        quick_istr key(ikey, 10), value(ivalue);
        ::put(c_, key.string(), value.string());
    }

    void remove(const Str &key) {
        aremove(c_, key, 0);
    }
    void remove(long ikey) {
        quick_istr key(ikey);
        remove(key.string());
    }
    bool remove_sync(long ikey) {
        quick_istr key(ikey);
        return ::remove(c_, key.string());
    }

    int ruscale_partsz() const {
        return ::rscale_partsz;
    }
    int ruscale_init_part_no() const {
        return ::rsinit_part;
    }
    long nseqkeys() const {
        return 16 * ::rscale_partsz;
    }
    void wait_all() {
        checkasync(c_, 2);
    }
    void puts_done() {
    }
    void rcu_quiesce() {
    }
    void notice(String s) {
        if (!quiet) {
            if (!s.empty() && s.back() == '\n')
                s = s.substr(0, -1);
            if (s.empty() || isspace((unsigned char) s[0]))
                fprintf(stderr, "%d%.*s\n", c_->childno, s.length(), s.data());
            else
                fprintf(stderr, "%d %.*s\n", c_->childno, s.length(), s.data());
        }
    }
    void notice(const char *fmt, ...) {
        if (!quiet) {
            va_list val;
            va_start(val, fmt);
            String x;
            if (!*fmt || isspace((unsigned char) *fmt))
                x = String(c_->childno) + fmt;
            else
                x = String(c_->childno) + String(" ") + fmt;
            vfprintf(stderr, x.c_str(), val);
            va_end(val);
        }
    }
    const Json& report(const Json& x) {
        return report_.merge(x);
    }
    void finish() {
        if (!quiet) {
            lcdf::StringAccum sa;
            double dv;
            if (report_.count("puts"))
                sa << " total " << report_.get("puts");
            if (report_.get("puts_per_sec", dv))
                sa.snprintf(100, " %.0f put/s", dv);
            if (report_.get("gets_per_sec", dv))
                sa.snprintf(100, " %.0f get/s", dv);
            if (!sa.empty())
                notice(sa.take_string());
        }
        printf("%s\n", report_.unparse().c_str());
    }
    kvrandom_random rand;
    struct child *c_;
    Json report_;
};

int setup_child(struct child &c, int childno);
void teardown_child(struct child &c);

#endif
