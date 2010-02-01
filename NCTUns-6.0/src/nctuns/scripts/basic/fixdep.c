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

/*
 * "Optimize" a list of dependencies as spit out by gcc -MD
 * for the nctuns build
 * ===========================================================================
 *
 * It is invoked as
 *
 *   fixdep <depfile> <target> <cmdline>
 *
 * and will read the dependency file <depfile>
 *
 * The transformed dependency snipped is written to stdout.
 *
 * It first generates a line
 *
 *   cmd_<target> = <cmdline>
 *
 * and then basically copies the .<target>.d file to stdout.
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <ctype.h>
#include <arpa/inet.h>

#define INT_CONF ntohl(0x434f4e46)

char *target;
char *depfile;
char *cmdline;

void usage(void)

{
	fprintf(stderr, "Usage: fixdep <depfile> <target> <cmdline>\n");
	exit(1);
}

/*
 * Print out the commandline prefixed with cmd_<target filename> :=
 */
void print_cmdline(void)
{
	printf("cmd_%s := %s\n\n", target, cmdline);
}

/* test is s ends in sub */
int strrcmp(char *s, char *sub)
{
	int slen = strlen(s);
	int sublen = strlen(sub);

	if (sublen > slen)
		return 1;

	return memcmp(s + slen - sublen, sub, sublen);
}

void parse_dep_file(void *map, size_t len)
{
	char *m = map;
	char *end = m + len;
	char *p;
	char s[PATH_MAX];

	p = strchr(m, ':');
	if (!p) {
		fprintf(stderr, "fixdep: parse error\n");
		exit(1);
	}
	memcpy(s, m, p-m); s[p-m] = 0;
	printf("deps_%s := \\\n", target);
	m = p+1;

	while (m < end) {
		while (m < end && (*m == ' ' || *m == '\\' || *m == '\n'))
			m++;
		p = m;
		while (p < end && *p != ' ') p++;
		if (p == end) {
			do p--; while (!isalnum(*p));
			p++;
		}
		memcpy(s, m, p-m); s[p-m] = 0;
		printf("  %s \\\n", s);
		m = p + 1;
	}
	printf("\n%s: $(deps_%s)\n\n", target, target);
	printf("$(deps_%s):\n", target);
}

void print_deps(void)
{
	struct stat st;
	int fd;
	void *map;

	fd = open(depfile, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "fixdep: ");
		perror(depfile);
		exit(2);
	}
	fstat(fd, &st);
	if (st.st_size == 0) {
		fprintf(stderr,"fixdep: %s is empty\n",depfile);
		close(fd);
		return;
	}
	map = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	if ((long) map == -1) {
		perror("fixdep: mmap");
		close(fd);
		return;
	}

	parse_dep_file(map, st.st_size);

	munmap(map, st.st_size);

	close(fd);
}

void traps(void)
{
	static char test[] __attribute__((aligned(sizeof(int)))) = "CONF";

	if (*(int *)test != INT_CONF) {
		fprintf(stderr, "fixdep: sizeof(int) != 4 or wrong endianess? %#x\n",
			*(int *)test);
		exit(2);
	}
}

int main(int argc, char *argv[])
{
	traps();

	if (argc != 4)
		usage();

	depfile = argv[1];
	target = argv[2];
	cmdline = argv[3];

	print_cmdline();
	print_deps();

	return 0;
}
