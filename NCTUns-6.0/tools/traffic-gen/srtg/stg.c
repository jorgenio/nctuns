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
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ctype.h>
#include <math.h>
#include <sys/time.h>
#include <netinet/tcp.h>
/*
 * add by bryan 05/12/30 
 */
#include <time.h>

#define MAX_RANDOM	0x7fffffff

char *data[500], *hostname = 0, *type, greedyLen[10240], *option, *dir;
int bw = 100, qlen = 50, delay = 0,
	port = 3000, repeat, pkt_no, tcp, dex, id = 0,
	alarmflag, countflag,
	totalsize = 0, writesize = 0, tcpwrite = 500, udpwrite = 500,
	SHOW = 0, BUG = 0, SELF = 0, TRACE = 0, ACTION = 0, TCPGREEDY = 0, UDPGREEDY = 0;
double on_time, wait_start_time = 0, send_time, *data2,
	tmin_ufo, tmax_ufo, exp_mean, exp_min, exp_max;
double *normalt;

/*
 * add by C.K. 06/12/26
 */
double hurst = 0.9;

struct sockaddr_in ser_addr, cli_addr;
struct timeval now;
extern int errno;

int usnd = 0;

char Usage[] = "\n\
Usage: stg  -modes  Hostname_or_IP  [-options]\n\
\n\
[-modes] ConfigFile mode:\n\
               -i ConfigFilename\n\
         Trace mode:\n\
               -trace TraceFilename\n\
         Self-similar mode:\n\
               -s AvgBw(kbyte/s) AvgPktSize(byte) Duration(second) LogFilename\n\
	       -H Hurst parameter (0.1 < H < 1.0, default 0.9)\n\
         TCP greedy mode:\n\
	       -t Duration(second)\n\
	 UDP greedy mode:(support broadcast)\n\
               -u PktSize(byte) Duration(second)\n\
               -m Bandwidth(Mbit/sec) MaxQueueLen(packets) (default 100 50)\n\
\n\
[-options]     -p ##   port number to send to (default 3000)\n\
               -v      view on screen\n\
               -seed   random seed (default current time)\n\
";


void read_file(char *);
void on_action(int);
void off_action(void);
double double_uniform(double, double);
int int_uniform(int, int);
double double_exponential(double, double, double);
void mysleep(double);
void myalarm(double);
void tcpudp_action(char *);
void gotalarm();
int writen(int, char *, int);
void Writen(int, char *, int);
void trace(char *);
int Nsendto(int, char *, int);
double gennor(double, double);
void SRA_FBM(double, int, double, double);
void self_similar(double, double, double, char *);
void errorExit(void);
void tcpGreedy(int);
void udpGreedy(int);
void greedyAction(void);
void mynormal(double, double, double *);

/************************************************* main *****************************************/
int main(int argc, char *argv[])
{
	int i, num = 0;
	double rate, avg_byte, period;
	char *trFile = NULL, *selfFile = NULL, *actionFile = NULL;

	dir = getenv("NCTUNS_WORKDIR");
	if (argc == 1)
		goto usage;

	for (i = 1; i < argc; i++) {

		if (strcmp(argv[i], "-i") == 0 && i < argc - 1) {

			ACTION = 1;
			i++;
			actionFile = argv[i];


		}
		else if (strcmp(argv[i], "-trace") == 0 && i < argc - 1) {

			TRACE = 1;
			i++;
			trFile = argv[i];

		}
		else if (strcmp(argv[i], "-s") == 0 && i < argc - 3) {

			SELF = 1;
			rate = atof(argv[++i]);
			avg_byte = atof(argv[++i]);
			period = atof(argv[++i]);
			selfFile = argv[++i];

			normalt = (double *)malloc((sizeof (double)) * 100000);
			mynormal(0, 1, normalt);

		}
		else if (strcmp(argv[i], "-H") == 0 && i < argc - 1) {
			i++;
			hurst = atof(argv[i]);

		}
		else if (strcmp(argv[i], "-p") == 0 && i < argc - 1) {

			i++;
			port = atoi(argv[i]);

		}
		else if (strcmp(argv[i], "-t") == 0 && i < argc - 1) {

			TCPGREEDY = 1;
			tcp = 1;
			tcpwrite = 8192;	/* set to const */
			on_time = atof(argv[++i]);

		}
		else if (strcmp(argv[i], "-u") == 0 && i < argc - 1) {

			UDPGREEDY = 1;
			tcp = 0;
			udpwrite = atoi(argv[++i]);
			on_time = atof(argv[++i]);
			delay = (qlen * udpwrite * 8) / bw;

		}
		else if (strcmp(argv[i], "-m") == 0 && i < argc - 1) {

			if (UDPGREEDY) {
				bw = atoi(argv[++i]);
				qlen = atof(argv[++i]);
				delay = (qlen * udpwrite * 8) / bw;	//micro-second
			}
			else {
				printf("[stg] -m option only in UDP greedy mode\n");
				exit(1);
			}

		}
		else if (strcmp(argv[i], "-v") == 0) {

			SHOW = 1;

		}
		else if (strcmp(argv[i], "-d") == 0) {

			BUG = 1;

		}
		else if (argv[i][0] != '-' && hostname == 0) {

			hostname = argv[i];

		}
		else if (strcmp(argv[i], "-seed") == 0 && i < argc - 1) {

			srandom((unsigned int)atoi(argv[++i]));

		}
		else {
			goto usage;
		}
	}

	if (port < 1024) {
		printf("\nPort number can't be less than 1024 !\n\n");
		exit(1);
	}
	else if (hostname == 0) {
		printf("\nHostname or IP should be set !\n\n");
		exit(1);
	}

	if (SELF) {
		if (rate < 0 || avg_byte < 0 || period < 0) {
			printf("\nAvgBw, AvgPktSize, and Duration can't be less than zero !\n\n");
			exit(1);
		}
		else if (avg_byte > 1500) {
			printf("\nAvgPktSize can't be more than 1500 bytes !\n\n");
			exit(1);
		}
		else if (hurst >= 1.0) {
			printf("\nHurst Parameter can't be greater than 1.0\n\n");
			exit(1);
		}
		else if (hurst <= 0.1) {
			printf("\nHurst Parameter can't be less than 0.1\n\n");
			exit(1);
		}
	}

	num = TRACE + SELF + ACTION + TCPGREEDY + UDPGREEDY;
	if (num > 1) {
		printf("\nCan not use more than one mode at one time !\n\n");
		goto usage;
	}
	else if (num <= 0) {
	      usage:printf("\n%s\n", Usage);
		exit(1);
	}

	if (ACTION)
		tcpudp_action(actionFile);
	if (TRACE)
		trace(trFile);
	if (SELF)
		self_similar(rate, avg_byte, period, selfFile);
	if (TCPGREEDY || UDPGREEDY)
		greedyAction();

	return 0;
}


/*********************************** read input file & set initial ******************************/
void read_file(char *input_file)
{
#define BUF_LEN		512
#define COMMAND_TOK	" \t:"

	int i = 0, j = 0;
	char str[BUF_LEN + 1];
	FILE *input_pt;
	char _input[512];

	if (dir)
		sprintf(_input, "%s/%s", dir, input_file);
	else
		sprintf(_input, "%s", input_file);
	input_pt = fopen(_input, "r");

	//  input_pt = fopen( input_file, "r");

	if (input_pt == NULL) {
		printf("Open ConfigFile fail ! \n");
		exit(1);
	}

	for (fgets(str, BUF_LEN, input_pt); !feof(input_pt); fgets(str, BUF_LEN, input_pt)) {
		char *tok;

		if ((tok = strchr(str, '\r')) || (tok = strchr(str, '\n')))
			*tok = '\0';

		if (strlen(str) == 0)
			continue;

		tok = strtok(str, COMMAND_TOK);
		while (tok) {
			if (!tok) {
				printf("ConfigFile descript error !\n");
				exit(1);
			}

			if (strcmp(tok, "type") == 0) {

				tok = strtok(NULL, COMMAND_TOK);
				if (strcmp(tok, "tcp") == 0)
					tcp = 1;
				else if (strcmp(tok, "udp") == 0)
					tcp = 0;
				else {
					printf("ConfigFile descript error !\n");
					exit(1);
				}

			}
			else if (strcmp(tok, "start_time") == 0) {

				tok = strtok(NULL, COMMAND_TOK);
				wait_start_time = atoi(tok);

			}
			else if (strcmp(tok, "on-off") == 0) {

				tok = strtok(NULL, COMMAND_TOK);
				repeat = atoi(tok);

			}
			else
				data[j++] = strdup(tok);

			tok = strtok(NULL, COMMAND_TOK);
		}
	}

	fclose(input_pt);

		/******* for debug ****************/
	if (BUG) {
		printf("%s\n", hostname);
		printf("%d\n", port);
		printf("type: %s\n", tcp == 0 ? "UDP" : "TCP");
		printf("%f\n", wait_start_time);
		printf("%d\n", repeat);
		for (i = 0; i < j; i++) {
			printf("%s\n", data[i]);
		}
	}
#undef BUF_LEN
#undef COMMAND_TOK
}

/************************************************  trace action *****************************/
void trace(char *trname)
{
	int trsfd, len, k, trtotalsize = 0;
	double inval, trstime;
	char buffer[2000];
	FILE *trpt;
	struct hostent *trhp;
	char _trinput[100];

	if (dir)
		sprintf(_trinput, "%s/%s", dir, trname);
	else
		sprintf(_trinput, "%s", trname);
	trpt = fopen(_trinput, "r");
	//trpt = fopen( trname, "r");

	if (trpt == NULL) {
		printf("Open TraceFile fail ! \n");
		exit(0);
	}

	if (SHOW || BUG) {
		if (TRACE && !SELF)
			printf("Trace file mode:\n");
	}

	bzero(&ser_addr, sizeof (ser_addr));
	ser_addr.sin_family = AF_INET;
	ser_addr.sin_port = htons(port);

	if (isdigit(hostname[0])) {
		ser_addr.sin_addr.s_addr = inet_addr(hostname);
		if (SHOW || BUG)
			printf("\nSend to: %s      port: %d\n\n", hostname, port);
	}
	else {
		if (SHOW || BUG)
			printf("\nSend to: %s      port: %d\n\n", hostname, port);

		trhp = gethostbyname(hostname);
		if (trhp == 0) {
			printf("client : %s not found !!\n", hostname);
			exit(1);
		}
		bcopy(trhp->h_addr_list[0], &ser_addr.sin_addr, 4);
		hostname = trhp->h_name;

		if (BUG)
			printf("%s  %s\n", inet_ntoa(ser_addr.sin_addr), hostname);
	}


	trsfd = socket(AF_INET, SOCK_DGRAM, 0);


	while (fscanf(trpt, "%d %lf", &len, &inval) != EOF) {
		id++;
		if (SHOW || BUG)
			printf("%7d   %6d      %4.6f    ", id, len, inval);
		if (len < 0 || inval < 0)
			errorExit();

		gettimeofday(&now, 0);
		trstime = (double)now.tv_sec + (double)now.tv_usec / 1000000.0;

		if (SHOW || BUG)
			printf("%10.6lf\n", trstime);

		sprintf(buffer, "%d+%lf+", id, trstime);
		Nsendto(trsfd, buffer, len);

		trtotalsize += len;
		mysleep(inval);
	}

	fclose(trpt);

	sprintf(buffer, "-1+%d+%d+", id, trtotalsize);
	for (k = 0; k < 50; k++) {
		Nsendto(trsfd, buffer, 50);
	}

}


/**************************************************** for udp & tcp action ****************************/
void tcpudp_action(char *in_file)
{
	int no, s, bufsize, j;
	char temp[500];
	struct hostent *hp;

	read_file(in_file);

	bzero(&ser_addr, sizeof (ser_addr));
	ser_addr.sin_family = AF_INET;
	ser_addr.sin_port = htons(port);

	if (isdigit(hostname[0])) {
		ser_addr.sin_addr.s_addr = inet_addr(hostname);
	}
	else {

		hp = gethostbyname(hostname);
		if (hp == 0) {
			printf("Client : %s not found !!\n", hostname);
			exit(1);
		}
		bcopy(hp->h_addr_list[0], &ser_addr.sin_addr, 4);
		hostname = hp->h_name;

		if (BUG)
			printf("%s  %s\n", inet_ntoa(ser_addr.sin_addr), hostname);
	}

	if (tcp) {

		s = socket(AF_INET, SOCK_STREAM, 0);
		if (s < 0) {
			perror(" Client : socket \n");
			exit(1);
		}

		bufsize = 1024 * 1024 * 1024;
		while (bufsize > 0) {
			if (setsockopt(s, SOL_SOCKET, SO_SNDBUF, &bufsize, sizeof (bufsize)) == 0) {
				if (!SHOW && BUG)
					printf("Final buf size is %d bytes \n", bufsize);
				break;
			}
			bufsize -= 1024;
		}

		if (bufsize <= 0) {
			perror(" Client(setsockopt error)");
			exit(1);
		}

		if (SHOW || BUG)
			printf("Send to %s        port: %d \n", hostname, port);

		if (connect(s, (struct sockaddr *)&ser_addr, sizeof (ser_addr)) < 0) {
			perror("Client(connect error)");
			exit(1);
		}
	}
	else {
		char *enable = (char *)1;

		s = socket(AF_INET, SOCK_DGRAM, 0);

		setsockopt(s, SOL_SOCKET, SO_BROADCAST, &enable, sizeof (enable));

		if (bind(s, (struct sockaddr *)&cli_addr, sizeof (cli_addr)) < 0) {
			perror("Client(bind error)");
			exit(1);
		}

	}


		/***** wait for start *****/
	if (wait_start_time > 0.0) {
		mysleep(wait_start_time);
		gettimeofday(&now, 0);
	}

		/***** start to action *****/
	for (no = 0; no < repeat; no++) {
		dex = 0;
		while (strcmp(data[dex], "end") != 0) {

			if (strcmp(data[dex], "on") == 0) {
				on_action(s);
				if (SHOW || BUG)
					printf("\n");
			}
			else if (strcmp(data[dex], "off") == 0) {
				off_action();
				if (SHOW || BUG)
					printf("\n");
			}
			dex++;
		}
	}
	//free(data);

	sprintf(temp, "-1+%d+%d", id, totalsize);
	/*
	 * if( tcp ){
	 * for( j=0 ; j<5 ; j++){
	 * if( write( s, temp, 30) <= 0 ){
	 * perror("Send start time error(write)");
	 * exit(1);
	 * }        
	 * }
	 * }
	 */
	if (!tcp) {
		sleep(5);
		for (j = 0; j < 5; j++) {
			Nsendto(s, temp, 50);
		}
	}
}

/************************************************************** on action ***********************************/
void on_action(int sfd)
{
	int writesize, len1, mean1, min1, max1;
	double t1, tmin_ufo, tmax_ufo, exp_mean, exp_min, exp_max;
	char buf[2000];

	dex++;
	if (strcmp(data[dex], "time") == 0) {
		on_time = (double)atof(data[++dex]);
		countflag = 1;
		if (SHOW || BUG)
			printf("on time: %10lf    ", on_time);
		if (on_time < 0.0)
			errorExit();

	}
	else if (strcmp(data[dex], "packet") == 0) {

		pkt_no = (int)atoi(data[++dex]);
		countflag = 0;

		if (SHOW || BUG)
			printf("pkt numbers: %d    ", pkt_no);
		if (pkt_no < 0)
			errorExit();

	}
	else {
		printf("ConfigFile descript error(1)\n");
		exit(1);
	}


	dex++;
	if (strcmp(data[dex], "const") == 0 && !tcp) {

		t1 = atof(data[++dex]);
		if (SHOW || BUG)
			printf("=> const  %lf   ", t1);
		if (t1 < 0.0)
			errorExit();

		if (strcmp(data[++dex], "length") == 0) {
			if (countflag) {
				dex++;
				if (strcmp(data[dex], "const") == 0) {
					len1 = atoi(data[++dex]);
					if (SHOW || BUG)
						printf("=> const   %d\n", len1);
					if (len1 <= 0)
						errorExit();
					do {

						gettimeofday(&now, 0);

						send_time =
							(double)now.tv_sec +
							(double)now.tv_usec / 1000000.0;
						id++;

						sprintf(buf, "%d+%17lf", id, send_time);

						writesize = Nsendto(sfd, buf, len1);

						totalsize += writesize;

						if (SHOW || BUG)
							printf("%17lf         %6d         %6d\n",
							       send_time, writesize, id);


						if ((on_time - t1) < 0) {
							mysleep(on_time);
							break;
						}
						else
							mysleep(t1);

						on_time -= t1;
					} while (on_time > 0);

				}
				else if (strcmp(data[dex], "uniform") == 0) {
					min1 = atoi(data[++dex]);
					max1 = atoi(data[++dex]);
					if (SHOW || BUG)
						printf("=> uniform   %d    %d\n", min1, max1);
					if (min1 <= 0 || max1 <= 0)
						errorExit();
					do {

						gettimeofday(&now, 0);

						send_time =
							(double)now.tv_sec +
							(double)now.tv_usec / 1000000.0;
						id++;
						len1 = int_uniform(min1, max1);

						sprintf(buf, "%d+%17lf", id, send_time);

						writesize = Nsendto(sfd, buf, len1);

						totalsize += writesize;
						if (SHOW || BUG)
							printf("%17lf         %6d         %6d\n",
							       send_time, writesize, id);

						if ((on_time - t1) < 0) {
							mysleep(on_time);
							break;
						}
						else
							mysleep(t1);

						on_time -= t1;
					} while (on_time > 0);

				}
				else if (strcmp(data[dex], "exponential") == 0) {
					mean1 = atoi(data[++dex]);
					min1 = atoi(data[++dex]);
					max1 = atoi(data[++dex]);
					if (SHOW || BUG)
						printf("=>exponential   %d  %d  %d\n",
						       mean1, min1, max1);
					if (min1 <= 0 || max1 <= 0 || mean1 <= 0)
						errorExit();
					do {

						gettimeofday(&now, 0);

						send_time =
							(double)now.tv_sec +
							(double)now.tv_usec / 1000000.0;
						id++;
						len1 = (int)double_exponential(mean1, min1, max1);
						sprintf(buf, "%d+%17lf", id, send_time);

						writesize = Nsendto(sfd, buf, len1);

						totalsize += writesize;
						if (SHOW || BUG)
							printf("%17lf         %6d         %6d\n",
							       send_time, writesize, id);

						if ((on_time - t1) < 0) {
							mysleep(on_time);
							break;
						}
						else
							mysleep(t1);

						on_time -= t1;
					} while (on_time > 0);

				}
				else {
					printf("descript file format error\n");
					exit(1);
				}
			}
			else {
				dex++;
				if (strcmp(data[dex], "const") == 0) {
					len1 = atoi(data[++dex]);
					if (SHOW || BUG)
						printf("=> const   %d\n", len1);
					if (len1 <= 0)
						errorExit();
					while (pkt_no > 0) {

						gettimeofday(&now, 0);

						send_time =
							(double)now.tv_sec +
							(double)now.tv_usec / 1000000.0;
						id++;
						sprintf(buf, "%d+%17lf", id, send_time);

						writesize = Nsendto(sfd, buf, len1);

						totalsize += writesize;
						if (SHOW || BUG)
							printf("%17lf         %6d         %6d\n",
							       send_time, writesize, id);

						mysleep(t1);
						pkt_no--;
					}
				}
				else if (strcmp(data[dex], "uniform") == 0) {
					min1 = atoi(data[++dex]);
					max1 = atoi(data[++dex]);
					if (SHOW || BUG)
						printf("=> uniform   %d   %d\n", min1, max1);
					if (min1 <= 0 || max1 <= 0)
						errorExit();
					while (pkt_no > 0) {
						gettimeofday(&now, 0);

						send_time =
							(double)now.tv_sec +
							(double)now.tv_usec / 1000000.0;
						id++;
						len1 = int_uniform(min1, max1);

						sprintf(buf, "%d+%17lf", id, send_time);

						writesize = Nsendto(sfd, buf, len1);

						totalsize += writesize;
						printf("%17lf         %6d         %6d\n",
						       send_time, writesize, id);

						mysleep(t1);
						pkt_no--;
					}
				}
				else if (strcmp(data[dex], "exponential") == 0) {
					mean1 = atoi(data[++dex]);
					min1 = atoi(data[++dex]);
					max1 = atoi(data[++dex]);
					if (SHOW || BUG)
						printf("=> exponential   %d  %d  %d\n",
						       mean1, min1, max1);
					if (mean1 <= 0 || min1 <= 0 || max1 <= 0)
						errorExit();
					while (pkt_no > 0) {
						gettimeofday(&now, 0);

						send_time =
							(double)now.tv_sec +
							(double)now.tv_usec / 1000000.0;
						id++;
						len1 = (int)double_exponential(mean1, min1, max1);
						sprintf(buf, "%d+%17lf", id, send_time);

						writesize = Nsendto(sfd, buf, len1);

						totalsize += writesize;
						if (SHOW || BUG)
							printf("%17lf         %6d         %6d\n",
							       send_time, writesize, id);

						mysleep(t1);
						pkt_no--;
					}
				}
				else {
					printf("ConfigFile descript error !\n");
					exit(1);
				}
			}

		}
		else {
			printf("ConfigFile descript error !\n");
			exit(1);
		}


	}
	else if (strcmp(data[dex], "uniform") == 0 && !tcp) {

		tmin_ufo = atof(data[++dex]);
		tmax_ufo = atof(data[++dex]);
		if (SHOW || BUG)
			printf("=> uniform   %lf   %lf\n", tmin_ufo, tmax_ufo);
		if (tmin_ufo < 0.0 || tmax_ufo < 0.0)
			errorExit();

		if (strcmp(data[++dex], "length") == 0) {
			if (countflag) {
				dex++;
				if (strcmp(data[dex], "const") == 0) {
					len1 = atoi(data[++dex]);
					if (SHOW || BUG)
						printf("                       => const   %d\n",
						       len1);
					if (len1 <= 0)
						errorExit();

					do {
						gettimeofday(&now, 0);

						send_time =
							(double)now.tv_sec +
							(double)now.tv_usec / 1000000.0;
						id++;
						sprintf(buf, "%d+%17lf+", id, send_time);

						writesize = Nsendto(sfd, buf, len1);

						totalsize += writesize;
						if (SHOW || BUG)
							printf("%17lf         %6d         %6d\n",
							       send_time, writesize, id);

						t1 = double_uniform(tmin_ufo, tmax_ufo);
						if ((on_time - t1) < 0) {
							mysleep(on_time);
							break;
						}
						else
							mysleep(t1);

						on_time -= t1;
					} while (on_time > 0);

				}
				else if (strcmp(data[dex], "uniform") == 0) {
					min1 = atoi(data[++dex]);
					max1 = atoi(data[++dex]);
					if (SHOW || BUG)
						printf("                       => uniform   %d  %d \n", min1, max1);
					if (min1 <= 0 || max1 <= 0)
						errorExit();
					do {

						gettimeofday(&now, 0);

						send_time =
							(double)now.tv_sec +
							(double)now.tv_usec / 1000000.0;
						id++;
						len1 = int_uniform(min1, max1);
						sprintf(buf, "%d+%17lf+", id, send_time);

						writesize = Nsendto(sfd, buf, len1);

						totalsize += writesize;
						if (SHOW || BUG)
							printf("%17lf         %6d         %6d\n",
							       send_time, writesize, id);

						t1 = double_uniform(tmin_ufo, tmax_ufo);
						if ((on_time - t1) < 0) {
							mysleep(on_time);
							break;
						}
						else
							mysleep(t1);

						on_time -= t1;
					} while (on_time > 0);

				}
				else if (strcmp(data[dex], "exponential") == 0) {
					mean1 = atoi(data[++dex]);
					min1 = atoi(data[++dex]);
					max1 = atoi(data[++dex]);
					if (SHOW || BUG)
						printf("                       => exponential   %d  %d  %d\n", mean1, min1, max1);
					if (mean1 <= 0 || min1 <= 0 || max1 <= 0)
						errorExit();

					do {
						gettimeofday(&now, 0);

						send_time =
							(double)now.tv_sec +
							(double)now.tv_usec / 1000000.0;
						id++;
						len1 = (int)double_exponential(mean1, min1, max1);
						sprintf(buf, "%d+%17lf", id, send_time);

						writesize = Nsendto(sfd, buf, len1);

						totalsize += writesize;
						if (SHOW || BUG)
							printf("%17lf         %6d         %6d\n",
							       send_time, writesize, id);

						t1 = double_uniform(tmin_ufo, tmax_ufo);
						if ((on_time - t1) < 0) {
							mysleep(on_time);
							break;
						}
						else
							mysleep(t1);

						on_time -= t1;
					} while (on_time > 0);
				}
				else {
					printf("ConfigFile descript error !\n");
					exit(1);
				}
			}
			else {
				dex++;
				if (strcmp(data[dex], "const") == 0) {
					len1 = atoi(data[++dex]);
					if (len1 <= 0)
						errorExit();
					if (SHOW || BUG)
						printf("                   => cosnt   %d\n", len1);
					while (pkt_no > 0) {

						gettimeofday(&now, 0);
						send_time =
							(double)now.tv_sec +
							(double)now.tv_usec / 1000000.0;
						id++;
						sprintf(buf, "%d+%17lf", id, send_time);

						writesize = Nsendto(sfd, buf, len1);

						totalsize += writesize;
						if (SHOW || BUG)
							printf("%17lf         %6d         %6d\n",
							       send_time, writesize, id);

						mysleep(double_uniform(tmin_ufo, tmax_ufo));
						pkt_no--;
					}
				}
				else if (strcmp(data[dex], "uniform") == 0) {
					min1 = atoi(data[++dex]);
					max1 = atoi(data[++dex]);
					if (SHOW || BUG)
						printf("                    => uniform   %d  %d\n",
						       min1, max1);
					if (min1 <= 0 || max1 <= 0)
						errorExit();
					while (pkt_no > 0) {
						gettimeofday(&now, 0);
						send_time =
							(double)now.tv_sec +
							(double)now.tv_usec / 1000000.0;
						id++;
						len1 = int_uniform(min1, max1);
						sprintf(buf, "%d+%17lf", id, send_time);


						writesize = Nsendto(sfd, buf, len1);

						totalsize += writesize;
						if (SHOW || BUG)
							printf("%17lf         %6d         %6d\n",
							       send_time, writesize, id);

						mysleep(double_uniform(tmin_ufo, tmax_ufo));
						pkt_no--;
					}
				}
				else if (strcmp(data[dex], "exponential") == 0) {
					mean1 = atoi(data[++dex]);
					min1 = atoi(data[++dex]);
					max1 = atoi(data[++dex]);
					if (SHOW || BUG)
						printf("                   => exponential   %d  %d  %d\n", mean1, min1, max1);
					if (mean1 <= 0 || min1 <= 0 || max1 <= 0)
						errorExit();
					while (pkt_no > 0) {
						gettimeofday(&now, 0);

						send_time =
							(double)now.tv_sec +
							(double)now.tv_usec / 1000000.0;
						id++;
						len1 = (int)double_exponential(mean1, min1, max1);
						sprintf(buf, "%d+%17lf", id, send_time);

						writesize = Nsendto(sfd, buf, len1);

						totalsize += writesize;
						if (SHOW || BUG)
							printf("%17lf         %6d         %6d\n",
							       send_time, writesize, id);

						mysleep(double_uniform(tmin_ufo, tmax_ufo));
						pkt_no--;
					}
				}
				else {
					printf("ConfigFile descript error !\n");
					exit(1);
				}
			}

		}
		else {
			printf("ConfigFile descript error !\n");
			exit(1);
		}

	}
	else if (strcmp(data[dex], "exponential") == 0 && !tcp) {
		exp_mean = atof(data[++dex]);
		exp_min = atof(data[++dex]);
		exp_max = atof(data[++dex]);
		if (SHOW || BUG)
			printf("=> exponential   %10lf   %10lf   %10lf\n",
			       exp_mean, exp_min, exp_max);
		if (exp_mean < 0.0 || exp_min < 0.0 || exp_max < 0.0)
			errorExit();

		if (strcmp(data[++dex], "length") == 0) {
			if (countflag) {
				dex++;
				if (strcmp(data[dex], "const") == 0) {
					len1 = atoi(data[++dex]);
					if (SHOW || BUG)
						printf("                       => const   %d\n",
						       len1);
					if (len1 <= 0)
						errorExit();
					do {

						gettimeofday(&now, 0);

						send_time =
							(double)now.tv_sec +
							(double)now.tv_usec / 1000000.0;
						id++;
						sprintf(buf, "%d+%17lf", id, send_time);

						writesize = Nsendto(sfd, buf, len1);

						totalsize += writesize;
						if (SHOW || BUG)
							printf("%17lf         %6d         %6d\n",
							       send_time, writesize, id);

						t1 = double_exponential(exp_mean, exp_min, exp_max);
						if ((on_time - t1) < 0) {
							mysleep(on_time);
							break;
						}
						else
							mysleep(t1);

						on_time -= t1;
					} while (on_time > 0);

				}
				else if (strcmp(data[dex], "uniform") == 0) {
					min1 = atoi(data[++dex]);
					max1 = atoi(data[++dex]);
					if (SHOW || BUG)
						printf("                       => uniform   %d  %d\n", min1, max1);
					if (min1 <= 0 || max1 <= 0)
						errorExit();

					do {

						gettimeofday(&now, 0);

						send_time =
							(double)now.tv_sec +
							(double)now.tv_usec / 1000000.0;
						id++;
						len1 = int_uniform(min1, max1);
						sprintf(buf, "%d+%17lf", id, send_time);

						writesize = Nsendto(sfd, buf, len1);

						totalsize += writesize;
						if (SHOW || BUG)
							printf("%17lf         %6d         %6d\n",
							       send_time, writesize, id);

						t1 = double_exponential(exp_mean, exp_min, exp_max);
						if ((on_time - t1) < 0) {
							mysleep(on_time);
							break;
						}
						else
							mysleep(t1);

						on_time -= t1;
					} while (on_time > 0);

				}
				else if (strcmp(data[dex], "exponential") == 0) {
					mean1 = atoi(data[++dex]);
					min1 = atoi(data[++dex]);
					max1 = atoi(data[++dex]);
					if (SHOW || BUG)
						printf("                       => exponential   %d  %d  %d\n", mean1, min1, max1);
					if (mean1 <= 0 || min1 <= 0 || max1 <= 0)
						errorExit();
					do {

						gettimeofday(&now, 0);

						send_time =
							(double)now.tv_sec +
							(double)now.tv_usec / 1000000.0;
						id++;
						len1 = (int)double_exponential(mean1, min1, max1);
						sprintf(buf, "%d+%17lf", id, send_time);

						writesize = Nsendto(sfd, buf, len1);

						totalsize += writesize;
						if (SHOW || BUG)
							printf("%17lf         %6d         %6d\n",
							       send_time, writesize, id);

						t1 = double_exponential(exp_mean, exp_min, exp_max);
						if ((on_time - t1) < 0) {
							mysleep(on_time);
							break;
						}
						else
							mysleep(t1);

						on_time -= t1;
					} while (on_time > 0);

				}
				else {
					printf("ConfigFile descript error !\n");
					exit(1);
				}
			}
			else {
				dex++;
				if (strcmp(data[dex], "const") == 0) {
					len1 = atoi(data[++dex]);
					if (SHOW || BUG)
						printf("                   => cosnt  %d\n", len1);
					if (len1 <= 0)
						errorExit();

					while (pkt_no > 0) {

						gettimeofday(&now, 0);

						send_time =
							(double)now.tv_sec +
							(double)now.tv_usec / 1000000.0;
						id++;
						sprintf(buf, "%d+%17lf", id, send_time);

						writesize = Nsendto(sfd, buf, len1);

						totalsize += writesize;
						if (SHOW || BUG)
							printf("%17lf         %6d         %6d\n",
							       send_time, writesize, id);

						mysleep(t1 =
							double_exponential(exp_mean, exp_min,
									   exp_max));
						pkt_no--;
					}
				}
				else if (strcmp(data[dex], "uniform") == 0) {
					min1 = atoi(data[++dex]);
					max1 = atoi(data[++dex]);
					if (SHOW || BUG)
						printf("                   => uniform   %d  %d\n",
						       min1, max1);
					if (min1 <= 0 || max1 <= 0)
						errorExit();

					while (pkt_no > 0) {
						gettimeofday(&now, 0);

						send_time =
							(double)now.tv_sec +
							(double)now.tv_usec / 1000000.0;
						id++;
						len1 = int_uniform(min1, max1);

						sprintf(buf, "%d+%17lf", id, send_time);

						writesize = Nsendto(sfd, buf, len1);

						totalsize += writesize;
						if (SHOW || BUG)
							printf("%17lf         %6d         %6d\n",
							       send_time, writesize, id);

						mysleep(t1 =
							double_exponential(exp_mean, exp_min,
									   exp_max));
						pkt_no--;
					}
				}
				else if (strcmp(data[dex], "exponential") == 0) {
					mean1 = atoi(data[++dex]);
					min1 = atoi(data[++dex]);
					max1 = atoi(data[++dex]);
					if (SHOW || BUG)
						printf("                   => exponential   %d  %d  %d\n", mean1, min1, max1);
					if (mean1 <= 0 || min1 <= 0 || max1 <= 0)
						errorExit();

					while (pkt_no > 0) {
						gettimeofday(&now, 0);

						send_time =
							(double)now.tv_sec +
							(double)now.tv_usec / 1000000.0;
						id++;
						len1 = (int)double_exponential(mean1, min1, max1);
						sprintf(buf, "%d+%17lf", id, send_time);

						writesize = Nsendto(sfd, buf, len1);

						totalsize += writesize;
						if (SHOW || BUG)
							printf("%17lf         %6d         %6d\n",
							       send_time, writesize, id);

						mysleep(t1 =
							double_exponential(exp_mean, exp_min,
									   exp_max));
						pkt_no--;
					}
				}
				else {
					printf("ConfigFile descript error !\n");
					exit(1);
				}
			}

		}
		else {
			printf("ConfigFile descript error !\n");
			exit(1);
		}


	}
	else if ((strcmp(data[dex], "greedy") == 0) && !tcp) {

		udpGreedy(sfd);

	}
	else if ((strcmp(data[dex], "greedy") == 0) && tcp) {

		tcpGreedy(sfd);

	}
	else {
		printf("ConfigFile descript error(2) !\n");
		exit(1);
	}

}

/*
 *  TCP & UDP greedy action
 */
void greedyAction(void)
{

	int s, bufsize, j;
	char temp[500], buf[9000];
	struct hostent *hp;

	bzero(&ser_addr, sizeof (ser_addr));
	ser_addr.sin_family = AF_INET;
	ser_addr.sin_port = htons(port);

	if (isdigit(hostname[0])) {
		ser_addr.sin_addr.s_addr = inet_addr(hostname);
	}
	else {

		hp = (struct hostent *)valloc(sizeof (struct hostent));
		hp = gethostbyname(hostname);
		if (hp == 0) {
			printf("Client : %s not found !!\n", hostname);
			exit(1);
		}
		bcopy(hp->h_addr_list[0], &ser_addr.sin_addr, 4);
		hostname = hp->h_name;

		if (BUG)
			printf("%s  %s\n", inet_ntoa(ser_addr.sin_addr), hostname);
	}

	if (tcp) {
		s = socket(AF_INET, SOCK_STREAM, 0);
		if (s < 0) {
			perror(" Client : socket \n");
			exit(1);
		}

		bufsize = 1024 * 1024;
		while (bufsize > 0) {
			if (setsockopt(s, SOL_SOCKET, SO_SNDBUF, &bufsize, sizeof (bufsize)) == 0) {
				if (!SHOW && BUG)
					printf("Final buf size is %d bytes \n", bufsize);
				break;
			}
			bufsize -= 1024;
		}

		if (bufsize <= 0) {
			perror(" Client(setsockopt error)");
			exit(1);
		}

		if (SHOW || BUG)
			printf("Send to %s        port: %d \n", hostname, port);

		if (connect(s, (struct sockaddr *)&ser_addr, sizeof (ser_addr)) < 0) {
			perror("Client(connect error)");
			exit(1);
		}
	}
	else {
		char *enable = (char *)1;

		s = socket(AF_INET, SOCK_DGRAM, 0);

		setsockopt(s, SOL_SOCKET, SO_BROADCAST, &enable, sizeof (enable));

		if (bind(s, (struct sockaddr *)&cli_addr, sizeof (cli_addr)) < 0) {
			perror("Client(bind error)");
			exit(1);
		}

	}


	gettimeofday(&now, 0);

		/***** start to action *****/
	if (TCPGREEDY) {

		tcpGreedy(s);

	}
	else if (UDPGREEDY) {

		if (SHOW || BUG)
			printf("=> const   %d\n", udpwrite);
		if (udpwrite <= 0)
			errorExit();

		alarmflag = 1;

		signal(SIGALRM, gotalarm);
		myalarm(on_time);
		while (alarmflag) {
			gettimeofday(&now, 0);

			send_time = (double)now.tv_sec + (double)now.tv_usec / 1000000.0;
			id++;

			sprintf(buf, "%d+%17lf", id, send_time);

			writesize = Nsendto(s, buf, udpwrite);
			if (usnd > qlen) {
				usnd = 0;
				usleep(delay);
			}
			else {
				usnd++;
			}

			totalsize += writesize;

			if (SHOW || BUG)
				printf("%17lf         %6d         %6d\n", send_time, writesize, id);
		}

	}

	sprintf(temp, "-1+%d+%d", id, totalsize);
	/*
	 * if( tcp ){
	 * for( j=0 ; j<5 ; j++){
	 * if( write( s, temp, 30) <= 0 ){
	 * perror("Send start time error(write)");
	 * exit(1);
	 * }        
	 * }
	 * }
	 */
	if (!tcp) {
		sleep(5);
		for (j = 0; j < 5; j++) {
			Nsendto(s, temp, 50);
		}
	}

}

/*
 * TCP greedy function
 */
void tcpGreedy(int tcpsfd)
{

	if (SHOW || BUG)
		printf("=> tcp greedy mode\n");
	alarmflag = 1;

	//kcliao for test
	signal(SIGALRM, gotalarm);
	myalarm(on_time);
	//alarm((int)on_time);

	while (alarmflag) {
		gettimeofday(&now, 0);

		send_time = (double)now.tv_sec + (double)now.tv_usec / 1000000.0;
		id++;
		if (BUG)
			printf("%lf    %d     %d\n", send_time, id, tcpwrite);
		//sprintf( greedyLen, "%d+%17lf+", id, send_time);

		Writen(tcpsfd, greedyLen, tcpwrite);
	}
	gettimeofday(&now, 0);
	if (BUG)
		printf("%17lf\n", (double)now.tv_sec + (double)now.tv_usec / 1000000.0);
}

/*
 * UDP greedy mode
 */
void udpGreedy(int udpsfd)
{

	int len1;

	if (SHOW || BUG)
		printf("=> udp greedy mode\n");
	dex += 2;
	if (strcmp(data[dex], "const") == 0) {
		len1 = atoi(data[++dex]);
		if (SHOW || BUG)
			printf("=> const  %d\n", len1);
		if (len1 <= 0)
			errorExit();


		if (countflag) {
			alarmflag = 1;

			signal(SIGALRM, gotalarm);
			myalarm(on_time);

			while (alarmflag) {
				gettimeofday(&now, 0);
				send_time = (double)now.tv_sec + (double)now.tv_usec / 1000000.0;
				id++;

				sprintf(greedyLen, "%d+%17lf+", id, send_time);

				writesize = Nsendto(udpsfd, greedyLen, len1);
				if (usnd > qlen) {
					usnd = 0;
					usleep(delay);
				}
				else {
					usnd++;
				}

				if (SHOW || BUG)
					printf("%17lf         %6d         %6d\n", send_time,
					       writesize, id);
				totalsize += writesize;
			}
		}
		else {
			while (pkt_no > 0) {
				gettimeofday(&now, 0);
				send_time = (double)now.tv_sec + (double)now.tv_usec / 1000000.0;
				id++;

				sprintf(greedyLen, "%d+%17lf+", id, send_time);

				writesize = Nsendto(udpsfd, greedyLen, len1);
				if (usnd > qlen) {
					usnd = 0;
					usleep(delay);
				}
				else {
					usnd++;
				}

				if (SHOW || BUG)
					printf("%17lf         %6d         %6d\n", send_time,
					       writesize, id);
				totalsize += writesize;
				pkt_no--;
			}
		}

	}
	else if (strcmp(data[dex], "uniform") == 0) {
		tmin_ufo = atoi(data[++dex]);
		tmax_ufo = atoi(data[++dex]);
		if (SHOW || BUG)
			printf("=> uniform   %f   %f\n", tmin_ufo, tmax_ufo);
		if (tmin_ufo <= 0 || tmax_ufo <= 0)
			errorExit();

		if (countflag) {
			alarmflag = 1;

			signal(SIGALRM, gotalarm);
			myalarm(on_time);

			while (alarmflag) {
				gettimeofday(&now, 0);
				send_time = (double)now.tv_sec + (double)now.tv_usec / 1000000.0;
				id++;
				len1 = int_uniform(tmin_ufo, tmax_ufo);

				sprintf(greedyLen, "%d+%17lf+", id, send_time);

				writesize = Nsendto(udpsfd, greedyLen, len1);
				if (usnd > qlen) {
					usnd = 0;
					usleep(delay);
				}
				else {
					usnd++;
				}

				if (SHOW || BUG)
					printf("%17lf         %6d         %6d\n", send_time,
					       writesize, id);
				totalsize += writesize;
			}
		}
		else {
			while (pkt_no > 0) {
				gettimeofday(&now, 0);
				send_time = (double)now.tv_sec + (double)now.tv_usec / 1000000.0;
				id++;
				len1 = int_uniform(tmin_ufo, tmax_ufo);

				sprintf(greedyLen, "%d+%17lf+", id, send_time);

				writesize = Nsendto(udpsfd, greedyLen, len1);

				if (SHOW || BUG)
					printf("%17lf         %6d         %6d\n", send_time,
					       writesize, id);
				totalsize += writesize;
				pkt_no--;
			}
		}

	}
	else if (strcmp(data[dex], "exponential") == 0) {
		exp_mean = atoi(data[++dex]);
		exp_min = atoi(data[++dex]);
		exp_max = atoi(data[++dex]);
		if (SHOW || BUG)
			printf("=> exponential   %f  %f  %f\n", exp_mean, exp_min, exp_max);
		if (exp_mean <= 0 || exp_min <= 0 || exp_max <= 0)
			errorExit();

		if (countflag) {
			alarmflag = 1;

			signal(SIGALRM, gotalarm);
			myalarm(on_time);

			while (alarmflag) {
				gettimeofday(&now, 0);
				send_time = (double)now.tv_sec + (double)now.tv_usec / 1000000.0;
				id++;
				len1 = (int)double_exponential(exp_mean, exp_min, exp_max);

				sprintf(greedyLen, "%d+%17lf+", id, send_time);

				writesize = Nsendto(udpsfd, greedyLen, len1);

				if (SHOW || BUG)
					printf("%17lf         %6d         %6d\n", send_time,
					       writesize, id);
				totalsize += writesize;
			}
		}
		else {
			while (pkt_no > 0) {
				gettimeofday(&now, 0);
				send_time = (double)now.tv_sec + (double)now.tv_usec / 1000000.0;
				id++;
				len1 = (int)double_exponential(exp_mean, exp_min, exp_max);

				sprintf(greedyLen, "%d+%17lf+", id, send_time);

				writesize = Nsendto(udpsfd, greedyLen, len1);

				if (SHOW || BUG)
					printf("%17lf         %6d         %6d\n", send_time,
					       writesize, id);
				totalsize += writesize;
				pkt_no--;
			}
		}

	}
	else {
		printf("ConfigFile descript error !\n");
		exit(1);
	}


}


/************************************************************** off action **********************/
void off_action(void)
{
	double waittime;

	dex += 2;
	waittime = atof(data[dex]);
	if (SHOW || BUG)
		printf("off time: wait for %lf seconds\n", waittime);
	if (waittime > 0)
		mysleep(waittime);
	if (SHOW || BUG)
		printf("\n");
}


/***************************************** generate uniform distribution( time )*********************/
double double_uniform(double min1, double max1)
{
	if (min1 > max1) {
		printf("usage:  uniform  min  max  =>  0 < min < max  \n");
		exit(1);
	}
	return ((min1 ==
		 max1) ? min1 : ((max1 -
				  min1) * (((double)random()) /
					   (double)((unsigned)MAX_RANDOM + 1)) + min1));
}

/********************************************** generate uniform distribution( packet )**********/
int int_uniform(int min2, int max2)
{
	if (min2 > max2) {
		printf("usage: uniform  min  max  =>  0 < min < max  \n");
		exit(1);
	}
	return ((min2 == max2) ? min2 : ((random() % (max2 - min2)) + min2));
}

/***************************************** generate exponential distribution( time )************/
double double_exponential(double mean1, double min1, double max1)
{
	double value;

	if (min1 > mean1 || mean1 > max1 || min1 > max1) {
		printf("usage: exponential  mean  min  max  => 0 < min < mean < max\n");
		exit(1);
	}

	do {
		value = -mean1 * log(((double)(random() + 1)) / (double)((unsigned)MAX_RANDOM + 1));
	} while ((value < min1) || (value > max1));

	return (value);
}


/*****************************  my sleep function : process double & int sleep time *********/
void mysleep(double stime)
{
	int my;

	stime *= 1000000;
	my = (int)(stime);
	if ((my % (1000000)) == 0)
		sleep(my / 1000000);
	else
		usleep(my);
}

/*************************  my alarm function : process double & int alarm time **************/
void myalarm(double otime)
{
	int mine;

	//kcliao__test
	//alarm((int)otime);
	//alarm(100);
	//return;

	otime *= 1000000;
	mine = (int)(otime);
	if ((mine % (1000000)) == 0) {
		alarm(mine / 1000000);
		if (BUG)
			printf("alarm signal\n");
	}
	else {
		ualarm(mine, 0);
		if (BUG)
			printf("ualarm signal\n");
	}
}

/************************************  get the alarm signal ********************************/
void gotalarm(sig)
{
	double realtime;

	alarmflag = 0;
	if (BUG) {
		printf("set alarmflag = 0 \n");
	}
	gettimeofday(&now, 0);
	realtime = (double)now.tv_sec + (double)now.tv_usec / 1000000.0;
	printf("[stg] now time is %f second ..\n", realtime);
}


/************************************ write "n" byte **************************************/
int writen(int fd, char *vptr, int n)
{
	int nleft, nwritten;
	char *ptr;

	ptr = vptr;
	nleft = n;
	while (nleft > 0) {
		if ((nwritten = write(fd, ptr, nleft)) <= 0) {
			if (errno == EINTR)
				nwritten = 0;
			else
				return (-1);
		}
		nleft -= nwritten;
		ptr += nwritten;
	}
	return (n);
}


void Writen(int fd, char *ptr, int nbytes)
{
	if (writen(fd, ptr, nbytes) != nbytes) {
		perror("Client(writen error)");
		printf("errno= %d\n", errno);
		exit(1);
	}
}

/********************************* Nsendto ************************************/
int Nsendto(int fd, char *buffer, int count)
{
	int cnt;

      again:
	cnt = sendto(fd, buffer, count, 0, (struct sockaddr *)&ser_addr, sizeof (ser_addr));

	if (cnt < 0 && errno == ENOBUFS) {
		//if( cnt==0 && errno == ENOBUFS ){
		//if( cnt == ENOBUFS ){
		printf("[stg] sendto return ENOBUFS\n");
		mysleep(0.001);
		errno = 0;
		goto again;
	}
	return (cnt);
}

/**********************************************************
* SRA algorithm
*
* Description :
* Saupe D. in Chapter 5 of "Fractals and Chaos"
* edited by A.J. Crilly and R.A. Earnshaw and H. Jones,
* Springer-Verlag, 1991.
**********************************************************
* data: real array of size 2 maxlevel + 1
* H: Hurst parameter (0 < H <1)
* maxlevel: maximum number of recursions
* M: mean; V: variance
**********************************************************/
void SRA_FBM(double H, int maxlevel, double M, double V)
{

/*********************************************
* i,j,d,dhalf,n,level: integers
* std: initial standard deviation
* Delta[]: array holding standard deviations
* gennor(M,V): normally distributed RNs using uniformly distributed RNs
**********************************************/
	int i = 0, j = 0, d = 0, dhalf = 0, n = 0, level = 0;
	double std = 0.0;
	double Delta[maxlevel];

	std = sqrt(1.0 - pow(2.0, (2 * H - 2)));
	for (i = 1; i <= maxlevel; i++) {
		Delta[i] = std * pow(0.5, (i * H)) * sqrt(0.5) * sqrt(1.0 - pow(2.0, (2 * H - 2)));
	}

	n = pow(2, maxlevel);
	data2[0] = 0.0;
	data2[n] = std * gennor(M, V);
	d = n;
	dhalf = d / 2;
	level = 1;
	while (level <= maxlevel) {
		for (i = dhalf; i <= (n - dhalf); i += d)
			data2[i] = 0.5 * (data2[i - dhalf] + data2[i + dhalf]);

		for (j = 0; j <= n; j += dhalf)
			data2[j] = data2[j] + Delta[level] * gennor(M, V);

		d = d / 2;
		dhalf = dhalf / 2;
		level = level + 1;
	}
}

double gennor(double mean, double var)
{
	double x = 0.0;
	int nor_idx = 0;

	x = (double)random() / (double)((unsigned)MAX_RANDOM + 1);
	nor_idx = (int)(x * 100000);

	return (normalt[nor_idx]);
}

void mynormal(double mean, double var, double *in_normal)
{
	double x = 0.0, y = 0.0, y2 = 0.0, step = 0.0, px = 0.0;
	int cy = 0, py = 0;
	int i = 0;

	step = (6 * var) / 1000;
	x = mean - 3 * var;
	px = x;
	while (i < 1000 && x <= (3 * var + mean)) {
		y = 1.0 / (var * sqrt(2 * M_PI)) * exp(-pow(x - mean, 2) / (2 * var * var));
		y2 = y2 + y * step;

		cy = (int)(y2 * 100000);
		while (py < cy) {
			in_normal[py] = px;
			py++;
		}
		py = cy;
		px = x;

		x += step;
		i++;
	}
	while (py < 100000) {
		in_normal[py] = mean + 3 * var;
		py++;
	}
	return;
}

/**************************** self-similar generator *************************/
void self_similar(double rate1, double avg_byte1, double period1, char *self_file)
{
	double min3 = 0.0, max3 = 0.0, scaling = 0.0, intval = 0.0, sum = 0.0;
	int i = 0, pkts = 0, level2 = 0, space = 0;
	char _self[300] = "";
	FILE *filept = NULL;

	intval = avg_byte1 / (double)(rate1 * pow(2, 10));
	pkts = (int)(period1 / intval);

	while (1) {
		if (pow(2, level2) > pkts)
			break;
		level2++;
	}
	space = pow(2, level2);

	if (SHOW || BUG)
		printf("Self-similar mode:\n");
	if (BUG)
		printf("intval: %lf     pkts: %d      level: %d    size: %d\n", intval, pkts,
		       level2, space);

	space = pow(2, level2);
	data2 = (double *)malloc(sizeof (double) * (space + 1));

	SRA_FBM(hurst, level2, 0, 1);

	for (i = 0; i < pkts; i++) {
		data2[i] = data2[i + 1] - data2[i];
	}

	for (i = 0; i < pkts; i++) {
		if (data2[i] > max3)
			max3 = data2[i];

		if (data2[i] < min3)
			min3 = data2[i];

		sum += data2[i];

	}

	if (dir)
		sprintf(_self, "%s/%s", dir, self_file);
	else
		sprintf(_self, "%s", self_file);

	filept = fopen(_self, "w");

	/*
	 * Modified by C.K. 06/12/26
	 */
	if (avg_byte1 >= 1160) {
		scaling = (1470 - avg_byte1) / max3;
		for (i = 0; i < pkts; i++) {
			data2[i] = (data2[i] * scaling) + avg_byte1;
			fprintf(filept, "%d  %lf\n", (int)data2[i], intval);
		}
	}
	else {
		for (i = 0; i < pkts; i++) {
			data2[i] = data2[i] - min3;
		}
		sum -= min3 * pkts;
		scaling = avg_byte1 / (sum / pkts);
		for (i = 0; i < pkts; i++) {
			data2[i] = data2[i] * scaling;
			data2[i] = (data2[i] > 1470) ? 1470 : data2[i];
			data2[i] = (data2[i] < 20) ? 20 : data2[i];
			fprintf(filept, "%d  %lf\n", (int)data2[i], intval);
		}
	}

	free(data2);
	fclose(filept);
	trace(self_file);

}

/*********************** process error input ********************************/
void errorExit(void)
{
	printf("\nConfigFile: Time value ,Pkt no. ,and Pkt size can't be equal to or less than zero !\n");
	exit(1);
}
