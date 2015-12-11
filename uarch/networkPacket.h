#ifndef MANIFOLD_UARCH_NETWORKPACKET_H
#define MANIFOLD_UARCH_NETWORKPACKET_H

namespace manifold {
namespace uarch {


class NetworkPacket {
public:
    static const int MAX_SIZE = 256;

    int get_type() { return type; }
    void set_type(int t) { type = t; }
    int get_src() { return src; }
    void set_src(int s) { src = s; }
    int get_src_port() { return src_port; }
    void set_src_port(int s) { src_port = s; }
    int get_dst() { return dst; }
    void set_dst(int d) { dst = d; }
    int get_dst_port() { return dst_port; }
    void set_dst_port(int d) { dst_port = d; }

    int type;
    int src;
    int src_port;
    int dst;
    int dst_port;
    char data[MAX_SIZE];
    int data_size;
};


} // namespace uarch
} //namespace manifold

#endif // MANIFOLD_UARCH_NETWORKPACKET_H
