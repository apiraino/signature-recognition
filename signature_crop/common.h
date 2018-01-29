#ifndef COMMON_H
#define COMMON_H

// man ftw
// but also http://stackoverflow.com/a/2198045
// #define _XOPEN_SOURCE 500

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>
#include <time.h>
#include <errno.h>
#include <libgen.h> // dirname, basename
#include <glob.h>
#include <dirent.h>
#include <assert.h>

// mkdir
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

// rmtree
#include <ftw.h>

#endif
