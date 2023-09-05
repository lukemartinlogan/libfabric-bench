//
// Created by lukemartinlogan on 8/9/23.
//

#include "fabric_bench/config_manager.h"
#include "fabric_bench/socket_client.h"
#include "fabric_bench/socket_server.h"

int main(int argc, char **argv) {
  if (argc != 2) {
    printf("USAGE: ./fabric_bench <config_file>\n");
    exit(1);
  }
  std::string real_path = argv[1];
  ConfigManager config;
  config.Load(real_path);

  SocketClient client;
  client.ClientInit(config.protocol_, config.port_, config.my_ip_);
  return 0;
}
