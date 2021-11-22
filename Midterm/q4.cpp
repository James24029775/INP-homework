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
#include	<sys/uio.h>		/* for iovec{} and readv/Writenv */
#include	<unistd.h>
#include	<sys/wait.h>
#include	<sys/un.h>		/* for Unix domain sockets */
#include <bits/stdc++.h>
#define	SA	struct sockaddr
#define MAXLINE 10000
#define INF -1
using namespace std;

#define M 101
#define VH 7
#define VW 11


typedef pair<int, int> PI;
typedef vector<vector<PI>> VPI;
typedef vector<vector<int>> VI;
vector<string> headfor;
void str_cli(FILE *fp, int sockfd);
ssize_t	 Readline(int, void *, size_t);
void BFS(vector<string> &map, VPI &parent, VI &dist);
void print_pth(VPI &parent);
void Writen(int fd, void *ptr, size_t nbytes);

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

bool checkLT(vector<string> &vmap, char sendline[])
{

    if (vmap[0][1] != ' ')
    {
        strcpy(sendline, "I\n");
        return false;
    }
    else if (vmap[1][0] != ' ')
    {
        strcpy(sendline, "J\n");
        return false;
    }
    strcpy(sendline, "K\n");
    return true;
}

void storeMap(vector<string>& map, vector<string>& vmap, int LTX, int xlen, int ylen)
{
    // cout << LTX << ' ' << xlen << ' ' << ylen << endl;
    for (int i = 0 ; i <= ylen ; i++)
    {
        // cout << LTX+i << endl;
        map[LTX+i] += vmap[i].substr(0, xlen);
    }
}

void
str_cli(FILE *fp, int sockfd)
{
	char	sendline[MAXLINE], recvline[MAXLINE];
    string dm;
    vector<string> map(M, "");
    vector<string> vmap(M, "");
    VPI parent(M, vector<PI>(M));
    VI dist(M, vector<int>(M));


    int pos;
    int x=0;
    int y=0;
    int z=0;
    int cnt=0;
    PI diff;
    bool fflg = false;
    bool bfirst = true;
    bool rflg = false;
    bool cflg = false;
    bool bflg = true;
    bool dflg = false;
    
    int wcnt=M;
    int hcnt=M;
    string tmp;
    int LTX=0, LTY=0;

    int s = 0;
    int move=11;
    int xtime=10;
    int ytime=14;
    int xlen, ylen;

    int back=110;
    int down=7;

	while (1) {
        if (recvline[0] == 'E') y = 0;

        bzero(&recvline, sizeof(recvline));
        if (Readline(sockfd, recvline, MAXLINE) == 0)
        {
            // cout << ("str_cli: server terminated prematurely\n");
            return;
        }

		fputs(recvline, stdout);
        
        if (recvline[2] == ' ' && recvline[6] == ' ' )
        {
            tmp = recvline;
            vmap[y++] = tmp.substr(7, 11);
        }

        // 找到左上角的點 -> 會超界
        if(recvline[0] == 'E' && !fflg && !rflg && !dflg)
        {
            bzero(&sendline, sizeof(sendline));
            fflg = checkLT(vmap, sendline);
            Writen(sockfd, sendline, strlen(sendline));
        }
        
        // 回歸左上角的點 -> 正在點上
        else if(recvline[0] == 'E' && fflg && !rflg && !dflg)
        {
            bzero(&sendline, sizeof(sendline));
            strcpy(sendline, "L\n");
            Writen(sockfd, sendline, strlen(sendline));
            rflg = true;
            continue;
        }
        
        // 探索右下地圖
        if (recvline[0] == 'E' && rflg && !cflg && bflg && !dflg)
        {
            // cout << "TIME: " << ytime << endl;
            xlen = (xtime == 1) ? 2 : 11;
            ylen = (ytime == 0) ? 2 : 6;
            storeMap(map, vmap, LTX, xlen, ylen);
            if (ytime == 0 && xtime == 1) 
            {
                // for (auto i : map) cout << i << endl;
                map[100] = "#####################################################################################################";
                
                dflg = true;
            }
            xtime--;
            cflg = true;
            if (xtime == 0)
            {
                xtime = 10;
                bflg = false;
                // for (auto i : map) cout << i << endl;
                // cout << map[0].length() << endl; 
                // if (s++ > 12 )break;
            }
        }
        if(recvline[0] == 'E' && rflg && cflg && bflg && !dflg)
        {
            bzero(&sendline, sizeof(sendline));
            strcpy(sendline, "L\n");
            Writen(sockfd, sendline, strlen(sendline));
            move--;
            if (move == 0) {
                cflg = false;
                move = 11;
            }
        }
        if(recvline[0] == 'E' && rflg && !bflg && !dflg)
        {
            bzero(&sendline, sizeof(sendline));
            if (back > 0)
            {
                strcpy(sendline, "J\n");
                back--;
            }
            else if (down > 0)
            {
                strcpy(sendline, "K\n");
                down--;
                LTX++;
            }
            
            if (back==0 && down==0)
            {
                // cout << ytime << endl;
                ytime--;      
                bflg = true;
                back = 110;
                down = 7;
            }
            Writen(sockfd, sendline, strlen(sendline));
        }

        if (recvline[0] == 'E' && dflg)
        {
            if (bfirst)
            {
                for(int i = 0 ; i < M ; i++ )
                {
                    if ((pos = map[i].find('E')) != -1) Epos = PI(i, pos);
                    if ((pos = map[i].find('*')) != -1) Spos = PI(i, pos);
                }
                BFS(map, parent, dist);

                print_pth(parent);
                // cout << "FIN" << endl;
                bfirst = false;
            }
            if (x >= headfor.size()) break;
            bzero(&sendline, sizeof(sendline));
            strcpy(sendline, headfor[x].c_str());
            // cout << sendline;
            x++;
            Writen(sockfd, sendline, strlen(sendline));
            
        }
	}
}


void print_pth(VPI &parent)
{
    PI cur = PI(Epos.first, Epos.second);
    
    int s = 0;
    stack<PI> stk;
    while (1)
    {
        stk.push(cur);
        if (cur.first == Spos.first && cur.second == Spos.second) break;
        // cout << "CUR: " << cur.first << ' ' <<  cur.second << endl;
        // cout << "START: " << Spos.first << ' ' << Spos.second << endl;

        cur = parent[cur.first][cur.second];
        // cout << s++ << endl;
        // stk.push(cur);
        // if (cur.first == Spos.first && cur.second == Spos.second) break;
        // cur = parent[cur.first][cur.second];
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

    for (int i = 0; i < M; i++){
        for (int j = 0; j < M; j++){
            dist[i][j] = INF;
        }
    }

    queue<PI> q;
    q.push(Spos);
    dist[Spos.first][Spos.second] = 0;
    parent[Spos.first][Spos.second] = Spos;

    while(!q.empty()){
        
        PI cur = q.front();
        q.pop();
        for (int i = 0; i < 4 ; i++) {
            int nextX = search_x[i]+cur.first;
            int nextY = search_y[i]+cur.second;
            if (nextX == Epos.first && nextY == Epos.second){
                // cout << "end" << endl;
                parent[nextX][nextY] = cur;
                dist[nextX][nextY] = dist[cur.first][cur.second] + 1;
                // for (auto i : map) cout << i << endl;
                return;
            }
            
            if (0 <= nextY && nextY < M && 0 <= nextX && nextX < M && (map[nextX][nextY] == '.') && (dist[nextX][nextY]==INF))
            {
                // cout << i << endl;
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
		cout << ("writen error");
}
