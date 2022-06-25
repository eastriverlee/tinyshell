#include "tinyshell.h"

static size_t current = 0;

static size_t count_(char **strings)
{
	size_t count = 0;

	while (strings[count])
		count++;
	return (count);
}

static void skip_delimiters(char const *s, char d)
{
	while (s[current] && s[current] == d)
		current++;
}

static size_t word_length(char const *s, char d)
{
	size_t	length = 0;

	while (s[current] && (s[current++] != d || !d))
		length++;
	return (length);
}

static char **assign_(char **words, char const *s, char d)
{
	size_t start, length, count;

	current = count = 0;
	skip_delimiters(s, d);
	while (s[current])
	{
		start = current;
		length = word_length(s, d);
		if (length)
			words[count++] = strndup(&s[start], length);
		skip_delimiters(s, d);
	}
	words[count] = NULL;
	return (words);
}

static char **get_word_spaces(char const *s, char d)
{
	size_t count;

	current = count = 0;
	skip_delimiters(s, d);
	while (s[current])
	{
		word_length(s, d);
		count++;
		skip_delimiters(s, d);
	}
	return (malloc(sizeof(char *) * ++count));
}

static char **split(char const *source, char delimiter)
{
	char **words = get_word_spaces(source, delimiter);

	return (assign_(words, source, delimiter));
}

char ***parse(char *line)
{
	char ***commands;
	char **splitted = split(line, '|');
	size_t count;

	count = count_(splitted);
	pipe_count = count - 1;
	commands = malloc(sizeof(char **) * (count + 1));
	commands[count] = NULL;
	while (count-- > 0)
		commands[count] = split(splitted[count], ' ');
	free_strings(splitted);
	free(line);
	return (commands);
}
