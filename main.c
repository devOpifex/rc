#include <stdio.h>
#include <pwd.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int set_home_dir(char *path) 
{
  char *home = getenv("HOME");
  if(home != NULL) {
    strcpy(path, home);
    return 0;
  }

  return -1;
}

int main()
{
  char path[256];
  int err = set_home_dir(path);

  if(err != 0){
    return 1;
  }

  printf("Path is %s\n", path);
  return 0;
}
