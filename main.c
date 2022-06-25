#include "tinyshell.h"

int pipe_count;
int I;
bool is_background;

static void run(char ***commands)
{
	I = -1;
	while (commands[++I])
		if (*commands[I])
			execute(commands[I]);
	while (I-- > 0)
		free_strings(commands[I]);
	free(commands);
}

static bool read_(char **line, const char *prompt)
{
	size_t i = 0, max = 4096;
	char letter;

	printf("%s", prompt);
	*line = malloc(max);
	while ((letter = getchar()) != '\n')
	{
		if (letter == EOF)
			exit(EXIT_SUCCESS);
		if (i == max)
			(*line) = realloc(*line, max *= 2);
		(*line)[i++] = letter;
	}
	(*line)[i] = 0;
	return (true);
}

static void check(char *mode, int count, char **arguments)
{
	if (count == 1)
		*mode = INTERACTIVE;
	else if (count == 3 && is_same(arguments[1], "-c"))
		*mode = COMMANDLINE;
	else
		exit_error("check");
}

static void parse_and_run(char *line)
{
	char ***commands;

	is_background = line[strlen(line) - 1] == '&';
	if ((commands = parse(line)))
		run(commands);
}

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
