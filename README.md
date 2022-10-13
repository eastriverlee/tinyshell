# tinyshell
a tiny shell made with c standard library, which handles multiple pipes and redirections

## features
- [x] interactive mode
- [x] commandline mode
- [x] multiple pipes  
- [x] multiple redirections  
- [x] any combinations  

# overview
this program consists of three major parts:  
- execute.c
- pipe.c
- redirect.c

know that there are three global variables used for the sake of simplicity,  
but you can easily changed them to pointer variables.

```c
extern int I;
extern int pipe_count;
extern bool is_background;
```

## execute.c
`bool fork_pipe_redirect(char **, io [2])` is like *the core of the core*.  
pretty much everything is happening here.  

well, except for the `execvp(3)` of course.
```c
static bool fork_pipe_redirect(char **command, io pipes[2])
{
	if (fork() == CHILD)
		if (redirect_in(command) && connect(pipes) && redirect_out(command))
			return (true);
	return (false);
}

void execute(char **command)
{
	static io pipes[2];
	bool is_child_process;

	if (pipe_count && pipe(pipes[CURRENT]) == ERROR)
		exit_error("pipe");
	is_child_process = fork_pipe_redirect(command, pipes);
	if (is_child_process)
	{
		execvp(command[0], command);
		exit_error("execvp");
	}
	if (!is_background)
		while (wait(NULL) >= 0);
	close_(pipes);
	alternate((int **)pipes);
}
```

## pipe.c
the pipe part is where **my BRILLIANCE shines the most**.  
especially in `void alternate(int **)`.  
this function literally alternate pipes.  

`io` type a.k.a `int [2]` get casted to `int **` and handled correctly.  
```c
void alternate(int **pipes)
{
	int	*pipe_current;

	pipe_current = pipes[CURRENT];
	pipes[CURRENT] = pipes[PREVIOUS];
	pipes[PREVIOUS] = pipe_current;
}
```
thus making connecting and closing pipes get **extremely** easy and straightforward.  
```c
#define is_last_command (I == pipe_count)

bool connect(io pipes[2])
{
	if (pipe_count)
	{
		if (is_last_command || I != 0)
			dup2(pipes[PREVIOUS][READ], STDIN_FILENO);
		if (I == 0 || !is_last_command)
			dup2(pipes[CURRENT][WRITE], STDOUT_FILENO);
	}
	return (true);
}

void close_(io pipes[2])
{
	if (pipe_count)
	{
		if (is_last_command || I != 0)
			close(pipes[PREVIOUS][READ]);
		if (I == 0 || !is_last_command)
			close(pipes[CURRENT][WRITE]);
	}
}
```

## redirect.c
redirection is the most boring part.  
this itself is explanatory,  
but a slight variance here and there makes it difficult to shorten the code.
```c
static bool next_redirect_out(char ***arguments)
{
	while (**arguments)
	{
		if (is_same(**arguments, ">") || is_same(**arguments, ">>"))
			return (true);
		(*arguments)++;
	}
	return (false);
}

static void	redirect_out_to(const char *filename, int option)
{
	static int output;

	if (output)
		close(output);
	if (option == TRUNCATE)
		output = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0777);
	else
		output = open(filename, O_WRONLY | O_CREAT | O_APPEND, 0777);
	if (output == ERROR)
		exit_error("open");
	dup2(output, STDOUT_FILENO);
	close(output);
}

bool redirect_out(char **arguments)
{
	char *filename;

	while (next_redirect_out(&arguments))
	{
		filename = strdup(arguments[1]);
		if (is_same(arguments[0], ">"))
			redirect_out_to(filename, TRUNCATE);
		else if (is_same(arguments[0], ">>"))
			redirect_out_to(filename, APPEND);
		erase_from(arguments, 2);
		free(filename);
	}
	return (true);
}
```
redirect in is almost the same as above, but simpler due to the lack of append operator.  
(it's been quite long, but handling heredoc was not pleasant, though that is not shown here)  

## main
after that it's just combining them.
```c
int	main(int count, char **arguments)
{
	char *line;
	char mode;

	check(&mode, count, arguments);
	if (mode == INTERACTIVE)
		while (read_(&line, "tinyshell$ "))
			parse_and_run(line);
	else
		parse_and_run(strdup(arguments[2]));
}
```
