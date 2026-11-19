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

int print_config(char *path)
{
  FILE *f = fopen(path, "r");

  if(f == NULL){
    perror("fopen");
    return 1;
  }

  char line[1028];
  while(fgets(line, sizeof line, f) != NULL){
    printf("%s", line);
  }
  
  fclose(f);

  return 0;
}

int parse_config(struct Cmd **head, char *path)
{
  FILE *f = fopen(path, "r");
  if (f == NULL) {
    perror("fopen");
    return 1;
  }

  char line[512];
  while (fgets(line, sizeof line, f) != NULL) {
    line[strcspn(line, "\r\n")] = '\0';
    if (line[0] == '\0' || line[0] == '#')
      continue;

    char *sep = strchr(line, ':');
    if (sep == NULL || sep == line) {
      fprintf(stderr, "%s: expecting <shortcut>:<expression>\n", path);
      fclose(f);
      return 1;
    }

    *sep = '\0';
    prepend_cmd(head, line, sep + 1);
  }

  fclose(f);
  return 0;
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
  fprintf(fl, "i:devtools::install()\n");
  fprintf(fl, "c:devtools::check(args = '--no-tests')\n");
  fprintf(fl, "sa:testthat::snapshot_accept()\n");
  fprintf(fl, "sr:testthat::snapshot_review()");

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

struct Cmd *get_cmd(struct Cmd *head, const char *shortcut)
{
  for (struct Cmd *p = head; p != NULL; p = p->next) {
    if (strcmp(p->shortcut, shortcut) == 0)
      return p;
  }
  return NULL;
}

int exec_command(char *command) 
{
  char str[1028];
  snprintf(str, sizeof str, "Rscript -e '%s'", command);

  FILE *e = popen(str, "r");
  if(!e){
    perror("popen");
    return 1;
  }

  char buf[4096];
  while(fgets(buf, sizeof buf, e)) {
    fputs(buf, stdout);
  }

  return pclose(e);
}

void print_help()
{
  printf("r creates R shortcuts\n\n");
  printf("Usage:\n \tr <command> <command>\n\n");
  printf("Commands:\n\n");
  printf("\t-commands \tlist of commands\n");
  printf("\t-config \tcreate example config at $HOME/.r\n");
  printf("\t<command> \tyour custom commands\n");
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
    fprintf(stderr, "Can't create path to config in $HOME\n");
    return 1;
  }

  // check if config flag
  if(argc < 2) {
    fprintf(stderr, "No arguments, need -config, -commands, or space separated commands.\n");
    return 1;
  }

  int cmp_help = strcmp(argv[1], "-help");

  if(cmp_help == 0){
    print_help();
    return 0;
  }

  int cmp_config = strcmp(argv[1], "-config");

  if(cmp_config == 0) {
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


  int cmp_commands = strcmp(argv[1], "-commands");

  if(cmp_commands == 0){
    print_config(config_path);
    return 0;
  }

  struct Cmd *head = NULL;
  int parsed = parse_config(&head, config_path);
  if(parsed > 0) {
    fprintf(stderr, "Error parsing config\n");
    return 1;
  }

  for(int i = 1; i < argc; i++){
    struct Cmd *command = get_cmd(head, argv[i]);

    if(command == NULL){
      fprintf(stderr, "Could not find shortcut `%s` in %s\n", argv[i], config_path);
      return 1;
    }

    int error = exec_command(command->command);
    if(error == 0){
      continue;
    }

    fprintf(stderr, "Failed to run command: %d", error);
  }

  return 0;
}
