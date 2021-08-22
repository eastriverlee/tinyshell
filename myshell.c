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
  o semicolons (with space in front)
  o ampersand (with space in front)
  o redirection
  o pipe
  o mix

  [interactive]
  o EOF
  o semicolons (with space in front)
  o ampersand (with space in front)
  o redirection
  o pipe
  o mix

************************************************************************************/

typedef enum {start, middle, end, nopipe} pipekind;
typedef int pipe_io[2];

char **full_command, **nextcommand, **command; 
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
bool is_end_of_file(int c);

#define is_same !strcmp
int main(int count, char *raw_input[])
{
	interactive = count == 1;
	commandline = count == 3 && is_same(raw_input[1], "-c");
	if (interactive) interact();
	else if (commandline) 
	{
		separate(raw_input[2]);
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

#define more_semicolon (semicolons > 0)
#define more_pipe (pipes > 0)
void run()
{
	size_t i;

	while(true)
	{ 
		pipes = 0;
		for (i = 0; command[i]; i++) if (is_same(command[i], "|")) pipes++;
		more_pipe ? connectpipe() : _run(nopipe); 
		if (more_semicolon)
		{
			command = &command[1];
			semicolons--;
		}
		else break;
	}
	free(full_command);
}

#define currentpipe (alternate ? _file : file)
#define nextpipe (alternate ? file : _file)
void connectpipe()
{
	size_t i, j;
	pipekind kind = start;
	alternate = false;

	for (i = 0; command[i]; i++) if (is_same(command[i], "|"))
	{
		erase(&command[i]);
		while (true)
		{
			if (pipe(currentpipe) != ERROR)
			{
				_run(kind);
				pipes--;
				if (!*command) return;
				for (j = 0; command[j]; j++) if (is_same(command[j], "|")) erase(&command[j]);;
				if (!more_pipe) 
				{
					_run(end);
					return;
				}
				alternate = !alternate;
			}
			else perror("pipe()");
			kind = middle;
		}
	};
}

void _run(pipekind kind)
{
	while (*command)
	{
		execute(command, kind); for (i = 0; i < next; i++) if (command) erase(&command[i]);;
		command = &command[next];
		if (more_pipe) break;
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
					if (!*command) exit(EXIT_SUCCESS);
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
						default: break;
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
						default: break;
					}
					if (!background) while(wait(NULL) > 0);
					getnextcommand();
	}
}

void redirect()
{
	size_t i;

	for (i = 0; command[i]; i++)
	{
		if (is_same(command[i], "<")) redirect_in(i++);	
		else if (is_same(command[i], ">")) redirect_out(i++);
		else if (is_same(command[i], "2>")) redirect_err(i++);
	}
	if (more_pipe) i++;
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
	full_command = malloc(sizeof(char *) * max);
	token = strtok(commands, " ");
	while (token) 
	{
		if (is_same(token, ";")) 
		{ 
			full_command[i] = NULL;
			semicolons++;
		}
		else
		{
			full_command[i] = malloc(sizeof(char) * (strlen(token) + 1));
			strcpy(full_command[i], token);
		}
		token = strtok(NULL, " ");
		if (i == max) full_command = realloc(full_command, max *= 2);
		i++;
	}
	full_command[i] = NULL;
	background = is_same(full_command[--i], "&");
	if (background) erase(&full_command[i]);
	command = full_command;
}

void erase(char **command)
{
	free(*command);
	*command = NULL;
}

void prompt()
{
	size_t i, max = 4096;
	int letter;
	char *input = malloc(sizeof(char) * max);

	printf("$ ");

	for (i = 0; (letter = getchar()) != '\n'; i++) 
	{
		if (!is_end_of_file(letter)) input[i] = letter;
		if (i == max) input = realloc(input, max *= 2);
	}
	input[i] = 0;
	separate(input);
	free(input);
}

bool is_end_of_file(int c)
{
	if (c == EOF) 
	{
		putchar('\n');
		exit(EXIT_SUCCESS);
	}
	return false;
}
