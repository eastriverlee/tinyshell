#include "tinyshell.h"

static bool next_redirect_in(char ***arguments)
{
	while (**arguments)
	{
		if (is_same(**arguments, "<"))
			return (true);
		(*arguments)++;
	}
	return (false);
}

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

static void	redirect_in_from(const char *filename)
{
	static int input;

	if (input)
		close(input);
	input = open(filename, O_RDONLY, 0777);
	if (input == ERROR)
		exit_error("open");
	dup2(input, STDIN_FILENO);
	close(input);
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

bool redirect_in(char **arguments)
{
	char *filename;

	while (next_redirect_in(&arguments))
	{
		filename = strdup(arguments[1]);
		if (!filename)
			exit_error("redirect_in");
		redirect_in_from(filename);
		erase_from(arguments, 2);
		free(filename);
	}
	return (true);
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
