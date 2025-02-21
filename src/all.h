#ifndef ALL_H
#define ALL_H

#define _GNU_SOURCE
#define _XOPEN_SOURCE 700
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <limits.h>

extern char **environ;

#define BUFFER_SIZE 4096

#endif /* ALL_H */
