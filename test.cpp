#include <string>
#include <cassert>

#include <rte_eal.h>
#include <rte_memcpy.h>
#include <rte_ethdev.h>

#include <netinet/ip.h>
#include <netinet/ether.h>

#include "common.hpp"
#include "gtest/gtest.h"

#include <fstream>

static uint32_t N;

#define MAXN 32

class NIC;
NIC **NICs;

int ip2nic[MAXN];

class NIC{
public:
    NIC(int _idx): idx(_idx) {}

    void init() {
        assert(!rte_eth_dev_info_get(idx, &m_devInfo) && "Get Dev info failed");
        m_conf.link_speeds = 0;
        m_conf.rxmode = {
            .mq_mode = ETH_MQ_RX_NONE,
            .mtu = 1500,
            .max_lro_pkt_size = 0,
            .split_hdr_size = 0
        };
        m_conf.txmode = {
            .mq_mode = ETH_MQ_TX_NONE,
            .offloads =
                DEV_TX_OFFLOAD_VLAN_INSERT |
                DEV_TX_OFFLOAD_IPV4_CKSUM  |
                DEV_TX_OFFLOAD_UDP_CKSUM   |
                DEV_TX_OFFLOAD_TCP_CKSUM   |
                DEV_TX_OFFLOAD_SCTP_CKSUM  |
                DEV_TX_OFFLOAD_TCP_TSO,
        };
        m_conf.lpbk_mode = 0;
        m_conf.rxmode.offloads &= m_devInfo.rx_offload_capa;
        m_conf.txmode.offloads &= m_devInfo.tx_offload_capa;
        uint16_t num_rx_desc = 128;
        uint16_t num_tx_desc = 128;
        assert(!rte_eth_dev_adjust_nb_rx_tx_desc(idx, &num_rx_desc, &num_tx_desc) && "Failed to set desc number");
        assert(!rte_eth_dev_configure(idx, 1, 1, &m_conf) && "Failed to configure NIC");

        m_mempool = rte_mempool_create(
            std::to_string(idx).c_str(),
            512,
            1514 + RTE_PKTMBUF_HEADROOM,
            64,
            sizeof(struct rte_pktmbuf_pool_private),
            rte_pktmbuf_pool_init, nullptr,
            rte_pktmbuf_init, nullptr, 
            rte_eth_dev_socket_id(idx), 0
        );
        assert(m_mempool && "Failed to create mempool");
        auto rxq_conf = m_devInfo.default_rxconf;
        rxq_conf.offloads = m_conf.rxmode.offloads;
        assert(!rte_eth_rx_queue_setup(idx, 0, num_rx_desc, rte_socket_id(), &rxq_conf, m_mempool) && "Failed to setup rx queue");

        auto txq_conf = m_devInfo.default_txconf;
        txq_conf.offloads = m_conf.txmode.offloads;
        assert(!rte_eth_tx_queue_setup(idx, 0, num_tx_desc, rte_socket_id(), &txq_conf) && "Failed to setup tx queue");

        assert(!rte_eth_promiscuous_enable(idx) && "Failed to enable promiscuous mode");
        assert(!rte_eth_dev_start(idx) && "Failed to start NIC");
    }
    
    // buf must large than 1500 bytes
    int read(char* buf) {
        if (rx_cnt == rx_read) {
            for (int i = 0; i < rx_cnt; i ++) {
                rte_pktmbuf_free(rx_buff[i]);
            }
            rx_cnt = rte_eth_rx_burst(idx, 0, rx_buff, 16);
            rx_read = 0;
        }
        if (rx_cnt == rx_read)
            return 0;
        auto cur = rx_read;
        ++rx_read;
        
        rte_memcpy(buf, rte_pktmbuf_mtod(rx_buff[cur], char*), rx_buff[cur]->pkt_len);
        return rx_buff[cur]->pkt_len;
    }

    int write(const char* buf, int len) {
        tx_buff[0] = rte_pktmbuf_alloc(m_mempool);
        assert (!this->tx_buff[0] && "Mempool ran out");
        tx_buff[0]->pkt_len = tx_buff[0]->data_len = len;
        tx_buff[0]->nb_segs = 1;
        tx_buff[0]->next = NULL;

        auto ret = rte_pktmbuf_mtod(tx_buff[0], char*);
        tx_buff[0]->ol_flags = 0;
        
        rte_memcpy(ret, buf, len);
        assert (rte_eth_tx_burst(idx, 0, tx_buff, 1) == 1 && "Failed to send packet");
        return 0;
    }

    void sendIP(const char* src, const char* dst, const char* payload, int len) {
        char tmp[1600];
        auto ethh = reinterpret_cast<struct ethhdr*>(tmp);
        ethh->h_proto = ETH_P_IP;

        auto iph = reinterpret_cast<struct iphdr*>(tmp + sizeof(struct ethhdr));
        iph->ihl = 5;
        iph->version = 4;
        iph->tos = 0;
        auto tot_len = (sizeof(struct ethhdr) + len + 3) >> 2 << 2;
        iph->tot_len = htons(tot_len);
        iph->id = 0;
        iph->frag_off = 0;
        iph->ttl = 24;
        iph->protocol = IPPROTO_TCP;
        iph->check = 0;
        iph->saddr = inet_addr(src);
        iph->daddr = inet_addr(dst);

        this->write(tmp, tot_len + sizeof(struct ethhdr));
    }

    int idx;

private:
    struct rte_eth_dev_info m_devInfo;
    struct rte_eth_conf m_conf;
    struct rte_mempool* m_mempool;
    
    uint16_t rx_cnt = 0;
    uint16_t rx_read = 0;
    struct rte_mbuf* rx_buff[16];
    struct rte_mbuf* tx_buff[1];
};

class {
public:
    void init() {
        listen_fd = socket(AF_INET, SOCK_STREAM, 0);
        assert(listen_fd > 0 && "Failed to allocate listen fd");

        sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(23233);
        addr.sin_addr.s_addr = inet_addr("10.1.0.1");

        assert(bind(listen_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(sockaddr)) == 0 && "Bind failed");
        assert(listen(fd, 128) == 0 && "Listen failed");

        fd = accept(listen_fd, nullptr, nullptr);
        assert(fd > 0 && "Failed to wait client connect");
    }

    void addLPM(uint32_t ip, uint8_t cidr, uint8_t port) {
        Message msg;
        msg.addr = ip;
        msg.cidr = cidr;
        msg.dst_port = port;
        msg.type = MessageType::AddRule;
        assert(msg.sendTo(fd) == MessageSendResult::Success && "Router closed");
    }

    void delLPM(uint32_t ip, uint8_t cidr) {
        Message msg;
        msg.addr = ip;
        msg.cidr = cidr;
        msg.type = MessageType::DelRule;
        assert(msg.sendTo(fd) == MessageSendResult::Success && "Router closed");
    }

private:
    int listen_fd;
    int fd;
} router;

MessageSendResult Message::sendTo(int fd) {
    this->length = htonl(static_cast<uint32_t>(sizeof(Message)));
    auto all = sizeof(Message);
    uint32_t sent = 0;
    while (sent < all) {
        auto tmp = send(fd,
            reinterpret_cast<char*>(this) + sent,
            all - sent, 0);
        if (tmp <= 0)
            return MessageSendResult::RouterClosed;
        sent += tmp;
    }
    this->length = sizeof(Message);
    return MessageSendResult::Success;
}

int main(int argc, char** argv) {
    if (argc != 2) {
        assert(0 && "./test <testcase/interactive>");
    }
    std::string option = std::string(argv[1]);
    if (option == "testcase") {
        argc --;
        ::testing::InitGoogleTest(&argc, argv + 1);
        router.init();
        
        /** Read settings **/
        std::ifstream fin;
        fin.open("../test_setting", std::ios_base::in);
        assert(fin.is_open() && "Failed to open test_setting");
        {
            std::string opt;
            while (fin >> opt) {
                if (opt == "N")
                    assert ((fin >> N) && "Failed to read N");
                else
                    assert (0 && "Unknown test_settings option");
            }
        }
        fin.close();
        
        /** validate options **/
        assert(N > 0 && "N should > 0");
        assert(N <= MAXN && "N should <= MAXN (32)");

        /** Init DPDK **/
        {
            int margc = 4;
            char** margv = new char*[margc];
            std::string argvs[] = {
                "./main",
                "-c",
                "0xff",
                "--log-level=critical"
            };
            for (int i = 0; i < margc; i ++) {
                margv[i] = (char*)argvs[i].c_str();
            }
            rte_eal_init(margc, margv);
            delete [] margv;
        }
        
        /** Init local NICs **/
        NICs = new NIC*[N];
        for (int i = 0; i < N; i ++) {
            NICs[i] = new NIC(i);
            NICs[i]->init();
        }

        /** Send settings to peer **/
        for (int i = 0; i < N; i ++) {
            router.addLPM(inet_addr(("10.1.1." + std::to_string(i + 1)).c_str()), 32, i);
        }

        /** Load remote settings **/
        for (int i = 0; i < N; i ++) {
            NICs[0]->sendIP("0.0.0.0", ("10.1.1." + std::to_string(i + 1)).c_str(),
                "123123", 6);
            char ret[1600];
            bool found = false;
            while (!found) {
                for (int j = 0; j < N; j ++) {
                    auto len = NICs[j]->read(ret);
                    if (len < 14 + 20 + 6)
                        continue;
                    char* ptr = ret + 34;
                    if (memcmp(ptr, "123123", 6) == 0) {
                        found = true;
                        ip2nic[i] = j;
                    }
                }
            }
        }

        return RUN_ALL_TESTS();
    } else if (option == "interactive") {
        router.init();
        assert(0 && "Interactive mode is working in progress");
        return 0;
    } else {
        assert(0 && "Unknown option");
    }
    
    return 0;
}
