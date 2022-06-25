#include "tinyshell.h"

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
