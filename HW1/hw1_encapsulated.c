#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <strings.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <errno.h>

# define SERV_PORT 8000
# define MAXLINE 1024
# define WELCOME "********************************\n** Welcome to the BBS server. **\n********************************\n"
# define SHOWME "% "
# define ALPHABET 58
# define LISTENQ 1      /* Maximum number of client connections*/
# define K 80
# define KK 8000
typedef struct sockaddr SA;

/*From textbook*/
ssize_t writen(int fd, const void *vptr, size_t n);
void    Writen(int fd, void *ptr, size_t nbytes);
void    err_sys(const char* x);

/*Myself*/
void    Initialization();
void    sendMessage(char* buf, char* msg, int sockfd);
void    DecideService(char *readBuf, int n, char **username, char **pwd, char **msg, int *type, int *cnt);
void    DoService(int type, int cnt, char *username, char *pwd, char *msg, int *userIndex, char *tmp);
void    popMsg(int userindex, int msgindex, char *tmp);
void    storeUsername(char *username, char *pwd);
void    showUserList(char *tmp);
void    showMsgBox(char *tmp, int index);
bool    isMsgLeft(int index);
bool    checkForm(int type, int cnt);
int     checkUserIfExist(char *username);
int     MessageBox(int sockfd);

char    accountName[K][K];
char    accountPwd[K][K];
char    msgBox[K][K][K][KK];
bool    loginOrNot[K];
int     msgWrite[K][K];
int     msgRead[K][K];
int     accountNum = 0;
int     userIndexForReg = -1, userIndexForMsg = -1, userIndexForRec = -1;


typedef struct msgIndex{
    int index;
    char name[K];
} msgIndex;

int main(int argc, char *argv[]){

    if (argc != 2){
        err_sys("server usage: ./hw1 [port number]");
    }

    /* Use IPV4 to connect network.*/
    struct sockaddr_in  cliaddr, servaddr;
    int                 listenfd, connfd;
    socklen_t           clilen;
    pid_t               childpid;

    /* Initiate a socket service.*/
    listenfd = socket(AF_INET, SOCK_STREAM, 0);

    /* Fill server's basic info.*/
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(atoi(argv[1]));

    /* Bind basic info with the socket service.*/
    bind(listenfd, (SA *) &servaddr, sizeof(servaddr));

    /* Set the server to LISTEN state.*/
    listen(listenfd, LISTENQ);

    /* Initialize global variables account*/
    Initialization();

    for( ; ; ){
        /* clilen is a value-return variable, and should store IPV4's readBuffer size in advance.*/
        /* Use Accept to build connection with client.*/
        clilen = sizeof(cliaddr);
        connfd = accept(listenfd, (SA *) &cliaddr, &clilen);

        /*If children process, transform state from LISTEN to CONNECTION.*/
        if ((childpid = fork()) == 0){
            close(listenfd);
            MessageBox(connfd);
            exit(0);
        }
        close(connfd);
    }

    return 0;
}


int MessageBox(int sockfd){
    ssize_t n;
    char readBuf[MAXLINE], writeBuf[MAXLINE], tmp[K];
    char *username, *pwd, *msg;
    int type=-1, cnt=0, userIndex=-1;
    bool isTrueForm=true;

    sendMessage(writeBuf, WELCOME, sockfd);                                 /*Deal with WELCOME*/


again:
    while (true)
    {
        /*Reset*/
        cnt = 0;
        type = -1;
        memset(tmp, '\0', K);
        pwd = "";
        username = "";
        msg = "";
        userIndexForReg = -1;
        userIndexForMsg = -1;
        userIndexForRec = -1;
        
        sendMessage(writeBuf, SHOWME, sockfd);                              /*Deal with "%"*/
        
        bzero(&readBuf, MAXLINE);
        if ((n = read(sockfd, readBuf, MAXLINE)) < 0) break;                /*Check for disconnection*/

        DecideService(readBuf, n, &username, &pwd, &msg, &type, &cnt);      /*Update username, pwd, msg, type, cnt*/

        DoService(type, cnt, username, pwd, msg, &userIndex, tmp);          /*Update userIndex, tmp*/

        sendMessage(writeBuf, tmp, sockfd);                                 

        if (type == 5) break;
    }

    if (n < 0 && errno == EINTR){
        goto again;
    }
    else if (n < 0){
        err_sys("MessageBox: read error.");
    }
}

void popMsg(int userindex, int msgindex, char *tmp)
{
    strcpy(tmp, msgBox[userindex][msgindex][msgRead[userindex][msgindex]]);
    strcat(tmp, "\n");
    msgRead[userindex][msgindex]++;
    if (msgRead[userindex][msgindex] == K) msgRead[userindex][msgindex] = 0;
}

bool isMsgLeft(int index)
{
    int i = 0;
    for ( ; i < K ; i++)
        if (msgWrite[index][i] - msgRead[index][i] != 0) return true;
    return false;
}

void Initialization()
{
    int i = 0;
    int j = 0;
    int k = 0;
    for ( ; i < K ; i++)
    {
        memset(accountName[i], '\0', K);
        memset(accountPwd[i], '\0', K);
        memset(msgWrite[i], 0, K);
        memset(msgRead[i], 0, K);
        for ( ; j < K ; j++)
            for ( ; k < K ; k++)
                memset(msgBox[i][j][K], '\0', KK);
    }
    memset(loginOrNot, false, K);
    
}

int cmpfunc (const void * a, const void * b)
{
   return strcmp((*(msgIndex*)a).name, (*(msgIndex*)b).name);
}

void showMsgBox(char *ans, int index)
{
    msgIndex msgindex[accountNum];
    int i = 0, target = 0, msgLeft = 0;
    char tmp[K];

    for(i = 0 ; i < accountNum ; i++)
    {
        msgindex[i].index = i;
        strcpy(msgindex[i].name, accountName[i]);
    }
    qsort(msgindex, accountNum, sizeof(msgIndex), cmpfunc);

    for (i = 0 ; i < accountNum ; i++)
    {
        target = msgindex[i].index;
        msgLeft = abs(msgWrite[index][target] - msgRead[index][target]);
        if (msgLeft)
        {
            sprintf(tmp, "%d", msgLeft);
            strcat(ans, tmp);
            strcat(ans, " message from ");
            strcat(ans, accountName[target]);
            strcat(ans, ".\n");
        }
    }
}

void showUserList(char *ans)
{
    msgIndex msgindex[accountNum];
    int i = 0;
    for(i = 0 ; i < accountNum ; i++)
    {
        msgindex[i].index = i;
        strcpy(msgindex[i].name, accountName[i]);
    }
    qsort(msgindex, accountNum, sizeof(msgIndex), cmpfunc);

    for (i = 0 ; i < accountNum ; i++)
    {
        strcat(ans, accountName[msgindex[i].index]);
        strcat(ans, "\n");
    }
}

void storeUsername(char *username, char *pwd)
{
    strcpy(accountName[accountNum], username);
    strcpy(accountPwd[accountNum], pwd);
    accountNum++;
}

int checkUserIfExist(char *username)
{
    int i = 0;
    for(; i < accountNum; i++)
    {
        if (strcmp(accountName[i], username) == 0) return i;
    }
    return -1;
}

/* Tips:
There is no passing by reference in C code, but C++ has.
1. You just can use passing by pointer in C code.
2. Username and pwd are pointer variables, and we want to update them to "address of target string" without return, 
    so need to pass the address of pointer variables to the function, so does msg.
*/
void DecideService(char *readBuf, int n, char **username, char **pwd, char **msg, int *type, int *cnt)
{
    char msg_t[MAXLINE], *order;
    readBuf[n-1] = '\0';
    strcpy(msg_t, readBuf);
    order = strtok(readBuf, " ");
    
    while (order != NULL)
    {
        if      (strcmp(order, "register")   == 0)    *type = 0;
        else if (strcmp(order, "login")      == 0)    *type = 1;
        else if (strcmp(order, "logout")     == 0)    *type = 2;
        else if (strcmp(order, "whoami")     == 0)    *type = 3;
        else if (strcmp(order, "list-user")  == 0)    *type = 4;
        else if (strcmp(order, "exit")       == 0)    *type = 5;
        else if (strcmp(order, "send")       == 0)    *type = 6;
        else if (strcmp(order, "list-msg")   == 0)    *type = 7;
        else if (strcmp(order, "receive")    == 0)    *type = 8;

        if (*type == 0 && *cnt == 1) *username = order;
        else if (*type == 0 && *cnt == 2) *pwd = order;
        
        if (*type == 1 && *cnt == 1) *username = order;
        else if (*type == 1 && *cnt == 2) *pwd = order;

        if (*type == 6 && *cnt == 1) *username = order;
        else if (*type == 6 && *cnt >=2)
        {
            *msg = strtok(msg_t, "\"");
            while (*msg != NULL) {
                *msg = strtok(NULL, "\"");
                break;
            }
        }

        if (*type == 8 && *cnt == 1) *username = order;

        order = strtok(NULL, " ");
        *cnt += 1;
    }
}


void DoService(int type, int cnt, char *username, char *pwd, char *msg, int *userIndex, char *tmp)
{
    int isTrueForm = checkForm(type, cnt);
    if (isTrueForm){
        switch(type){
            // Tackle Register
            case 0:
                // If not exists, return -1, or return user index.
                userIndexForReg = checkUserIfExist(username);
                if (userIndexForReg == -1){
                    storeUsername(username, pwd);
                    strcpy(tmp, "Register successfully.\n");
                }
                else {
                    strcpy(tmp, "Username is already used.\n");
                }
                break;

            // Tackle Login
            case 1:
                if (loginOrNot[*userIndex]){
                    strcat(tmp, "Please logout first.\n");
                }
                else{
                    *userIndex = checkUserIfExist(username);
                    if (loginOrNot[*userIndex]){
                        strcpy(tmp, "Please logout first.\n");
                    }
                    else if (*userIndex == -1){
                        strcpy(tmp, "Login failed.\n");
                    }
                    else if(strcmp(accountPwd[*userIndex], pwd) != 0){
                        strcpy(tmp, "Login failed.\n");
                    }
                    else{
                        strcat(tmp, "Welcome, ");
                        strcat(tmp, username);
                        strcat(tmp, ".\n");
                        loginOrNot[*userIndex] = true;
                    }
                }
                break;

            // Tackle Logout
            case 2:
                if (*userIndex == -1){
                    strcpy(tmp, "Please login first.\n");
                }
                else{
                    loginOrNot[*userIndex] = false;
                    strcat(tmp, "Bye, ");
                    strcat(tmp, accountName[*userIndex]);
                    strcat(tmp, ".\n");
                    *userIndex = -1;
                }
                break;

            // Tackle Who am I
            case 3:
                if (loginOrNot[*userIndex]){
                    strcat(tmp, accountName[*userIndex]);
                    strcat(tmp, "\n");
                }
                else{
                    strcpy(tmp, "Please login first.\n");
                }
                break;

            // Tackle List-user
            case 4:
                showUserList(tmp);
                break;

            // Tackle Exit
            case 5:
                if (loginOrNot[*userIndex]){
                    strcat(tmp, "Bye, ");
                    strcat(tmp, accountName[*userIndex]);
                    strcat(tmp, ".\n");
                }
                break;

            // Tackle Send msg
            case 6:
                if (loginOrNot[*userIndex] == false){
                    strcpy(tmp, "Please login first.\n");
                }
                else if ((userIndexForMsg = checkUserIfExist(username)) != -1){
                    strcpy(msgBox[userIndexForMsg][*userIndex][msgWrite[userIndexForMsg][*userIndex]], msg);
                    msgWrite[userIndexForMsg][*userIndex]++;
                    if (msgWrite[userIndexForMsg][*userIndex] == K) msgWrite[userIndexForMsg][*userIndex] = 0;
                }
                else{
                    strcpy(tmp, "User not existed.\n");
                }
                break;

            // Tackle List-msg
            case 7:
                if (loginOrNot[*userIndex] == false){
                    strcpy(tmp, "Please login first.\n");
                }
                else if (isMsgLeft(*userIndex)){
                    showMsgBox(tmp, *userIndex);
                    break;
                }
                else{
                    strcpy(tmp, "Your message box is empty.\n");
                }
                break;

            // Tackle Receive
            case 8:
                if (loginOrNot[*userIndex] == false){
                    strcpy(tmp, "Please login first.\n");
                }
                else if ((userIndexForRec = checkUserIfExist(username)) != -1){
                    if (msgWrite[*userIndex][userIndexForRec] - msgRead[*userIndex][userIndexForRec] > 0){
                        popMsg(*userIndex, userIndexForRec, tmp);
                    }
                    break;
                }
                else{
                    strcpy(tmp, "User not existed.\n");
                }
                break;

            default:
                strcpy(tmp, "Invalid service!\n");
                break;
        }
    }
    else{
        switch(type){
            case 0: 
                strcpy(tmp, "Usage: register <username> <password>\n"); 
                break;
            case 1: 
                strcpy(tmp, "Usage: login <username> <password>\n");
                break;
            case 6:
                strcpy(tmp, "Usage: send <username> <message>\n");
                break;
            case 8:
                strcpy(tmp, "Usage: receive <username>\n");
                break;

        }
    }
}


void sendMessage(char* buf, char* msg, int sockfd)
{
    bzero(buf, MAXLINE);
    strcpy(buf, msg);
    writen(sockfd, buf, MAXLINE);
}

bool checkForm(int type, int cnt){
    if (type == 0 && cnt != 3) return false;
    if (type == 1 && cnt != 3) return false;
    if (type == 6 && cnt <= 2) return false;
    if (type == 8 && cnt != 2) return false;
    return true;
}

void err_sys(const char* x) 
{ 
    perror(x); 
    exit(1); 
}

/* include writen */
/* Write "n" bytes to a descriptor. */
ssize_t	writen(int fd, const void *vptr, size_t n)
{
	size_t		nleft;
	ssize_t		nwritten;
	const char	*ptr;

	ptr = vptr;
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

void Writen(int fd, void *ptr, size_t nbytes)
{
	if (writen(fd, ptr, nbytes) != nbytes)
		err_sys("writen error");
}
