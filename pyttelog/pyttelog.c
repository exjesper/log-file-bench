#define _GNU_SOURCE
#include <stdio.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <dirent.h>
#include <string.h>


struct logfile
{
  int filedesc;
  char* path;
  off_t size;
};


struct filering
{
  struct logfile* files;
  int64_t index;
  size_t size;
};

struct filering* build_ring(size_t size) {
  struct filering* r = malloc(sizeof(struct filering));
  struct logfile* files = malloc(size * sizeof(struct logfile));
  r->files = files;
  r->size = size;
  r->index = -1;
}

struct logfile get_new_file(struct filering* r, char * path) {
  struct logfile f;
  size_t index = r->index + 1;
  char* buffer = malloc(1024);
  snprintf(buffer, 1024, "%s.%lu.%lu",path, time(NULL), index);
  f.path = buffer;
  f.filedesc = open(buffer, O_WRONLY | O_CREAT, 0644);
  f.size = lseek(f.filedesc, 0, SEEK_END);
  return f;
}

struct logfile open_file(char* file) {
  struct logfile f;
  char* buffer = malloc(1024);
  strncpy(buffer, file, 1024);
  f.path = buffer;
  f.filedesc = open(buffer, O_WRONLY | O_CREAT, 0644);
  f.size = lseek(f.filedesc, 0, SEEK_END);
  close(f.filedesc);
  return f;
}

struct logfile get_current_file(struct filering* r) {
  return r->files[r->index % r->size];
}

void add_file(struct filering* r, struct logfile f) {
  size_t new_index = r->index + 1;
  size_t size = r->size;

  // Delete existing file when rolling over 
  if (new_index >= size) {
    struct logfile old_file = r->files[new_index % size];
    printf("deleting %i %s\n",new_index, old_file.path);
    unlink(old_file.path);
    free(old_file.path);
  }

  r->files[new_index % size] = f;
  r->index = new_index;
}

struct logfile new_file(struct filering* r, char * path) {
  struct logfile f = get_new_file(r, path);
  add_file(r, f);
  return f;
}


void find_old_files(struct filering* r, char * file) {
  size_t len = strlen(file);
  struct dirent **namelist;
  int n;
  n = scandir(".", &namelist, 0, alphasort);
  int i = 0;
  while (i < n) {
    size_t cur_file_len = strlen(namelist[i]->d_name);
    if (cur_file_len >= len && !strncmp(namelist[i]->d_name, file, len)) {
      add_file(r, open_file(namelist[i]->d_name));
    }
    free(namelist[i]);
    ++i;
  }
  free(namelist);
}




int main(int argc, char *argv[])
{
  if (argc < 3) {
    return -2;
  }
  
  char* lfile = argv[2];
  int ringsize = atoi(argv[1]);
  if (ringsize < 2) return -2;
  struct filering* r = build_ring(ringsize);
  find_old_files(r, lfile);
  struct logfile current_file;
  if (r->index == -1) {
    current_file = new_file(r, lfile);
  } else {
    current_file = get_current_file(r);
    current_file.filedesc = open(current_file.path, O_WRONLY | O_CREAT, 0644);
  }
  char buff[1024];
  setvbuf(stdin, buff, _IOFBF, 1024);
  const size_t READSIZE = 64000;

  int efd = epoll_create(5);
  struct epoll_event event;
  event.events = EPOLLIN;
  event.data.fd = 0;

  epoll_ctl(efd, EPOLL_CTL_ADD, 0, &event);

  ssize_t s = 0;
  uint64_t c = 0;
  uint32_t slen = 300;
  do {
    ++c;
    epoll_wait(efd, &event, 1, -1);
    s = splice(0, NULL, current_file.filedesc, NULL, READSIZE, SPLICE_F_MOVE);
    if (s > 0) {
      current_file.size += s;
      // File rotation
      if (current_file.size > 100000000) {
        struct logfile f = new_file(r, lfile);
        close(current_file.filedesc);
        current_file = f;
      }
    }
    usleep(440); 
    //printf("data %d\n", s);

    // dynamic change sleep time
    /*
    if (s < READSIZE - 10000) {
      slen = slen < 50000 ? slen + 10 : slen;
      usleep(slen);
    }
   else {
     slen = slen > 10 ? slen - 10 : slen;
   }
   */
    //if(s < 1) {
    // printf("here %d\n", s);
    // }
    // printf("%i\n", s);
    //usleep(3000);
  } while( s != 0);
  close(current_file.filedesc);
  printf("%d\n", BUFSIZ);
  return 0;
}
