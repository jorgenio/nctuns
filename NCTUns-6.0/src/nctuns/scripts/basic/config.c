/*
 * Copyright (c) from 2000 to 2009
 * 
 * Network and System Laboratory 
 * Department of Computer Science 
 * College of Computer Science
 * National Chiao Tung University, Taiwan
 * All Rights Reserved.
 * 
 * This source code file is part of the NCTUns 6.0 network simulator.
 *
 * Permission to use, copy, modify, and distribute this software and
 * its documentation is hereby granted (excluding for commercial or
 * for-profit use), provided that both the copyright notice and this
 * permission notice appear in all copies of the software, derivative
 * works, or modified versions, and any portions thereof, and that
 * both notices appear in supporting documentation, and that credit
 * is given to National Chiao Tung University, Taiwan in all publications 
 * reporting on direct or indirect use of this code or its derivatives.
 *
 * National Chiao Tung University, Taiwan makes no representations 
 * about the suitability of this software for any purpose. It is provided 
 * "AS IS" without express or implied warranty.
 *
 * A Web site containing the latest NCTUns 6.0 network simulator software 
 * and its documentations is set up at http://NSL.csie.nctu.edu.tw/nctuns.html.
 *
 * Project Chief-Technology-Officer
 * 
 * Prof. Shie-Yuan Wang <shieyuan@csie.nctu.edu.tw>
 * National Chiao Tung University, Taiwan
 *
 * 09/01/2009
 */

#include <stdio.h>
#include <string.h>
#include <time.h>

#define PREFIX		"CONFIG_"

int main(int argc, char **argv)
{
	char buf[200], *ptr;
	FILE *fp;
	time_t time_sec;
	int i;

	if (argc < 2) {
		fprintf(stderr, "Usage: config <config_file>\n");
		return (1);
	}

	if ((fp = fopen(argv[1], "r")) == NULL) {
		fprintf(stderr, "Cannot open the file '%s'\n", argv[0]);
		return (1);
	}

	time(&time_sec);
	/* output header */
	printf(
		"/*\n"
		" * Automatically generated C config: don't edit\n"
		" * %s"
		" */\n", ctime(&time_sec));

	for (fgets(buf, 200, fp); !feof(fp); fgets(buf, 200, fp)) {
		i = 0;
		/* skip whitespace */
		while (buf[i] == ' ' || buf[i] == '\t')
			i++;

		/* skip the comment line or prefix isn't CONFIG_.... */
		if (buf[i] == '#' || (strncmp(&buf[i], PREFIX, 7))) {
			continue;
		}

		/* chop the newline charactor */
		if ((ptr = strchr(&buf[i], '\n')))
			*ptr = '\0';

		/* skip the non-integrity config line */
		if (!(ptr = strchr(&buf[i], '=')))
			continue;

		*ptr++ = '\0';

		/* skip whitespace */
		while(*ptr == ' ' || *ptr == '\t')
			ptr++;

		switch (ptr[i]) {
			case 'n':
				continue;
			case 'y':
				printf("#define %s 1\n", &buf[i]);
				break;
			default:
				printf("#define %s %s\n", &buf[i], ptr);
		};
	}

	fclose(fp);
	return (0);
}
