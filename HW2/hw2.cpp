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
#include <iostream>
#include <algorithm>
#include <ctime>
#include <vector>
using namespace std;

# define SERV_PORT 8000
# define MAXLINE 1024
# define WELCOME (char*)"********************************\n** Welcome to the BBS server. **\n********************************\n"
# define PROMPT (char*)"% "
# define MaxConnection 10   /* Maximum number of client connections*/
# define K 80
# define KK 8000
typedef struct sockaddr SA;

/*From textbook*/
ssize_t writen(int fd, const void *vptr, size_t n);
void    Writen(int fd, void *ptr, size_t nbytes);
void    err_sys(const char* x);

/*Myself*/
ssize_t Readline(int fd, void *ptr, size_t maxlen);
void    Initialization();
void    sendMessage(string msg, int sockfd);
void    DecideService(char *readBuf, int n, string *username, string *pwd, string *msg, string *bname, string *comment, int *type, int *cnt, int *qpost, string *title, string *content);
void    DoService(int type, int cnt, string username, string pwd, string msg, string bname, string title, string content, string comment, int qpost, int clientIdx, string *tmp);
void    popMsg(int idx, int msgindex, string *tmp);
void    storeUsername(string username, string pwd);
void    storeBoardInfo(string bname, string username);
void    storePost(string bname, string username, string title, string content);
void    storeComment(int qpost, string name, string comment);
void    showBoardList(string *tmp);
void    showUserList(string tmp);
void    showMsgBox(string *tmp, int index);
void    showPostList(string *tmp, int index);
void    showPost(string *tmp, int qpost);
void    Welcome(int sockfd);
void    showPrompt(int sockfd);
bool    MessageBox(int sockfd, int clientIdx);
bool    isMsgLeft(int index);
string  accessTime();
string  tranBR(string content);

bool    checkForm(int type, int cnt, string bname, string title, string content, string comment, int qpost);
int     checkUserIfExist(string username);
int     checkBnameIfExist(string bname);

typedef class userInfo{
    public:
        string name;
        string pwd;
        bool   loginOrNot;
        vector<string> msgbox[K];
        userInfo() {loginOrNot = false;}
} userInfo;

typedef class commentInfo{
    public:
        string author;
        string comment;
        commentInfo(string a, string b):author(a), comment(b){}
} commentInfo;

typedef class postInfo{
    public:
        string author;
        string title;
        string content;
        string date;
        vector<commentInfo> VcommentInfo;
        postInfo(string a, string b, string c, string d): author(a), title(b), content(c), date(d){} 
} postInfo;

typedef class boardInfo{
    public:
        string author;
        string bname;
        vector<int> Vpost;
        boardInfo(string a, string b): author(a), bname(b) {}
} boardInfo;

typedef struct msgIndex{
    int index;
    string name;
} msgIndex;


/*HW1 variables*/
char    msgBox[K][K][K][KK];
int     msgWrite[K][K];
int     msgRead[K][K];
int     acnt = 0;

vector<userInfo> VuserInfo;

/*HW2 variables*/
char    boardName[K][KK];
char    boardAuthor[K][KK];
char    boardContent[K][KK];
bool    CUseOrNot[MaxConnection];
int     userIndex[MaxConnection];
int     bcnt = 0;
int     pcnt = 0;

vector<boardInfo> VboardInfo;
vector<postInfo> VpostInfo;
vector<bool>    VpostExist;

int main(int argc, char *argv[])
{

    if (argc != 2){
        err_sys("server usage: ./hw1 [port number]");
    }

    /* Use IPV4 to connect network.*/
    struct sockaddr_in  cliaddr, servaddr;
    int                 listenfd, connfd;
    socklen_t           clilen;
    pid_t               childpid;

    /* Use select to multiplex.*/
    int                 sockfd;
    int                 maxi, maxfd;
    int                 nready, client[FD_SETSIZE];
    fd_set              rset, allset;

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
    listen(listenfd, MaxConnection);

    /* Initialize global variables account*/
    Initialization();

    /* Initialize select's variables*/
    maxi = -1;
    maxfd = listenfd;
    for (int i = 0 ; i < FD_SETSIZE ; i++)
        client[i] = -1;
    FD_ZERO(&allset);
    FD_SET(listenfd, &allset);

    for( ; ; ){
        rset = allset;
        nready = select(maxfd+1, &rset, NULL, NULL, NULL);

        if (FD_ISSET(listenfd, &rset))
        {
            /* clilen is a value-return variable, and should store IPV4's readBuffer size in advance.*/
            /* Use Accept to build connection with client.*/
            clilen = sizeof(cliaddr);
            connfd = accept(listenfd, (SA *) &cliaddr, &clilen);
            Welcome(connfd);
            showPrompt(connfd);
            
            /* Update select's parameter.*/
            int tmp;
            for (int i = 0 ; i < FD_SETSIZE ; i++)
            {
                if (client[i] == -1)
                {
                    client[i] = connfd;
                    tmp = i;
                    break;
                }
            }

            if (tmp > MaxConnection) err_sys("too many clients");
            if (tmp > maxi) maxi = tmp;
            if (connfd > maxfd) maxfd = connfd;
            FD_SET(connfd, &allset);
            if (--nready <= 0) continue;
        }
        
        for (int i = 0 ; i <= maxi ; i++)
        {
            // if ((sockfd = client[i]) == -1) continue;
            sockfd = client[i];
            
            if (FD_ISSET(sockfd, &rset)) {
                CUseOrNot[i] = MessageBox(sockfd, i);   /*Tackle Msgbox function*/

                if (!CUseOrNot[i])                      /*Mean connection failed or client type "exit"*/
                {
                    userIndex[i]       = -1;
                    close(sockfd);
                    FD_CLR(sockfd, &allset);
                    client[i] = -1;
                }
                else showPrompt(sockfd);                /*Deal with "%"*/

				if (--nready <= 0)
					break;				/* no more readable descriptors */
			}
        }
    }
    return 0;
}


bool MessageBox(int sockfd, int clientIdx)
{
    char readBuf[MAXLINE];
    ssize_t n;
    string username, pwd, msg, bname, comment, title, content, tmp;
    int type=-1, cnt=0, qpost=-1;
    bool isTrueForm=true;

again:
    /*Reset*/
    cnt = 0;
    type = -1;
    qpost = -1;
    comment = "";
    pwd = "";
    username = "";
    bname = "";
    msg = "";
    title = "";
    content = "";
    tmp = "";
    
    bzero(&readBuf, MAXLINE);
    /*Check for disconnection*/
    if ((n = Readline(sockfd, readBuf, MAXLINE)) < 0) return -1;                                

    else
    {
        /*Update username, pwd, msg, bname, type, cnt*/
        DecideService(readBuf, n, &username, &pwd, &msg, &bname, &comment, &type, &cnt, &qpost, &title, &content);

        /*Update userIndex[] base on clientIdx, tmp*/
        DoService(type, cnt, username, pwd, msg, bname, title, content, comment, qpost-1, clientIdx, &tmp);

        for ( int z = 0 ; z < MaxConnection ; z++){
            cout << userIndex[z] << ' ';
        }
        cout << endl;
        
        sendMessage(tmp, sockfd);                     

        if (n < 0 && errno == EINTR){
            goto again;
        }
        else if (n < 0){
            err_sys("MessageBox: read error.");
        }
        return type != 5;
    }    
}

void Welcome(int sockfd)
{
    char writeBuf[MAXLINE];
    sendMessage(WELCOME, sockfd);                                 /*Deal with WELCOME*/
}

void showPrompt(int sockfd)
{
    char writeBuf[MAXLINE];
    sendMessage(PROMPT, sockfd);                                 /*Deal with %*/
}

void popMsg(int idx, int msgindex, string *tmp)
{
    *tmp = msgBox[idx][msgindex][msgRead[idx][msgindex]];
    *tmp += "\n";
    msgRead[idx][msgindex]++;
    if (msgRead[idx][msgindex] == K) msgRead[idx][msgindex] = 0;
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
    for ( i=0 ; i < K ; i++)
    {
        memset(boardName[i], '\0', KK);
        memset(boardAuthor[i], '\0', KK);
        memset(boardContent[i], '\0', K);
        memset(msgWrite[i], 0, K);
        memset(msgRead[i], 0, K);
        for ( ; j < K ; j++)
            for ( ; k < K ; k++)
                memset(msgBox[i][j][K], '\0', KK);
    }
    for ( i = 0 ; i < MaxConnection ; i++)
    {
        userIndex[i]       = -1;
    }
    memset(CUseOrNot, false, MaxConnection);
}

int cmpfunc (const void * a, const void * b)
{
    string nameA = (*(msgIndex*)a).name;
    string nameB = (*(msgIndex*)b).name;
    return nameA.compare(nameB);
}

void showPost(string *tmp, int qpost)
{
    *tmp = "Author: " + VpostInfo[qpost].author + "\nTitle: " + VpostInfo[qpost].title + "\nDate: " + VpostInfo[qpost].date + "\n--\n" + VpostInfo[qpost].content + "\n--\n";
    for (int i = 0 ; i < VpostInfo[qpost].VcommentInfo.size() ; i++)
    {
        *tmp += VpostInfo[qpost].VcommentInfo[i].author + ": " + VpostInfo[qpost].VcommentInfo[i].comment;
    }
}

void showPostList(string *tmp, int index)
{
    char i2a[K];
    int pi;
    *tmp += (string)"S/N Title Author Date\n";
    for (int i = 0 ; i < VboardInfo[index].Vpost.size() ; i++)
    {
        pi = VboardInfo[index].Vpost[i];
        if (VpostExist[pi])
        {
            sprintf(i2a, "%d", pi+1);
            *tmp += i2a + (string)" " + VpostInfo[pi].title + " " + VpostInfo[pi].author + " " + VpostInfo[pi].date + "\n";
        }
    }
}

void showMsgBox(string *tmp, int index)
{
    msgIndex msgindex[acnt];
    int i = 0, target = 0, msgLeft = 0;
    char i2a[K];

    for(i = 0 ; i < acnt ; i++)
    {
        msgindex[i].index = i;
        msgindex[i].name = VuserInfo[i].name;
    }
    qsort(msgindex, acnt, sizeof(msgIndex), cmpfunc);

    for (i = 0 ; i < acnt ; i++)
    {
        target = msgindex[i].index;
        msgLeft = abs(msgWrite[index][target] - msgRead[index][target]);
        if (msgLeft)
        {
            sprintf(i2a, "%d", msgLeft);
            *tmp += i2a + (string)" message from " + VuserInfo[i].name + ".\n";
        }
    }
}


void showBoardList(string *tmp)
{
    char i2a[K];
    *tmp = "Index Name Moderator\n";
    for (int i = 0 ; i < bcnt ; i++)
    {
        sprintf(i2a, "%d", i+1);
        *tmp += i2a + (string)" " + VboardInfo[i].bname + " " + VboardInfo[i].author + "\n";
    }
}

void showUserList(string *tmp)
{
    msgIndex msgindex[acnt];
    int i = 0;
    for(i = 0 ; i < acnt ; i++)
    {
        msgindex[i].index = i;
        msgindex[i].name = VuserInfo[i].name;
    }
    qsort(msgindex, acnt, sizeof(msgIndex), cmpfunc);

    for (i = 0 ; i < acnt ; i++)
    {
        *tmp += VuserInfo[msgindex[i].index].name + "\n";
    }
}

void storeComment(int qpost, string author, string comment)
{
    commentInfo tmp(author, comment);
    VpostInfo[qpost].VcommentInfo.push_back(tmp);
}

void storePost(string bname, string author, string title, string content)
{
    int index;
    string date = accessTime();
    if ((index = checkBnameIfExist(bname)) != -1)
    {
        string t_content = tranBR(content);
        postInfo tmp(author, title, t_content, date);
        
        VpostInfo.push_back(tmp);
        VpostExist.push_back(true);
        VboardInfo[index].Vpost.push_back(pcnt);
        pcnt++;
    }
}

string tranBR(string content)
{
    string delimiter = "<br>";
    string ans;
    size_t pos;
    while ((pos = content.find(delimiter)) != string::npos) {
        ans += content.substr(0, pos);
        ans += "\n";
        content.erase(0, pos + delimiter.length());
    }
    ans += content;
    return ans;
}

void storeBoardInfo(string bname, string username)
{
    boardInfo tmp(username, bname);
    VboardInfo.push_back(tmp);
    bcnt++;
}

void storeUsername(string username, string pwd)
{
    userInfo tmp;
    tmp.name = username;
    tmp.pwd = pwd;
    VuserInfo.push_back(tmp);
    acnt++;
}

int checkUserIfExist(string username)
{
    for(int i = 0; i < acnt; i++)
    {
        if (username.compare(VuserInfo[i].name) == 0) return i;
    }
    return -1;
}

/* Tips:
    There is no passing by reference in C code, but C++ has.
    1. You just can use passing by pointer in C code.
    2. Username and pwd are pointer variables, and we want to update them to "address of target string" without return, 
        so need to pass the address of pointer variables to the function, so does msg.
*/
void DecideService(char *readBuf, int n, string *username, string *pwd, string *msg, string *bname, string *comment,
                    int *type, int *cnt, int *qpost, string *title, string *content)
{
    char *order;
    string msg_t = readBuf;
    readBuf[n-1] = '\0';
    order = strtok(readBuf, " ");
    
    while (order != NULL)
    {
        /*HW1 operation*/
        if      (strcmp(order, "register")       == 0)    *type = 0;
        else if (strcmp(order, "login")          == 0)    *type = 1;
        else if (strcmp(order, "logout")         == 0)    *type = 2;
        else if (strcmp(order, "whoami")         == 0)    *type = 3;
        else if (strcmp(order, "list-user")      == 0)    *type = 4;
        else if (strcmp(order, "exit")           == 0)    *type = 5;
        else if (strcmp(order, "send")           == 0)    *type = 6;
        else if (strcmp(order, "list-msg")       == 0)    *type = 7;
        else if (strcmp(order, "receive")        == 0)    *type = 8;
        /*HW2 operation*/
        else if (strcmp(order, "create-board")   == 0)    *type = 9;
        else if (strcmp(order, "create-post")    == 0)    *type = 10;
        else if (strcmp(order, "list-board")     == 0)    *type = 11;
        else if (strcmp(order, "list-post")      == 0)    *type = 12;
        else if (strcmp(order, "read")           == 0)    *type = 13;
        else if (strcmp(order, "delete-post")    == 0)    *type = 14;
        else if (strcmp(order, "update-post")    == 0)    *type = 15;
        else if (strcmp(order, "comment")        == 0)    *type = 16;


        if (*type == 0 && *cnt == 1) *username = order;
        else if (*type == 0 && *cnt == 2) *pwd = order;
        
        if (*type == 1 && *cnt == 1) *username = order;
        else if (*type == 1 && *cnt == 2) *pwd = order;

        // if (*type == 6 && *cnt == 1) *username = order;
        // else if (*type == 6 && *cnt >=2)
        // {
        //     *msg = strtok(msg_t, "\"");
        //     while (*msg != NULL) {
        //         *msg = strtok(NULL, "\"");
        //         break;
        //     }
        // }

        if (*type == 8 && *cnt == 1) *username = order;

        if (*type == 9 && *cnt == 1) *bname    = order;

        if (*type == 10 && *cnt == 1)
        {
            vector<string> words{};
            string delimiter = "--";
            size_t pos;
            bool first = true;
            while ((pos = msg_t.find(delimiter)) != string::npos) {
                words.push_back(msg_t.substr(0, pos));
                msg_t.erase(0, pos + delimiter.length());
            }
            words.push_back(msg_t);
            delimiter = " ";
            int sp;
            string str_cpy;
            for (const auto &str : words) {
                if (first){
                    *bname = str;
                    if ((sp = bname->find(delimiter)) == -1) continue;
                    bname->erase(0, sp + delimiter.length());
                    if ((sp = bname->find(delimiter)) == -1) continue;
                    bname->erase(sp, sp + delimiter.length());
                    first = false;
                }
                else{
                    sp = str.find(delimiter);
                    string key = str.substr(0, sp);
                    str_cpy = str;
                    str_cpy.erase(0, sp + delimiter.length());
                    str_cpy.erase(std::remove(str_cpy.begin(), str_cpy.end(), '\n'), str_cpy.end());
                    if (key.compare("title") == 0)          *title = str_cpy;
                    else if (key.compare("content") == 0)   *content = str_cpy;
                }
            }
        }

        if (*type == 12 && *cnt == 1) *bname   = order;

        if (*type == 13 && *cnt == 1) *qpost   = atoi(order);

        if (*type == 14 && *cnt == 1) *qpost   = atoi(order);

        if (*type == 15 && *cnt == 1) *qpost   = atoi(order);
        else if (*type == 15 && *cnt == 2)
        {
            vector<string> words{};
            string delimiter = "--";
            size_t pos;
            bool first = true;
            while ((pos = msg_t.find(delimiter)) != string::npos) {
                words.push_back(msg_t.substr(0, pos));
                msg_t.erase(0, pos + delimiter.length());
            }
            words.push_back(msg_t);
            
            delimiter = " ";
            for (const auto &str : words) {
                if (first) first = false;
                else{
                    int sp = str.find(" ");
                    string key = str.substr(0, sp);
                    string str_cpy = str;
                    str_cpy.erase(0, sp + delimiter.length());
                    str_cpy.erase(std::remove(str_cpy.begin(), str_cpy.end(), '\n'), str_cpy.end());
                    if (key.compare("title") == 0)          *title = str_cpy;
                    else if (key.compare("content") == 0)   *content = str_cpy;
                }
            }
        }

        if (*type == 16 && *cnt == 1) *qpost   = atoi(order);
        else if (*type == 16 && *cnt == 2)
        {
            string delimiter = " ";
            size_t pos;
            int cnt_ = 0;
            while ((pos = msg_t.find(delimiter)) != string::npos) {
                if (cnt_ == 2) break;
                msg_t.erase(0, pos + delimiter.length());
                cnt_++;
            }
            *comment = msg_t;
        }


        order = strtok(NULL, " ");
        *cnt += 1;
    }
}

void DoService(int type, int cnt, string username, string pwd, string msg, string bname, 
    string title, string content, string comment, int qpost, int clientIdx, string *tmp)
{
    int RegForClient;
    int MsgForClient;
    int RecForClient;
    int CBForClient;
    int CPForClient;
    int LPForClient;

    int isTrueForm = checkForm(type, cnt, bname, title, content, comment, qpost);
    if (isTrueForm)
    {
        switch(type)
        {
            // Tackle Register
            case 0:
                // If not exists, return -1, or return user index.
                RegForClient = checkUserIfExist(username);
                if (RegForClient == -1){
                    storeUsername(username, pwd);
                    *tmp = "Register successfully.\n";
                }
                else {
                    *tmp = "Username is already used.\n";
                }
                break;

            // Tackle Login
            case 1:
                if (VuserInfo.size() == 0) {
                    *tmp = "Login failed.\n";
                }
                else if (userIndex[clientIdx] != -1){
                    *tmp = "Please logout first.\n";
                }
                else{
                    userIndex[clientIdx] = checkUserIfExist(username);
                    if (VuserInfo[userIndex[clientIdx]].loginOrNot){
                        *tmp = "Please logout first.\n";
                        userIndex[clientIdx] = -1;
                    }
                    else if (userIndex[clientIdx] == -1){
                        *tmp = "Login failed.\n";
                    }
                    else if(pwd.compare(VuserInfo[userIndex[clientIdx]].pwd) != 0){
                        *tmp = "Login failed.\n";
                    }
                    else{
                        *tmp = "Welcome, ";
                        *tmp += (string)username+ ".\n";
                        VuserInfo[userIndex[clientIdx]].loginOrNot = true;
                    }
                }
                break;

            // Tackle Logout
            case 2:
                if (userIndex[clientIdx] == -1){
                    *tmp = "Please login first.\n";
                }
                else{
                    VuserInfo[userIndex[clientIdx]].loginOrNot = false;
                    *tmp = "Bye, ";
                    *tmp += VuserInfo[userIndex[clientIdx]].name + ".\n";
                    userIndex[clientIdx] = -1;
                }
                break;

            // Tackle Who am I
            case 3:
                if (VuserInfo.size() == 0) {
                    *tmp = "Please login first.\n";
                }
                else if (VuserInfo[userIndex[clientIdx]].loginOrNot){
                    *tmp = VuserInfo[userIndex[clientIdx]].name;
                    *tmp += "\n";
                }
                else{
                    *tmp = "Please login first.\n";
                }
                break;

            // Tackle List-user
            case 4:
                showUserList(tmp);
                break;

            // Tackle Exit
            case 5:
                if (VuserInfo.size() == 0) {
                    *tmp = "Bye.\n";
                }
                else if (VuserInfo[userIndex[clientIdx]].loginOrNot){
                    VuserInfo[userIndex[clientIdx]].loginOrNot = false;
                    *tmp = "Bye, ";
                    *tmp += VuserInfo[userIndex[clientIdx]].name + ".\n";
                }
                break;

            // Tackle Send msg
            case 6:
                if (VuserInfo.size() == 0) {
                    *tmp = "Please login first.\n";
                }
                else if (VuserInfo[userIndex[clientIdx]].loginOrNot == false){
                    *tmp = "Please login first.\n";
                }
                else if ((MsgForClient = checkUserIfExist(username)) != -1){
                    VuserInfo[MsgForClient].msgbox[userIndex[clientIdx]].push_back(msg);
                    msgWrite[MsgForClient][userIndex[clientIdx]]++;
                    if (msgWrite[MsgForClient][userIndex[clientIdx]] == K) msgWrite[MsgForClient][userIndex[clientIdx]] = 0;
                }
                else{
                    *tmp = "User not existed.\n";
                }
                break;

            // Tackle List-msg
            case 7:
                if (VuserInfo.size() == 0) {
                    *tmp = "Please login first.\n";
                }
                else if (VuserInfo[userIndex[clientIdx]].loginOrNot == false){
                    *tmp = "Please login first.\n";
                }
                else if (isMsgLeft(userIndex[clientIdx])){
                    showMsgBox(tmp, userIndex[clientIdx]);
                    break;
                }
                else{
                    *tmp = "Your message box is empty.\n";
                }
                break;

            // Tackle Receive
            case 8:
                if (VuserInfo.size() == 0) {
                    *tmp = "Please login first.\n";
                }
                else if (VuserInfo[userIndex[clientIdx]].loginOrNot == false){
                    *tmp = "Please login first.\n";
                }
                else if ((RecForClient = checkUserIfExist(username)) != -1){
                    if (msgWrite[userIndex[clientIdx]][RecForClient] - msgRead[userIndex[clientIdx]][RecForClient] > 0){
                        popMsg(userIndex[clientIdx], RecForClient, tmp);
                    }
                    break;
                }
                else{
                    *tmp = "User not existed.\n";
                }
                break;

            // Tackle create-board
            case 9:
                if (VuserInfo.size() == 0) {
                    *tmp = "Please login first.\n";
                }
                else if (VuserInfo[userIndex[clientIdx]].loginOrNot == false){
                    *tmp= "Please login first.\n";
                }
                else if ((CBForClient = checkBnameIfExist(bname)) == -1){
                    storeBoardInfo(bname, VuserInfo[userIndex[clientIdx]].name);
                    *tmp = "Create board successfully.\n";
                }
                else{
                    *tmp = "Board already exists.\n";
                }
                break;

            // Tackle create-post
            case 10:
                if (VuserInfo.size() == 0) {
                    *tmp = "Please login first.\n";
                }
                else if (VuserInfo[userIndex[clientIdx]].loginOrNot == false){
                    *tmp = "Please login first.\n";
                }
                else if ((CPForClient = checkBnameIfExist(bname)) != -1){
                    storePost(bname, VuserInfo[userIndex[clientIdx]].name, title, content);
                    *tmp = "Create post successfully.\n";
                }
                else{
                    *tmp = "Board does not exist.\n";
                }
                break;

            // Tackle list-board
            case 11:
                showBoardList(tmp);
                break;

            // Tackle list-post
            case 12:
                if ((LPForClient = checkBnameIfExist(bname)) != -1){
                    showPostList(tmp, LPForClient);
                }
                else{
                    *tmp = "Board does not exist.\n";
                }
                break;

            
            // Tackle read
            case 13:
                if (qpost < pcnt && VpostExist[qpost]){
                    showPost(tmp, qpost);
                }
                else{
                    *tmp = "Post does not exist.\n";
                }
                break;

            // Tackle delete post
            case 14:
                if (VuserInfo.size() == 0) {
                    *tmp = "Please login first.\n";
                }
                else if (VuserInfo[userIndex[clientIdx]].loginOrNot == false){
                    *tmp = "Please login first.\n";
                }
                else if (!VpostExist[qpost] && qpost >= pcnt){
                    *tmp = "Post does not exist.\n";
                }
                else if (VuserInfo[userIndex[clientIdx]].name != VpostInfo[qpost].author){
                    *tmp = "Not the post owner.\n";
                }
                else{
                    VpostExist[qpost] = false;
                    *tmp = "Delete successfully.\n";
                }
                break;

            // Tackle update post
            case 15:
                if (VuserInfo.size() == 0) {
                    *tmp = "Please login first.\n";
                }
                else if (VuserInfo[userIndex[clientIdx]].loginOrNot == false){
                    *tmp = "Please login first.\n";
                }
                else if (!VpostExist[qpost] && qpost >= pcnt){
                    *tmp = "Post does not exist.\n";
                }
                else if (VuserInfo[userIndex[clientIdx]].name != VpostInfo[qpost].author){
                    *tmp = "Not the post owner.\n";
                }
                else{
                    if (title.length() != 0)    VpostInfo[qpost].title = title;
                    else if (content.length() != 0)  VpostInfo[qpost].content = tranBR(content);
                    *tmp = "Update successfully.\n";
                }
                break;
            
            // Tackle comment
            case 16:
                if (VuserInfo.size() == 0) {
                    *tmp = "Please login first.\n";
                }
                else if (VuserInfo[userIndex[clientIdx]].loginOrNot == false){
                    *tmp = "Please login first.\n";
                }
                else if (!VpostExist[qpost] && qpost >= pcnt){
                    *tmp = "Post does not exist.\n";
                }
                else{
                    storeComment(qpost, VuserInfo[userIndex[clientIdx]].name, comment);
                    *tmp = "Comment successfully.\n";
                }
                break;


            default:
                *tmp = "Invalid service!\n";
                break;
        }
    }
    else
    {
        switch(type){
            case 0: 
                *tmp = "Usage: register <username> <password>\n"; 
                break;
            case 1: 
                *tmp = "Usage: login <username> <password>\n";
                break;
            case 6:
                *tmp = "Usage: send <username> <message>\n";
                break;
            case 8:
                *tmp = "Usage: receive <username>\n";
                break;
            case 9:
                *tmp = "Usage: create-board <name>\n";
                break;
            case 10:
                *tmp = "Usage: create-post <board-name> --title <title> --content <content>\n";
                break;
            case 12:
                *tmp = "Usage: list-post <board-name>\n";
                break;
            case 13:
                *tmp = "Usage: read <post-S/N>\n";
                break;
            case 14:
                *tmp = "Usage: delete-post <post-S/N>\n";
                break;
            case 15:
                *tmp = "Usage: update-post <post-S/N> --title/content <new>\n";
                break;
            case 16:
                *tmp = "Usage: comment <post-S/N> <comment>\n";
                break;

        }
    }
}

string accessTime()
{
    char i2a[K];
    string date;
    time_t now = time(0);
    tm *ltm = localtime(&now);
    sprintf(i2a, "%d", 1 + ltm->tm_mon);
    date += i2a + (string)"/";
    sprintf(i2a, "%d", ltm->tm_mday);
    date += i2a;
    return date;
}

void sendMessage(string msg, int sockfd)
{
    char buf[MAXLINE];
    bzero(buf, MAXLINE);
    strcpy(buf, (char*)msg.c_str());
    writen(sockfd, buf, strlen(buf));
}

bool checkForm(int type, int cnt, string bname, string title, string content, string comment, int qpost){
    if (type == 0 && cnt != 3) return false;
    if (type == 1 && cnt != 3) return false;
    if (type == 6 && cnt <= 2) return false;
    if (type == 8 && cnt != 2) return false;
    if (type == 9 && cnt != 2) return false;
    if (type == 10 && (bname == "" || title == "" || content == "")) return false;
    if (type == 12 && cnt != 2) return false;
    if (type == 13 && cnt != 2) return false;
    if (type == 14 && cnt != 2) return false;
    if (type == 15 && (title == "" && content == "") || qpost == -1) return false;
    if (type == 16 && (comment == "" || qpost == -1))    return false;
    return true;
}


int checkBnameIfExist(string bname)
{
    int i = 0;
    for(; i < bcnt; i++)
    {
        if (bname.compare(VboardInfo[i].bname) == 0) return i;
    }
    return -1;
}

/* include writen */
/* Write "n" bytes to a descriptor. */
ssize_t	writen(int fd, const void *vptr, size_t n)
{
	size_t		nleft;
	ssize_t		nwritten;
	const char	*ptr;

	ptr = (const char*)vptr;
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
		err_sys("readline error");
	return(n);
}

void err_sys(const char* x) 
{ 
    perror(x); 
    exit(1); 
}