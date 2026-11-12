#include <stddef.h>
#include <stdio.h>
#include <pwd.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct Cmd {
  char *shortcut;
  char *command;
  struct Cmd *next;
};

static char *copy_string(const char *str) 
{
  size_t len = strlen(str);
  char *copy = malloc(len);

  if(copy == NULL) {
    perror("malloc");
    return "999";
  }

  memcpy(copy, str, len);
  return copy;
}

struct Cmd *create_cmd(char *shortcut, char *command) 
{
  struct Cmd *c = malloc(sizeof(struct Cmd));

  if(c == NULL) {
    fprintf(stderr, "Could not allocate...\n");
    return c;
  }

  c->shortcut = copy_string(shortcut);
  c->command = copy_string(command);
  return c;
}

void prepend_cmd(struct Cmd **head, char *shortcut, char *command)
{
  struct Cmd *c = create_cmd(shortcut, command);
  c->next = *head;
  *head = c;
}

int set_home_dir(char *path) 
{
  char *home = getenv("HOME");
  if(home != NULL) {
    strcpy(path, home);
    return 0;
  }

  return -1;
}

int template_config(char *config_path) 
{
  FILE *fl = fopen(config_path, "w");
  if(fl == NULL){
    fprintf(stderr, "Failed to write to %s\n", config_path);
    return 1;
  }

  fprintf(fl, "d:devtools::document()\n");
  fprintf(fl, "t:devtools::test()\n");
  fprintf(fl, "c:devtools::check(args = '--no-tests')\n");
  fprintf(fl, "sa:testthat::snapshot_accept()\n");
  fprintf(fl, "sr:testthat::snapshot_review()\n");

  fclose(fl);

  return 0;
}

int file_exists(char *path) {
  FILE *fl = fopen(path, "r");

  if(fl == NULL) {
    return 0;
  }

  fclose(fl);
  return 1;
}

int main(int argc, char *argv[])
{
  // get home directory
  char config_path[256];
  int err = set_home_dir(config_path);

  if(err != 0){
    fprintf(stderr, "Can't find user home directory");
    return 1;
  }

  // find .r
  size_t config_path_len = strlen(config_path);
  if(config_path_len + 4 <= sizeof config_path) {
    config_path[config_path_len] = '/';
    config_path[config_path_len+1] = '.';
    config_path[config_path_len+2] = 'r';
    config_path[config_path_len+3] = '\0';
  } else {
    fprintf(stderr, "Can't create path to config in $HOME");
    return 1;
  }

  // check if config flag
  if(argc > 1) {
    int cmp = strcmp(argv[1], "config");

    // unsupported - we only have config right now
    if(cmp != 0) {
      fprintf(stderr, "Unsupported argument\n");
      return -1;
    }

    int exists = file_exists(config_path);
    if(exists > 0) {
      printf("The config file already exists remove `%s` and try again\n", config_path);
      return 0;
    }

    int success = template_config(config_path);

    if(success < 1) {
      return 1;
    }

    printf("Config `%s` created\n", config_path);
    return 0;
  }

  int success = file_exists(config_path);

  if(success < 1) {
    fprintf(stderr, "Config file not found, run `r config`\n");
    return 1;
  }

  printf("Proceed\n");

  return 0;
}
