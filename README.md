Aster server
------------
HTTP/1.1 server in C

> [!NOTE]
> The project is still in the development stage without a planned release date.

### Overview
This is a zero-dependency C implementation of HTTP/1.1 server, specified by
[RFC 9112](https://www.rfc-editor.org/info/rfc9112),
[RFC 9110](https://www.rfc-editor.org/info/rfc9110),
[RFC 2616](https://www.rfc-editor.org/info/rfc2616) and
[RFC 2068](https://www.rfc-editor.org/info/rfc2068); featuring manually written
parser (via a finite state machine).

### Setup
To build the server, run
```sh
make
```
or, for a release build,
```sh
make bin/release
```
You can launch the server as follows:
```sh
sudo ./bin/server
```

### Security
The parser is designed to reject with `400 Bad Request` all messages deviating
from specifications (like `SP` before header colon `:`), containing obsolete
spec, and those containing elements that are now used as attack vectors (desync
attacks such as `CL.TE`).

### Goals
This is largely an educational project, which, in practice, may be run:
- inside a private network or a container cluster;
- as a purpose-built static origin;
- in embedded environments;
- behind a reverse proxy or a CDN (with caution).

### Roadmap
- [ ] HTTP/1.1 implementation
- [ ] HTTP/1.0 support
- [ ] HTTP/0.9 support
- [ ] Proxy support
