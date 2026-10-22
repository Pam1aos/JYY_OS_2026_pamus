#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>


#define MAX_PROCS 4096

typedef struct Pidinfo{
    pid_t pid, ppid;
    char comm[256];
    int first_child_index;
    int next_sibling_index;
}PidInfo;

static PidInfo pidinfos[MAX_PROCS];
static int pid_count = 0;

static int cmp_child(const void *a, const void *b){
    int tmp_a = *(const int *)a;
    int tmp_b = *(const int *)b;

    int pid_a = pidinfos[tmp_a].pid;
    int pid_b = pidinfos[tmp_b].pid;

    return pid_a - pid_b;
}


void printUsage() {
    printf("Usage:\n");
//    printf("  labyrinth --map map.txt --player id\n");
//    printf("  labyrinth -m map.txt -p id\n");
//    printf("  labyrinth --map map.txt --player id --move direction\n");
//    printf("  labyrinth --version\n");
}

static int read_comm(pid_t pid, char *buf, size_t n) {
    char path[64];
    snprintf(path, sizeof(path), "/proc/%d/comm", pid);
    FILE *f = fopen(path, "r");
    if (!f) return -1;
    if (!fgets(buf, (int)n, f)) { fclose(f); return -1; }
    buf[strcspn(buf, "\n")] = 0;
    fclose(f);
    return 0;
}

static int get_ppid_from_stat(pid_t pid, pid_t *ppid_out) {
    char path[64], line[4096];
    snprintf(path, sizeof(path), "/proc/%d/stat", pid);
    FILE *f = fopen(path, "r");
    if (!f) return -1;
    if (!fgets(line, sizeof(line), f)) { fclose(f); return -1; }
    fclose(f);

    int id, ppid;
    char comm[256], state;
    if (sscanf(line, "%d (%255[^)]) %c %d", &id, comm, &state, &ppid) != 4) return -1;
    *ppid_out = (pid_t)ppid;
    return 0;
}




static int buildPidInfo() {
/*
 *   获取当前进程和父进程的 PID 和 comm
 */

    pid_t self = getpid();
    pid_t parent = getppid();

    char self_comm[256] = "?", parent_comm[256] = "?";
    read_comm(self, self_comm, sizeof self_comm);
    read_comm(parent, parent_comm, sizeof parent_comm);

//    printf("%s(%d)\n", parent_comm, parent);

/*
 *     打开 /proc 虚拟目录，初始化遍历环境,
 *
 *     循环遍历所有目录项 + 过滤筛选兄弟进程
 */

    DIR *d = opendir("/proc");
    if (!d) { perror("opendir /proc"); return 1; }

    struct dirent *de;
    while ((de = readdir(d)) != NULL) {
        if (!isdigit((unsigned char)de->d_name[0])) continue;
        pid_t pid = (pid_t)atoi(de->d_name);

        pid_t ppid;
        if (get_ppid_from_stat(pid, &ppid) != 0) continue;
//        if (ppid != parent) continue;


        if(pid_count>= MAX_PROCS)break;

        pidinfos[pid_count].pid = pid;
        pidinfos[pid_count].ppid = ppid;
        pidinfos[pid_count].first_child_index = -1;
        pidinfos[pid_count].next_sibling_index = -1;

        char comm[256] = "?";
        if (read_comm(pid, pidinfos[pid_count].comm, sizeof(pidinfos[pid_count].comm)) != 0)
            snprintf(pidinfos[pid_count].comm, sizeof (pidinfos[pid_count].comm), "?");
        pid_count++;
//        printf("  |- %s(%d)%s\n", comm, pid, (pid == self) ? "  <== me" : "");
    }
    closedir(d);

    return 0;
}


static int find_pid_index(pid_t pid) {
    for (int i = 0; i < pid_count; i++) {
        if (pidinfos[i].pid == pid)
            return i;
    }
    return -1;
}


static int link_proc_tree(void){
    for (int i = 0; i < pid_count; ++i) {
        int parent = find_pid_index(pidinfos[i].ppid);

        if(parent<0)continue;

        pidinfos[i].next_sibling_index = pidinfos[parent].first_child_index;
        pidinfos[parent].first_child_index = i;
    }

    return 0;
}

void print_Tree(PidInfo* pidInfo,int index,const char *prefix,int is_last,int show_sp,int show_ns){
    if(index<0 || index>=pid_count)return;

    PidInfo* p = &pidInfo[index];

//    printf("%s",prefix);
//    if(is_last) {
//        printf("%s└─ %s(%d)\n");
//    }else  {
//        printf("%s├─ %s(%d)\n");
//    }
//
//
//    if (show_sp)
//        printf("%s(%d)\n", p->comm, p->pid);
//    else
//        printf("%s\n", p->comm);
    printf("%s", prefix);
    if (is_last) {
        if (show_sp) {
            printf("└─ %s(%d)\n", p->comm, p->pid);
        } else {
            printf("└─ %s\n", p->comm);
        }
    } else {
        if (show_sp) {
            printf("├─ %s(%d)\n", p->comm, p->pid);
        } else {
            printf("├─ %s\n", p->comm);
        }
    }


    char new_prefix[64];
    if(is_last){
        snprintf(new_prefix,sizeof(new_prefix),"%s    ",prefix);
    }else{
        snprintf(new_prefix,sizeof(new_prefix),"%s│   ",prefix);
    }

    if(show_ns) {
        int kids[256];
        int kid_cnt=0;
        int cur=p->first_child_index;

        while(cur!=-1 && kid_cnt<256){
            kids[kid_cnt++] = cur;
            cur = pidInfo[cur].next_sibling_index;
        }

        qsort(kids,kid_cnt,sizeof(int),cmp_child);

        for (int i = 0; i < kid_cnt; i++)
        {
            int kid_idx = kids[i];
            int kid_is_last = (i == kid_cnt - 1);
            print_Tree(pidInfo, kid_idx, new_prefix, kid_is_last, show_sp, show_ns);
        }
        /*/
         * to be continue
         */
    }else{
        int cur = p->first_child_index;
        while (cur!=-1){
            int next_child = pidInfo[cur].next_sibling_index;
            print_Tree(pidInfo,cur,new_prefix,next_child==-1,show_sp,show_ns);
            cur = next_child;
        }

    }
}

int main(int argc, char *argv[]) {

//    char* show-pids = NULL;
//    char* numeric-sort = NULL;
//    char* version =
    int show_sp = 0;
    int show_ns = 0;
    int show_version = 0;

    static struct option long_options[]={
            {"show-pids",no_argument,NULL,'p'},
            {"numeric-sort",no_argument,NULL,'n'},
            {"version",no_argument,NULL,'V'},
            {0,0,0,0}
    };

    int opt;
    while((opt = getopt_long(argc, argv, "pnV", long_options, NULL))!= -1){
        switch (opt) {
            case 'p':
                show_sp = 1;
                break;
            case 'n':
                show_ns = 1;
                break;
            case 'V':
                show_version = 1;
                break;
            case '?':
                fprintf(stderr, "Unknown option or missing argument!\n");
                printUsage();
                return 1;
        }
    }


    if (optind < argc) {
        fprintf(stderr, "错误：多余参数\n");
        return 1;
    }

    if(show_version){
        printf("pstree: 1.0.0\n");
        return 0;
    }


//    if(show_sp){
        if(buildPidInfo()!=0) return -1;
        link_proc_tree();
        print_Tree(pidinfos,find_pid_index(getpid()),"",1,show_sp,show_ns);
//    }



    return 0;
}
