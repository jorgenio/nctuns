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

#include "entry_interface.h"


using namespace Ctrl_sch;


/*
 * Member function definitions of class `Entry_interface'.
 */

/*
 * Constructor
 */
Entry_interface::Entry_interface()
{
}

/*
 * Destructor
 */
Entry_interface::~Entry_interface()
{
}


/*
 * Member function definitions of class `Entry'.
 */

/*
 * Constructor
 */
Entry::Entry()
: _node_id(UNKNOWN_NODE_ID)
, _hops_to_nbr(UNKNOWN_HOPS_TO_NBR)
, _est_prop_delay(UNKNOWN_EST_PROP_DELAY)
, _next_tx_mx(UNKNOWN_XMT_MX)
, _tx_holdoff_exp(0)
{
}

/*
 * Destructor
 */
Entry::~Entry()
{
}
