#define _POSIX_C_SOURCE 200112L
#define _XOPEN_SOURCE 600

#include <arpa/inet.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <netdb.h>
#include <assert.h>
#include "parser.h"
#include "response.h"
#include "datetime.h"

#define MAXDATASIZE 1024
#define SERVER "aster/0.0.0-alpha"
#define ENTITY "<!DOCTYPE html><html>" \
		"<head><title>main</title></head>" \
		"<body>hello</body>" \
		"</html>"
#define NOT_FOUND "<!DOCTYPE html><html>" \
		"<head><title>not found</title></head>" \
		"<body>not found</body>" \
		"</html>"

/*
 * ai_ for AddrInfo
 * gai_ for GetAddrInfo
 * sin_ for Sockaddr_IN
 * sa_ for SockAddr
 * AF_ for Address Family
 * PF_ for Protocol Family
 */

/* prune dead forked processes */
static void zombie_killer(int s) {
	int saved_errno;

	(void)s; /* unused parameter warning */

	saved_errno = errno; /* errno may be overwritten during waitpid */

	while (waitpid(-1, NULL, WNOHANG) > 0);

	errno = saved_errno;
}

/* retrieve socket address (v4 or v6) from a generic sockaddr struct
   cast to either sockaddr_in* or sockaddr_in6* */
static void *get_sockaddr_in(struct sockaddr *sa) {
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in *)sa)->sin_addr);
	}
	return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

/* bind the socket to the first available bindable address, returns the fd of
   the socket or -1 on error */
static int bind_local_address(void) {
	struct addrinfo addr_hints, *server_info, *addr;
	int listener_fd = -1;
	int yes = 1;
	int retval;
	char addrstr[INET6_ADDRSTRLEN];

	memset(&addr_hints, 0, sizeof addr_hints);
	addr_hints.ai_family = AF_INET; /* IPv4 */
	addr_hints.ai_socktype = SOCK_STREAM; /* TCP */
	addr_hints.ai_flags = AI_PASSIVE; /* `bind()`able */

	retval = getaddrinfo(NULL, "http", &addr_hints, &server_info);
	if (retval != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(retval));
		return -1;
	}

	for (addr = server_info; addr != NULL; addr = addr->ai_next) {
		listener_fd = socket(addr->ai_family,
				addr->ai_socktype,
				addr->ai_protocol);
		if (listener_fd == -1) {
			perror("socket");
			continue;
		}
		retval = setsockopt(listener_fd,
				SOL_SOCKET,
				SO_REUSEADDR,
				&yes, sizeof yes);
		if (retval == -1) {
			perror("setsockopt");
			return -1;
		}
		retval = bind(listener_fd, addr->ai_addr, addr->ai_addrlen);
		if (retval == -1) {
			close(listener_fd);
			listener_fd = -1;
			perror("bind");
			continue;
		}
		break;
	}

	if (listener_fd == -1) {
		return -1;
	}

	if (listen(listener_fd, SOMAXCONN) == -1) {
		perror("listen");
		close(listener_fd);
		return -1;
	}

	inet_ntop(addr->ai_addr->sa_family,
			get_sockaddr_in(addr->ai_addr),
			addrstr, sizeof addrstr);

	printf("server: listening on %s...\n", addrstr);

	freeaddrinfo(server_info);

	return listener_fd;
}

static void handle_client(int client_fd) {
	char buf[MAXDATASIZE];

	struct http_request req = new_request();
	struct parse_ctx ctx = parse_ctx_init(&req);
	enum parse_result res;

	struct http_response reply = new_response();
	char datetime[30] = {0};

	ssize_t num_bytes = recv(client_fd, buf, MAXDATASIZE - 1, 0);
	if (num_bytes == -1) {
		perror("recv");
		parse_ctx_free(&ctx);
		http_request_free(&req);
		exit(1);
	}

	printf("request:\n%.*s\n", (int)num_bytes, buf);

	while ((res = feed(&ctx, buf, (size_t)num_bytes)) != PR_COMPLETE) {
		num_bytes = recv(client_fd, buf, MAXDATASIZE - 1, 0);
		if (num_bytes == -1) {
			perror("recv");
			parse_ctx_free(&ctx);
			http_request_free(&req);
			exit(1);
		}
		if (num_bytes == 0) {
			break;
		}
	}
	get_current_time(datetime);
	if (res == PR_NEED_MORE) {
		append_to_response(&reply,
			"HTTP/1.1 400 Bad Request" CRLF
			"Server: " SERVER CRLF
			"Content-Length: 0" CRLF
			"Connection: close" CRLF
			"Date: ");
		append_to_response(&reply, datetime);
		append_to_response(&reply,
			CRLF CRLF);
	} else if (ctx.state > PS_DONE) {
		if (ctx.state == PS_ERROR) {
			append_to_response(&reply,
				"HTTP/1.1 400 Bad Request" CRLF
				"Server: " SERVER CRLF
				"Content-Length: 0" CRLF
				"Connection: close" CRLF
				"Date: ");
			append_to_response(&reply, datetime);
			append_to_response(&reply,
				CRLF CRLF);
		} else if (req.method == HM_UNK) {
			append_to_response(&reply,
				"HTTP/1.1 501 Not Implemented" CRLF
				"Server: " SERVER CRLF
				"Content-Length: 0" CRLF
				"Connection: close" CRLF
				"Date: ");
			append_to_response(&reply, datetime);
			append_to_response(&reply,
				CRLF CRLF);
		} else {
			assert(0);
		}
	} else {
		if (req.path.len == 1 && !slice_str_cmp(&req.path, "/")) {
			append_to_response(&reply,
				"HTTP/1.1 200 OK" CRLF
				"Server: " SERVER CRLF
				"Content-Length: 78" CRLF
				"Connection: close" CRLF
				"Date: ");
			append_to_response(&reply, datetime);
			append_to_response(&reply,
				CRLF CRLF
				ENTITY);
		} else {
			append_to_response(&reply,
				"HTTP/1.1 404 Not Found" CRLF
				"Server: " SERVER CRLF
				"Content-Length: 88" CRLF
				"Connection: close" CRLF
				"Date: ");
			append_to_response(&reply, datetime);
			append_to_response(&reply,
				CRLF CRLF
				NOT_FOUND);
		}
	}

	send(client_fd, reply.buf, reply.len, 0);
	parse_ctx_free(&ctx);
	http_request_free(&req);
	http_response_free(&reply);
}

int main(void) {
	int listener_fd, client_fd;
	struct sockaddr_storage client_addr;
	struct sigaction sigact;
	socklen_t sin_size;
	char addrstr[INET6_ADDRSTRLEN];
	pid_t fork_pid;

	listener_fd = bind_local_address();

	if (listener_fd == -1) {
		fprintf(stderr, "server: failed to bind\n");
		close(listener_fd);
		return 1;
	}

	sigact.sa_handler = zombie_killer;
	sigemptyset(&sigact.sa_mask);
	sigact.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sigact, NULL) == -1) {
		perror("sigaction");
		close(listener_fd);
		return 1;
	}

	while (1) {
		sin_size = sizeof client_addr;
		client_fd = accept(listener_fd,
				(struct sockaddr*)&client_addr,
				&sin_size);

		if (client_fd == -1) {
			perror("accept");
			continue;
		}

		inet_ntop(client_addr.ss_family,
				get_sockaddr_in((struct sockaddr*)&client_addr),
				addrstr, sizeof addrstr);
		printf("server: received connection from %s\n", addrstr);

		fork_pid = fork();

		if (fork_pid == -1) {
			perror("fork");
			close(listener_fd);
			return 1;
		}

		if (!fork_pid) { /* child */
			/* child inherits a copy of parent's file descriptors */
			/* sockfd is passive and isn't used by the child */
			close(listener_fd);

			handle_client(client_fd);
			close(client_fd);

			return 0;
		} /* parent */
		close(client_fd);
	}
	return 0;
}
