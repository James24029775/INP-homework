
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
#define MAXLINE 3000

#define	SA	struct sockaddr

using namespace std;
ssize_t
Readline(int fd, void *ptr, size_t maxlen);
void
Writen(int fd, void *ptr, size_t nbytes);
void str_cli(FILE *fp, int sockudp, const SA *pudpserv, socklen_t servlen);

char str[INET_ADDRSTRLEN];

typedef pair<int, int> PI;
typedef vector<vector<int>> VVI;
typedef vector<vector<PI>> VPI;

int h = 9, w = 10;

PI SS, EE;

int dx[8] = {-1, -1, -1, 0, 0, 1, 1, 1};
int dy[8] = {-1, 0, 1, -1, 1, -1, 0, 1};

int
main(int argc, char **argv)
{
	int					sockudp;
	struct sockaddr_in	udpserv, TAserv;

	if (argc != 3){
		cout << ("usage: tcpcli <IPaddress> <IP port>") << endl;
        return 0;
    }

    // TA's sock addr
    char *ptr, **pptr;
    struct hostent *hptr;
    hptr = gethostbyname(argv[1]);
    pptr = hptr->h_addr_list;
    inet_ntop(hptr->h_addrtype, *pptr, str, sizeof(str));

    bzero(&TAserv, sizeof(TAserv));
	TAserv.sin_family = AF_INET;
	TAserv.sin_port        = htons(atoi(argv[2]));
    inet_pton(AF_INET, str, (SA*) &TAserv.sin_addr);

    sockudp = socket(AF_INET, SOCK_DGRAM, 0);

	str_cli(stdin, sockudp, (SA *) &TAserv, sizeof(TAserv));		/* do it all */
	exit(0);
}

struct a 
{
    uint16_t value;
    char payload[0];
} __attribute__((packed));

void pack(string tmp, PI cord, unsigned char *sendline)
{
    struct a *pa1 = (struct a*) sendline;
    struct a *pa2 = (struct a*) (sendline + sizeof(struct a));
    pa1->value = htons((uint16_t)cord.first);
    pa2->value = htons((uint16_t)cord.second);
    memcpy(pa2->payload, tmp.c_str(), 1);
}

void
str_cli(FILE *fp, int sockudp, const SA *TAserv, socklen_t servlen)
{
	char	sendline[MAXLINE], recvline[MAXLINE];
    unsigned char buf[5];
    string tmp;
    bzero(sendline, MAXLINE);
    bzero(recvline, MAXLINE);
    bzero(buf, 5);
    string ans;
    int pos, nextx, nexty;
    bool first = true, firstplay = true;

    vector<vector<char>> lastmap(h, vector<char>(w));
    vector<vector<char>> map(h, vector<char>(w));
    vector<vector<int>> cnt(h, vector<int>(w));
    vector<vector<int>> mine(h, vector<int>(w));
    PI cord = PI(0, 0);
    bool same;

    for (int i = 0 ; i < h ; i++)
    {
        for (int j = 0 ; j < w-1 ; j++)
        {            
            cnt[i][j] = -1;
            mine[i][j] = -1;
            lastmap[i][j] = '0';
        }
    }

    while(1)
    {
        if (firstplay)
        {
            tmp = "O";
            pack(tmp, cord, buf);
            sendto(sockudp, buf, 5, 0, TAserv, servlen);
            sendto(sockudp, buf, 5, 0, TAserv, servlen);
            recvfrom(sockudp, recvline, MAXLINE, 0, NULL, NULL);
            bzero(buf, 5);
            tmp = "G";
            pack(tmp, cord, buf);
            sendto(sockudp, buf, 5, 0, TAserv, servlen);
            recvfrom(sockudp, recvline, MAXLINE, 0, NULL, NULL);

            cout << recvline << endl;
            
            for (int i = 0 ; i < h ; i++)
            {
                for (int j = 0 ; j < w-1 ; j++)
                {
                    if(recvline[i*w+j] == '\n') continue;
                    map[i][j] = recvline[i*w+j];
                    if((int)map[i][j] == 46) cnt[i][j] = 0;
                    if((int)map[i][j] == 77) cnt[i][j] = -2;
                    if(48<= (int)map[i][j] && (int)map[i][j] <= 57) cnt[i][j] = (int)map[i][j]-48;
                }
            }
            firstplay = false;
        }
        else{
            same = true;

            for (int i = 0 ; i < 9 ; i++)
            {
                for (int j = 0 ; j < 9 ; j++)
                {
                    if(lastmap[i][j] != map[i][j]){
                        same = false;
                        break;
                    }
                }
            }

            if (!same){
                for (int i = 0 ; i < 9 ; i++)
                {
                    for (int j = 0 ; j < 9 ; j++)
                    {
                        lastmap[i][j] = map[i][j];
                    }
                }

                int hash;
                for (int i = 0 ; i < 9 ; i++)
                {
                    for (int j = 0 ; j < 9 ; j++)
                    {
                        if(cnt[i][j] <= 0) continue;
                        hash = 0;
                        for (int k = 0 ; k < 8 ; k++)
                        {
                            nextx = i + dx[k];
                            nexty = j + dy[k];
                            if (nextx < 0 || nextx >= 9 || nexty < 0 || nexty >= 9) continue;
                            if (cnt[nextx][nexty] == -1 || cnt[nextx][nexty] == -2) hash++;
                        }
                        if(hash == cnt[i][j]){
                            for (int k = 0 ; k < 8 ; k++)
                            {
                                nextx = i + dx[k];
                                nexty = j + dy[k];
                                if (nextx < 0 || nextx >= 9 || nexty < 0 || nexty >= 9) continue;
                                if (cnt[nextx][nexty] == -1){
                                    if(mine[nextx][nexty] == -1){
                                        mine[nextx][nexty] = 1;
                                        tmp = "M";
                                        pack(tmp, PI(nexty, nextx), buf);
                                        sendto(sockudp, buf, 5, 0, TAserv, servlen);
                                        bzero(buf, 5);
                                    }
                                }
                            }
                        }
                    }
                }

                tmp = "G";
                pack(tmp, cord, buf);
                sendto(sockudp, buf, 5, 0, TAserv, servlen);
                recvfrom(sockudp, recvline, MAXLINE, 0, NULL, NULL);
                cout << recvline << endl;
                
                for (int i = 0 ; i < h ; i++)
                {
                    for (int j = 0 ; j < w-1 ; j++)
                    {
                        if(recvline[i*w+j] == '\n') continue;
                        map[i][j] = recvline[i*w+j];
                        if((int)map[i][j] == 46) cnt[i][j] = 0;
                        if((int)map[i][j] == 77) cnt[i][j] = -2;
                        if(48<= (int)map[i][j] && (int)map[i][j] <= 57) cnt[i][j] = (int)map[i][j]-48;
                    }
                }

                int bomb;
                for (int i = 0 ; i < 9 ; i++)
                {
                    for (int j = 0 ; j < 9 ; j++)
                    {
                        if(cnt[i][j] <= 0) continue;
                        bomb = 0;
                        for (int k = 0 ; k < 8 ; k++)
                        {
                            nextx = i + dx[k];
                            nexty = j + dy[k];
                            if (nextx < 0 || nextx >= 9 || nexty < 0 || nexty >= 9) continue;
                            if(cnt[nextx][nexty] == -2) bomb++;
                        }
                        
                        if(bomb == cnt[i][j]){
                            for (int k = 0 ; k < 8 ; k++)
                            {
                                nextx = i + dx[k];
                                nexty = j + dy[k];
                                if (nextx < 0 || nextx >= 9 || nexty < 0 || nexty >= 9) continue;
                                if (cnt[nextx][nexty] == -1){
                                    tmp = "O";
                                    pack(tmp, PI(nexty, nextx), buf);
                                    sendto(sockudp, buf, 5, 0, TAserv, servlen);
                                    bzero(buf, 5);
                                }
                            }
                        }
                    }
                }

                tmp = "G";
                pack(tmp, cord, buf);
                sendto(sockudp, buf, 5, 0, TAserv, servlen);
                recvfrom(sockudp, recvline, MAXLINE, 0, NULL, NULL);
                cout << recvline << endl;
                for (int i = 0 ; i < h ; i++)
                {
                    for (int j = 0 ; j < w-1 ; j++)
                    {
                        if(recvline[i*w+j] == '\n') continue;
                        map[i][j] = recvline[i*w+j];
                        if((int)map[i][j] == 46) cnt[i][j] = 0;
                        if((int)map[i][j] == 77) cnt[i][j] = -2;
                        if(48<= (int)map[i][j] && (int)map[i][j] <= 57) cnt[i][j] = (int)map[i][j]-48;
                    }
                }
            }
            else{
                bzero(buf, 5);
                tmp = "O";
                cout << cord.first << ' ' << cord.second << endl;
                cord.first++;
                if(cord.first == 9) {
                    cord.first = 0;
                    cord.second++;
                }
                pack(tmp, cord, buf);
                sendto(sockudp, buf, 5, 0, TAserv, servlen);
                
                tmp = "G";
                pack(tmp, cord, buf);
                sendto(sockudp, buf, 5, 0, TAserv, servlen);
                recvfrom(sockudp, recvline, MAXLINE, 0, NULL, NULL);

                cout << recvline << endl;
                
                for (int i = 0 ; i < h ; i++)
                {
                    for (int j = 0 ; j < w-1 ; j++)
                    {
                        if(recvline[i*w+j] == '\n') continue;
                        map[i][j] = recvline[i*w+j];
                        if((int)map[i][j] == 46) cnt[i][j] = 0;
                        if((int)map[i][j] == 77) cnt[i][j] = -2;
                        if(48<= (int)map[i][j] && (int)map[i][j] <= 57) cnt[i][j] = (int)map[i][j]-48;
                    }
                }
            }
        }
        
        bzero(buf, 5);
        bzero(sendline, MAXLINE);
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

	if ( (n = readline(fd, ptr, maxlen)) < 0)
		cout << ("readline error") << endl;
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
