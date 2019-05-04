//
// Created by Nie R on 2019/5/1.
//

#ifndef NLOGGER_NLOGGER_MMAP_H
#define NLOGGER_NLOGGER_MMAP_H

#ifndef NLOGGER_MMAP_CACHE_MODE
#define NLOGGER_MMAP_CACHE_MODE 1
#endif

#ifndef NLOGGER_MEMORY_CACHE_MODE
#define NLOGGER_MEMORY_CACHE_MODE 2
#endif

#ifndef NLOGGER_INIT_CACHE_FAILED
#define NLOGGER_INIT_CACHE_FAILED 0
#endif

#ifndef NLOGGER_MMAP_CACHE_SIZE
#define NLOGGER_MMAP_CACHE_SIZE 150 * 1024 //150k
#endif


#ifndef NLOGGER_MEMORY_CACHE_SIZE
#define NLOGGER_MEMORY_CACHE_SIZE 150 * 1024 //150k
#endif





//#include <stdio.h>
//#include <unistd.h>
//#include<sys/mman.h>
//#include <fcntl.h>
//#include <string.h>

int init_cache_nlogger(char *mmap_cache_file_path, char **mmap_cache_buffer);

#endif //NLOGGER_NLOGGER_MMAP_H