#include "ramfs.h"
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <stdio.h>

#define RF (-1)
//#define MAX (int)(9e8)
#define FILE_NUM 65539
#define FILE_NAME_LEN 32
#define PATH_LEN 1024
#define eq(A, B) (strcmp(A,B) == 0)
#define NameFindFile NFF
#define FdFindIndex FFI
#define CheckFd(i) (i >= 0 && i < FILE_NUM && fd_status[i])
#define CheckR(num) (!(num & O_WRONLY))
#define CheckW(num) (num & O_WRONLY || num & O_RDWR)
//#define R (-2)
//#define W (-4)
//#define RW (-8)

typedef struct node {
  enum { FILE_NODE, DIR_NODE } type;
  struct node *dirents; // if it's a dir, there's subentries | Attention: that's a Linked list which means that you should make the ptr pointing at head node
  struct node *next_sib; // everyone need, to be a tree, 2 dimensions
  void *content; // if it's a file, there's data content ; and it's actually a ptr, pt at the board.
//  int subs; // number of subentries for dir
  int size; // size of file, how many bytes in the file
  char *name; // it's short name
} Node;

typedef struct fd {
  int offset; // for file, offset
  int flags; // for file, open way
  Node *f;
} Fd;

Node root;

//char board[MAX] = {0}; //ok? maybe
//bool status[MAX] = {0};

Fd fds[FILE_NUM]; // handle, you just go through it until found it able, if too slow then optimize
Node nodes[FILE_NUM]; // the real node, where the real contents lie
bool fd_status[FILE_NUM] = {0};
bool node_status[FILE_NUM] = {false};
// whether is permitted to use a new-created function, JUST in this file! (*)
// file-tree, handle, big file

/**
 * 添加⽂件和⽬录，就是往树⾥添加节点；
 * 删除⽂件，就是删除节点；
 * 读取内容，就是从 content ⾥复制出⼀段...
 */

// To Start the Project, you must know that how to start a struct
// To establish a real structure
// is that difficult? no!
// first try the all define outsides
bool CheckName(const char *the_name) {
  int len = (int)strlen(the_name);// only {c  1  .}
  if (len > FILE_NAME_LEN + 1) return false;
  for (int i = 0; i < len - 1; i++)
    if (!(isalnum(the_name[i]) || the_name[i] == '.'))
      return false;

  return true;
}

Node *NameFindFile (const Node *father, const char *name) {
  assert(father != NULL);

  Node *to_test = father->dirents;
  while (to_test != NULL && !eq(to_test->name, name)) {
    to_test = to_test->next_sib;
  }
  return to_test;
}

int FdFindIndex(Fd *ptr) { // remain this func
  int i = 0;
  for (;fd_status[i] != 0; i++);

  assert(i < FILE_NUM);
  fd_status[i] = true; // make sure not to cover the useful data

  fds[i].f = ptr->f;

  if (ptr->f->type == FILE_NODE) { // NO waiting here, do what it exactly needs to be, 2 special cases
    fds[i].flags = ptr->flags;
    fds[i].offset = ptr->offset;
  }
  if (fds[i].flags & O_TRUNC && CheckW(fds[i].flags)) {
    sprintf(fds[i].f->content, "%s", "\0"); // maybe problematic, because after contents remain alive
  }
  return i;
}

int NodeFindIndex() {
  int i = 0;
  for (;node_status[i] != 0; i++);
  node_status[i] = true;
  return i;
}

Node *touch(char *path) {
  Node *father = &root;

  char *s = path;
  s = strtok(path, "/");

  for (; s != NULL; s = strtok(NULL, "/")) {
    Node *tmp = NFF(father, s); // in this loop, there must be contents
    if (tmp == NULL) {
      if (strtok(NULL, "/") == NULL && CheckName(s)) {
        int idx = NodeFindIndex();
        nodes[idx].type = FILE_NODE;
        nodes[idx].dirents = NULL;
        nodes[idx].name = s;
        nodes[idx].content = NULL;

        nodes[idx].next_sib = father->dirents; // Head Insert
        father->dirents = nodes + idx;
        return nodes + idx;
      }
      return NULL;
    }
    father = tmp;
  }

  return father; // in case there already exists
}

Node *trans(char *path) { // thinking about it, maybe problem? find the truth! persist! GanBaDie!
  Node *father = &root;

  char *s = path; // nothing matters
  s = strtok(path, "/");

  for (; s != NULL; s = strtok(NULL, "/")) {
    father = NFF(father, s); // in this loop, there must be contents
    if (father == NULL) return NULL;
    //printf("%s\n", s);  operate the s
}

  return father;
}

void init_ramfs() { // do anything you wanna do here
  root.name = "/";
  root.type = DIR_NODE;
  root.next_sib = NULL;
//  root.content = RF;
  root.dirents = NULL;
//  root.size = RF;
//  root.subs = 0;
}

/**
 * O_APPEND 02000 以追加模式打开⽂件。即打开后，⽂件描述符的偏移量指向⽂件的末尾。若⽆此标志，则指向⽂件的开头
 * O_CREAT 0100 如果 pathname 不存在，就创建这个⽂件，但如果这个⽬录中的⽗⽬录不存在，则创建失败；如果存在则正常打开
 * O_TRUNC 01000 如果 pathname 是⼀个存在的⽂件，并且同时以可写⽅式 (O_WRONLY/O_RDWR) 打开了⽂件，则⽂件内容被清空
 * O_RDONLY 00 以只读⽅式打开
 * O_WRONLY 01 以只写⽅式打开
 * O_RDWR 02 以可读可写⽅式打开
 */

// concerning the handle : you can create an ARRAY !
// 同时存在的⽂件与⽬录不会超过 65536 个。
// 同时活跃着的⽂件描述符 (FD = 114514) 不会超过 4096 个。
// 打开 ramfs 中的文件。如果成功，返回一个文件描述符（一个非负整数），用于标识这个文件。
// 如果打开失败，则返回一个 -1。
int ropen(const char *pathname, int flags) {
  // 打开 ramfs 中already exist的文件。如果成功，返回一个文件描述符（**一个非负整数**），用于标识这个文件。
  // 如果打开失败，则返回一个 -1。
  if (strlen(pathname) > PATH_LEN + 1) return RF;

  struct node *open = trans((char *)pathname);
  if ((flags & O_CREAT) == 0 && open == NULL) return RF;
  else {
    if (flags & O_CREAT) {
      open = touch((char *)pathname);
      if (open == NULL) return RF; // need to create a new file, this is a REAL create
    }
    Fd fd;
    fd.f = open;
    if (open->type == FILE_NODE) {
      // write sth here, I find that to be pure is tough.
      fd.flags = flags;
      // define how to open this file, made, jie xi flags, Fuck
      // here you need to really think about that how to control the memory
      // here you need to know how to clear the data, one segment by one, catch head and just cover it with 0, without too many thoughts
      // if touch the tail of the content, status == 1, board == RF
      if ((flags & O_APPEND) && (open->content)) {
        fd.offset = fd.f->size; // pointing at the end of the file, will read nothing
      } else {
        fd.offset = 0;
      }
    }
    return FFI(&fd);
  }
}

int rclose(int fd) {
  if (!CheckFd(fd)) return RF;
  else {
    fd_status[fd] = false; // not to clear the content
    return 0;
  }
}

// 向 fd 中的*偏移量位置*写入以 buf 开始的至多 count 字节，覆盖文件原有的数据(如果 count 超过 buf 的大小，仍继续写入)，
// 将 fd 的偏移量后移 count，并返回实际成功写入的字节数(count)。
// 如果写入的位置超过了原来的文件末尾，则自动为该文件扩容.
// 如果 fd 不是一个可写的文件描述符，或 fd 指向的是一个目录，则返回 -1
ssize_t rwrite(int fd, const void *buf, size_t count) {
  if (!CheckFd(fd) || fds[fd].f->type == DIR_NODE || !CheckW(fds[fd].flags)) return RF;

  if (fds[fd].f->content == NULL) { // which means it's a null file
//    size_t size = count; // ?
    void *ctt = malloc(count);
    fds[fd].f->content = ctt;
    memcpy(ctt, buf, count); // actually if is a num array, and buf < count, then transboundary! HOPE NOT HAPPEN
  } else if () {

  }
  //todo : size may enhance

  fds[fd].offset += (int)count; // offset moves
  return (ssize_t)count; // always so
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