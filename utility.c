#include "tinyshell.h"

void exit_error(const char *source)
{
	perror(source);
	_exit(EXIT_FAILURE);
}

void free_strings(char **strings)
{
	size_t i = 0;

	while (strings[i])
		free(strings[i++]);
	free(strings);
}

static void pull_back(char **arguments)
{
	size_t i = 0;

	free(arguments[i]);
	do
		arguments[i] = arguments[i + 1];
	while (arguments[++i]);
}

void erase_from(char **strings, size_t count)
{
	size_t i = 0;

	while (*strings && i++ < count)
		pull_back(strings);
}
