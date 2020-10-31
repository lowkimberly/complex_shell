/*
  Kimberly Low
  lowk2@rpi.edu
  Assignment 5
  A shell with redirection of input and output and pipes
  explort mypath = your directories here, separated by $
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <errno.h>

void build_path_array(char* all_path, char* exec_paths[1024],int* path_count) {
  //we make path count start at 1 because 0 holds current working directory
  exec_paths[0]=(char*)"./";
  
  char* path_tok;
  path_tok = strtok (all_path,"$");
  while (path_tok != NULL) {
    exec_paths[(*path_count)] = path_tok;
    (*path_count)++;
    path_tok = strtok (NULL, "$");
  }
  return;
}


void build_arg_array(char* clean_input, char* exec_args[1024], int* exec_args_len) {
  int arg_count=0;
  char* tok;
  tok = strtok (clean_input," ");
  while (tok != NULL) {
    exec_args[arg_count] = tok;
    arg_count++;
    tok = strtok (NULL, " ");
  }
  (*exec_args_len)=arg_count;
  return;
}

int search_path(char* exec_args[1024], char* exec_paths[1024], int path_count, int exec_args_len) {  
  //manipulate my inputs around
  FILE* writeto = fopen("infile.txt","w");
  FILE* readfrom = fopen("outfile.txt","r+");
  char* buffer=malloc(sizeof(char)*1024);

  while (fgets(buffer,1024,readfrom) != NULL) {
    fprintf(writeto,"%s",buffer);
  }

  fclose(writeto);
  fclose(readfrom);
  rewind(stdout);

  //redirect input and output
  int argindex;
  for(argindex = 0; argindex < exec_args_len; argindex++ ) {
    if (strcmp(exec_args[argindex],"<")==0) {
      int provided_in = open(exec_args[argindex+1], O_RDONLY, S_IRUSR | S_IWUSR | S_IXUSR);
      dup2(provided_in,STDIN_FILENO);
    }
    else if (strcmp(exec_args[argindex],">")==0) {
      int provided_out = open(exec_args[argindex+1], O_WRONLY, S_IRUSR | S_IWUSR | S_IXUSR);
      dup2(provided_out,STDOUT_FILENO);
    }
  }

  //now do actual path stuff
  int all_paths_index;
  for(all_paths_index = 0; all_paths_index < path_count; all_paths_index++) {
    char* checking_path=malloc(1024*sizeof(char));
    strcpy(checking_path,exec_paths[all_paths_index]);
    strcat(checking_path,exec_args[0]);
    execv(checking_path,exec_args);
  }
  // the command wasn't found. Do this.
    printf("Command not found.\n");
  return 0;
}


int main(int argc, char* argv[]) {
  char* history_list[1024];
  int history_counter=0;

  // split up paths
  char* exec_paths[1024];
  int path_count=1;
  build_path_array(getenv("MYPATH"),exec_paths,&path_count);
  
  while (1==1) {
    //strip the newline at the end, and add it to history
    char* raw_input = malloc(1024*sizeof(char));
    char* clean_input = malloc(1024*sizeof(char));
    printf("> ");
    fgets(raw_input, 1024, stdin);
    strncpy(clean_input, raw_input, strlen(raw_input)-1);
    history_list[history_counter] = clean_input;
    history_counter++;

    if (strcmp(clean_input,(char*) "quit")==0) {
      break;
    }
    else if (strcmp(clean_input,(char*) "history")==0) {
      int i;
      for(i=0;i<history_counter;i++) {
	printf("%s\n",history_list[i]);
      }
    }
    else {
      //list of pipe commands
      char* pipe_list[1024]; //array of pipe strings
      int num_of_pipes=0;
      char* pipes_tok = strtok (clean_input,"|");
      while (pipes_tok != NULL) {
	pipe_list[num_of_pipes] = pipes_tok;
	num_of_pipes++;
	pipes_tok = strtok (NULL, "|");
      }

      //Save keyboard input, screen output
      int oldin = dup(STDIN_FILENO);
      int oldout = dup(STDOUT_FILENO);

      fopen("outfile.txt","w+");
      fopen("infile.txt","w+");
      int readfd = open("infile.txt",O_RDWR);
      int writefd = open("outfile.txt",O_RDWR);
      dup2(readfd,STDIN_FILENO);
      dup2(writefd,STDOUT_FILENO);

      pid_t p;
      int pipe_index; 
      for(pipe_index = 0; pipe_index<num_of_pipes; pipe_index++) {
	p=fork();
	if (p==0) {
	  char* exec_args[1024];
	  int* exec_args_len=malloc(sizeof(int));
	  build_arg_array(pipe_list[pipe_index],exec_args,exec_args_len);
	  search_path(exec_args, exec_paths, path_count,(*exec_args_len));
	  return 0;
	}
	else if (p > 0) {
	  wait(NULL);
	}
	else {
	  perror("Could not fork.");
	}
      }//this ends for loop

      dup2(oldin,STDIN_FILENO);
      dup2(oldout,STDOUT_FILENO);

      char* final_buf=malloc(1024*sizeof(char));
      FILE* final_out=fopen("outfile.txt","r");
      while (fgets(final_buf,1024,final_out)!=NULL) {
	printf("%s",final_buf);
      }
    }
  }
  return 0;
}
