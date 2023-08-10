//
// Created by lukemartinlogan on 8/9/23.
//

#ifndef LIBFABRIC_BENCH_SRC_TCP_CLIENT_H_
#define LIBFABRIC_BENCH_SRC_TCP_CLIENT_H_

#include "hermes_shm/util/logging.h"

#include <vector>
#include <list>
#include <string>
#include <memory>

#include <rdma/fabric.h>
#include <rdma/fi_domain.h>
#include <rdma/fi_endpoint.h>
#include <rdma/fi_cm.h>

struct SocketClient {
  std::vector<char> data_;
  struct fi_info* info_;          /**< General fabric info */
  struct fi_info *hints_;       /**< Properties for creating info */
  struct fid_fabric* fabric_;   /**< Fabric ID */
  struct fid_domain* domain_;   /**< Fabric domain */
  struct fid_av *av_;           /**< Address vector */
  struct fid_ep* ep_;           /**< Active endpoint */
  struct fid_eq *eq_;           /**< Emission queue (RDMA) */
  struct fid_cq *cq_;           /**< Completion queue (RDMA) */
  std::string ip_addr_, port_str_;
  struct fi_eq_attr eq_attr = {
      .wait_obj = FI_WAIT_UNSPEC,
  };
  struct fi_cq_attr cq_attr = {
      .wait_obj = FI_WAIT_NONE,
  };


  char* copy_string(const std::string &str) {
    char* ret = new char[str.size() + 1];
    std::copy(str.begin(), str.end(), ret);
    ret[str.size()] = '\0';
    return ret;
  }

  int ClientInit(const std::string &provider, int port, const std::string &ip_addr) {
    int ret;

    // Initialize common data structures
    // Allocate fabric info
    hints_ = fi_allocinfo();
    hints_->fabric_attr->prov_name = copy_string(provider);
    hints_->caps = FI_MSG;
    hints_->ep_attr->type = FI_EP_MSG;
    hints_->domain_attr->mr_mode = FI_MR_BASIC;
    ip_addr_ = ip_addr;
    port_str_ = std::to_string(port);
    ret = fi_getinfo(FI_VERSION(1, 14),
                     ip_addr_.c_str(), port_str_.c_str(),
                     0, hints_, &info_);
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

    // Create an active endpoint
    ret = fi_endpoint(domain_, info_, &ep_, NULL);
    if (ret) {
      HELOG(kError, "Failed to initialize endpoint");
      return ret;
    }

    // Create emission queue
    ret = fi_eq_open(fabric_, &eq_attr, &eq_, NULL);
    if (ret) {
      perror("fi_eq_open");
      return ret;
    }
    ret = fi_ep_bind(ep_, &eq_->fid, 0);
    if (ret) {
      perror("fi_pep_bind(eq)");
      return ret;
    }

    // Create completion queue
//    ret = fi_cq_open(domain_, &cq_attr, &cq_, NULL);
//    if (ret) {
//      perror("fi_cq_open");
//      return ret;
//    }
//    ret = fi_ep_bind(ep_, &cq_->fid, FI_TRANSMIT | FI_RECV | FI_SEND);
//    if (ret) {
//      perror("fi_pep_bind(cq)");
//      return ret;
//    }

    // Connect to server
    ret = fi_connect(ep_, info_->dest_addr, NULL, 0);
    if (ret) {
      HELOG(kError, "Failed to enable endpoint: {}", fi_strerror(-ret));
      return ret;
    }

    /* Wait for the connection to be established */
    struct fi_eq_cm_entry entry;
    uint32_t event;
    ret = fi_eq_sread(eq_, &event, &entry, sizeof entry, -1, 0);
    if (ret != sizeof entry) {
      HILOG(kError, "Failed to wait for event: {} {}", ret, fi_strerror(-ret));
      struct fi_eq_err_entry err_entry;
      fi_eq_readerr(eq_, &err_entry, 0);
      HELOG(kError, "Detailed error code: {} {}\n", err_entry.err, fi_strerror(err_entry.err));
      HELOG(kError, "Error data: {}\n", err_entry.err_data);
      return (int) ret;
    }

    if (event != FI_CONNECTED || entry.fid != &ep_->fid) {
      HELOG(kError, "Unexpected CM event");
      return -FI_EOTHER;
    }

    return ret;
  }

  void Send() {
  }

  void Recv() {
  }
};

#endif  // LIBFABRIC_BENCH_SRC_TCP_CLIENT_H_
