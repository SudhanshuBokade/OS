#include <stdio.h>
#include <unistd.h>			// fork(), exec() ,getpid()
#include <sys/wait.h>		// wait()
#include <signal.h>			// signal()
#include <fcntl.h>			// close(), open()
#include <string.h>
#include <stdlib.h>			// exit()


/*parseInput will parse the input string into multiple commands or a single command
 with arguments depending on the delimiter (&&, ##, >, or spaces).*/
int parseInput(char* input_str, char** cmd_list)
{	
	// input_str    = the input string we got using getline
	// cmd_list = array of strings containing list of single commands seperated 
	//                by the detected delimeter

	int retval;
	
	// getting newstring without \n, i.e without the last character
	int len = strlen(input_str);
	char* string_without_newline = (char *)malloc(len-1);   

	for(int j = 0; j < len-1; j++)
		string_without_newline[j] = input_str[j];
	strcat(string_without_newline," ");

	// getting a copy of the new string without "\n"
	char* duplicate_str = strdup(string_without_newline);
	
	// checking for the presence and type of delimiters
	if(strstr(duplicate_str, "&&") != NULL) 
	{
		// We have to isolate each command seperated by delimeter && 
		int i = 0;
		char* sep_str;

		while((sep_str = strsep(&duplicate_str,"&&")) != NULL )
		{
			int count = 0;

			// We can store isolated commands in cmd_list
			strcpy(cmd_list[i],sep_str);
			i++;
		}
		cmd_list[i]=NULL;

		//  for parallel execution of multiple commands set retval=1
		retval = 1;  				
	}
	else if(strstr(duplicate_str, "##") != NULL)
	{
		// isolating each command seperated by delimeter ## 
		int i = 0;
		char* sep_str;

		while((sep_str = strsep(&duplicate_str,"##")) != NULL )
		{
			int count = 0;

			// we need to store the isolated commands in cmd_list
			strcpy(cmd_list[i],sep_str);
			i++;
		}
		cmd_list[i]=NULL;

		// for sequential execution of multiple commands set retval=2
		retval = 2;					
	}
	else if(strstr(duplicate_str, ">") != NULL)
	{
		// we should isolate each command seperated by delimeter > 
		int i = 0;
		char* sep_str;
		while((sep_str = strsep(&duplicate_str,">")) != NULL )
		{
			int count = 0;


			strcpy(cmd_list[i],sep_str);
			i++;
		}
		cmd_list[i]=NULL;

		// for command redirection set retval=3
		retval = 3;					
	}
	else
	{
		//in case of no delimiter, pass the entire command to command list
		char* token = duplicate_str;
		cmd_list[0]=token;
		cmd_list[1]=NULL;
		//for  single command execution set retval=4
		retval = 4;					
	}

	return retval;
}

/* get_commandArgs function  breaks the isolated command strings into their constituent command
and command arguments, and return the array of strings */
char** get_commandArgs(char* cmd)                 //cmd stands for command
{
	//remove new line character from the string
	cmd = strsep(&cmd, "\n");		

	// We have to remove whitespaces before a command if there are present
	while(*cmd == ' ')					
		cmd++;
	
	// creating duplicate command string
	char* dup_command = strdup(cmd);

	// Now ,get the command in a string, to check for "cd"
	char* first_sep = strsep(&cmd, " ");

	// need to change directory if "cd" is encountered
	if (strcmp(first_sep,"cd")==0)
	{
		chdir(strsep(&cmd, "\n "));
	}
	else
	{
		// allocating memory to the array of strings, to store the command with its arguments
		char** cmd_args;
		cmd_args = (char**)malloc(sizeof(char*)*10);
		for(int i=0; i<10; i++)
			cmd_args[i] = (char*)malloc(50*sizeof(char));

		char* sep_str;
		int i = 0;

		//if there's a string with spaces, else directly assign the string to cmd_args[i]
		if (strstr(dup_command, " ")!=NULL)
		{	
			// we have to iterate over the space seperated arguments
			while ((sep_str = strsep(&dup_command," ")) != NULL)					
			{
				// remove whitespaces before a command if there are present
				while(*sep_str == ' ')					
					sep_str++;
				cmd_args[i] = strsep(&sep_str, " ");	
				i++;
			}
			
			// if current string is an empty string, assign NULL
			if(strlen(cmd_args[i-1])==0)
				cmd_args[i-1] = NULL;
			else
				cmd_args[i] = NULL;

			i=0;
			return cmd_args;
		}
		else
		{
			// assign the entire command in case of no spaces
			strcpy(cmd_args[0], dup_command);
			cmd_args[1] = NULL;
			return cmd_args;
		}
		
	}
	return NULL;
}

// executeCommand function  forks a new process to execute a command
void executeCommand(char** cmd_list)
{

	// get the command and arg list from given command
	char** cmd_args = get_commandArgs(cmd_list[0]);

	// fork process
	pid_t pid = fork();

	//child Process
	if (pid==0)          
	{
		//we have to execute the command and it's args in child
		int retval = execvp(cmd_args[0], cmd_args);

		 //execvp error code
		if (retval < 0)     
		{
			printf("Shell: Incorrect command\n");
			exit(1);
		}
	} 

	// Parent Process
	else if(pid)
	{
		//wait for the child to terminate
		wait(NULL);
	}

	// if fork fails, then exit
	else
		exit(0);
}

// executeCommandRedirection function will run a single command with output redirected to an output file specificed by user
void executeCommandRedirection(char** cmd_list)
{
	// getting the command and arg list from given command
	char* command = cmd_list[0];

	// getting the string that contains filename
	char* file_name = cmd_list[1]; 

	// removing whitespaces before a filename if there are present
	while(*file_name == ' ')			
		file_name++;

	// Closing stdout
	

	// Now ,open given file, or create if not found one
	int filefd = open(file_name, O_WRONLY|O_CREAT|O_APPEND, 0666);	
	
	// fork process
	int pid = fork();

	// child Process
	if (pid ==0) 
	{
		close(STDOUT_FILENO);
		dup(filefd);
		// get the command args to execute
		char** cmd_args = get_commandArgs(command);
		int retval = execvp(cmd_args[0], cmd_args);

		//execvp error code
		if (retval < 0)      
		{
			printf("Shell: Incorrect command\n");
			exit(1);
		}
	} 

	// Parent Process
	else if(pid)
	{
		close(filefd);
		int t=wait(NULL);
	}

	// if fork fails, then exit
	else
		exit(0);
}

// executeParallel function will run multiple commands in parallel
void executeParallelCommands(char** cmd_list)
{
	int i = 0;
	while(cmd_list[i]!=NULL)
	{
		char* dup_command = strdup(cmd_list[i]);
		if (get_commandArgs(dup_command) == NULL)
		{
			i++;
			continue;
		}
		else if(strlen(cmd_list[i])>0)
		{
			// fork process
			int pid = fork();
			if (pid == 0)	
			{
				
				//we have to enable signals again for child processes	
				signal(SIGINT, SIG_DFL);
				signal(SIGTSTP, SIG_DFL);	

				// get the command and arg list from given command
				char** command_args = get_commandArgs(cmd_list[i]);

				int return_val = execvp(command_args[0], command_args);

				//execvp error code
				if (return_val < 0)					
				{	 
					printf("Shell: Incorrect command\n");
					exit(1);
				}
				exit(0);
			}
			// Parent Process
			else if (pid)
			{
				//no need to put wait statement here as it needs to run parallely
				//parent process
				i++;			
				if(cmd_list[i+1]!=NULL)
					wait(NULL);
				else
					continue;	
			}
			// if fork fails, then exit
			else
			{ 
				exit(1);	
			}
		}
		else
		{
			i++;
		}
					
	}
}

// executeSequentialCommands will run multiple commands in parallel
void executeSequentialCommands(char** cmd_list)
{	

	int i = 0;
	while(cmd_list[i]!=NULL)
	{	
		// get the command and arg list from given command
		char* dup_command = strdup(cmd_list[i]);

		// special case for "cd"
		if(strstr(dup_command,"cd")!=NULL)
		{
			get_commandArgs(cmd_list[i]);
			i++;
		}
		// case where there's empty string
		else if(strlen(cmd_list[i])>0)
		{

			int pid = fork(); //making child process

			// child process
			if (pid == 0)		
			{
				//we have to enable signals again for child processes
				signal(SIGINT, SIG_DFL);
				signal(SIGTSTP, SIG_DFL);		

				// parsing each command
				char** command_args = get_commandArgs(cmd_list[i]); 
				int return_val = execvp(command_args[0], command_args);
			
				//execvp error code
				if (return_val < 0)					
				{	 
					printf("Shell: Incorrect command\n");
					exit(1);
				}	
	
			}

			// Parent Process
			else if (pid)
			{
				// we have to enable signals again for child processes
				wait(NULL);
				i++;
			}	
			else
			{ 
				exit(1);
			}
		}
		else
		{
			i++;
		}
		
	}
}

// It is Signal Handler for SIGINT (CTRL + C)
void sigintHandler(int sig_num)
{
    /* Reseting handler to catch SIGINT next time. */
    signal(SIGINT, sigintHandler);
    printf("\n Not able to be terminated using Ctrl+C, enter 'exit' \n");
}

// It is Signal Handler for SIGTSTP (CTRL + Z)
void sighandler(int sig_num)
{
    // Reseting handler for catching SIGTSTP next time
    signal(SIGTSTP, sighandler);
    printf("\nUnable to terminate using Ctrl+Z, enter 'exit'\n");      
}

int main()
{
	// Initial declarations are mentioned below
	char* str=NULL;
	size_t str_len = 64;

	signal(SIGINT, sigintHandler);
	signal(SIGTSTP, sighandler);
	
	// This loop will keep your shell running until user exits.
	while(1)	
	{
		char curr_dir[64];

		// Print the prompt in format - currentWorkingDirectory$
		getcwd(curr_dir, 64);
		//printing cuurent directory
		printf("%s$",curr_dir);          

		// accepting input with 'getline()'
		getline(&str,&str_len,stdin);   

		// do allocate memory for the list of strings to strore isolated
		// commands, seperated by delimiters
		char** cmd_list;
		cmd_list = (char**)malloc(sizeof(char*)*10);
		for(int i=0; i<10; i++)
			cmd_list[i] = (char*)malloc(50*sizeof(char));

		// When user uses exit command		
		if(strcmp(str, "exit\n")==0)
		{
			printf("Exiting shell...\n");
			break;
		}

		// We have to Parse input with 'strsep()' for different symbols (&&, ##, >) and for spaces.
		int ret = parseInput(str, cmd_list); 
	
		if(ret==1)
			executeParallelCommands(cmd_list);		// This function invokes when user wants to run multiple commands in parallel (commands separated by &&)
		else if(ret==2)
			executeSequentialCommands(cmd_list);	// This function invokes when user wants to run multiple commands sequentially (commands separated by ##)
		else if(ret==3)
			executeCommandRedirection(cmd_list);	// This function invokes when user wants redirect output of a single command to and output file specificed by user
		else
			executeCommand(cmd_list);		// This function invokes when user wants to run a single commands
				
	}
	
	return 0;
}
