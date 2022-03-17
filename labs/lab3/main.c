/*
* Description: Using Lab 1 and 2 to implement the lfcat function
*
* Author: Lukas Hanson
*
* Date: 2020/10/16
*
* Notes: None
*/

/*-------------------------Preprocessor Directives---------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "command.h"
/*---------------------------------------------------------------------------*/
/*-----------------------------Program Main----------------------------------*/
int main() {
	/* Main Function Variables */
	FILE* infile = stdin;
	FILE* outfile = stdout;
	size_t n = BUFSIZ;
	char* line = (char *) malloc(n * sizeof(char));
	char* token;
	int exit_flag = 0;

	freopen("output.txt", "w", outfile);

	while (1) {
		getline(&line, &n, infile);

		token = strtok(line, " ");

		if (*token == '\n') {
			continue;
		}

		while (token != NULL) {
			fprintf(outfile, "\n");

			if (token == NULL) {
				break;
			}

			else if (strncmp(token, "lfcat", 5) == 0) {
				lfcat();
				break;
			}

			else if (strncmp(token, "exit", 4) == 0) {
				exit_flag = 1;
				break;
			}

			else {
				fprintf(outfile, "Error: Unrecognized command!\n");
			}

			token = strtok(NULL, " ");
		}

		if (exit_flag) {
			break;
		}
	}

	free(line);

	return 1;
}
/*-----------------------------Program End-----------------------------------*/
