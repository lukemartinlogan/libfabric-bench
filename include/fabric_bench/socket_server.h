//
// Created by lukemartinlogan on 8/9/23.
//

#ifndef LIBFABRIC_BENCH_SRC_TCP_SERVER_H_
#define LIBFABRIC_BENCH_SRC_TCP_SERVER_H_

#include "socket_client.h"
#include <thread>

struct SocketServer {
  std::vector<char> data_;
  struct fi_info* info_;          /**< General fabric info */
  struct fi_info *hints_;       /**< Properties for creating info */
  struct fid_fabric* fabric_;   /**< Fabric ID */
  struct fid_domain* domain_;   /**< Fabric domain */
  struct fid_av *av_;           /**< Address vector */
  struct fid_pep *pep_;         /**< Passive endpoint */
  struct fid_ep* ep_;           /**< Active endpoint */
  struct fid_eq *eq_;           /**< Emission queue */
  struct fid_cq *cq_;           /**< Completion queue */
  struct fid_mr* mr_;           /**< Memory region (RDMA) */
  std::list<std::unique_ptr<SocketClient>> clients_;
  std::unique_ptr<std::thread> accept_thread_;
  std::string ip_addr_, port_str_;
  struct fi_eq_attr eq_attr = {
      .wait_obj = FI_WAIT_UNSPEC,
  };

  char* copy_string(const std::string &str) {
    char* ret = new char[str.size() + 1];
    std::copy(str.begin(), str.end(), ret);
    ret[str.size()] = '\0';
    return ret;
  }

  int ServerInit(const std::string &provider, int port, const std::string &ip_addr) {
    int ret;

    // Allocate hints
    hints_ = fi_allocinfo();
    hints_->fabric_attr->prov_name = copy_string(provider);
    hints_->caps = FI_MSG;
    hints_->ep_attr->type = FI_EP_MSG;
    hints_->domain_attr->mr_mode = FI_MR_BASIC;
    hints_->addr_format = FI_SOCKADDR_IN;
    ip_addr_ = ip_addr;
    port_str_ = std::to_string(port);
    ret = fi_getinfo(FI_VERSION(1, 14),
                     ip_addr_.c_str(), port_str_.c_str(),
                     FI_SOURCE, hints_, &info_);
    if (ret) {
      HELOG(kError, "Failed to get fabric info");
      return ret;
    }

    // Open a fabric domain & initialize endpoint
    ret = fi_fabric(info_->fabric_attr, &fabric_, NULL);
    if (ret) {
      HELOG(kError, "Failed to initialize fabric");
      return ret;
    }
    ret = fi_domain(fabric_, info_, &domain_, NULL);
    if (ret) {
      HELOG(kError, "Failed to initialize domain");
      return ret;
    }

    // Create passive endpoint
    ret = fi_passive_ep(fabric_, info_, &pep_, NULL);
    if (ret) {
      HELOG(kError, "Failed to initialize server endpoint");
      return ret;
    }

    // Create emission queue
    ret = fi_eq_open(fabric_, &eq_attr, &eq_, NULL);
    if (ret) {
      perror("fi_eq_open");
      return ret;
    }
    ret = fi_pep_bind(pep_, &eq_->fid, 0);
    if (ret) {
      perror("fi_pep_bind(eq)");
      return ret;
    }

    // Listen for new connections
    ret = fi_listen(pep_);
    if (ret) {
      HELOG(kError, "Failed to listen for new connections");
      return ret;
    }

    // Accept thread
    HILOG(kInfo, "Starting accept thread");
    ServerAccept();
    // accept_thread_ = std::make_unique<std::thread>(&SocketServer::ServerAccept, this);

    return ret;
  }

  void Join() {
    accept_thread_->join();
  }

  int ServerAccept() {
    int ret;
    uint32_t event;
    struct fi_eq_cm_entry entry;

    while (true) {
      // Detect connection request
      ret = fi_eq_sread(eq_, &event, &entry, sizeof(entry), -1, 0);
      if (ret != sizeof(entry)) {
        HILOG(kError, "Failed to read from event queue: {}", fi_strerror(-ret));
        return ret;
      }
      if (event != FI_CONNREQ) {
        HILOG(kError, "Unexpected event: {}", event);
        return -1;
      }
      HILOG(kInfo, "Received connection request");

      // Accept the endpoint
      ret = fi_accept(ep_, NULL, 0);
      if (ret) {
        HELOG(kError, "Failed to accept endpoint");
        return ret;
      }

      // Register the client connection
      clients_.emplace_back();
      auto &client = clients_.back();

      // Create endpoint to the server
      ret = fi_endpoint(domain_, client->info_, &ep_, NULL);
      if (ret) {
        HELOG(kError, "Failed to create endpoint");
        return ret;
      }

      // Open emission queue
      struct fi_eq_attr eq_attr = {
          .size = 0,
          .flags = 0,
          .wait_obj = FI_WAIT_UNSPEC,
          .signaling_vector = 0,
          .wait_set = NULL,
      };
      ret = fi_eq_open(fabric_, &eq_attr, &eq_, NULL);
      if (ret) {
        HELOG(kError, "Failed to open emission queue")
        return ret;
      }
      fi_ep_bind(ep_, &eq_->fid,
                 FI_SEND | FI_RECV | FI_READ | FI_WRITE | FI_REMOTE_READ | FI_REMOTE_WRITE);

      // Enable the ep for
      ret = fi_enable(ep_);
      if (ret) {
        HELOG(kError, "Failed to enable endpoint");
        return ret;
      }

      return ret;
    }
  }

  void Send() {
  }

  void Recv() {
  }
};

#endif  // LIBFABRIC_BENCH_SRC_TCP_SERVER_H_
