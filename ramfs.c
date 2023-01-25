#include "ramfs.h"
#include <stdlib.h>
#include <stdbool.h>

#define RF (-1)
#define MAX (int)(9e8)

typedef struct node {
  enum { FILE_NODE, DIR_NODE } type;
  struct node *dirents; // if it's a dir, there's subentries | Attention: that's a Linked list which means that you should make the ptr pointing at head node
  struct node *next_sib; // everyone need, to be a tree, 2 dimensions
  void *content; // if it's a file, there's data content
  int subs; // number of subentries for dir
  int size; // size of file
  char *name; // it's short name
} Node;

typedef struct fd {
  int offset;
  int flags;
  Node *f;
} Fd;

Node root;

char board[MAX]; // TODO: check is ok...
bool status[MAX];

/**
 * 添加⽂件和⽬录，就是往树⾥添加节点；
 * 删除⽂件，就是删除节点；
 * 读取内容，就是从 content ⾥复制出⼀段...
 */

// To Start the Project, you must know that how to start a struct
// To establish a real structure
// is that difficult? no!
// first try the all define outsides

void init_ramfs() {
  root.name = "/";
  root.type = DIR_NODE;
  root.next_sib = NULL;
  root.content = NULL;
  root.dirents = NULL;
  root.size = RF;
  root.subs = 0;
}

/**
 * O_APPEND 02000 以追加模式打开⽂件。即打开后，⽂件描述符的偏移量指向⽂件的末尾。若⽆此标志，则指向⽂件的开头
 * O_CREAT 0100 如果 pathname 不存在，就创建这个⽂件，但如果这个⽬录中的⽗⽬录不存在，则创建失败；如果存在则正常打开
 * O_TRUNC 01000 如果 pathname 是⼀个存在的⽂件，并且同时以可写⽅式 (O_WRONLY/O_RDWR) 打开了⽂件，则⽂件内容被清空
 * O_RDONLY 00 以只读⽅式打开
 * O_WRONLY 01 以只写⽅式打开
 * O_RDWR 02 以可读可写⽅式打开
 */

// 打开 ramfs 中的文件。如果成功，返回一个文件描述符（一个非负整数），用于标识这个文件。
// 如果打开失败，则返回一个 -1。
int ropen(const char *pathname, int flags) {
  // TODO();
}

int rclose(int fd) {
  // TODO();
}

// 向 fd 中的偏移量位置写入以 buf 开始的至多 count 字节，覆盖文件原有的数据(如果 count 超过 buf 的大小，仍继续写入)，将 fd 的偏移量后移 count，并返回实际成功写入的字节数。如果写入的位置超过了原来的文件末尾，则自动为该文件扩容.
// 如果 fd 不是一个可写的文件描述符，或 fd 指向的是一个目录，则返回 -1
ssize_t rwrite(int fd, const void *buf, size_t count) {
  // TODO();
}

ssize_t rread(int fd, void *buf, size_t count) {
  // TODO();
}

off_t rseek(int fd, off_t offset, int whence) {
  // TODO();
}

int rmkdir(const char *pathname) {
  // TODO();
}

int rrmdir(const char *pathname) {
  // TODO();
}

int runlink(const char *pathname) {
  // TODO();
}

