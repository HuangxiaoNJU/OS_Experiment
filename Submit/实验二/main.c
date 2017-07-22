#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MAX 256
#define TRUE  1
#define FALSE 0

typedef short bool;
typedef unsigned char u8;   //1字节
typedef unsigned short u16; //2字节
typedef unsigned int u32;   //4字节

#pragma pack (1)
typedef struct {
    u16  BPB_BytsPerSec;    //每扇区字节数
    u8   BPB_SecPerClus;    //每簇扇区数
    u16  BPB_RsvdSecCnt;    //Boot记录占用的扇区数
    u8   BPB_NumFATs;       //FAT表个数
    u16  BPB_RootEntCnt;    //根目录最大文件数
    u16  BPB_TotSec16;
    u8   BPB_Media;
    u16  BPB_FATSz16;       //FAT扇区数
    u16  BPB_SecPerTrk;
    u16  BPB_NumHeads;
    u32  BPB_HiddSec;
    u32  BPB_TotSec32;      //如果BPB_FATSz16为0，该值为FAT扇区数
} BPB;

typedef struct {
    char DIR_Name[11];  //文件名
    u8   DIR_Attr;      //文件属性
    char reserved[10];
    u16  DIR_WrtTime;
    u16  DIR_WrtDate;
    u16  DIR_FstClus;   //开始簇号
    u32  DIR_FileSize;  //文件大小
} RootEntry;
#pragma pack ()

int  BytsPerSec;    //每扇区字节数
int  SecPerClus;    //每簇扇区数
int  RsvdSecCnt;    //Boot记录占用的扇区数
int  NumFATs;       //FAT表个数
int  RootEntCnt;    //根目录最大文件数
int  FATSz;         //FAT扇区数

extern void myprint(char* msg, int len);
extern void changeColor();
extern void retColor();

void readBPB(FILE* fat12);
bool fileFilter(char* fileName);
bool isDirectory(RootEntry* entry);
bool isEmptyDirectory(FILE* fat12, int startClus);
char* getRealName(RootEntry* entry);
void printChildren(FILE* fat12, char* directory, int startClus);
void printAllFiles(FILE* fat12);
void toUpperCase(char* str);
RootEntry** getRootEntries(FILE* fat12);
RootEntry** getClusEntries(FILE* fat12, int clus);
int searchDirectory(FILE* fat12, char* input);
RootEntry* searchClusFile(FILE* fat12, char* fileName, int startClus);
RootEntry* searchRootFile(FILE* fat12, char* fileName);
void listInputDirFiles(FILE* fat12, char* input);
void printInputFile(FILE* fat12, char* input);
void countDirectory(FILE* fat12, char* dirName, int clus, int level);
void printFile(FILE* fat12, RootEntry* entry);
int getFATValue(FILE* fat12, int num);


int main() {
    FILE* floppy = fopen("a.img", "rb");
    readBPB(floppy);
    // 打印所有文件
    printAllFiles(floppy);

    // 接受用户输入
    char input[MAX];
    while(TRUE) {
        memset(input, 0, sizeof(input));
        char* prompt = "Input: ";
        myprint(prompt, strlen(prompt));
        scanf("%s", input);
        if(strcmp(input, "exit") == 0) {
            break;
        }
        
        if(strcmp(input, "count") == 0 || strcmp(input, "COUNT") == 0) {
            scanf("%s", input);
            toUpperCase(input);
            char inputCopy[strlen(input)];
            strcpy(inputCopy, input);
            int clus = searchDirectory(floppy, inputCopy);
            countDirectory(floppy, input, clus, 0);
            continue;
        }
        
        toUpperCase(input);
        
        // 输入为目录
        if (input[strlen(input) - 1] == '/') {
            listInputDirFiles(floppy, input);
        }
        // 输入为文件
        else {
            printInputFile(floppy, input);
        }
        putchar('\n');
    }
    
    fclose(floppy);
    return 0;
}

/*
 * 打印输入目录下所有文件
 * 目录不存在则提示
 */
void listInputDirFiles(FILE* fat12, char* input) {
    char inputCopy[MAX];
    strcpy(inputCopy, input);
    int clus = searchDirectory(fat12, inputCopy);
    if (clus >= 2) {
        input[strlen(input) - 1] = '\0';
        printChildren(fat12, input, clus);
    } else {
        char* prompt = "Unknown Path!\n";
        myprint(prompt, strlen(prompt));
    }
}

/*
 * 打印输入文件中内容
 * 文件不存在则提示
 */
void printInputFile(FILE* fat12, char* input) {
    RootEntry* fileEntry = NULL;
    // 获取文件目录
    char* ptr1;
    for (ptr1 = input + strlen(input) - 1; ptr1 != input; ptr1--) {
        if (*ptr1 == '/') {
            break;
        }
    }
    // 非根目录下文件
    if (ptr1 != input) {
        // 获取文件目录
        char directory[MAX];
        char* ptr = directory;
        for (char* ptr2 = input; ptr2 != ptr1; *(ptr++) = *(ptr2++));
        *ptr = '\0';
        int clus = searchDirectory(fat12, directory);
        // 文件目录不存在
        if (clus < 0) {
            char* prompt = "Unknown Path!\n";
            myprint(prompt, strlen(prompt));
            free(fileEntry);
            return;
        }
        // 获取文件名
        char fileName[MAX];
        strcpy(fileName, ptr1 + 1);
        fileEntry = searchClusFile(fat12, fileName, clus);
    } else {
        fileEntry = searchRootFile(fat12, input);
    }
    // 文件不存在
    if (fileEntry == NULL) {
        char* prompt = "Unknown Path!\n";
        myprint(prompt, strlen(prompt));
        free(fileEntry);
        return;
    }
    printFile(fat12, fileEntry);
    free(fileEntry);
}

void countDirectory(FILE* fat12, char* dirName, int clus, int level) {
    if (clus < 2) {
        char* prompt = "Not a directory\n";
        myprint(prompt, strlen(prompt));
        return;
    }
    for (int i = 0; i < level; i++) {
        putchar('\t');
    }
    int file = 0, directory = 0;
    RootEntry** entries = getClusEntries(fat12, clus);
    for (RootEntry** ptr = entries; *ptr != NULL; ptr++) {
        RootEntry* entry = *ptr;
        if (!fileFilter(entry->DIR_Name)) {
            continue;
        }
        if (!isDirectory(entry)) {
            file++;
        } else {
            directory++;
        }
    }
    printf("%s: %d file, %d directory\n", dirName, file, directory);
    for (RootEntry** ptr = entries; *ptr != NULL; ptr++) {
        RootEntry* entry = *ptr;
        if (!fileFilter(entry->DIR_Name)) {
            continue;
        }
        if (isDirectory(entry)) {
            countDirectory(fat12, getRealName(entry), entry->DIR_FstClus, level + 1);
        }
    }
    free(entries);
}

/*
 * 打印文件内容
 */
void printFile(FILE* fat12, RootEntry* entry) {
    // 计算数据区偏移
    int rootDirSectors = (RootEntCnt * 32 + BytsPerSec - 1) / BytsPerSec;
    int dataStartOffset = (RsvdSecCnt + NumFATs * FATSz + rootDirSectors) * BytsPerSec;
    
    int currentClus = entry->DIR_FstClus;
    int leftSize = entry->DIR_FileSize;
    while (currentClus < 0xFF8) {
        if (currentClus == 0xFF7) {
            char* prompt = "Broken Cluster!\n";
            myprint(prompt, strlen(prompt));
            break;
        }
        char* content = (char*) calloc(SecPerClus * BytsPerSec + 1, sizeof(char*));
        int base = dataStartOffset + (currentClus - 2) * BytsPerSec;
        fseek(fat12, base, SEEK_SET);
        if (leftSize > SecPerClus * BytsPerSec) {
            fread(content, 1, SecPerClus * BytsPerSec, fat12);
            leftSize -= SecPerClus * BytsPerSec;
        } else {
            fread(content, 1, leftSize, fat12);
            leftSize = 0;
        }
        //printf("%s", content);
        myprint(content, strlen(content));
        free(content);
        currentClus = getFATValue(fat12, currentClus);
    }
}

int getFATValue(FILE* fat12, int num) {
    //FAT1的偏移字节
    int fatBase = RsvdSecCnt * BytsPerSec;
    //FAT项的偏移字节
    int fatPos = fatBase + num * 3 / 2;
    
    u16* doubleBytes = (u16*) malloc(sizeof(u16));
    fseek(fat12, fatPos, SEEK_SET);
    fread(doubleBytes, 1, 2, fat12);
    
    int value;
    if (num % 2 == 0) {
        value = *doubleBytes % 0x01000;
    } else {
        value = *doubleBytes >> 4;
    }
    free(doubleBytes);
    return value;
}

/*
 * 搜索文件（根目录内）
 */
RootEntry* searchRootFile(FILE* fat12, char* fileName) {
    RootEntry** entries = getRootEntries(fat12);
    for (RootEntry** ptr = entries; *ptr != NULL; ptr++) {
        RootEntry* entry = *ptr;
        if (isDirectory(entry)) {
            continue;
        }
        char* realName = getRealName(entry);
        if (strcmp(fileName, realName) == 0) {
            free(realName);
            return entry;
        }
        free(realName);
    }
    return NULL;
}

/*
 * 搜索文件
 */
RootEntry* searchClusFile(FILE* fat12, char* fileName, int startClus) {
    RootEntry** entries = getClusEntries(fat12, startClus);
    for (RootEntry** ptr = entries; *ptr != NULL; ptr++) {
        RootEntry* entry = *ptr;
        if (isDirectory(entry)) {
            continue;
        }
        char* realName = getRealName(entry);
        if (strcmp(fileName, realName) == 0) {
            free(realName);
            return entry;
        }
        free(realName);
    }
    return NULL;
}


/*
 * 判断输入路径是否存在
 * 若存在，返回开始簇号，若不存在，返回负值
 */
int searchDirectory(FILE* fat12, char* input) {
    int clus = -1;
    char* directory = NULL;
    directory = strtok(input, "/");
    
    RootEntry** rootEntries = getRootEntries(fat12);
    for (RootEntry** ptr = rootEntries; *ptr != NULL; ptr++) {
        RootEntry* rootEntry = *ptr;
        if (!isDirectory(rootEntry)) {
            continue;
        }
        char* realName = getRealName(rootEntry);
        if (strcmp(realName, directory) == 0) {
            clus = rootEntry->DIR_FstClus;
            free(realName);
            break;
        }
        free(realName);
    }
    free(rootEntries);
    
    if (clus < 0) {
        return -1;
    }
    
    while((directory = strtok(NULL, "/")) != NULL) {
        bool isExist = FALSE;
        RootEntry** entries = getClusEntries(fat12, clus);
        for (RootEntry** ptr = entries; *ptr != NULL; ptr++) {
            RootEntry* entry = *ptr;
            if (!isDirectory(entry)) {
                continue;
            }
            char* realName = getRealName(entry);
            if (strcmp(realName, directory) == 0) {
                isExist = TRUE;
                clus = entry->DIR_FstClus;
                free(realName);
                break;
            }
            free(realName);
        }
        free(entries);
        if (!isExist) {
            return -1;
        }
    }
    return clus;
}

/*
 * 小写转大写
 */
void toUpperCase(char* str) {
    for (char* ptr = str; *ptr != '\0'; ptr++) {
        if ('a' <= *ptr && *ptr <= 'z') {
            *ptr = *ptr - 'a' + 'A';
        }
    }
}

/*
 * 读取引导扇区BPB
 */
void readBPB(FILE* fat12) {
    BPB* bpb = (BPB*) malloc(sizeof(BPB));
    //BPB从偏移11个字节处开始
    fseek(fat12, 11, SEEK_SET);
    //BPB长度为25字节
    fread(bpb, 1, 25, fat12);
    BytsPerSec = bpb->BPB_BytsPerSec;
    SecPerClus = bpb->BPB_SecPerClus;
    RsvdSecCnt = bpb->BPB_RsvdSecCnt;
    NumFATs    = bpb->BPB_NumFATs;
    RootEntCnt = bpb->BPB_RootEntCnt;
    FATSz      = bpb->BPB_FATSz16;
    free(bpb);
}

/*
 * 过滤文件
 */
bool fileFilter(char* fileName) {
    int res = TRUE;
    for (int i = 0; i < 0xB; i++) {
        char c = fileName[i];
        if(('0' <= c && c <= '9') ||
           ('a' <= c && c <= 'z') ||
           ('A' <= c && c <= 'Z') ||
           c == ' ') {
            continue;
        }
        res = FALSE;
        break;
    }
    return res;
}

/*
 * 判断是否为目录
 */
bool isDirectory(RootEntry* entry) {
    return (entry->DIR_Attr & 0x10) != 0;
}

/*
 * 判断是否为空目录
 */
bool isEmptyDirectory(FILE* fat12, int startClus) {
    // 计算数据区开始偏移
    int rootDirSectors = (RootEntCnt * 32 + BytsPerSec - 1) / BytsPerSec;
    int dataStartOffset = (RsvdSecCnt + NumFATs * FATSz + rootDirSectors) * BytsPerSec;
    int base = dataStartOffset + (startClus - 2) * BytsPerSec;
    RootEntry* entry = (RootEntry*) malloc(sizeof(RootEntry));
    for (int i = 0; i < BytsPerSec / 32; i++) {
        fseek(fat12, base, SEEK_SET);
        fread(entry, 1, 32, fat12);
        base += 32;
        if(fileFilter(entry->DIR_Name)) {
            return FALSE;
        }
    }
    return TRUE;
}

/*
 * 获取文件／目录名
 */
char* getRealName(RootEntry* entry) {
    char* realName = (char*)calloc(MAX, sizeof(char*));
    memset(realName, '\0', sizeof(*realName));
    char* ptr = realName;
    for (int i = 0; i < 8 && entry->DIR_Name[i] != ' '; i++) {
        *(ptr++) = entry->DIR_Name[i];
    }
    if (isDirectory(entry)) {
        return realName;
    }
    *(ptr++) = '.';
    for (int i = 8; i < 11 && entry->DIR_Name[i] != ' '; i++) {
        *(ptr++) = entry->DIR_Name[i];
    }
    return realName;
}

/*
 * 获取某簇中所有条目
 */
RootEntry** getClusEntries(FILE* fat12, int clus) {
    RootEntry** entries = (RootEntry**)calloc(BytsPerSec / 32, sizeof(RootEntry*));
    RootEntry** ptr = entries;
    // 计算簇号开始偏移
    int rootDirSectors = (RootEntCnt * 32 + BytsPerSec - 1) / BytsPerSec;
    int dataStartOffset = (RsvdSecCnt + NumFATs * FATSz + rootDirSectors) * BytsPerSec;
    int base = dataStartOffset + (clus - 2) * BytsPerSec;
    for (int i = 0; i < BytsPerSec / 32; i++) {
        RootEntry* entry = (RootEntry*) malloc(sizeof(RootEntry));
        fseek(fat12, base, SEEK_SET);
        fread(entry, 1, 32, fat12);
        base += 32;
        // 过滤文件
        if(!fileFilter(entry->DIR_Name)) {
            free(entry);
            continue;
        }
        *(ptr++) = entry;
    }
    return entries;
}

/*
 * 打印子目录所有文件
 */
void printChildren(FILE* fat12, char* directory, int startClus) {
    RootEntry** entries = getClusEntries(fat12, startClus);
    for (RootEntry** ptr = entries; *ptr != NULL; ptr++) {
        RootEntry* entry = *ptr;
        char* temp = (char*)calloc(MAX, sizeof(char*));
        strcpy(temp, directory);
        strcat(directory, "/");
        //strcat(directory, getRealName(entry));
        char* realName = getRealName(entry);
        if (isDirectory(entry)) {
            if (isEmptyDirectory(fat12, entry->DIR_FstClus)) {
                changeColor();
                myprint(directory, strlen(directory));
                myprint(realName, strlen(realName));
                putchar('\n');
                retColor();
            } else {
                printChildren(fat12, strcat(directory, realName), entry->DIR_FstClus);
            }
        } else {
            changeColor();
            myprint(directory, strlen(directory));
            retColor();
            myprint(realName, strlen(realName));
            putchar('\n');
            free(realName);
            return;
        }
        free(realName);
        strcpy(directory, temp);
        free(temp);
    }
    free(entries);
}

/*
 * 获取根目录中所有条目
 */
RootEntry** getRootEntries(FILE* fat12) {
    RootEntry** rootEntries = (RootEntry**)calloc(RootEntCnt, sizeof(RootEntry*));
    RootEntry** ptr = rootEntries;
    int base = (RsvdSecCnt + NumFATs * FATSz) * BytsPerSec;
    for (int i = 0; i < RootEntCnt; i++) {
        RootEntry* rootEntry = (RootEntry*) malloc(sizeof(RootEntry));
        fseek(fat12, base, SEEK_SET);
        fread(rootEntry, 1, 32, fat12);
        base += 32;
        // 过滤文件
        if(!fileFilter(rootEntry->DIR_Name)) {
            free(rootEntry);
            continue;
        }
        *(ptr++) = rootEntry;
    }
    return rootEntries;
}

/*
 * 打印FAT12软盘中所有文件
 */
void printAllFiles(FILE* fat12) {
    RootEntry** rootEntries = getRootEntries(fat12);
    for (RootEntry** ptr = rootEntries; *ptr != NULL; ptr++) {
        RootEntry* rootEntry = *ptr;
        char* fileName = getRealName(rootEntry);
        if (isDirectory(rootEntry)) {
            // 空目录直接打印
            if(isEmptyDirectory(fat12, rootEntry->DIR_FstClus)) {
                changeColor();
                myprint(fileName, strlen(fileName));
                retColor();
                putchar('\n');
            }
            // 非空目录打印子目录文件
            else {
                printChildren(fat12, fileName, rootEntry->DIR_FstClus);
            }
        } else {
            myprint(fileName, strlen(fileName));
            putchar('\n');
        }
        free(fileName);
    }
    free(rootEntries);
}
