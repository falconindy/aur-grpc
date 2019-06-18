#include <getopt.h>

#include "server/server.hh"

[[noreturn]] void usage() {
  printf("%s [-l listen_address]\n", program_invocation_short_name);
  exit(0);
}

int main(int argc, char** argv) {
  const char* listen_address = "127.0.0.1:9000";

  int opt;
  while ((opt = getopt(argc, argv, "hl:")) != -1) {
    switch (opt) {
      case 'h':
        usage();
      case 'l':
        listen_address = optarg;
        break;
      case '?':
        exit(1);
    }
  }

  aur::Server(listen_address).Run();
  return 0;
}
