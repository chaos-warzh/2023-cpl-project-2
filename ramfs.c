#include "ramfs.h"
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

#define RF (-1)
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
  struct node *dirents; // if it's a dir, there's subentries
  struct node *next_sib; // everyone need, to be a tree, 2 dimensions
  void *content; // if it's a file, there's data content
  size_t size; // size of file, how many bytes in the file
  char *name;
} Node;

typedef struct fd {
  int offset;
  int flags;
  Node *f;
} Fd;

Node root;

Fd fds[FILE_NUM]; // handle, if too slow then optimize
Node nodes[FILE_NUM]; // the real node
bool fd_status[FILE_NUM] = {0};
bool node_status[FILE_NUM] = {false};

bool CheckName(const char *the_name) {
  int len = (int)strlen(the_name);// only {c  1  .}
  if (strlen(the_name) > FILE_NAME_LEN) return false;
  for (int i = 0; i < len; i++)
    if (!(isalnum(the_name[i]) || the_name[i] == '.'))
      return false;

  return true;
}

Node *NameFindFile (const Node *father, const char *name) {

  Node *to_test = father->dirents;
  while (to_test != NULL && !eq(to_test->name, name)) {
    to_test = to_test->next_sib;
  }
  return to_test;
}

int FdFindIndex(Fd *ptr) {
  int i = 0;
  for (;fd_status[i] != 0; i++);

  fd_status[i] = true;

  fds[i].f = ptr->f;

  if (ptr->f->type == FILE_NODE) {
    fds[i].flags = ptr->flags;
    fds[i].offset = ptr->offset;
  }
  if (fds[i].flags & O_TRUNC && CheckW(fds[i].flags)) {
    free(fds[i].f->content);
    fds[i].f->content = NULL;
    fds[i].f->size = 0;
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
    Node *tmp = NFF(father, s);

    if (tmp == NULL) {
      if (strtok(NULL, "/") == NULL && CheckName(s) && father->type == DIR_NODE) {

        int idx = NodeFindIndex();
        nodes[idx].type = FILE_NODE;
        nodes[idx].dirents = NULL;
        char *new_name = malloc(strlen(s) + 5); // to free

        sprintf(new_name, "%s", s);
        nodes[idx].name = new_name;
        nodes[idx].content = NULL;
        nodes[idx].size = 0;

        nodes[idx].next_sib = father->dirents; // Head Insert
        father->dirents = nodes + idx;
        return nodes + idx;
      }
      return NULL;
    }
    father = tmp;
  }
  return NULL;
}

Node *trans(char *path) {
  if (path[0] != '/') return NULL;
  Node *father = &root;

  char pth[PATH_LEN + 5];
  sprintf(pth, "%s", path);

  char *s = pth;
  s = strtok(pth, "/");

  for (; s != NULL; s = strtok(NULL, "/")) {
    father = NFF(father, s);
    if (father == NULL) return NULL;
  }
  return father;
}

void init_ramfs() {
  root.name = "/";
  root.type = DIR_NODE;
  root.next_sib = NULL;
  root.dirents = NULL;
}

int ropen(const char *pathname, int flags) {
  if (strlen(pathname) > PATH_LEN) return RF;
  struct node *open = trans((char *)pathname);
  if (!(flags & O_CREAT) && open == NULL) return RF;
  else {
    if (flags & O_CREAT && open == NULL) {
      open = touch((char *)pathname);
      if (open == NULL) return RF;
    }
    Fd fd;
    fd.f = open;
    if (open->type == FILE_NODE) {
      fd.flags = flags;
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

ssize_t rwrite(int fd, const void *buf, size_t count) {
  if (!CheckFd(fd) || fds[fd].f->type == DIR_NODE || !CheckW(fds[fd].flags)) return RF; // exist, is file, writable

  int ofst = fds[fd].offset;
  size_t sze = fds[fd].f->size;
  if (ofst + count <= sze) { // no need to expand
    memcpy((fds[fd].f->content + ofst), buf, count);
  } else {
    void *ctt = fds[fd].f->content;
    ctt = realloc(ctt, ofst + count);
    fds[fd].f->size = (int)(ofst + count);
    if (ofst > sze) memset(ctt + sze, 0, ofst - sze); // possess the middle to make them "\0"
    memcpy(ctt + ofst, buf, count);
    fds[fd].f->content = ctt;
  }

  fds[fd].offset += (int)count;
  return (ssize_t)count; // always so
}


ssize_t rread(int fd, void *buf, size_t count) {
  if (!CheckFd(fd) || fds[fd].f->type == DIR_NODE || !CheckR(fds[fd].flags)) return RF; // once opened, is file, readable

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

int rmkdir(const char *pathname) {
  if (strlen(pathname) > PATH_LEN) return RF;

  Node *dir_node = touch((char *)pathname);
  if (dir_node == NULL) return RF;
  dir_node->type = DIR_NODE;
  return 0;
}

void Rm(const char *pathname, int type) {
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
  Rm(pathname, DIR_NODE);
  return 0;
}

int runlink(const char *pathname) {
  Node *file = trans((char *)pathname);
  if (file == NULL || file->type != FILE_NODE) return RF;// not exist, not a dir
  Rm(pathname, FILE_NODE);
  return 0;
}