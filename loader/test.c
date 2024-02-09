#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "include/loader_priv.h"

int main(int argc, char *argv[])
{
  int fd;
  const char *memblock;
  struct stat sb;

  fd = open(argv[1], O_RDONLY);
  fstat(fd, &sb);
  printf("Size: %llu\n", (uint64_t)sb.st_size);
  memblock = mmap(NULL, sb.st_size, PROT_READ, MAP_SHARED, fd, 0);

  read_elf(memblock);
}