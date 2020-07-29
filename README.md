# aur-grpc

An experimental AUR server using gRPC.

## Why would you do this?

I don't expect this to be taken too seriously. This was mostly a chance for me
to play with gRPC, though admittedly I'm not using a ton of features. The API
that this server provides is largely very similar to the one that the AUR
provides.  There are, however, a few key differences, some of which try to
satisfy some long-standing feature requests against the AUR:

* The `Info` command is renamed `Lookup` as a more suitable name. As explained
  next, it's been expanded a bit.
* Most of the `search_by` options that the AUR's search command supports are
  moved to the Lookup method. This is the sensible thing to do for the
  depends-like searches where there's no actual searching, but only exact
  matching.
* `Lookup` responses indicate which input names yielded no packages.
* The `Search` method supports simple pattern matching with shell-style globs,
  e.g. "\*-git" or "python-\*". As a consequence of this, search terms are now
  implcitly anchored. Searching for "auracle" won't get you what you want (i.e.
  auracle-git).
* All methods support field masks in order to reduce the amount of data the AUR
  gives you back. For example, if you only want name and pkgver, you can ask
  for just those fields. I suspect this should be a "requirement" or else you
  only get package names back. The advantage of doing this is twofold: a) on the
  server side, you know what clients are using which fields, and b) the client is
  immune to backwards compatible extensions of the API. For now, without a
  package mask, the `Lookup` and `Resolve` methods return all fields, and the
  `Search` method only returns package names (doing so allows for very large but
  compact result sets).
* There's a new `Resolve` method which does dependency resolution. For example,
  you can ask for packages that satisfy the dependency `systemd>246`.
* The package abstraction doesn't expose any of the numeric IDs. There's literally
  nothing you can do with these internal database keys.

Ideally, some of these improvements get traction in the current implementation
of the AUR. This server exists as a nice proof of concept of what can be done.

## Limitations

I'm not using the AUR itself as a backend. I've included a script in this repo
which ingests the AUR's package list and scrapes all the packages with auracle.
These JSONs are then converted to serialized protobuf and read into memory on
startup with indexes on various fields of interest. So, this server ends up
being quiet fast, but static. The in-memory DB is thread-safe and has the
ability to be reloaded on SIGHUP, so a separate process following the RSS/Atom
package feed could inject new data and trigger a reload.

There's no TLS support in the gRPC server. If you're thinking of exposing this
to the Internet, don't. If you're still thinking about it, put it behind nginx
or envoy to do TLS termination.

## Setup

1. Install deps: grpc, protobuf (also gtest/gmock if you want unit tests).
1. Build everything: `meson build --buildtype=debugoptimized && ninja -C build`
1. Create the local database: `tools/create_db` (requires auracle)
1. Run the server: `build/server`
1. Issues queries against the server with `build/client` (or `grpc_cli`)
