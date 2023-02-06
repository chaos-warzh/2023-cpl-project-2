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

typedef struct node {
  enum { FILE_NODE, DIR_NODE } type;
  struct node *dirents; // if it's a dir, there's subentries | Attention: that's a Linked list which means that you should make the ptr pointing at head node
  struct node *next_sib; // everyone need, to be a tree, 2 dimensions
  void *content; // if it's a file, there's data content ; and it's actually a ptr, pt at the board.
//  int subs; // number of subentries for dir
  size_t size; // size of file, how many bytes in the file
  char *name; // it's short name
} Node;

typedef struct fd {
  int offset; // for file, offset
  int flags; // for file, open way
  Node *f;
} Fd;

Node root;

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
  if (strlen(the_name) > FILE_NAME_LEN) return false;
  for (int i = 0; i < len; i++)
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
    free(fds[i].f->content);
    fds[i].f->content = NULL;
    fds[i].f->size = 0;
    // maybe problematic, because after contents remain alive
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
  if (path[0] != '/') return NULL;
  Node *father = &root;

  char pth[PATH_LEN + 5];
  sprintf(pth, "%s", path);

  char *s = pth;
  s = strtok(pth, "/");

  for (; s != NULL; s = strtok(NULL, "/")) {
    Node *tmp = NFF(father, s); // in this loop, there must be contents

    if (tmp == NULL) {
      if (strtok(NULL, "/") == NULL && CheckName(s) /**&& father->type == DIR_NODE*/) {

        int idx = NodeFindIndex();
        nodes[idx].type = FILE_NODE;
        nodes[idx].dirents = NULL;
        char *new_name = malloc(strlen(s) + 5); // to free

        sprintf(new_name, "%s", s);
        nodes[idx].name = new_name;
        nodes[idx].content = NULL;
        nodes[idx].size = 0;//

        nodes[idx].next_sib = father->dirents; // Head Insert
        father->dirents = nodes + idx;
        return nodes + idx;
      }
      return NULL;
    }
    father = tmp;
  }

  return NULL; // in case there already exists, return NULL, must restrict new it
}

Node *trans(char *path) { // thinking about it, maybe problem? find the truth! persist! GanBaDie!
  if (path[0] != '/') return NULL;
  Node *father = &root;

  char pth[PATH_LEN + 5];
  sprintf(pth, "%s", path);

  char *s = pth; // nothing matters
  s = strtok(pth, "/");

  for (; s != NULL; s = strtok(NULL, "/")) {
    father = NFF(father, s); // in this loop, there must be contents
    if (father == NULL) return NULL;
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
}

// concerning the handle : you can create an ARRAY !
// 同时存在的⽂件与⽬录不会超过 65536 个。
// 同时活跃着的⽂件描述符 (FD = 114514) 不会超过 4096 个。
// 打开 ramfs 中的文件。如果成功，返回一个文件描述符（一个非负整数），用于标识这个文件。
// 如果打开失败，则返回一个 -1。
int ropen(const char *pathname, int flags) {
  // 打开 ramfs 中already exist的文件。如果成功，返回一个文件描述符（**一个非负整数**），用于标识这个文件。
  // 如果打开失败，则返回一个 -1。
  if (strlen(pathname) > PATH_LEN) return RF;

  struct node *open = trans((char *)pathname);
  if (!(flags & O_CREAT && CheckW(flags)) && open == NULL) return RF;
  else {
    if (flags & O_CREAT && CheckW(flags)) {
      open = touch((char *)pathname);
      if (open == NULL) return RF; // need to create a new file, this is a REAL create
    }
    Fd fd;
    fd.f = open;
    if (open->type == FILE_NODE) {
      fd.flags = flags;
      // define how to open this file, made, jie xi flags, Fuck
      // here you need to really think about that how to control the memory
      if ((flags & O_APPEND) && (open->content)) {
        fd.offset = (int)fd.f->size; // pointing at the end of the file(EOF), will read nothing
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
  if (!CheckFd(fd) || fds[fd].f->type == DIR_NODE || !CheckW(fds[fd].flags)) return RF; // exist, is file, writable
  assert(fds[fd].offset >= 0);

  int ofst = fds[fd].offset; // maybe enhance, maybe not
  size_t sze = fds[fd].f->size;
  if (ofst + count <= sze) { // no need to expand
    memcpy((fds[fd].f->content + ofst), buf, count);
  } else {
    void *ctt = fds[fd].f->content;
    ctt = realloc(ctt, ofst + count);
    fds[fd].f->size = (int)(ofst + count);
    if (ofst > sze) memset(ctt + sze, 0, ofst - sze); // possess the middle to make them "\0" on your hands???
    memcpy(ctt + ofst, buf, count);
    fds[fd].f->content = ctt;
  }

  fds[fd].offset += (int)count; // offset moves
  return (ssize_t)count; // always so
}


ssize_t rread(int fd, void *buf, size_t count) {
  if (!CheckFd(fd) || fds[fd].f->type == DIR_NODE || !CheckR(fds[fd].flags)) return RF; // exist(once opened), is file, readable
  assert(fds[fd].offset >= 0); // normal offset

  int ofst = fds[fd].offset; // maybe enhance, maybe not
  size_t sze = fds[fd].f->size;
  if (ofst >= sze) return 0;
  else {
    size_t readable_bytes = sze - ofst;
    size_t true_read = readable_bytes > count ? count : readable_bytes;

    memcpy(buf, fds[fd].f->content + ofst, true_read);
    fds[fd].offset += (int)true_read;
    return (ssize_t)true_read;
  }

}

/** SEEK_SET 0   将文件描述符的偏移量设置到 offset 指向的位置
*   SEEK_CUR 1   将文件描述符的偏移量设置到 当前位置 + offset 字节的位置
*   SEEK_END 2   将文件描述符的偏移量设置到 文件末尾 + offset 字节的位置
**/
off_t rseek(int fd, off_t offset, int whence) {
  if (!CheckFd(fd) || fds[fd].f->type == DIR_NODE) return RF; // exist(once opened), is file
  if (whence != SEEK_SET && whence != SEEK_CUR && whence != SEEK_END) return RF; // I add it

  int ofst = fds[fd].offset;
  int final_set = 0;

  if (whence == SEEK_CUR) final_set = ofst;
  if (whence == SEEK_END) final_set = (int)fds[fd].f->size;
  final_set += (int)offset;
  if (final_set < 0) return RF;
  else {
    fds[fd].offset = final_set;
    return final_set;
  }
}

int rmkdir(const char *pathname) { // make it simple, for large amount of use
  if (strlen(pathname) > PATH_LEN) return RF;

  Node *dir_node = touch((char *)pathname);
  if (dir_node == NULL) return RF;
  dir_node->type = DIR_NODE;
  return 0;
}
/**
 * what should you do here
 * 1: free (name)
 * 2: get rid of the nodes[]
 * 3: make his sibs / father forget him
 * from low to high.
 */

void Rm(const char *pathname, int type) {
  // do the 3 things
  Node *father = &root;

  char pth[PATH_LEN + 5];
  sprintf(pth, "%s", pathname);

  char *s = pth;
  s = strtok(pth, "/");

  Node *record_father = NULL;
  for (; s != NULL; s = strtok(NULL, "/")) {
    record_father = father;
    father = NFF(father, s);
  }
  // remove it from family
  assert(record_father != NULL);
  Node *elder_son = record_father->dirents;
  if (elder_son == father)
    record_father->dirents = father->next_sib;
  else
    for (Node *iter = elder_son; iter != NULL; iter = iter->next_sib)
      if (iter->next_sib == father)
        iter->next_sib = father->next_sib;
  // get rid of the nodes[]
  int ind = (int)(father - nodes);
  node_status[ind] = false;
  // free ctts and name
  free(father->name);
  if (type == FILE_NODE)
    free(father->content);
}

int rrmdir(const char *pathname) {
  Node *dir = trans((char *)pathname);
  if (dir == NULL || dir->type != DIR_NODE || dir->dirents != NULL || strcmp(dir->name, "/") == 0) return RF; // not exist, not empty, not a dir
  Rm(pathname, DIR_NODE);//  how to remove it ? first you must find his father, sibs, then kill it from nodes[]//
  return 0;
}

int runlink(const char *pathname) {
  Node *file = trans((char *)pathname);
  if (file == NULL || file->type != FILE_NODE) return RF;// not exist, not a dir
  Rm(pathname, FILE_NODE);// make his father and sibs give it up
  return 0;
}