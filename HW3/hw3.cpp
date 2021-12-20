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

string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                        "abcdefghijklmnopqrstuvwxyz"
                        "0123456789+/";

/*From textbook*/
ssize_t writen(int fd, const void *vptr, size_t n);
void    Writen(int fd, void *ptr, size_t nbytes);
void    err_sys(const char* x);

/*Myself*/
ssize_t Readline(int fd, void *ptr, size_t maxlen);
void    Initialization();
void    sendMessage(string msg, int sockfd);
void    DecideService(char *readBuf, int n, string *username, string *pwd, string *msg, string *bname, string *comment, int *type, int *cnt, int *qpost, string *title, string *content, string *port, string *version, string *chatMsg);
void    DoService(int sockfd, int &type, int cnt, string username, string pwd, string msg, string bname, string title, string content, string comment, int qpost, string port, string version, int sockfd_udp, string chatMsg, int clientIdx, string *tmp);
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
void    bindPort(int port, int version);
bool    MessageBox(int sockfd, int clientIdx, int sockfd_udp);
bool    isMsgLeft(int index);
void    storePort(int sockfd, int port, int version, int clientIdx);
void    updateHistory(string fullMsg, int clientIdx);
void    oneForAll(int sockfd_udp, string name, string chatMsg, int clientIdx);
void    chatRoom(int sockfd_udp);
void    upack_v1(char* pkt, string &name, string &chatMsg);
void    upack_v2(char* pkt, string &name, string &chatMsg);

string  filterMsg(int userIndex, string chatMsg);
string  accessTime();
string  tranBR(string content);
string  mergeName(int clientIdx, string chatMsg);
string  to_base64(string const &data);
string  from_base64(string const &data);

bool    checkForm(int type, int cnt, string bname, string title, string content, string comment, int qpost);
int     checkUserIfExist(string username);
int     checkBnameIfExist(string bname);
int     findClientIdx(struct sockaddr_in target);

typedef class userInfo
{
    public:
        string name;
        string pwd;
        bool   loginOrNot, inChatroomOrNot, ban;
        int    black;
        vector<string> msgbox[K];
        userInfo() {loginOrNot = false; inChatroomOrNot = false; ban = false; black = 0;}
} userInfo;

typedef class commentInfo
{
    public:
        string author;
        string comment;
        commentInfo(string a, string b):author(a), comment(b){}
} commentInfo;

typedef class postInfo
{
    public:
        string author;
        string title;
        string content;
        string date;
        vector<commentInfo> VcommentInfo;
        postInfo(string a, string b, string c, string d): author(a), title(b), content(c), date(d){} 
} postInfo;

typedef class boardInfo
{
    public:
        string author;
        string bname;
        vector<int> Vpost;
        boardInfo(string a, string b): author(a), bname(b) {}
} boardInfo;

typedef struct msgIndex
{
    int index;
    string name;
} msgIndex;

typedef class talkerInfo
{
    public:
        int port;
        int version;
        struct sockaddr_in cliaddr;
        talkerInfo(){}
        talkerInfo(int a, int b):port(a), version(b){}
} talkerInfo;

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

/*HW3 variables*/
talkerInfo VtalkerInfo[MaxConnection];
string  history = "";
vector<string>  filteringList = {"how", "you", "or", "pek0", "tea", "ha", "kon", "pain", "Starburst Stream"};


int main(int argc, char *argv[])
{
    if (argc != 2){
        err_sys("server usage: ./hw3 [port number]");
    }

    /* Use IPV4 to connect network.*/
    struct sockaddr_in  cliaddr, servaddr;
    int                 listenfd_tcp, sockfd_udp, connfd;
    socklen_t           clilen;
    pid_t               childpid;

    /* Use select to multiplex.*/
    int                 sockfd;
    int                 maxi, maxfd;
    int                 nready, client[FD_SETSIZE];
    fd_set              rset, allset;

    /* Initiate a socket service.*/
    listenfd_tcp = socket(AF_INET, SOCK_STREAM, 0);
    sockfd_udp = socket(AF_INET, SOCK_DGRAM, 0);

    int flag = 1, len = sizeof(int);
    setsockopt(listenfd_tcp, SOL_SOCKET, SO_REUSEADDR, &flag, len);
    
    /* Fill server's basic info.*/
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(atoi(argv[1]));

    /* Bind basic info with the socket service.*/
    bind(listenfd_tcp, (SA *) &servaddr, sizeof(servaddr));
    bind(sockfd_udp, (SA *) &servaddr, sizeof(servaddr));

    /* Set the server to LISTEN state.*/
    listen(listenfd_tcp, MaxConnection);

    /* Initialize global variables account*/
    Initialization();

    /* Initialize select's variables*/
    maxi = -1;
    maxfd = listenfd_tcp > sockfd_udp ? listenfd_tcp : sockfd_udp;
    for (int i = 0 ; i < FD_SETSIZE ; i++)
        client[i] = -1;
    FD_ZERO(&allset);
    FD_SET(listenfd_tcp, &allset);
    FD_SET(sockfd_udp, &allset);

    for( ; ; ){
        rset = allset;
        nready = select(maxfd+1, &rset, NULL, NULL, NULL);

        if (FD_ISSET(sockfd_udp, &rset)) 
        {
            chatRoom(sockfd_udp);
        }

        if (FD_ISSET(listenfd_tcp, &rset))
        {
            /* clilen is a value-return variable, and should store IPV4's readBuffer size in advance.*/
            /* Use Accept to build connection with client.*/
            clilen = sizeof(cliaddr);
            connfd = accept(listenfd_tcp, (SA *) &cliaddr, &clilen);
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
            if ((sockfd = client[i]) == -1) continue;
            sockfd = client[i];
            
            if (VuserInfo.size() != 0)
            {
                if (VuserInfo[userIndex[i]].ban)
                {
                    char tmp[1000];
                    string ttmp = tmp;
                    bzero(&tmp, 1000);
                    sprintf(tmp, "Bye, %s.\n", VuserInfo[userIndex[i]].name.c_str());
                    ttmp = tmp;
                    sendMessage(ttmp, sockfd);
                    showPrompt(sockfd);
                    VuserInfo[userIndex[i]].loginOrNot = false;
                    userIndex[i] = -1;
                    
                }
            }

            if (FD_ISSET(sockfd, &rset)) 
            {
                CUseOrNot[i] = MessageBox(sockfd, i, sockfd_udp);   /*Tackle Msgbox function*/
                if (CUseOrNot[i])                      /*Mean connection failed or client type "exit"*/
                {
                    userIndex[i]       = -1;
                    close(sockfd);
                    FD_CLR(sockfd, &allset);
                    client[i] = -1;
                }
                else showPrompt(sockfd);                /*Deal with "%"*/

                if (--nready <= 0)
                    break;              /* no more readable descriptors */

            }
        }

        

        for (int i = 0 ; i <maxi ; i++)
            cout << "H: " << VuserInfo[userIndex[i]].black << ' ' << VuserInfo[userIndex[i]].loginOrNot << endl;
        
    }
    return 0;
}

string to_base64(string const &data) 
{
    int counter = 0;
    uint32_t bit_stream = 0;
    string encoded = "";
    int offset = 0;
    for (unsigned char c : data) {
        auto num_val = static_cast<unsigned int>(c);
        offset = 16 - counter % 3 * 8;
        bit_stream += num_val << offset;
        if (offset == 16) {
        encoded += base64_chars.at(bit_stream >> 18 & 0x3f);
        }
        if (offset == 8) {
        encoded += base64_chars.at(bit_stream >> 12 & 0x3f);
        }
        if (offset == 0 && counter != 3) {
        encoded += base64_chars.at(bit_stream >> 6 & 0x3f);
        encoded += base64_chars.at(bit_stream & 0x3f);
        bit_stream = 0;
        }
        counter++;
    }
    if (offset == 16) {
        encoded += base64_chars.at(bit_stream >> 12 & 0x3f);
        encoded += "==";
    }
    if (offset == 8) {
        encoded += base64_chars.at(bit_stream >> 6 & 0x3f);
        encoded += '=';
    }
    return encoded;
}

string from_base64(string const &data) 
{
    int counter = 0;
    uint32_t bit_stream = 0;
    string decoded = "";
    int offset = 0;
    for (unsigned char c : data) {
        auto num_val = base64_chars.find(c);
        if (num_val != string::npos) {
            offset = 18 - counter % 4 * 6;
            bit_stream += num_val << offset;
            if (offset == 12) {
                decoded += static_cast<char>(bit_stream >> 16 & 0xff);
            }
            if (offset == 6) {
                decoded += static_cast<char>(bit_stream >> 8 & 0xff);
            }
            if (offset == 0 && counter != 4) {
                decoded += static_cast<char>(bit_stream & 0xff);
                bit_stream = 0;
            }
        } 
        else if (c != '=') {
            return string();
        }
        counter++;
    }
    return decoded;
}

void upack_v1(unsigned char* pkt, string &name, string &chatMsg)
{
    uint16_t name_len, msg_len;
    int i;
    name = "";
    chatMsg = "";

    name_len = ((uint16_t)pkt[2] << 8 | pkt[3]);
    for (i = 4 ; i < 4+name_len ; i++)
    {
        name += pkt[i];
    }
    msg_len = ((uint16_t)pkt[i] << 8 | pkt[i+1]);
    i += 2;
    chatMsg = "";
    for (; i < 6+name_len+msg_len ; i ++)
    {
        chatMsg += pkt[i];
    }
}

void upack_v2(unsigned char* pkt, string &name, string &chatMsg)
{
    int i;
    string name_t = "", chatMsg_t = "";
    bool flg = true;

    for (i = 2 ; i < strlen((char*)pkt) ; i++)
    {
        if (flg)
        {
            if (pkt[i] == '\n')
                flg = false;
            else
                name_t += pkt[i];
        }
        else
        {
            if (pkt[i] == '\n')
                break;
            chatMsg_t += pkt[i];
        }
    }  
    name = from_base64(name_t);
    chatMsg = from_base64(chatMsg_t);
}

string mergeName(int clientIdx, string chatMsg)
{
    string ans = VuserInfo[userIndex[clientIdx]].name + (string)":" + chatMsg;
    return ans;
}

int findClientIdx(struct sockaddr_in target)
{
    for (int i = 0 ; i < MaxConnection ; i++)
    {
        // if (htons(VtalkerInfo[i].cliaddr.sin_port) == target)
        if (target.sin_port == VtalkerInfo[i].cliaddr.sin_port)
            return i;
    }
    return -1;
}

void chatRoom(int sockfd_udp)
{
    int n, clientIdx, version;
    unsigned char pkt[MAXLINE], msg_t[MAXLINE];
    struct sockaddr_in cliaddr;
    socklen_t len = sizeof(cliaddr);
    string fullMsg;
    
    if (!VuserInfo[userIndex[clientIdx]].ban)
    {
        n = recvfrom(sockfd_udp, pkt, MAXLINE, 0, (SA*) &cliaddr, &len);
        clientIdx = findClientIdx(cliaddr);
        version = VtalkerInfo[clientIdx].version;
    
        string name, chatMsg;
        if (version == 1)
            upack_v1(pkt, name, chatMsg);
        else if (version == 2)
            upack_v2(pkt, name, chatMsg);

        chatMsg = filterMsg(userIndex[clientIdx], chatMsg);
        fullMsg = mergeName(clientIdx, chatMsg);
        updateHistory(fullMsg, clientIdx);
        oneForAll(sockfd_udp, name, chatMsg, clientIdx);
    }
}


bool MessageBox(int sockfd, int clientIdx, int sockfd_udp)
{
    char readBuf[MAXLINE];
    ssize_t n;
    string username, pwd, msg, bname, comment, title, content, tmp, chatMsg, port, version;
    int type=-1, cnt=0, qpost=-1;
    bool isTrueForm=true;

again:
    /*Reset*/
    cnt = 0;
    type = -1;
    qpost = -1;
    chatMsg = "";
    comment = "";
    pwd = "";
    username = "";
    bname = "";
    msg = "";
    title = "";
    content = "";
    tmp = "";
    port = version = "";
    
    bzero(&readBuf, MAXLINE);

    /*Check for disconnection*/
    if ((n = Readline(sockfd, readBuf, MAXLINE)) < 0) return -1;                                

    else
    {
        /*Update username, pwd, msg, bname, type, cnt*/
        DecideService(readBuf, n, &username, &pwd, &msg, &bname, &comment, &type, &cnt, &qpost, &title, &content, &port, &version, &chatMsg);

        /*Update userIndex[] base on clientIdx, tmp*/
        DoService(sockfd, type, cnt, username, pwd, msg, bname, title, content, comment, qpost-1, port, version, sockfd_udp, chatMsg, clientIdx, &tmp);
        
        sendMessage(tmp, sockfd);

        if (n < 0 && errno == EINTR){
            goto again;
        }
        else if (n < 0){
            err_sys("MessageBox: read error.");
        }
        return (type == 5);
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

int cmpfunc(const void * a, const void * b)
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
                int *type, int *cnt, int *qpost, string *title, string *content, string *port, string *version, string *chatMsg)
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
        /*HW3 operation*/
        else if (strcmp(order, "enter-chat-room")== 0)    *type = 17;
        // else if (strcmp(order, "chat")           == 0)    *type = 18;


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
                    str_cpy.erase(remove(str_cpy.begin(), str_cpy.end(), '\n'), str_cpy.end());
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
                    str_cpy.erase(remove(str_cpy.begin(), str_cpy.end(), '\n'), str_cpy.end());
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

        if (*type == 17 && *cnt == 1) *port    = order;
        else if (*type == 17 && *cnt == 2) *version = order;

        // if (*type == 18 && *cnt == 1)
        // {
        //     string delimiter = " ";
        //     size_t pos = msg_t.find(delimiter);
        //     msg_t.erase(0, pos + delimiter.length());
        //     *chatMsg = msg_t;
        //     break;
        // }

        order = strtok(NULL, " ");
        *cnt += 1;
    }
}

bool isdigit(string input)
{
    int tmp;
    for (int i = 0 ; i < input.length() ; i++)
    {
        tmp = (int)input[i];
        if (47 > tmp || tmp > 57) return false;
    }
    return true;
}

void DoService(int sockfd, int &type, int cnt, string username, string pwd, string msg, string bname, 
    string title, string content, string comment, int qpost, string port, string version, int sockfd_udp, string chatMsg, int clientIdx, string *tmp)
{
    int RegForClient;
    int MsgForClient;
    int RecForClient;
    int CBForClient;
    int CPForClient;
    int LPForClient;
    char ttmp[500];
    int tmpForPort;
    bool valid_exit = false;

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
                        userIndex[clientIdx] = -1;
                    }
                    else if (VuserInfo[userIndex[clientIdx]].ban){
                        sprintf(ttmp, "We don't welcome %s!\n", VuserInfo[userIndex[clientIdx]].name.c_str());
                        *tmp = ttmp;
                        userIndex[clientIdx] = -1;
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
                    *tmp = "";
                }
                else if (VuserInfo[userIndex[clientIdx]].loginOrNot){
                    VuserInfo[userIndex[clientIdx]].loginOrNot = false;
                    *tmp = "Bye, ";
                    *tmp += VuserInfo[userIndex[clientIdx]].name + ".\n";
                    userIndex[clientIdx] = -1;            
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

            // Tackle enter-chat-room
            case 17:
                if (VuserInfo.size() == 0) {
                    *tmp = "Please login first.\n";
                }
                else if (VuserInfo[userIndex[clientIdx]].loginOrNot == false){
                    *tmp = "Please login first.\n";
                }
                else if (!isdigit(port)) {
                    sprintf(ttmp, "Port %s is not valid.\n", port.c_str());
                    *tmp = ttmp;
                }
                else if (!isdigit(version)) {
                    sprintf(ttmp, "Version %s is not supported.\n", version.c_str());
                    *tmp = ttmp;
                }
                else if (atoi(port.c_str()) < 1 || atoi(port.c_str()) > 65535) {
                    sprintf(ttmp, "Port %s is not valid.\n", port.c_str());
                    *tmp = ttmp;
                }
                else if (atoi(version.c_str()) != 1 && atoi(version.c_str()) != 2) {
                    sprintf(ttmp, "Version %s is not supported.\n", version.c_str());
                    *tmp = ttmp;
                }
                else if (VuserInfo[userIndex[clientIdx]].loginOrNot == false){
                    *tmp = "Please login first.\n";
                }
                else {
                    sprintf(ttmp, "Welcome to public chat room.\nPort:%s\nVersion:%s\n%s", port.c_str(), version.c_str(), history.c_str());
                    *tmp = ttmp;
                    storePort(sockfd, atoi(port.c_str()), atoi(version.c_str()), clientIdx);
                    VuserInfo[userIndex[clientIdx]].inChatroomOrNot = true;
                }
                break;
            
            // Default
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
            case 2:
                *tmp = "Usage: logout\n";
                break;
            case 5:
                *tmp = "Usage: exit\n";
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
            case 17:
                *tmp = "Usage: enter-chat-room <port> <version>\n";
                break;
            case 18:
                *tmp = "Usage: chat <message>\n";
                break;
        }
    }
    if (type == 5) type = (isTrueForm == 1) ? 5 : -1;
}

struct a 
{
    unsigned char flag;
    unsigned char version;
    unsigned char payload[0];
} __attribute__((packed));

struct b 
{
    unsigned short len;
    /* The function of data[0] is to be a pointer when filling data. */
    unsigned char data[0];
} __attribute__((packed));

void packetVer1(string &name, string &chatMsg, unsigned char *buf)
{
    uint16_t name_len    = (uint16_t)name.length();
    uint16_t chatMsg_len = (uint16_t)chatMsg.length();

    struct a *pa  = (struct a*) buf;
    struct b *pb1 = (struct b*) (buf + sizeof(struct a));
    struct b *pb2 = (struct b*) (buf + sizeof(struct a) + sizeof(struct b) + name_len);
    pa->flag = 0x01;
    pa->version = 0x01;
    pb1->len = htons(name_len);
    memcpy(pb1->data, name.c_str(), name_len);
    pb2->len = htons(chatMsg_len);
    memcpy(pb2->data, chatMsg.c_str(), chatMsg_len);
}

void packetVer2(string name, string chatMsg, char *buf)
{
    name = to_base64(name);
    chatMsg = to_base64(chatMsg);

    sprintf(buf, "\x01\x02%s\n%s\n", name.c_str(), chatMsg.c_str());
}

void oneForAll(int sockfd_udp, string name, string chatMsg, int clientIdx)
{
    int port, version;
    struct sockaddr_in cliaddr;
    socklen_t len = sizeof(cliaddr);
    unsigned char buf_1[4096];
    char buf_2[4096], buf[10];

    if (!VuserInfo[userIndex[clientIdx]].ban)
    {
        for (int i = 0 ; i < MaxConnection ; i++)
        {
            version = VtalkerInfo[i].version;
            cliaddr = VtalkerInfo[i].cliaddr;
            if      (version == 1) {
                packetVer1(name, chatMsg, buf_1);
                sendto(sockfd_udp, buf_1, 6+name.length()+chatMsg.length(), 0, (SA*) &cliaddr, len);
            }
            else if (version == 2) {
                packetVer2(name, chatMsg, buf_2);
                sendto(sockfd_udp, buf_2, strlen(buf_2), 0, (SA*) &cliaddr, len);
            }
        }
    }

    for (int i = 0 ; i < MaxConnection ; i++)
        cout << VuserInfo[userIndex[i]].black << ' ';
    cout << endl;
}

string filterMsg(int userIndex, string chatMsg)
{
    string delimiter;
    size_t pos;
    string ans;
    bool flg = false;
    for (int i = 0 ; i < filteringList.size(); i++)
    {
        ans = "";
        delimiter = filteringList[i];
        while ((pos = chatMsg.find(delimiter)) != string::npos) 
        {
            flg = true;
            ans += chatMsg.substr(0, pos);
            for (int j = 0 ; j < delimiter.length(); j++)
                ans += "*";
            chatMsg.erase(0, pos + delimiter.length());
        }
        ans += chatMsg;
        chatMsg = ans;
    }

    if (flg) VuserInfo[userIndex].black++;
    return ans;
}

void updateHistory(string fullMsg, int clientIdx)
{
    history = history + fullMsg + '\n';
    if (VuserInfo[userIndex[clientIdx]].black > 2)
    {
        VuserInfo[userIndex[clientIdx]].ban = true;
        VtalkerInfo[clientIdx].cliaddr.sin_port = htons(-1);
    }
}

void storePort(int sockfd, int port, int version, int clientIdx)
{
    struct sockaddr_in cliaddr;
    socklen_t len = sizeof(cliaddr);

    int tmpForPort;
    talkerInfo tmp(port, version);
    cliaddr.sin_family = AF_INET;
    cliaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    cliaddr.sin_port = htons(port);
    tmp.cliaddr = cliaddr;
    VtalkerInfo[clientIdx] = tmp;
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

bool checkForm(int type, int cnt, string bname, string title, string content, string comment, int qpost)
{
    if (type == 0 && cnt != 3) return false;
    if (type == 1 && cnt != 3) return false;
    if (type == 2 && cnt > 1)  return false;
    if (type == 5 && cnt > 1)  return false;
    if (type == 6 && cnt <= 2) return false;
    if (type == 8 && cnt != 2) return false;
    if (type == 9 && cnt != 2) return false;
    if (type == 10 && (bname == "" || title == "" || content == "")) return false;
    if (type == 12 && cnt != 2) return false;
    if (type == 13 && cnt != 2) return false;
    if (type == 14 && cnt != 2) return false;
    if (type == 15 && (title == "" && content == "") || qpost == -1) return false;
    if (type == 16 && (comment == "" || qpost == -1))    return false;
    if (type == 17 && cnt != 3) return false;
    if (type == 18 && cnt < 1) return false;
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
ssize_t writen(int fd, const void *vptr, size_t n)
{
    size_t      nleft;
    ssize_t     nwritten;
    const char  *ptr;

    ptr = (const char*)vptr;
    nleft = n;
    while (nleft > 0) {
        if ( (nwritten = write(fd, ptr, nleft)) <= 0) {
            if (nwritten < 0 && errno == EINTR)
                nwritten = 0;       /* and call write() again */
            else
                return(-1);         /* error */
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

static int  read_cnt;
static char *read_ptr;
static char read_buf[MAXLINE];

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
    ssize_t n, rc;
    char    c, *ptr;

    ptr = (char*)vptr;
    for (n = 1; n < maxlen; n++) {
        if ( (rc = my_read(fd, &c)) == 1) {
            *ptr++ = c;
            if (c == '\n')
                break;  /* newline is stored, like fgets() */
        } else if (rc == 0) {
            *ptr = 0;
            return(n - 1);  /* EOF, n - 1 bytes were read */
        } else
            return(-1);     /* error, errno set by read() */
    }

    *ptr = 0;   /* null terminate like fgets() */
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
    ssize_t     n;

    if ( (n = readline(fd, ptr, maxlen)) < 0)
        err_sys("readline error");
    return(n);
}

void err_sys(const char* x) 
{ 
    perror(x); 
    exit(1); 
}
