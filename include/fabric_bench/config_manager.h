//
// Created by lukemartinlogan on 8/9/23.
//

#ifndef FABRIC_INCLUDE_FABRIC_BENCH_CONFIG_MANAGER_H_
#define FABRIC_INCLUDE_FABRIC_BENCH_CONFIG_MANAGER_H_

#include <yaml-cpp/yaml.h>
#include "hermes_shm/util/config_parse.h"

#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <ifaddrs.h>
#include <arpa/inet.h>

class ConfigManager {
 public:
  std::vector<std::string> host_names_;
  int port_;
  std::string domain_;
  std::string protocol_;
  std::string my_ip_;

 public:
  void Load(const std::string &path) {
    try {
      YAML::Node yaml_conf = YAML::LoadFile(path);
      ParseYAML(yaml_conf);
    } catch (std::exception &e) {
      HELOG(kFatal, e.what())
    }
  }

  void ParseYAML(YAML::Node yaml_conf) {
    if (yaml_conf["host_names"]) {
      // NOTE(llogan): host file is prioritized
      std::vector<std::string> raw_host_names;
      for (YAML::Node host_name_gen : yaml_conf["host_names"]) {
        std::string host_names = host_name_gen.as<std::string>();
        hshm::ConfigParse::ParseHostNameString(host_names, raw_host_names);
      }
      for (auto &host_name : raw_host_names) {
        host_names_.push_back(_GetIpAddress(host_name));
      }
    }
    if (yaml_conf["domain"]) {
      domain_ = yaml_conf["domain"].as<std::string>();
    }
    if (yaml_conf["protocol"]) {
      protocol_ = yaml_conf["protocol"].as<std::string>();
    }
    if (yaml_conf["port"]) {
      port_ = yaml_conf["port"].as<int>();
    }

    _FindThisHost();
  }

  /** Get the node ID of this machine according to hostfile */
  int _FindThisHost() {
    int node_id = 1;
    for (std::string &ip_addr : host_names_) {
      if (_IsAddressLocal(ip_addr)) {
        my_ip_ = ip_addr;
        return node_id;
      }
      ++node_id;
    }
    HELOG(kFatal, "Could not identify this host");
    return -1;
  }

  /** Check if an IP address is local to this machine */
  bool _IsAddressLocal(const std::string &addr) {
    struct ifaddrs* ifAddrList = nullptr;
    bool found = false;

    if (getifaddrs(&ifAddrList) == -1) {
      perror("getifaddrs");
      return false;
    }

    for (struct ifaddrs* ifAddr = ifAddrList;
         ifAddr != nullptr; ifAddr = ifAddr->ifa_next) {
      if (ifAddr->ifa_addr == nullptr ||
          ifAddr->ifa_addr->sa_family != AF_INET) {
        continue;
      }

      struct sockaddr_in* sin =
          reinterpret_cast<struct sockaddr_in*>(ifAddr->ifa_addr);
      char ipAddress[INET_ADDRSTRLEN] = {0};
      inet_ntop(AF_INET, &(sin->sin_addr), ipAddress, INET_ADDRSTRLEN);

      if (addr == ipAddress) {
        found = true;
        break;
      }
    }

    freeifaddrs(ifAddrList);
    return found;
  }

  /** Get IPv4 address from the host with "host_name" */
  std::string _GetIpAddress(const std::string &host_name) {
    struct hostent hostname_info = {};
    struct hostent *hostname_result;
    int hostname_error = 0;
    char hostname_buffer[4096] = {};
#ifdef __APPLE__
    hostname_result = gethostbyname(host_name.c_str());
  in_addr **addr_list = (struct in_addr **)hostname_result->h_addr_list;
#else
    int gethostbyname_result =
        gethostbyname_r(host_name.c_str(), &hostname_info, hostname_buffer,
                        4096, &hostname_result, &hostname_error);
    if (gethostbyname_result != 0) {
      HELOG(kFatal, hstrerror(h_errno))
    }
    in_addr **addr_list = (struct in_addr **)hostname_info.h_addr_list;
#endif
    if (!addr_list[0]) {
      HELOG(kFatal, hstrerror(h_errno))
    }

    char ip_address[INET_ADDRSTRLEN] = {0};
    const char *inet_result =
        inet_ntop(AF_INET, addr_list[0], ip_address, INET_ADDRSTRLEN);
    if (!inet_result) {
      perror("inet_ntop");
      HELOG(kFatal, "inet_ntop failed");
    }
    return ip_address;
  }

};


#endif //FABRIC_INCLUDE_FABRIC_BENCH_CONFIG_MANAGER_H_
