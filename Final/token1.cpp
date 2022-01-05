
/* If anything changes in the following list of #includes, must change
   acsite.m4 also, for configure's tests. */

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

#ifdef	HAVE_SYS_SELECT_H
# include	<sys/select.h>	/* for convenience */
#endif

#ifdef	HAVE_SYS_SYSCTL_H
#ifdef	HAVE_SYS_PARAM_H
# include	<sys/param.h>	/* OpenBSD prereq for sysctl.h */
#endif
# include	<sys/sysctl.h>
#endif

#ifdef	HAVE_POLL_H
# include	<poll.h>		/* for convenience */
#endif

#ifdef	HAVE_SYS_EVENT_H
# include	<sys/event.h>	/* for kqueue */
#endif

#ifdef	HAVE_STRINGS_H
# include	<strings.h>		/* for convenience */
#endif

/* Three headers are normally needed for socket/file ioctl's:
 * <sys/ioctl.h>, <sys/filio.h>, and <sys/sockio.h>.
 */
#ifdef	HAVE_SYS_IOCTL_H
# include	<sys/ioctl.h>
#endif
#ifdef	HAVE_SYS_FILIO_H
# include	<sys/filio.h>
#endif
#ifdef	HAVE_SYS_SOCKIO_H
# include	<sys/sockio.h>
#endif

#ifdef	HAVE_PTHREAD_H
# include	<pthread.h>
#endif

#ifdef HAVE_NET_IF_DL_H
# include	<net/if_dl.h>
#endif

// #include	<netinet/sctp.h>
#include<bits/stdc++.h>
#define MAXLINE 1000

#define	SA	struct sockaddr

using namespace std;
ssize_t
Readline(int fd, void *ptr, size_t maxlen);
void
Writen(int fd, void *ptr, size_t nbytes);
void str_cli(FILE *fp, int socktcp, int sockudp, const SA *pservaddr, socklen_t servlen);

char str[INET_ADDRSTRLEN];

int
main(int argc, char **argv)
{
	int					socktcp, sockudp;
	struct sockaddr_in	servaddr, udpserv;

	if (argc != 3){
		cout << ("usage: tcpcli <IPaddress> <IP port>") << endl;
        return 0;
    }

    socktcp = socket(AF_INET, SOCK_STREAM, 0);
    sockudp = socket(AF_INET, SOCK_DGRAM, 0);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port        = htons(atoi(argv[2]));



    char *ptr, **pptr;
    struct hostent *hptr;
    hptr = gethostbyname(argv[1]);
    pptr = hptr->h_addr_list;
    inet_ntop(hptr->h_addrtype, *pptr, str, sizeof(str));
    inet_pton(AF_INET, str, (SA*) &servaddr.sin_addr);


    bzero(&udpserv, sizeof(udpserv));
	udpserv.sin_family      = AF_INET;
	udpserv.sin_addr.s_addr = htonl(INADDR_ANY);
	udpserv.sin_port        = htons(55555);
    bind(sockudp, (SA *) &udpserv, sizeof(udpserv));


	connect(socktcp, (SA *) &servaddr, sizeof(servaddr));
	str_cli(stdin, socktcp, sockudp, (SA *) &servaddr, sizeof(servaddr));		/* do it all */

	exit(0);
}

void
str_cli(FILE *fp, int socktcp, int sockudp, const SA *pservaddr, socklen_t servlen)
{
	char	sendline[MAXLINE], recvline[MAXLINE];

    string IP;
    string ans;
    int pos;
    string ttt;

    while(Readline(socktcp, recvline, MAXLINE) != 0)
    {
        fputs(recvline, stdout);
        bzero(sendline, MAXLINE);
        if (recvline[0] == 'E'){
            ttt = "55555";
            ttt += '\n';
            memcpy(sendline, ttt.c_str(), ttt.length());
            Writen(socktcp, sendline, strlen(sendline));
        }

        if (recvline[0] == 'W'){

            IP = recvline;
            pos = IP.find("1");
            IP.erase(0, pos);
            pos = IP.find(":");
            IP.erase(pos, IP.length());
        }

        if (recvline[0] == 'C'){
            struct sockaddr_in	tmp;
            socklen_t tlen = sizeof(tmp);

            bzero(recvline, MAXLINE);
            recvfrom(sockudp, recvline, MAXLINE, 0, NULL, NULL);
            cout << recvline << endl;
            ttt = recvline;
            ttt += '\n';
            memcpy(sendline, ttt.c_str(), ttt.length());
            Writen(socktcp, sendline, strlen(sendline));
        }
        bzero(recvline, MAXLINE);
    }
}

// tmux new-session \; send-keys 'nc -lu 55555' C-m \; split-window -v \; send-keys './l1 token1 10100' C-m

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

	if ( (n = readline(fd, ptr, maxlen)) < 0){
		cout << ("readline error") << endl;
        return -1;
    }
	return(n);
}

ssize_t						/* Write "n" bytes to a descriptor. */
writen(int fd, const void *vptr, size_t n)
{
	size_t		nleft;
	ssize_t		nwritten;
	const char	*ptr;

	ptr = (char*)vptr;
	nleft = n;
	while (nleft > 0) {
		if ( (nwritten = write(fd, ptr, nleft)) <= 0) {
			if (nwritten < 0 && errno == EINTR)
				nwritten = 0;		/* and call write() again */
			else
				return(-1);			/* error */
		}

		nleft -= nwritten;
		ptr   += nwritten;
	}
	return(n);
}
/* end writen */

void
Writen(int fd, void *ptr, size_t nbytes)
{
	if (writen(fd, ptr, nbytes) != nbytes)
    {
		cout << ("writen error") << endl;
        exit(0);
    }
}
