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

#ifndef __NCTUNS_rcst_queue_config_h__
#define __NCTUNS_rcst_queue_config_h__

#include <stdint.h>

#define CONFSTR_FLOW	"Flow"
#define CONFSTR_QUEUESTRATEGY   "QueueStrategy"
#define CONFSTR_RT      "RT"
#define CONFSTR_VRRT    "VR-RT"
#define CONFSTR_VRJT    "VR-JT"
#define CONFSTR_JT      "JT"

class Rcst_queue_basic;
/*
 * Rcst Queue Basic, this class will be inherited by all queue type instance
 */
class Rcst_queue_config {

public:
	/*
	 * struct of Flow configuration
	 */
	struct flow_conf {
		uint32_t	id;
		uint32_t	queue_id;
		uint32_t	src_ip;
		uint32_t	dst_ip;
		uint32_t	src_mask;
		uint32_t	dst_mask;
		uint32_t	src_port;
		uint32_t	dst_port;
		int		proto;
		struct flow_conf *next;
	};

	/*
	 * struct of QueueStrategy configuration
	 */
	struct queue_conf {
		uint32_t	id;
		uint32_t	queue_id;
		uint16_t	priority;
		enum Rcst_queue_basic::rcst_queue_type	type;
		uint32_t	queue_len; // Unit --> ATM cell.
		uint32_t	cra_rate; // Unit --> bits/s.
		uint32_t	max_rbdc_rate; // Unit --> bits/s.
		uint32_t	max_vbdc_rate; // Unit --> bits/s.
		struct queue_conf *next;
	};


private:
	/*
	 * private member
	 */
	struct flow_conf	*_flow_head;
	struct flow_conf	*_flow_tail;

	struct queue_conf	*_queue_head;
	struct queue_conf	*_queue_tail;

	uint32_t	_flow_cnt;
	uint32_t	_queue_cnt;

private:
	/*
	 * private function
	 */
	int	_parse_flow(char *buf);
	int	_parse_queue(char *buf);

	/*
	 * append linked list function for flow and queue
	 */
	void	_append_flow(struct flow_conf *flow);
	void	_append_queue(struct queue_conf *queue);

	/*
	 * free all linked list memory for flow and queue
	 */
	void	_free_flow_list(struct flow_conf *head);
	void	_free_queue_list(struct queue_conf *head);

	void	_parse_ip(uint32_t *sip, uint32_t *smask, char *ip);

public:
	/*
	 * public function
	 */
	Rcst_queue_config(char *filename);
	~Rcst_queue_config();

	int	parse_conf(char *filename);
	void	clean_conf();

	/*
	 * compare all flow rule, which can be matched
	 */
	int	match_flow_rule(uint32_t sip, uint16_t sport, uint32_t dip, uint16_t dport, int proto);

	/*
	 * get count of flow or queue rule
	 */
	inline uint32_t get_flow_rule_cnt() {
		return _flow_cnt;
	};

	inline uint32_t get_queue_rule_cnt() {
		return _queue_cnt;
	};

	/*
	 * get head of linked list
	 */
	inline struct flow_conf *get_flow_pointer() {
		return _flow_head;
	};

	inline struct queue_conf *get_queue_pointer() {
		return _queue_head;
	}
}; 

#endif	/* __NCTUNS_rcst_queue_config_h__ */
