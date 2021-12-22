#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <pwd.h>

#define MAX_COMMAND_SIZE 10
#define MAX_COMMAND_LENGTH 100
#define BUFFSIZE 1024
#define OUT 0
#define IN 1
#define MAX_LEN 20

// 进程链表
typedef struct ps_info
{
    char pname[MAX_LEN];    //  进程名称
    char user[MAX_LEN];     // 所有者
    int  pid;               // PID
    struct ps_info *next;   // 指向下一个进程链表
}mps;

char buffer[BUFFSIZE] = {0};    // 用户输入
char bufCpy[BUFFSIZE] = {0};    // 复制一份，用来进行其他操作
char *arglist[MAX_COMMAND_SIZE] = {0};    // 指令列表
int args_num = 0;           // 指令数量
char curPath[BUFFSIZE];     // 当前的路径
char command[MAX_COMMAND_SIZE][MAX_COMMAND_LENGTH];      // 最多10个指令，每条指令最长100个字符
int commandNum;                                 // 已经输入指令数目
char history[MAX_COMMAND_SIZE][BUFFSIZE];                // 存放历史命令
extern char ** environ;

void get_user_input(char buf[]);

void parse(char buf[]);

void do_cmd(int argc, char *argv[]);

int callcd(int argc);

int callEcho();

void callHelp();

int callJobs();

void callEnviron();

int printHistory(char command[MAX_COMMAND_SIZE][MAX_COMMAND_LENGTH]);

mps *trav_dir(char dir[]);
int read_info(char d_name[],struct ps_info *p1);
void uid_to_name(uid_t uid, struct ps_info *p1);
int is_num(char p_name[]);
void print_ps(struct ps_info *head);

/**
 * 主函数，程序入口
 * @return
 */
int main() {
    while (1) {
        getcwd(curPath, BUFFSIZE);
        printf("[myShell %s]$", curPath);
        get_user_input(buffer);
        parse(buffer);
        strcpy(history[commandNum++], buffer);
        do_cmd(args_num, arglist);

    }
    return 0;
}

/**
 * 得到用户输入
 * @param buf
 */
void get_user_input(char buf[]) {
    memset(buf, 0x00, BUFFSIZE);
    memset(bufCpy, 0x00, BUFFSIZE);
    fgets(buf, BUFFSIZE, stdin);
    buf[strlen(buf) - 1] = '\0';
}
/**
 * 解析指令
 * @param buf
 */
void parse(char buf[]) {
    int i, j;
    for (i = 0; i < MAX_COMMAND_SIZE; i++) {
        arglist[i] = NULL;
        for (j = 0; j < MAX_COMMAND_LENGTH; j++)
            command[i][j] = '\0';
    }
    args_num = 0;

    // 设置command的指令
    int len = strlen(buf);
    strcpy(bufCpy, buf);

    for (i = 0, j = 0; i < len; ++i) {
        if (buf[i] != ' ') {
            command[args_num][j++] = buf[i];
        } else {
            if (j != 0) {
                command[args_num][j] = '\0';
                ++args_num;
                j = 0;
            }
        }
    }
    if (j != 0) {
        command[args_num][j] = '\0';
    }

    // 设置arglist的指令
    args_num = 0;
    int flg = OUT;
    for (i = 0; buf[i] != '\0'; i++) {
        if (flg == OUT && !isspace(buf[i])) {
            flg = IN;
            arglist[args_num++] = buf + i;
        } else if (flg == IN && isspace(buf[i])) {
            flg = OUT;
            buf[i] = '\0';
        }
    }
    arglist[args_num] = NULL;

}

/**
 * 执行指令
 * @param argc
 * @param argv
 */
void do_cmd(int argc, char *argv[]) {
    // cd指令
    if (strcmp(argv[0], "cd") == 0) {
        // do cd
        int res = callcd(argc);
        if (!res) {
            printf("cd指令输入错误!\n");
        }
    } else if (strcmp(argv[0], "echo") == 0) {
        // do echo
        callEcho();
    } else if (strcmp(argv[0], "environ") == 0) {
        // do environ
        callEnviron();
    } else if (strcmp(argv[0], "history") == 0) {
        // do history
        printHistory(command);
    } else if (strcmp(argv[0], "jobs") == 0) {
        // do jobs
        callJobs();
    } else if (strcmp(argv[0], "help") == 0 || strcmp(argv[0], "!") == 0) {
        // do help
        callHelp();
    } else if (strcmp(argv[0], "quit") == 0 || strcmp(argv[0], "exit") == 0 || strcmp(argv[0], "bye") == 0) {
        // do exit
        printf("bey!\n");
        exit(0);
    } else {
        int pid = fork();

        if (pid == -1) {
            printf("创建子进程失败\n");
            return;
        } else if (pid == 0) {
            execvp(argv[0], argv);

            printf("%s: 命令输入错误\n", argv[0]);
            exit(1);
        } else {
            int status;
            waitpid(pid, &status, 0);
            int err = WEXITSTATUS(status);

            if(err) {
                printf("ERROR: %s\n", strerror(err));
            }
        }
    }
}

/**
 * 执行CD指令
 * @param argc 指令数目
 * @return
 */
int callcd(int argc) {
    int result = 1;

    if (argc ==  1) {
        char *res = getcwd(curPath, BUFFSIZE);
        printf("当前目录为: %s\n", res);
    } else if (argc == 2) {
        int ret = chdir(command[1]);

        if (ret == -1) {
            result = 0;
        }
    } else {
        printf("指令数目错误！\n");
    }

    if (result) {
        char* res = getcwd(curPath, BUFFSIZE);
        if (res == NULL) {
            printf("文件路径不存在!");
        }
        return result;
    }
    return 0;
}

/**
 * 执行echo指令
 * @return
 */
int callEcho() {
    char *p = strchr(bufCpy, 'o');
    p++;
    while (*p != '\0') {
        p++;
        if (*p != ' ') break;
    }

    if (*p != '\0') {
        printf("%s\n", p);
    }

    return 1;
}

/**
 * print the help for user.
 */
void callHelp() {
    printf("command help:\n");
    printf("cd: cd+路径名称\n");
    printf("echo: echo+字符串\n");
    printf("history: history+num. the command will print the history of num that you cin.\n");
    printf("environ: if you enter it , then it will print the environment var.\n");
    printf("jobs: print the process.\n");
    printf("help: print the help.\n");
    printf("exit bye quit: leave the program running.\n");
    printf("\n");
}

/**
 * 执行jobs指令，打印进程信息
 * @return
 */
int callJobs() {
    mps *head,*link;

    head=trav_dir("/proc/");
    if(head==NULL)
        printf("traverse dir error\n");
    print_ps(head);

    while(head!=NULL)        //释放链表
    {
        link=head;
        head=head->next;
        free(link);
    }

    // 回到之前的目录
    chdir(curPath);

    return 0;
}

/**
 * 遍历 ”/proc/“ 目录
 * @param dir
 * @return
 */
mps *trav_dir(char dir[]) {
    DIR *dir_p;
    mps *head, *p1, *p2;
    struct dirent *dirent_p;
    struct stat infoBuf;

    // 打开文件失败
    if ((dir_p = opendir(dir)) == NULL) {
        printf("open the directory %s error\n", dir);
    } else {    // 成功打开
        head = p1 = p2 = (struct ps_info *) malloc(sizeof(struct ps_info)); // 分配内存

        // 遍历目录
        while ((dirent_p = readdir(dir_p)) != NULL) {
            if ((is_num(dirent_p->d_name)) == 0) { // 里面的进程名称都是数字
                if (p1 == NULL) {   // 内存分配错误，错误 退出程序
                    printf("内存分配错误\n");
                    exit(0);
                }

                if (read_info(dirent_p->d_name, p1) != 0) {
                    printf("read_info error!\n");
                    exit(0);
                }

                p2->next = p1;      // 插入新节点
                p2 = p1;
                p1 = (struct ps_info*) malloc(sizeof(struct ps_info));
            }
        }
    }

    p2->next = NULL;

    return head;    // 返回头结点
}

/**
 * 判断是否是数字
 * @param dir
 * @return 0 is a num, -1 is not a num.
 */
int is_num(char dir[]) {
    int i, len;
    len = strlen(dir);

    if (len == 0) { //长度为0，错误
        return -1;
    }
    for (i = 0; i < len; i++) {
        if (dir[i] < '0' || dir[i] > '9') { // 不是数字
            return -1;
        }
    }

    return 0;
}

/**
 * 读出信息，并写入到p1中
 * @param d_name
 * @param p1
 * @return
 */
int read_info(char d_name[], struct ps_info *p1) {
    FILE *fd;
    char dir[MAX_LEN];
    struct stat infoBuf;

    // dir = /proc/d_name;
    sprintf(dir, "%s/%s", "/proc/", d_name);

    chdir("/proc"); // 进入/proc目录

    if (stat(d_name, &infoBuf) == -1) { // 获取文件的详细信息
        fprintf(stderr, "获取文件信息失败！\n");
    } else {
        uid_to_name(infoBuf.st_uid, p1);
    }

    chdir(dir);
    if ((fd = fopen("stat", "r")) == NULL) {
        printf("打开文件失败!\n");
        exit(0);
    }

    while (2 == fscanf(fd, "%d %s\n", &(p1->pid), p1->pname)) {
        break;
    }

    fclose(fd);

    return 0;
}

/**
 * 通过getpwuid来得知所有关于该uid的相关信息
 * struct passwd ：
 * @param uid
 * @param p1
 */
void uid_to_name(uid_t uid, struct ps_info *p1) {
    struct passwd *getpwuid(), *pw_p;
    static char numstr[10];

    if ((pw_p = getpwuid(uid)) == NULL) {
        sprintf(numstr, "%d", uid);
        strcpy(p1->user, numstr);
    } else {
        strcpy(p1->user, pw_p->pw_name);
    }
}

/**
 * 输出各进程的信息
 * @param head
 */
void print_ps(struct ps_info *head) {
    mps *p;

    printf("PID\t\tUSER\t\tPNAME\n");
    for (p = head; p != NULL; p = p->next) {
        printf("%d\t\t%s\t\t%s\n", p->pid, p->user, p->pname);
    }
}


void callEnviron() {
    char **envir = environ;
    while (*envir) {
        fprintf(stdout, "%s\n", *envir);
        envir++;
    }
}

int printHistory(char command[MAX_COMMAND_SIZE][MAX_COMMAND_LENGTH]) {
    int n = atoi(command[1]);   // 将数目从字符串转为int
    int i;
    for (i = n; i > 0 && commandNum - i >= 0; i--) {
        printf("%d\t%s\n", n - i + 1, history[commandNum - i]);
    }
    return 0;
}