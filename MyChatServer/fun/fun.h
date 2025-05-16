#ifndef FUN_H
#define FUN_H


int set_nonblocking(int fd);
void add_fd(int epollfd, int fd, bool one_shot, void* ptr = nullptr);
void remove_fd(int epollfd, int fd);
void mod_fd(int epollfd, int fd, int ev);

#endif // FUN_H