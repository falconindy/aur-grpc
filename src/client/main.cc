#include <ctype.h>
#include <getopt.h>

#include <string_view>

#include "client/client.hh"
#include "google/protobuf/util/field_mask_util.h"

[[noreturn]] void usage() {
  printf("%s [-a server_address] command args...\n",
         program_invocation_short_name);
  // clang-format off
  printf(
      "Commands\n"
      "  lookup             lookup one to many packages by name\n"
      "  search             search for packages by name/desc\n"
      "  resolve            find packages matching given depstrings\n"
      "\n"
      "Options\n"
      "  -m MASK            a list of fields, comma-delimited, to mask in response\n"
      "  -l LOOKUP_BY       lookup by the given field (name, pkgbase, maintainer,\n"
      "                         depends, makedepends, checkdepends, optdepends)\n"
      "  -s SEARCH_BY       search by given corpus (name, name_desc)\n"
      "  -o LOGIC           search using given set logic (disjunctive, conjunctive)\n"
      "\n");
  // clang-format on
  exit(0);
}

// Look, I don't know. This would be a whole lot better if we could just use
// absl methods, but we can't. We just can't. No more questions.
// https://github.com/grpc/grpc/issues/23640
std::string MakeEnumName(const std::string& prefix, const std::string& suffix) {
  std::string out = prefix;

  for (auto ltr : suffix) {
    out += static_cast<char>(toupper(ltr));
  }

  return out;
}

int main(int argc, char** argv) {
  const char* server_address = "localhost:9000";
  aur::v1::AurClient::CallOptions call_options;

  int opt;
  while ((opt = getopt(argc, argv, "a:l:m:ho:s:")) != -1) {
    switch (opt) {
      case 'a':
        server_address = optarg;
        break;
      case 'l':
        aur::v1::LookupRequest::LookupBy_Parse(
            MakeEnumName("LOOKUPBY_", optarg), &call_options.lookup_by);
        break;
      case 'h':
        usage();
      case 'm':
        google::protobuf::util::FieldMaskUtil::FromString(
            optarg, &call_options.field_mask);
        break;
      case 'o':
        aur::v1::SearchRequest::SearchLogic_Parse(
            MakeEnumName("SEARCHLOGIC_", optarg), &call_options.search_logic);
        break;
      case 's':
        aur::v1::SearchRequest::SearchBy_Parse(
            MakeEnumName("SEARCHBY_", optarg), &call_options.search_by);
        break;
      case '?':
        exit(1);
    }
  }

  argc -= optind - 1;
  argv += optind - 1;

  if (argc < 3) {
    printf("not enough args\n");
    return 42;
  }

  aur::v1::AurClient client(
      grpc::CreateChannel(server_address, grpc::InsecureChannelCredentials()));

  std::string_view action(argv[1]);
  std::vector<std::string> args(argv + 2, argv + argc);

  if (action == "lookup") {
    client.Lookup(args, call_options);
  } else if (action == "search") {
    client.Search(args, call_options);
  } else if (action == "resolve") {
    client.Resolve(args, call_options);
  } else {
    printf("error: unknown action %s\n", argv[1]);
  }

  return 0;
}
