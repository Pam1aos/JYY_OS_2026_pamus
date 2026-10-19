#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <testkit.h>
#include "labyrinth.h"
#include "getopt.h"


static int dr[] = {-1, 1, 0, 0};
static int dc[] = {0, 0, -1, 1};


void printUsage() {
    printf("Usage:\n");
    printf("  labyrinth --map map.txt --player id\n");
    printf("  labyrinth -m map.txt -p id\n");
    printf("  labyrinth --map map.txt --player id --move direction\n");
    printf("  labyrinth --version\n");
}

int main(int argc, char *argv[]) {
    // TODO: Implement this function
    char* map_file = NULL;
    char* player_id = NULL;
    char* move_dir = NULL;
    int show_version = 0;
    Labyrinth lab;
    static struct option long_options[]={
            {"map",required_argument,NULL,'m'},
            {"player",required_argument,NULL,'p'},
            {"move",required_argument,NULL,100},
            {"version",no_argument,NULL,101},
            {0,0,0,0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "m:p:",long_options,NULL)) != -1) {
        switch (opt) {
            case 'm':
                map_file= optarg;
                break;
            case 'p':
                player_id = optarg;
                if (strlen(optarg) != 1 || !isValidPlayer(optarg[0])){
                    fprintf(stderr, "错误：无效player ID，只能是单个数字0-9\n");
                    return 1;
                }
                break;
            case 100:
                move_dir = optarg;
                break;
            case 101:
                show_version =1;
                break;

            case '?':
                fprintf(stderr, "错误：未知选项或缺少参数！\n");
                printUsage();
                return EXIT_FAILURE;
            default:
                fprintf(stderr, "参数解析异常\n");
                return EXIT_FAILURE;
        }
    }


    if (optind < argc) {
        fprintf(stderr, "错误：多余参数\n");
        return 1;
    }

    if (show_version)
    {
        printf("Labyrinth Game\n");
        printf("%s\n", VERSION_INFO);
        return 0;
    }


    if (map_file == NULL)
    {
        fprintf(stderr, "错误：必须指定地图文件 --map / -m\n");
        printUsage();
        return 1;
    }


    if (move_dir != NULL && player_id == NULL)
    {
        fprintf(stderr, "错误：--move 必须搭配 --player 使用\n");
        printUsage();
        return 1;
    }

    if (map_file == NULL || player_id == NULL) {
        fprintf(stderr, "错误：必须同时指定 --map 和 --player\n");
        printUsage();
        return 1;
    }


    if (!loadMap(&lab, map_file))
    {
        return 1;
    }

    if (move_dir == NULL) {
        for (int i = 0; i < lab.rows; i++) printf("%s\n", lab.map[i]);
        return 0;
    }

    if (player_id != NULL && move_dir != NULL)
    {
        char pid = player_id[0];
        if (findPlayer(&lab, pid).row == -1) {
            Position p = findFirstEmptySpace(&lab);
            if (p.row == -1) {
                fprintf(stderr, "错误：地图没有空地，无法放置玩家\n");
                return 1;
            }
            lab.map[p.row][p.col] = pid;
        }

        bool ok = movePlayer(&lab, pid, move_dir);
        if (!ok)
        {
            fprintf(stderr, "移动失败：无法向%s移动\n", move_dir);
            return 1;
        }

        if (!saveMap(&lab, map_file))
        {
            fprintf(stderr, "错误：保存地图失败\n");
            return 1;
        }
    }


    return 0;
}



bool isValidPlayer(char playerId) {
    // TODO: Implement this function
//    assert();
    return playerId >= '0' && playerId <= '9';
}

bool loadMap(Labyrinth *labyrinth, const char *filename) {
    // TODO: Implement this function
    FILE* file = fopen(filename,"r");

    if(file==NULL){
        fprintf(stderr, "错误：地图文件 %s 不存在或无法打开\n", filename);
        return false;
    }

    char buf[MAX_COLS+2];
    int row=0;

    while (fgets(buf,sizeof(buf),file)!=NULL){
        int len=0;
        while(buf[len]!='\0'&&buf[len]!='\n')  len++;
        buf[len]='\0';

        if(row==0){
            labyrinth->cols=len;
        } else{
            if(len!=labyrinth->cols){
                fprintf(stderr, "错误：地图文件 %s 格式错误\n", filename);
                fclose(file);
                return false;
            }
        }

        for (int i = 0; i < labyrinth->cols; ++i) {
            char c=buf[i];
            if(c!='#'&&c!='.'&&!(c >= '0' && c <= '9')){
                fprintf(stderr, "错误：地图文件 %c 包含非法字符\n", c);
                fclose(file);
                return false;
            }

            labyrinth->map[row][i]=c;
        }
        labyrinth->map[row][labyrinth->cols] = '\0';
        row++;
    }

    labyrinth->rows = row;

    fclose(file);
    if (!isConnected(labyrinth)) return false;

    return true;
}

Position findPlayer(Labyrinth *labyrinth, char playerId) {
    // TODO: Implement this function

    int m = labyrinth->rows;
    int n= labyrinth->cols;
    for (int i = 0; i < m; ++i) {
        for(int j=0;j<n;j++){
            if (labyrinth->map[i][j] == playerId)
            {
                Position pos;
                pos.row = i;
                pos.col = j;
                return pos;
            }
        }
    }

    Position pos = {-1, -1};

    return pos;
}

Position findFirstEmptySpace(Labyrinth *labyrinth) {
    // TODO: Implement this function

    int m = labyrinth->rows;
    int n= labyrinth->cols;
    for (int i = 0; i < m; ++i) {
        for(int j=0;j<n;j++){
            if (labyrinth->map[i][j] == '.')
            {
                Position pos;
                pos.row = i;
                pos.col = j;
                return pos;
            }
        }
    }
    Position pos = {-1, -1};
    return pos;
}

bool isEmptySpace(Labyrinth *labyrinth, int row, int col) {
    // TODO: Implement this function
    if(row<0 || row>=labyrinth->rows ||col<0 ||col>=labyrinth->cols)
        return false;

    char c=labyrinth->map[row][col];
    if(c=='.')  return true;
    return false;
}



bool movePlayer(Labyrinth *labyrinth, char playerId, const char *direction) {
    // TODO: Implement this function
//    switch (direction) {
//        case 'up':
//            Position pos = findPlayer(labyrinth,playerId)
//            if(isEmptySpace(labyrinth,playerId,pos.row+1,pos.col)){
//                labyrinth->map[pos.row+1][pos.col]=playerId;
//            }
//        case 'down':
//            Position pos = findPlayer(labyrinth,playerId)
//            if(isEmptySpace(labyrinth,playerId,pos.row,pos.col-1)){
//                labyrinth->map[pos.row][pos.col-1]=playerId;
//            }
//        case 'left':
//            Position pos = findPlayer(labyrinth,playerId)
//            if(isEmptySpace(labyrinth,playerId,pos.row,pos.col-1)){
//                labyrinth->map[pos.row][pos.col-1]=playerId;
//            }
//        case 'right':
//            Position pos = findPlayer(labyrinth,playerId)
//            if(isEmptySpace(labyrinth,playerId,pos.row,pos.col+1)){
//                labyrinth->map[pos.row][pos.co+1l]=playerId;
//            }

//    }
    Position pos = findPlayer(labyrinth, playerId);
    if (pos.row == -1 ||pos.col==-1) {
        return false;
    }

    int newRow = pos.row;
    int newCol = pos.col;

    if (strcmp(direction, "up") == 0) {
        newRow = pos.row - 1;
    } else if (strcmp(direction, "down") == 0) {
        newRow = pos.row + 1;
    } else if (strcmp(direction, "left") == 0) {
        newCol = pos.col - 1;
    } else if (strcmp(direction, "right") == 0) {
        newCol = pos.col + 1;
    } else {
        return false;
    }

    if (isEmptySpace(labyrinth, newRow, newCol)) {
        labyrinth->map[pos.row][pos.col] = '.';
        labyrinth->map[newRow][newCol] = playerId;
        return true;
    }

    return false;
}

bool saveMap(Labyrinth *labyrinth, const char *filename) {
    // TODO: Implement this function
    FILE* file = fopen(filename,"w");

    if(file==NULL){
        fprintf(stderr, "错误：无法打开文件 %s 用于写入\n", filename);
        return false;
    }

    for(int i=0;i<labyrinth->rows;i++){
        fputs(labyrinth->map[i],file);
        fputc('\n',file);
    }

    fclose(file);
    return true;
}

// Check if all empty spaces are connected using DFS
void dfs(Labyrinth *labyrinth, int row, int col, bool visited[MAX_ROWS][MAX_COLS]) {
    // TODO: Implement this function


    if(row<0 || col<0 ||row>=labyrinth->rows ||col>=labyrinth->cols){
        return;
    }

    if(labyrinth->map[row][col]!='.' || visited[row][col]){
        return;
    }

    visited[row][col]=true;

    for(int i=0;i<4;i++){
        int nr = row+dr[i];
        int nc = col+dc[i];
        dfs(labyrinth, nr, nc, visited);
    }

}

bool isConnected(Labyrinth *labyrinth) {
    // TODO: Implement this function
    bool visited[MAX_ROWS][MAX_COLS]={false};

    int total = 0;
    int nr=-1,nc=-1;
    int m=labyrinth->rows,n=labyrinth->cols;
    for (int i = 0; i < m; ++i) {
        for (int j = 0; j < n; ++j) {

            if(labyrinth->map[i][j]=='.'){
                total++;
                if(nr==-1){
                    nr=i,nc=j;
                }
            }
        }
    }

    if(total==0){
        return true;
    }

    dfs(labyrinth,nr,nc,visited);
    
    int res=0;
    for (int i = 0; i < m; ++i) {
        for (int j = 0; j < n; ++j) {
            if(visited[i][j]){
                res++;
            }
        }
    }



    return total==res;
}
