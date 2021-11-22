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
#define H 21
#define W 79
#define INF -1
using namespace std;


typedef pair<int, int> PI;
typedef vector<vector<PI>> VPI;
typedef vector<vector<int>> VI;
vector<string> headfor;
void str_cli(FILE *fp, int sockfd);
ssize_t	 Readline(int, void *, size_t);
void BFS(vector<string> &map, VPI &parent, VI &dist);
void print_pth(VPI &parent);

PI Epos, Spos;

int search_x[] = {0, 0, 1, -1};
int search_y[] = {1, -1, 0, 0};



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
    string dm;
    vector<string> map(100);
    VPI parent(100, vector<PI>(100));
    VI dist(100, vector<int>(100));


    int pos;
    int x=0;
    PI diff;

    int z=0;
    bool first = true;
    

	while (1) {
        if (Readline(sockfd, recvline, MAXLINE) == 0)
        {
            // cout << ("str_cli: server terminated prematurely\n");
            return;
        }
			

		fputs(recvline, stdout);
        if (recvline[0] == '#') 
        {
            map[z++] = recvline;
        }

        else if (recvline[0] == 'E')
        {
            if (first)
            {
                for(int i = 0 ; i < map.size() ; i++ )
                {
                    if ((pos = map[i].find('E')) != -1) Epos = PI(i, pos);
                    if ((pos = map[i].find('*')) != -1) Spos = PI(i, pos);
                }
                BFS(map, parent, dist);

                print_pth(parent);
                first = false;
            }
            if (x >= headfor.size()) break;
            strcpy(sendline, headfor[x].c_str());
            cout << sendline;
            x++;
            write(sockfd, sendline, strlen(sendline));
            
        }
	}
}


void print_pth(VPI &parent)
{
    PI cur = parent[Epos.first][Epos.second];
    
    stack<PI> stk;
    stk.push(PI(Epos.first, Epos.second));
    while (1)
    {
        stk.push(cur);
        if (cur.first == Spos.first && cur.second == Spos.second) break;
        cur = parent[cur.first][cur.second];
    }


    PI next;
    cur = stk.top();
    while (1)
    {
        stk.pop();
        if (stk.empty()) {break;}
        next = stk.top();

        if(next.first-cur.first== 0&&next.second-cur.second== 1) headfor.push_back("D\n");
        else if(next.first-cur.first== 0&&next.second-cur.second==-1) headfor.push_back("A\n");
        else if(next.first-cur.first== 1&&next.second-cur.second== 0) headfor.push_back("S\n");
        else if(next.first-cur.first==-1&&next.second-cur.second== 0) headfor.push_back("W\n");
        cur = next;
    }
    
}


void BFS(vector<string> &map, VPI &parent, VI &dist)
{

    for (int i = 0; i < H; i++){
        for (int j = 0; j < W; j++){
            dist[i][j] = INF;
        }
    }

    queue<PI> q;
    q.push(Spos);
    dist[Spos.first][Spos.second] = 0;
    parent[Spos.first][Spos.second] = PI(Spos.first, Spos.second);

    while(!q.empty()){
        
        PI cur = q.front();
        q.pop();
        for (int i = 0; i < 4 ; i++) {
            int nextX = search_x[i]+cur.first;
            int nextY = search_y[i]+cur.second;
            if (nextX == Epos.first && nextY == Epos.second){
                parent[nextX][nextY] = cur;
                dist[nextX][nextY] = dist[cur.first][cur.second] + 1;
                return;
            }
            
            if (0 <= nextY && nextY < W && 0 <= nextX && nextX < H && (map[nextX][nextY] == '.') && (dist[nextX][nextY]==INF))
            {
                q.push(PI(nextX, nextY));
                parent[nextX][nextY] = cur;
                dist[nextX][nextY] = dist[cur.first][cur.second] + 1;
            }
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
