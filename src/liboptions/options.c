/* command line options and config file parsing
 *
 * (C) 2018 by Andreas Eversberg <jolly@eversberg.eu>
 * All Rights Reserved
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "options.h"
#include "../libdebug/debug.h"

typedef struct option {
	struct option *next;
	int short_option;
	const char *long_option;
	int parameter_count;
} option_t;

static option_t *option_head = NULL;
static option_t **option_tailp = &option_head;
static int first_option = 1;

void option_add(int short_option, const char *long_option, int parameter_count)
{
	option_t *option;

	/* check if option already exists */
	for (option = option_head; option; option = option->next) {
		if (option->short_option == short_option
		 || !strcmp(option->long_option, long_option)) {
			PDEBUG(DOPTIONS, DEBUG_ERROR, "Option '%s' added twice, please fix!\n", option->long_option);
			abort();
		}
	}

	option = calloc(1, sizeof(*option));
	if (!option) {
		PDEBUG(DOPTIONS, DEBUG_ERROR, "No mem!\n");
		abort();
	}

	option->short_option = short_option;
	option->long_option = long_option;
	option->parameter_count = parameter_count;
	*option_tailp = option;
	option_tailp = &(option->next);
}

// FIXME: support more than one option */
int options_config_file(const char *config_file, int (*handle_options)(int short_option, int argi, char *argv[]))
{
	static const char *home;
	char config[256];
	FILE *fp;
	char buffer[256], opt[256], param[256], *p, *argv[1];
	int line;
	int rc = 1;
	int i;
	option_t *option;

	/* open config file */
	home = getenv("HOME");
        if (home == NULL)
		return 1;
	sprintf(config, "%s/%s", home, config_file + 2);
		
	fp = fopen(config, "r");
	if (!fp) {
		PDEBUG(DOPTIONS, DEBUG_INFO, "Config file '%s' seems not to exist, using command line options only.\n", config);
		return 1;
	}

	/* parse config file */
	line = 0;
	while((fgets(buffer, sizeof(buffer), fp))) {
		line++;
		/* prevent buffer overflow */
		buffer[sizeof(buffer) - 1] = '\0';
		/* cut away new-line and white spaces */
		while (buffer[0] && buffer[strlen(buffer) - 1] <= ' ')
			 buffer[strlen(buffer) - 1] = '\0';
		p = buffer;
		/* remove white spaces in front of first keyword */
		while (*p > '\0' && *p <= ' ')
			p++;
		/* ignore '#' lines */
		if (*p == '#')
			continue;
		/* get option form line */
		i = 0;
		while (*p > ' ')
			opt[i++] = *p++;
		opt[i] = '\0';
		if (opt[0] == '\0')
			continue;
		/* skip white spaces behind option */
		while (*p > '\0' && *p <= ' ')
			p++;
		/* get param from line */
		i = 0;
		while (*p > ' ')
			param[i++] = *p++;
		param[i] = '\0';
		/* search option */
		for (option = option_head; option; option = option->next) {
			if (opt[0] == option->short_option && opt[1] == '\0') {
				PDEBUG(DOPTIONS, DEBUG_INFO, "Config file option '%s' ('%s'), parameter '%s'\n", opt, option->long_option, param);
				break;
			}
			if (!strcmp(opt, option->long_option)) {
				PDEBUG(DOPTIONS, DEBUG_INFO, "Config file option '%s', parameter '%s'\n", opt, param);
				break;
			}
		}
		if (!option) {
			PDEBUG(DOPTIONS, DEBUG_ERROR, "Given option '%s' in config file '%s' at line %d is not a valid option, use '-h' for help!\n", opt, config_file, line);
			rc = -EINVAL;
			goto done;
		}
		if (option->parameter_count && !param[0]) {
			PDEBUG(DOPTIONS, DEBUG_ERROR, "Given option '%s' in config file '%s' at line %d requires %d parameter(s), use '-h' for help!\n", opt, config_file, line,  option->parameter_count);
			return -EINVAL;
		}
		argv[0] = strdup(param);
		rc = handle_options(option->short_option, 0, argv);
		if (rc <= 0)
			goto done;
		first_option = 0;
	}

done:
	/* close config file */
	fclose(fp);

	return rc;
}

int options_command_line(int argc, char *argv[], int (*handle_options)(int short_option, int argi, char *argv[]))
{
	option_t *option;
	char params[256];
	int argi, i;
	int rc;

	for (argi = 1; argi < argc; argi++) {
		if (argv[argi][0] == '-') {
			if (argv[argi][1] != '-') {
				if (strlen(argv[argi]) != 2) {
					PDEBUG(DOPTIONS, DEBUG_ERROR, "Given command line option '%s' exceeds one character, use '-h' for help!\n", argv[argi]);
					return -EINVAL;
				}
				/* -x */
				for (option = option_head; option; option = option->next) {
					if (argv[argi][1] == option->short_option) {
						if (option->parameter_count && argi + option->parameter_count < argc) {
							params[0] = '\0';
							for (i = 0; i < option->parameter_count; i++)
								sprintf(strchr(params, '\0'), " '%s'", argv[argi + 1 + i]);
							PDEBUG(DOPTIONS, DEBUG_INFO, "Command line option '%s' ('--%s'), parameter%s\n", argv[argi], option->long_option, params);
						} else
							PDEBUG(DOPTIONS, DEBUG_INFO, "Command line option '%s' ('--%s')\n", argv[argi], option->long_option);
						break;
					}
				}
			} else {
				/* --xxxxxx */
				for (option = option_head; option; option = option->next) {
					if (!strcmp(argv[argi] + 2, option->long_option)) {
						if (option->parameter_count && argi + option->parameter_count < argc) {
							params[0] = '\0';
							for (i = 0; i < option->parameter_count; i++)
								sprintf(strchr(params, '\0'), " '%s'", argv[argi + 1 + i]);
							PDEBUG(DOPTIONS, DEBUG_INFO, "Command line option '%s', parameter%s\n", argv[argi], params);
						} else
							PDEBUG(DOPTIONS, DEBUG_INFO, "Command line option '%s'\n", argv[argi]);
						break;
					}
				}
			}
			if (!option) {
				PDEBUG(DOPTIONS, DEBUG_ERROR, "Given command line option '%s' is not a valid option, use '-h' for help!\n", argv[argi]);
				return -EINVAL;
			}
			if (argi + option->parameter_count >= argc) {
				PDEBUG(DOPTIONS, DEBUG_ERROR, "Given command line option '%s' requires %d parameter(s), use '-h' for help!\n", argv[argi], option->parameter_count);
				return -EINVAL;
			}
			rc = handle_options(option->short_option, argi + 1, argv);
			if (rc <= 0)
				return rc;
			first_option = 0;
			argi += option->parameter_count;
		} else
			break;
	}

	/* no more options, so we check if there is an option after a non-option parameter */
	for (i = argi; i < argc; i++) {
		if (argv[i][0] == '-') {
			PDEBUG(DOPTIONS, DEBUG_ERROR, "Given command line option '%s' behind command line parameter '%s' not allowed! Please put all command line options before command line parameter(s).\n", argv[i], argv[argi]);
			return -EINVAL;
		}
	}

	return argi;
}

int option_is_first(void)
{
	return first_option;
}

