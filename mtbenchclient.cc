// #include "json.hh"
// #include <sys/socket.h>
// #include <unistd.h>

#include "mtbenchclient.hh"


//  TODO: these are all copied from mtclient. I don't like how they're structured.

const char *TEMPserverip = "127.0.0.1";

// static int children = 1;
// static uint64_t nkeys = 0;
// static int prefixLen = 0;
// static int keylen = 0;
// static uint64_t limit = ~uint64_t(0);
// double TEMPduration = 10;
// double TEMPduration2 = 0;
int TEMPudpflag = 0;
// int TEMPquiet = 0;
int TEMPfirst_server_port = 2117;
// Should all child processes connects to the same UDP PORT on server
bool TEMPshare_server_port = false;
volatile bool TEMPtimeout[2] = {false, false};
int TEMPfirst_local_port = 0;
const char *TEMPinput = NULL;
// static int rsinit_part = 0;
int TEMPkvtest_first_seed = 0;
// static int rscale_partsz = 0;
// static int getratio = -1;
// static int minkeyletter = '0';
// static int maxkeyletter = '9';

//  LATER: could implement.
// KVConn *create_KVConn(int id) {

// }

kvbench_client::kvbench_client(int id = 0) {
    //  Based on mtclient's run_child().

    struct sockaddr_in sin;
    int ret, yes = 1;

    // struct child c;
    // bzero(&c, sizeof(c));
    // c.childno = childno;

    if(TEMPudpflag){
        udp = 1;
        s = socket(AF_INET, SOCK_DGRAM, 0);
    } else {
        s = socket(AF_INET, SOCK_STREAM, 0);
    }
    if (TEMPfirst_local_port) {
        bzero(&sin, sizeof(sin));
        sin.sin_family = AF_INET;
        sin.sin_port = htons(TEMPfirst_local_port + (id % 48));
        ret = ::bind(s, (struct sockaddr *) &sin, sizeof(sin));
        if (ret < 0) {
        perror("bind");
        //  TODO: not sure if I want to do the exits here, later in the function, and in KVConn constructor.
        exit(1);
        }
    }

    assert(s >= 0);
    setsockopt(s, IPPROTO_TCP, TCP_NODELAY, &yes, sizeof(yes));

    bzero(&sin, sizeof(sin));
    sin.sin_family = AF_INET;
    if (TEMPudpflag && !TEMPshare_server_port)
        sin.sin_port = htons(TEMPfirst_server_port + (id % 48));
    else
        sin.sin_port = htons(TEMPfirst_server_port);
    sin.sin_addr.s_addr = inet_addr(TEMPserverip);
    ret = connect(s, (struct sockaddr *) &sin, sizeof(sin));
    if(ret < 0){
        perror("connect");
        exit(1);
    }

    conn = new KVConn(s, !TEMPudpflag);
}

kvbench_client::~kvbench_client() {
    // Based on mtclient's run_child().

    delete conn;
    close(s);
}

int kvbench_client::get(const Str &key, char *val, int max) {
    // Based on mtclient's get()

    unsigned sseq = seq_;
    ++seq_;

    conn->sendgetwhole(key, sseq);
    conn->flush();

    const Json &result = conn->receive();
    always_assert(result && result[0] == sseq);
    if (!result[2])
        //  TODO: I presume this means not found?
        return -1;
    always_assert(result.size() == 3 && result[2].is_s()
                  && result[2].as_s().length() <= max);
    memcpy(val, result[2].as_s().data(), result[2].as_s().length());
    return result[2].as_s().length();
}

int kvbench_client::put(const Str &key, const Str &val) {
    // Based on mtclient's put()

    unsigned int sseq = seq_;
    conn->sendputwhole(key, val, sseq);
    conn->flush();

    const Json &result = conn->receive();
    always_assert(result && result[0] == sseq);

    //  TODO: in get, the increment occurs pre-send. Why does it occur post-send here?
    ++seq_;

    return 0;
}

bool kvbench_client::remove(const Str &key)
{
    // Based on mtclient's remove()

    unsigned int sseq = seq_;
    conn->sendremove(key, sseq);
    conn->flush();

    const Json &result = conn->receive();
    always_assert(result && result[0] == sseq);

    //  TODO: how does the return value work
    return result[2].to_b();
}