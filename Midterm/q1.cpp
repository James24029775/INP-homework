#include	<sys/types.h>	/* basic system data types */
#include	<sys/socket.h>	/* basic socket definitions */
#if TIME_WITH_SYS_TIME
#include	<sys/time.h>	/* timeval{} for select() */
#include	<time.h>		/* timespec{} for pselect() */
#else
#if HAVE_SYS_TIME_H
#include	<sys/time.h>	/* includes <time.h> unsafely */
#else
#include	<time.h>		/* old system? */
#endif
#endif
#include	<netinet/in.h>	/* sockaddr_in{} and other Internet defns */
#include	<arpa/inet.h>	/* inet(3) functions */
#include	<errno.h>
#include	<fcntl.h>		/* for nonblocking */
#include	<netdb.h>
#include	<signal.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<sys/stat.h>	/* for S_xxx file mode constants */
#include	<sys/uio.h>		/* for iovec{} and readv/writev */
#include	<unistd.h>
#include	<sys/wait.h>
#include	<sys/un.h>		/* for Unix domain sockets */
#include <bits/stdc++.h>
#define	SA	struct sockaddr
#define MAXLINE 1000
using namespace std;

void str_cli(FILE *fp, int sockfd);
ssize_t	 Readline(int, void *, size_t);

int
main(int argc, char **argv)
{
	int					sockfd;
	struct sockaddr_in	servaddr;

	if (argc != 3){
		cout << ("usage: tcpcli <IPaddress> <IPport>\n");
        return 0;
    }

	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(atoi(argv[2]));

    char *ptr, **pptr;
    char str[INET_ADDRSTRLEN];
    struct hostent *hptr;


    hptr = gethostbyname(argv[1]);
    pptr = hptr->h_addr_list;
    inet_ntop(hptr->h_addrtype, *pptr, str, sizeof(str));

	inet_pton(AF_INET, str, &servaddr.sin_addr);

	connect(sockfd, (SA *) &servaddr, sizeof(servaddr));

	str_cli(stdin, sockfd);		/* do it all */

	exit(0);
}


void
str_cli(FILE *fp, int sockfd)
{
	char	sendline[MAXLINE], recvline[MAXLINE];
    string tmp, dm;
    int pos;
    dm = " ";

	while (1) {
        if (Readline(sockfd, recvline, MAXLINE) == 0)
        {
            // cout << ("str_cli: server terminated prematurely\n");
            return;
        }
			

		fputs(recvline, stdout);
        if (recvline[0] == '#') 
        {
            tmp = recvline;
            pos = tmp.find(" ");
            tmp.erase(0, pos+dm.length());
            pos = tmp.find(" ");
            tmp.erase(0, pos+dm.length());
            strcpy(sendline, tmp.c_str());

            write(sockfd, sendline, strlen(sendline));
        }
	
	}
}


static int	read_cnt;
static char	*read_ptr;
static char	read_buf[MAXLINE];

static ssize_t
my_read(int fd, char *ptr)
{

	if (read_cnt <= 0) {
again:
		if ( (read_cnt = read(fd, read_buf, sizeof(read_buf))) < 0) {
			if (errno == EINTR)
				goto again;
			return(-1);
		} else if (read_cnt == 0)
			return(0);
		read_ptr = read_buf;
	}

	read_cnt--;
	*ptr = *read_ptr++;
	return(1);
}

ssize_t
readline(int fd, void *vptr, size_t maxlen)
{
	ssize_t	n, rc;
	char	c, *ptr;

	ptr = (char*)vptr;
	for (n = 1; n < maxlen; n++) {
		if ( (rc = my_read(fd, &c)) == 1) {
			*ptr++ = c;
			if (c == '\n')
				break;	/* newline is stored, like fgets() */
		} else if (rc == 0) {
			*ptr = 0;
			return(n - 1);	/* EOF, n - 1 bytes were read */
		} else
			return(-1);		/* error, errno set by read() */
	}

	*ptr = 0;	/* null terminate like fgets() */
	return(n);
}

ssize_t
readlinebuf(void **vptrptr)
{
	if (read_cnt)
		*vptrptr = read_ptr;
	return(read_cnt);
}
/* end readline */

ssize_t
Readline(int fd, void *ptr, size_t maxlen)
{
	ssize_t		n;

	if ( (n = readline(fd, ptr, maxlen)) < 0)
		cout << ("readline error");
	return(n);
}
