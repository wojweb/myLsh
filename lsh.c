#include<sys/types.h>
#include<sys/wait.h>
#include<unistd.h>
#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<fcntl.h>
#include<signal.h>

void lsh_loop(void);
char *lsh_read_line(void);
char **lsh_split_line(char*);
int lsh_launch(char **);
void lsh_redirection_out(char **args, int position);
void lsh_redirection_in(char **args, int position);
int lsh_cd(char **args);
int lsh_exit(char **args);
int lsh_execute(char **args); 
void lsh_signal_handler(int sig);

int main(int argc, char **argv)
{
   //Load config files, if any.
   
   //Run command loop.
   lsh_loop();

   //Perform any shutdown/cleanup


   return EXIT_SUCCESS;
}

void lsh_loop(void)
{
   char *line;
   char **args;
   int status = 1;

   do{
	  printf("lsh: ");
	  line = lsh_read_line();
	  args = lsh_split_line(line);
	  
	  

	  status = lsh_launch(args);

	  //free(line);
	  //free(args);
   } while(status);
}

char *lsh_read_line(void)
{
   char *line = NULL;
   size_t bufsize = 0;
	   getline(&line, &bufsize, stdin);
   
   if(line[0] == '\0'){
	  printf("\n");
	  exit(0);
   }
   
   return line;
}

#define LSH_TOK_BUFSIZE 64
#define LSH_TOK_DELIM " \t\r\n\a"
char **lsh_split_line(char *line)
{
   int bufsize = LSH_TOK_BUFSIZE, position = 0;
   char **tokens = malloc(bufsize * sizeof(char*));
   char *token;

   if(!tokens){
	  fprintf(stderr, "lsh: allocation error\n");
	  exit(EXIT_FAILURE);
   }
   
   token = strtok(line, LSH_TOK_DELIM);
   while(token != NULL){
	  tokens[position] = token;
	  position++;
	  
	  if(position >= bufsize){
		 bufsize += LSH_TOK_BUFSIZE;
		 tokens = realloc(tokens, bufsize *sizeof(char*));
		 if(!tokens){
	  	fprintf(stderr, "lsh: allocation error\n");
		  exit(EXIT_FAILURE);
		 }
	  }
   token = strtok(NULL, LSH_TOK_DELIM);
   }
   tokens[position] = NULL;
   return tokens;
}

void lsh_redirection_out(char **args, int position)
{
   int fd;
   args[position] = NULL; 
   if(args[position + 1] != NULL){
	  fd = open(args[position + 1], O_RDWR | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR);
	  dup2(fd, 1);
	  close(fd);
   }
}

void lsh_redirection_in(char **args, int position)
{
   int fd;
   args[position] = NULL; 
   if(args[position + 1] != NULL){
	  fd = open(args[position + 1], O_RDONLY);
	  dup2(fd, 0);
	  close(fd);
   }
}

void lsh_redirection_err(char **args, int position)
{
   int fd;
   args[position] = NULL; 
   if(args[position + 1] != NULL){
	  fd = open(args[position + 1], O_RDWR | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR);
	  dup2(fd, 2);
	  close(fd);
   }
}

int lsh_pipe(char **args, int position)
{
   int pipefd[2]; // [0] FD do odczytu
   int pid;		  // [1] FD do zapisu

   args[position] = NULL;
   pipe(pipefd);
   pid = fork();
   int saveFD = dup(1);

   if(pid == 0)
   {
	  //replace standard input with input part of pipe
	  close(pipefd[1]); // zamyka zapisywanie do pipe
	  dup2(pipefd[0], 0); // zmienia odczyt na stdin
	  close(pipefd[0]); //zamyka odczyt
	  lsh_launch(args + position + 1);
	  
	  return 0;
	  
   } else {
	  //replace standard output with output part of pipe

	  close(pipefd[0]); // zamyka zapis
	  dup2(pipefd[1], 1); //zmienia zapis do stdout 
	  close(pipefd[1]); // zamyka zapis
	  lsh_launch(args);
	  dup2(saveFD, 1);

   
	  	  
   }

   
   return 1;
   


}
   
void proc_exit()
{  
   int wstat;
   pid_t pid;
   pid = wait3(&wstat, WNOHANG, (struct rusage *) NULL);
   
;
}
void proc_no()
{
}
   
int lsh_launch(char **args)
{
   
	  
       
   int waitforchild = 1;

   pid_t pid, wpid;
   int status;
   int safeFD0 = dup(0);
   int safeFD1 = dup(1);
   int safeFD2 = dup(2);

   if(args[0] == NULL) //an empty command entered
	  return 1;

   if(strcmp(args[0], "cd") == 0)
	  return lsh_cd(args);

   if(strcmp(args[0], "exit") == 0)
	  return lsh_exit(args);

   for(int i=0; args[i] != NULL; i++){
	  if(strcmp(args[i], "|") == 0){
		 return	lsh_pipe(args, i);
	  }else if(strcmp(args[i], "&") == 0){
		 waitforchild = 0;
		 args[i] = NULL;
	  }
   }
	  signal(SIGCHLD, proc_exit);
   pid = fork();
   if(pid == 0){
	  //Child process
	  
	  for(int i=0; args[i] != NULL; i++){
		 if(strcmp(args[i], ">>") == 0){
			lsh_redirection_out(args, i);
		 } else if(strcmp(args[i], "<<") == 0){
			lsh_redirection_in(args, i);
		 } else if(strcmp(args[i], "2>") == 0){
			lsh_redirection_err(args, i);
		 }
	  }
   
	  if(execvp(args[0], args) == -1){
		 perror("lsh");
	  }
	  exit(EXIT_FAILURE);
   } else if(waitforchild){
	  //parent process
	  do{
		 wpid = wait(&status); //return process id
	  } while((!WIFEXITED(status) && !WIFSIGNALED(status)));
   }
   signal(SIGINT, lsh_signal_handler);
   dup2(safeFD0, 0);
   dup2(safeFD1, 1);
   dup2(safeFD2, 2);
   return 1;
}

int lsh_cd(char **args)
{
   if(args[1] == NULL){
	  fprintf(stderr, "lsh: expected argument to \"cd\"\n");
   }else {
	  if(chdir(args[1]) != 0){
		 perror("lsh");
	  }
   }
   return 1;
}

int lsh_exit(char **args)
{
   return 0;
}

void lsh_signal_handler(int sig)
{
   signal(sig, SIG_IGN);
   printf("\n");
   signal(SIGINT, SIG_IGN);
} 
