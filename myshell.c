// JUL 01 2020 / myshell / LEE EASTRIVER
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#define ERROR -1

/************************************************************************************

CHECKLIST

[commandline]
o semicolons
o redirection
o pipe
o background
o mix

[interactive]
o EOF
o semicolons
o redirection
o pipe
o background
o mix

************************************************************************************/

typedef enum {start, middle, end, nopipe} pipekind;
typedef int pipe_io[2];
char **fullcommand, **nextcommand, **command; 
char input_filename[512], output_filename[512], error_filename[512];
size_t i, semicolons, next, pipes;
bool interactive, commandline, background, alternate;
int infile, outfile, errfile;
pipe_io file, _file;
pid_t pid;

void run();
void separate(char *commands);
void prompt();
void interact();
void connectpipe();
void _run(pipekind kind);
void redirect();
void redirect_in(size_t index);
void redirect_out(size_t index);
void redirect_err(size_t index);
void execute(char **command, pipekind kind);
void erase(char **command);
bool eof(int c);


int main(int count, char *rawinput[])
{
	interactive = count == 1;
	commandline = count == 3 && !strcmp("-c", rawinput[1]);
	if (interactive) interact();
	else if (commandline) 
	{
		separate(rawinput[2]);
		run();
	}
	else printf("myshell: invalid input\n");
}

void interact()
{
	while (true)
	{
		prompt();
		run();
	}
}

#define moresemicolon (semicolons > 0)
#define morepipe (pipes > 0)
void run()
{
	size_t i;
	while(true)
	{ 
		pipes = 0;
		for(i = 0; command[i] != NULL; i++) if (!strcmp(command[i], "|")) pipes++;;
		if (morepipe) connectpipe();
		else _run(nopipe); 
		if (moresemicolon)
		{
			command = &command[1];
			semicolons--;
		}
		else break;
	}
	free(fullcommand);
}

#define currentpipe (alternate ? _file : file)
#define nextpipe (alternate ? file : _file)
void connectpipe()
{
	size_t i, j;
	pipekind kind = start;
	alternate = false;

	for (i = 0; command[i] != NULL; i++) if (!strcmp(command[i], "|"))
	{
		erase(&command[i]);
		while (true)
		{
			if (pipe(currentpipe) != ERROR)
			{
				_run(kind);
				pipes--;
				if (*command == NULL) return;
				for (j = 0; command[j] != NULL; j++) if (!strcmp(command[j], "|")) erase(&command[j]);;
				if (!morepipe) 
				{
					_run(end);
					return;
				}
				alternate = alternate ? false : true;
			}
			else perror("pipe()");
			kind = middle;
		}
	};
}

void _run(pipekind kind)
{
	while (*command != NULL)
	{
		execute(command, kind);
		for (i = 0; i < next; i++) if (command != NULL) erase(&command[i]);;
		command = &command[next];
		if (morepipe) break;
	}
}

#define READ 0
#define WRITE 1
#define CHILD 0
#define PARENT default
#define previouspipe nextpipe
void execute(char **command, pipekind kind)
{
	void (*getnextcommand)() = redirect;

	switch (pid = fork())
	{
		case ERROR: perror("fork()"); break;
		case CHILD: 
			    if (*command == NULL) exit(EXIT_SUCCESS);
			    switch (kind)
			    {
				    case start: 
				    		dup2(currentpipe[WRITE], STDOUT_FILENO);
						break;
				    case middle: 
				    		dup2(previouspipe[READ], STDIN_FILENO); 
				    		dup2(currentpipe[WRITE], STDOUT_FILENO);
				    		break;
				    case end:
						dup2(currentpipe[READ], STDIN_FILENO);
				    default:	break;
			    }
			    redirect();
			    execvp(*command, command);
			    perror(*command);
			    _exit(EXIT_FAILURE);
		PARENT:
			    switch (kind)
			    {
				    case start: 
				    		  close(currentpipe[WRITE]);
				    		  break;
				    case middle:  
				    		  close(previouspipe[READ]);
				    		  close(currentpipe[WRITE]);
						  break;
				    case end: 
						  close(currentpipe[READ]);
			    	    default:	  break;
			    }
			    if (!background) while(wait(NULL) > 0);;
			    getnextcommand();
	}
}

void redirect()
{
	size_t i;
	for (i = 0; command[i] != NULL; i++)
	{
		if (!strcmp(command[i], "<")) redirect_in(i++);	
		else if (!strcmp(command[i], ">")) redirect_out(i++);
		else if (!strcmp(command[i], "2>")) redirect_err(i++);
	}
	if (morepipe) i++;
	next = i;
}

void redirect_in(size_t i)
{
	if (pid == CHILD) 
	{
		erase(&command[i]);
		strcpy(input_filename, command[++i]);
		erase(&command[i]);
		infile = open(input_filename, O_RDONLY);
		if (infile != ERROR)
		{
			dup2(infile, STDIN_FILENO);
			close(infile);
		}
		else 
		{
			perror(input_filename);
			exit(EXIT_SUCCESS);
		}
	}
}

void redirect_out(size_t i)
{
	if (pid == CHILD) 
	{
		erase(&command[i]);
		strcpy(output_filename, command[++i]);
		erase(&command[i]);
		outfile = open(output_filename, O_RDWR | O_CREAT, 0777);
		if (outfile != ERROR)
		{
			dup2(outfile, STDOUT_FILENO);
			close(outfile);
		}
		else 
		{
			perror(output_filename);
			exit(EXIT_SUCCESS);
		}

	}
}

void redirect_err(size_t i)
{
	if (pid == CHILD) 
	{
		erase(&command[i]);
		strcpy(error_filename, command[++i]);
		erase(&command[i]);
		errfile = open(error_filename, O_RDWR | O_CREAT, 0777);
		if (errfile != ERROR)
		{
			dup2(errfile, STDERR_FILENO);
			close(errfile);
		}
		else 
		{
			perror(error_filename);
			exit(EXIT_SUCCESS);
		}

	}
}

void separate(char *commands)
{
	size_t max = 1024;
	char *token;
	i = 0;
	semicolons = 0;
	fullcommand = (char **)malloc(sizeof(char *) * max);
	token = strtok(commands, " ");
	while (token != NULL) 
	{
		if (!strcmp(token, ";")) 
		{ 
			fullcommand[i] = NULL;
			semicolons++;
		}
		else
		{
			fullcommand[i] = malloc(sizeof(char) * (strlen(token) + 1));
			strcpy(fullcommand[i], token);
		}
		token = strtok(NULL, " ");
		if (i == max) fullcommand = realloc(fullcommand, max *= 2);
		i++;
	}
	fullcommand[i] = NULL;
	if (!strcmp(fullcommand[--i], "&")) 
	{
		background = true;
		erase(&fullcommand[i]);
	}
	else background = false;
	command = fullcommand;
}

void erase(char **command)
{
	free(*command);
	*command = NULL;
}


void prompt()
{
	size_t i = 0, max = 4096;
	char letter;
	char *input = malloc(sizeof(char) * max);

	printf("$ ");

	for (i = 0; (letter = getchar()) != '\n'; i++) 
	{
		if (!eof(letter)) input[i] = letter;
		if (i == max) input = realloc(input, max *= 2);
	}
	input[i] = 0;
	separate(input);
	free(input);
}

bool eof(int c)
{
	if (c == EOF) 
	{
		putchar('\n');
		exit(EXIT_SUCCESS);
	}
	else return false;
}
