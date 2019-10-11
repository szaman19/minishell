#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
//limits
#define MAX_TOKENS 100
#define MAX_STRING_LEN 100

size_t MAX_LINE_LEN = 10000;


// builtin commands
#define EXIT_STR "exit"
#define EXIT_CMD 0
#define UNKNOWN_CMD 99

typedef struct {
	char  *name;
	pid_t pid; 
	int pos; 
}job ;

FILE *fp; // file struct for stdin
char **tokens;
char *line;

job * bg_jobs;
int jobs_pos = 0;

void signal_handler(int signum){
	printf("\n Caught Signal %d. Closing shell ...\n", signum);
	// exit(1);
	printf("%s\n","Type exit to quit." );
}

void initialize()
{

	// allocate space for the whole line
	assert( (line = malloc(sizeof(char) * MAX_STRING_LEN)) != NULL);

	// allocate space for individual tokens
	assert( (tokens = malloc(sizeof(char*)*MAX_TOKENS)) != NULL);

	// open stdin as a file pointer 
	assert( (fp = fdopen(STDIN_FILENO, "r")) != NULL);
	
	assert( (bg_jobs = malloc(sizeof( job) * MAX_TOKENS)) != NULL);

}

int tokenize (char * string)
{
	int token_count = 0;
	int size = MAX_TOKENS;
	char *this_token;

	while ( (this_token = strsep( &string, " \t\v\f\n\r")) != NULL) {

		if (*this_token == '\0') continue;

		tokens[token_count] = this_token;

		//printf("Token %d: %s\n", token_count, tokens[token_count]);

		token_count++;

		// if there are more tokens than space ,reallocate more space
		if(token_count >= size){
			size*=2;

			assert ( (tokens = realloc(tokens, sizeof(char*) * size)) != NULL);
		}
	}
	return token_count;
}

int read_command() 
{

	// getline will reallocate if input exceeds max length
	assert( getline(&line, &MAX_LINE_LEN, fp) > -1); 

	//printf("Shell read this line: %s\n", line);

	int ret_val = 0;
	ret_val = tokenize(line);
	return ret_val;
}
void clean_jobs_queue(){
	job * cleaned_job_list; 

	assert((cleaned_job_list = malloc(sizeof(job)*MAX_TOKENS) )!= NULL);

	int counter = 0;
	for (int i = 0; i < MAX_TOKENS; ++i){
		if(bg_jobs[i].name && bg_jobs[i].pos != 1){
			job temp = {bg_jobs[i].name, bg_jobs[i].pid, 0};
			cleaned_job_list[counter] = temp;
			counter++;
		}
	}

	free(bg_jobs);
	bg_jobs = cleaned_job_list;
}
void  list_jobs(){
	int counter = 0; 
	for (int i = 0; i < MAX_TOKENS; ++i){
		if(bg_jobs[i].name){
			counter++;
			pid_t id = bg_jobs[i].pid;
			int status;
			int ret ;
			ret = waitpid(id, &status, WNOHANG);
			if (ret > 0){
				bg_jobs[i].pos = 1; 
				printf("Command %s with PID %d Status: %s \n", bg_jobs[i].name, bg_jobs[i].pid,"terminated");	
			}
			else if (ret < 0){
				bg_jobs[i].pos = 1;
				printf("Command %s with PID %d Status: %s \n", bg_jobs[i].name, bg_jobs[i].pid,"terminated");	
			}
			else{	
				printf("Command %s with PID %d Status: %s \n", bg_jobs[i].name, bg_jobs[i].pid, "active");
			}
		}
	}

	if(counter > 5){
		clean_jobs_queue();
	}	
}


void bring_fore(int pid){
	int status = 0;
	waitpid(pid, &status, 0);
		
}

// void check_pipe()
void call_exe(int num_tokens){
	pid_t  pid; 
	pid = fork();
	if (pid < 0){
		perror("Fork failed:");
		exit(1);
	}
	if(pid ==0){	
		tokens[num_tokens] = '\0';	
		if(execvp(tokens[0],tokens) == -1){
			fprintf(stderr, "Command not found \n");
		}
		exit(0);
	}
	if(pid >0){
		int ret; 
		int status;
		ret = waitpid(pid,&status,0);	
		if (ret > 0){
			free(tokens);
			assert( (tokens = malloc(sizeof(char*)*MAX_TOKENS)) != NULL);
		}
	}	

}
int run_command(int num_tokens) {
	
	if(!tokens[0]){
		//check if empty enter
		return 1;
	}
	
	// tokens[0] is not empty after here
	//
	//
	//
	if (strcmp( tokens[0], EXIT_STR ) == 0){
		return EXIT_CMD;
	}
	else if (strcmp(tokens[0],"fg") == 0 ){

		//printf("put to foreground");	
		int pid = atoi(tokens[1]);
		bring_fore(pid);
		return 1;

	}
	else if (strcmp(tokens[0],"listjobs") == 0){
		list_jobs();
		return 1;
	}
	else{
		// Token is not empty here	
		if(num_tokens > 0){
			
			if (num_tokens > 1){
				char * amp = tokens[num_tokens - 1];
				if (strcmp(amp, "&") == 0){
				
					//printf("background task started \n");

					char * temp[num_tokens];

					for (int i = 0; i < num_tokens-1; ++i){
				 		//printf("%s \n", tokens[i]);
						temp[i] = tokens[i];
					}

					temp[num_tokens-1] = NULL; 
					pid_t  pid; 
					pid = fork();
					if (pid < 0){
						perror("Fork failed:");
						exit(1);
					}
					if(pid ==0){
						//This is the child process
		
						//fprintf(stdout,"number of tokens: %d \n" , num_tokens);	
				
						if(execvp(tokens[0],temp) == -1){
							fprintf(stderr, "Command not found \n");
						}
						exit(0);
					}
					if(pid >0){ 
						//waitpid(pid, &t, WNOHANG);
						char * str; 

						str = malloc(sizeof(char *) * MAX_TOKENS);
					
						//char * t = 0;
						strcpy(str, tokens[0]);
					
						//printf("%s, %d \n",str, pid);
						job  jobs  = {str, pid, 0};

						int i = 0; 
						while(bg_jobs[i].name){
							i++;
						}
						bg_jobs[i] = jobs;	
					}
				}else{
					// printf("%s \n", tokens[num_tokens - 1] );	
					call_exe(num_tokens);

				}
			}else{
					// printf("%s \n", tokens[num_tokens - 1] );	
					call_exe(num_tokens);
			}	
		}
	}
	return UNKNOWN_CMD;
}

int main()
{
	
	initialize();
	
	int num_tokens = 0;
	
	signal(SIGINT, signal_handler);
	do {
		printf("sh550> ");
		num_tokens = read_command();
		
	} while( run_command(num_tokens) != EXIT_CMD );

	return 0;
}

