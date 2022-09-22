// SPDX-License-Identifier: MIT

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>


#define NCM_EN_MAGIC "persistent_ncm_on"

int main(int argc, char **argv) {
  int fd;
  char buff[32];
  fd = open("/dev/mtdblock12", O_RDONLY);
  if (fd < 0) {
    fprintf(stderr,"Error opening misc partition to read ncm flag \n");
    return 1;
  }
  lseek(fd, 128, SEEK_SET);
  read(fd, buff, sizeof(NCM_EN_MAGIC));
  close(fd);
  if (strcmp(buff, NCM_EN_MAGIC) == 0) {
    fprintf(stdout, "Persistent NCM is enabled\n");
    return 1;
  }
  
  fprintf(stdout, "Persistent NCM is disabled\n");
  return 0;
}

