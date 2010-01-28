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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "../common/sch_info.h"
#include "../ret/queue/rcst_queue_basic.h"
#include "rcst_queue_config.h"

/*
 * Constructor
 */
Rcst_queue_config::Rcst_queue_config(char *filename)
: _flow_head(NULL), _flow_tail(NULL), _queue_head(NULL), _queue_tail(NULL),
_flow_cnt(0), _queue_cnt(0)
{
	/*
	 * nothing to do
	 */
	parse_conf(filename);
}


/*
 * Destructor
 */
Rcst_queue_config::~Rcst_queue_config()
{
	clean_conf();
}


/*
 * parse Rcst Queue configuration from the filename, it include flow and queue configuration
 */
int Rcst_queue_config::parse_conf(char *filename)
{
	FILE *fp;
	char buf[200];

	if (!filename)
		return (-1);
	/* open queue configure file */
	if (!(fp = fopen(filename, "r"))) {
		printf("[RCST_QUEUE_CONFIG] Warning: Cannot open file %s", filename);
		return (-1);
	}

	for (fgets(buf, 200, fp); !feof(fp); fgets(buf, 200, fp)) {
		/*
		 * Flow configuration
		 */
		if (!strncmp(buf, CONFSTR_FLOW ": ", 6))
			_parse_flow(buf);

		/*
		 * Queue configuration
		 */
		else if (!strncmp(buf, CONFSTR_QUEUESTRATEGY ": ", 15))
			_parse_queue(buf);
	}

	fclose(fp);
	return (0);
}


/*
 * parse configuration string of flow setting
 */
int Rcst_queue_config::_parse_flow(char *buf)
{
	char srcip[20], dstip[20], proto[10];
	char srcport[10], dstport[10];
	struct flow_conf *para = new struct flow_conf;
	struct protoent *ent = NULL;

	/*
	 * first token is AggregateFlowID
	 * sscanf will return the num of matching string. if
	 * this value is less than four, that's mean format
	 * fault.
	 */
	if (sscanf(buf, CONFSTR_FLOW ": "
		"%u_%[^_]_%[^_]_%[^_]_%[^_]_%[^_\n]",
		&para->queue_id, srcip, dstip,
		srcport, dstport, proto) != 6) {

		delete para;
		return (-1);
	}

	/*
	 * the queue_id zero is reserved for control queue so that can't be
	 * assigned to any flow configuration
	 */
	if (para->queue_id == 0) {
		delete para;
		return (-1);
	}

	para->src_port = htons(atoi(srcport));
	para->dst_port = htons(atoi(dstport));
	para->id = _flow_cnt++;
	para->next = NULL;

	/*
	 * parse src and dst ip, mask
	 */
	_parse_ip(&para->src_ip, &para->src_mask, srcip);
	_parse_ip(&para->dst_ip, &para->dst_mask, dstip);

	/*
	 * parase protocol, can't find it
	 */
	if (proto[0] == '*')
		para->proto = 0;
	else if (!(ent = getprotobyname(proto))) {
		delete para;
		return (-1);
	}
	else
		para->proto = ent->p_proto;

	_append_flow(para);

	return (0);
}


/*
 * parse configuration string of flow setting
 */
int Rcst_queue_config::_parse_queue(char *buf)
{
	char tmp1[50], tmp2[50], type[10], qlen[10];
	struct queue_conf *para = new struct queue_conf;

	/*
	 * first token is AggregateFlowID
	 * sscanf will return the num of matching string. if
	 * this value is less than four, that's mean format
	 * fault.
	 */
 
	if (sscanf(buf, CONFSTR_QUEUESTRATEGY ": "
		"%u_%[^_]_%[^_\n]_%[^_\n]_%[^\n]",
		&para->queue_id, type, qlen, tmp1, tmp2) < 3) {

		delete para;
		return (-1);
	}

	para->id = _queue_cnt++;
	para->priority = 0;
	para->max_vbdc_rate = 0;
	para->queue_len = atol(qlen);
	para->next = NULL;
	if (!strcmp(type, CONFSTR_RT)) {
		/*
		 * Parsing CRA rate
		 */
		sscanf(tmp1, "%u", &para->cra_rate);
		para->type = Rcst_queue_basic::RT;
	}
	else if (!strcmp(type, CONFSTR_JT)) {
		para->type = Rcst_queue_basic::JT;
	}
	else if (!strcmp(type, CONFSTR_VRRT)) {
		/*
		 * Parsing CRA and Max RBDC rate
		 */
		sscanf(tmp1, "%u", &para->cra_rate);
		sscanf(tmp2, "%u", &para->max_rbdc_rate);
		para->type = Rcst_queue_basic::VR_RT;
	}
	else if (!strcmp(type, CONFSTR_VRJT)) {
		/*
		 * Parsing RBDC rate
		 */
		para->type = Rcst_queue_basic::VR_JT;
		sscanf(tmp1, "%u", &para->max_rbdc_rate);
	}
	else {
		printf("[Rcst_queue_config] Warning: Cannot identify this queue type\n");

		delete para;
		return (-1);
	}

	_append_queue(para);
	return (0);
}


/*
 * append flow configuration struct into linked list
 */
void Rcst_queue_config::_append_flow(struct flow_conf *flow)
{
	if (!_flow_head)
		_flow_head = _flow_tail = flow;
	else {
		_flow_tail->next = flow;
		_flow_tail = flow;
	}
}


/*
 * append queue configuration struct into linked list
 */
void Rcst_queue_config::_append_queue(struct queue_conf *queue)
{
	struct queue_conf *head;

	if (!_queue_head)
		_queue_head = _queue_tail = queue;
	else {
		/*
		 * find whether exist duplication queue_id
		 */
		head = _queue_head;

		while (head) {
			if (queue->queue_id == head->queue_id) {
				printf("[Rcst_queue_config] "
					"duplication queue_id = %u\n",
					queue->queue_id);

				delete queue;
				return;
			}
			head = head->next;
		}
		_queue_tail->next = queue;
		_queue_tail = queue;
	}
}

void Rcst_queue_config::clean_conf()
{
	_free_flow_list(_flow_head);
	_free_queue_list(_queue_head);
	_flow_head = _flow_tail = NULL;
	_queue_head = _queue_tail = NULL;
}


/*
 * compare all flow rule, which can be matched, and return queue_id
 */
int Rcst_queue_config::match_flow_rule(uint32_t sip, uint16_t sport, uint32_t dip, uint16_t dport, int proto)
{
	struct flow_conf *rule;

	/*
	 * no any rules in linked list
	 */
	if (!(rule = _flow_head))
		return (0);

	/* to match every rule */
	while (rule) {
		/*
		 * compare protocol
		 */
		if (rule->proto != 0 && rule->proto != proto) {
			rule = rule->next;
			continue;
		}

		/*
		 * if source ip & distination ip is match this rule
		 */
		if ((sip & rule->src_mask) == rule->src_ip &&
			(dip & rule->dst_mask) == rule->dst_ip) {

			/*
			 * if rule->sport is zero or rule->dport is zero mean
			 * matched all port, else is matched specified port
			 *
			 * Here, ports will keep network-order scheme.
			 */
			if ((rule->src_port == 0 || sport == rule->src_port) &&
				(rule->dst_port == 0 || dport == rule->dst_port)) {

				return rule->queue_id;
			}
		}
		rule = rule->next;
	}

	/* can't match any rules, default deny it */
	return (0);
}


/*
 * free all entries in flow config linked list
 */
void Rcst_queue_config::_free_flow_list(struct flow_conf *head)
{
	struct flow_conf *flow;

	while (head) {
		flow = head;
		head = head->next;
		delete(flow);
	}
}


/*
 * free all entries in queue config linked list
 */
void Rcst_queue_config::_free_queue_list(struct queue_conf *head)
{
	struct queue_conf *queue;

	while (head) {
		queue = head;
		head = head->next;
		delete(queue);
	}
}

/*
 * handle to parse ip from configuration and store in struct sss_rule
 */
void Rcst_queue_config::_parse_ip(uint32_t *sip, uint32_t *smask, char *ip)
{
	char *ptr;
	int i, mask = 32;
	uint32_t ip_addr = 0;
	uint32_t ip_mask = 0;

	/*
	 * star is mean don't care
	 */
	if (ip[0] == '*') {
		*sip = 0;
		*smask = 0;
		return;
	}

	/* parse netmask */
	if ((ptr = strchr(ip, '/')) != NULL) {
		*ptr++ = '\0';
		mask = atoi(ptr);
	}

	/* transfer ip address */
	ip_addr = inet_addr(ip);

	/* transfer netmask */
	for (i = 0; i<mask ; i++) {
		ip_mask = ip_mask >> 1 | 0x80000000;
	}

	*smask = htonl(ip_mask);
	*sip = ip_addr & *smask;
}

