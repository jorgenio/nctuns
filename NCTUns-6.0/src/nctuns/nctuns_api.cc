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
#include <assert.h>
#include <math.h>
#include <sys/types.h>
#include <sys/uio.h>

#include <heap.h>
#include <regcom.h>
#include <mbinder.h>
#include <tclBinder.h>
#include <packet.h>
#include <config.h>
#include <gbind.h>


#include <tun_mmap.h>
#include <maptable.h>
#include <scheduler.h>
#include <ethernet.h>
#include <nctuns_api.h>
#include <nctuns-dep/node.h>
#include <antenna_pattern.h>
#include <tclBinder.h>
#include <tclObject.h>
#include <IPC/ns.h>

#if IPC == 0
#include <dispatcher.h>
#include <command_server.h>
#include <sysrt.h>
#endif

extern scheduler *scheduler_;
extern TclObject cmd_Interp_;
extern NslObject **nodelist;
extern RegTable         RegTable_;
extern GroupTable       grp_table;

char *script_ = 0;
char *FILEDIR_ = 0;


/*----------------------------------------------------------------------------
 * str_to_macaddr()
 *
 * Form a 48-bits IEEE 802 address with numerical representation
 * by analyzing input-string, which contains the ieee 802 address
 * with textual representation.
 *
 * Arguments:
 *      str             6 byte IEEE 802 address with textual representation
 *                      of address.
 *      mac             a ptr to 48-bits space to store ieee 802 address.
 *
 * Returns:
 *      nothing
 *
 * Side effects:
 *      the contents of the space pointed to by mac is filled with
 *      numerical representation of addr.
 *---------------------------------------------------------------------------*/
void str_to_macaddr(const char *str, u_char *mac) {

        u_int            tmp[20];

        sscanf(str, "%x:%x:%x:%x:%x:%x", tmp, (tmp+1), (tmp+ 2), (tmp+3),
               (tmp+4), (tmp+5));

        mac[0] = tmp[0]; mac[1] = tmp[1];
        mac[2] = tmp[2]; mac[3] = tmp[3];
        mac[4] = tmp[4]; mac[5] = tmp[5];
        return;
}


/*---------------------------------------------------------------------------
 * macaddr_to_str()
 *
 * Form an IEEE 802 mac address of the form xx:xx:xx:xx:xx:xx, which is
 * the textual representation by converting numerical representaion of
 * 48-bits ieee 802 mac address.
 *
 * Arguments:
 *      mac             ptr to a 48-bits ieee 802 address with
 *                      numerical representation.
 *      str             ptr to a buffer space to store the ieee 802
 *                      address with textual representation.
 *
 * Returns:
 *      nothing
 *
 * Side effects:
 *      the contents of the space pointed by str is filled
 *      with textual representation of ieee 802 mac addr.
 *---------------------------------------------------------------------------*/
void macaddr_to_str(u_char *mac, char *str) {

        sprintf(str, "%x:%x:%x:%x:%x:%x", mac[0], mac[1], mac[2], mac[3],
                mac[4], mac[5]);
        return;
}


/*---------------------------------------------------------------------------
 * ipv4addr_to_str()
 *
 * Form an IPv4 address of the form xx.xx.xx.xx, which is textual
 * representaion by converting numerical representation of IPv4 addr.
 *
 * Arguments:
 *      ipv4addr        an unsigned-long integer to store the IPv4
 *                      address with numerical representation of
 *                      4-bytes IPv4 addr.
 *      str             ptr to a buffer space to store the IPv4 addr.
 *                      with textual representation.
 *
 * Returns:
 *      nothing
 *
 * Side effects:
 *      the contents of the space pointed by str is filled with
 *      textual representation of IPv4 address.
 *---------------------------------------------------------------------------*/
void ipv4addr_to_str(u_long ipv4addr, char *str) {

        u_char          *p;

        p = (u_char *)&ipv4addr;
        sprintf(str, "%d.%d.%d.%d", p[0], p[1], p[2], p[3]);
        return;
}


/*---------------------------------------------------------------------------
 * str_to_ipv4addr()
 *
 * Form an IPv4 address with numerical representation by converting
 * textual representation of IPv4 address.
 *
 * Arguments:
 *      str             ptr to a IPv4 address with textual
 *                      representation.
 *      ipv4addr        an unsigned-long integer to store the IPv4
 *                      address with numerical representation.
 * Returns:
 *      nothing
 *
 * Side effects:
 *      the contents of ipv4addr variable is filled with the IPv4
 *      address with numerical representation.
 *---------------------------------------------------------------------------*/
void str_to_ipv4addr() {

}


/*---------------------------------------------------------------------------
 * vbind()
 *
 * Bind a variable to Tcl Script. After doing this function call, the
 * variable intented to be assigned value in Tcl Script will be handled
 * automatically.
 *
 * Arguments:
 *      obj             a ptr to a NslObject-type object, which indicates
 *                      which object has intention to bind a variable to
 *                      Tcl Script.
 *      name            a ptr to a space, which stores the variable name
 *                      used in Tcl Script.
 *      var             a ptr to an integer, which will be assigned valued
 *                      by Tcl Script.
 *
 * Returns:
 *      1               success
 *      < 0             failure
 *
 * Side effects:
 *      this request will be registered into the binding-table and in
 *      Tcl Script parsing phase upon matching the registered entry,
 *      the variable been registerd into binding-table will be
 *      assigned a value, which is specified in Tcl Script.
 *---------------------------------------------------------------------------*/
int vbind(NslObject *obj, const char *name, int *var) {

        *var = 0;
        return(cmd_Interp_.Bind_(obj, name, var));
}


/*---------------------------------------------------------------------------
 * vbind()
 *
 * Bind a variable to Tcl Script. After doing this function call, the
 * variable intented to be assigned value in Tcl Script will be handled
 * automatically.
 *
 * Arguments:
 *      obj             a ptr to a NslObject-type object, which indicates
 *                      which object has intention to bind a variable to
 *                      Tcl Script.
 *      name            a ptr to a space, which stores the variable name
 *                      used in Tcl Script.
 *      var             a ptr to an double floating-point, which will be
 *                      assigned valued by Tcl Script.
 *
 * Returns:
 *      1               success
 *      < 0             failure
 *
 * Side effects:
 *      this request will be registered into the binding-table and in
 *      Tcl Script parsing phase upon matching the registered entry,
 *      the variable been registerd into binding-table will be
 *      assigned a value, which is specified in Tcl Script.
 *---------------------------------------------------------------------------*/
int vbind(NslObject *obj, const char *name, double *var) {

        *var = 0.0;
        return(cmd_Interp_.Bind_(obj, name, var));
}


/*---------------------------------------------------------------------------
 * vbind()
 *
 * Bind a variable to Tcl Script. After doing this function call, the
 * variable intented to be assigned value in Tcl Script will be handled
 * automatically.
 *
 * Arguments:
 *      obj             a ptr to a NslObject-type object, which indicates
 *                      which object has intention to bind a variable to
 *                      Tcl Script.
 *      name            a ptr to a space, which stores the variable name
 *                      used in Tcl Script.
 *      var             a ptr to a floating-point , which will be assigned
 *                      valued by Tcl Script.
 *
 * Returns:
 *      1               success
 *      < 0             failure
 *
 * Side effects:
 *      this request will be registered into the binding-table and in
 *      Tcl Script parsing phase upon matching the registered entry,
 *      the variable been registerd into binding-table will be
 *      assigned a value, which is specified in Tcl Script.
 *---------------------------------------------------------------------------*/
int vbind(NslObject *obj, const char *name, float *var) {

        *var = 0.0;
        return(cmd_Interp_.Bind_(obj, name, var));
}


/*---------------------------------------------------------------------------
 * vbind()
 *
 * Bind a variable to Tcl Script. After doing this function call, the
 * variable intented to be assigned value in Tcl Script will be handled
 * automatically.
 *
 * Arguments:
 *      obj             a ptr to a NslObject-type object, which indicates
 *                      which object has intention to bind a variable to
 *                      Tcl Script.
 *      name            a ptr to a space, which stores the variable name
 *                      used in Tcl Script.
 *      var             a ptr to an continuous buffer space, which will be
 *                      assigned valued by Tcl Script.
 *
 * Returns:
 *      1               success
 *      < 0             failure
 *
 * Side effects:
 *      this request will be registered into the binding-table and in
 *      Tcl Script parsing phase upon matching the registered entry,
 *      the variable been registerd into binding-table will be
 *      assigned a value, which is specified in Tcl Script.
 *----------------------------------------------------------------------------*/
int vbind(NslObject *obj, const char *name, u_char *var) {

        *var = 0;
        return(cmd_Interp_.Bind_bool(obj, name, var));
}


/*---------------------------------------------------------------------------
 * vbind_bool()
 *
 * Bind a variable to Tcl Script. After doing this function call, the
 * variable intented to be assigned value in Tcl Script will be handled
 * automatically.
 *
 * Arguments:
 *      obj             a ptr to a NslObject-type object, which indicates
 *                      which object has intention to bind a variable to
 *                      Tcl Script.
 *      name            a ptr to a space, which stores the variable name
 *                      used in Tcl Script.
 *      var             a ptr to an unsigned-char, which will be
 *                      assigned valued by Tcl Script.
 *
 * Returns:
 *      1               success
 *      < 0             failure
 *
 * Side effects:
 *      this request will be registered into the binding-table and in
 *      Tcl Script parsing phase upon matching the registered entry,
 *      the variable been registerd into binding-table will be
 *      assigned a value, which is specified in Tcl Script.
 *---------------------------------------------------------------------------*/
int vbind_bool(NslObject *obj, const char *name, u_char *var) {

        *var = 0;
        return(cmd_Interp_.Bind_bool(obj, name, var));
}

/*---------------------------------------------------------------------------
 * vbind_ip()
 *
 * Bind a variable to Tcl Script. After doing this function call, the
 * variable intented to be assigned value in Tcl Script will be handled
 * automatically.
 *
 * Arguments:
 *      obj             a ptr to a NslObject-type object, which indicates
 *                      which object has intention to bind a variable to
 *                      Tcl Script.
 *      name            a ptr to a space, which stores the variable name
 *                      used in Tcl Script.
 *      var             a ptr to an unsigned-long integer, which will be
 *                      assigned valued by Tcl Script.
 *      Returns:
 *      1               success
 *      < 0             failure
 *
 * Side effects:
 *      this request will be registered into the binding-table and in
 *      Tcl Script parsing phase upon matching the registered entry,
 *      the variable been registerd into binding-table will be
 *      assigned a value, which is specified in Tcl Script.
 *---------------------------------------------------------------------------*/
int vbind_ip(NslObject *obj, const char *name, u_long *var) {

        *var = 0;
        return(cmd_Interp_.Bind_ip(obj, name, var));
}


/*---------------------------------------------------------------------------
 * vbind()
 *
 * Bind a variable to Tcl Script. After doing this function call, the
 * variable intented to be assigned value in Tcl Script will be handled
 * automatically.
 *
 * Arguments:
 *      obj             a ptr to a NslObject-type object, which indicates
 *                      which object has intention to bind a variable to
 *                      Tcl Script.
 *      name            a ptr to a space, which stores the variable name
 *                      used in Tcl Script.
 *      var             a double ptr to char, which will be
 *                      assigned valued by Tcl Script.
 *
 * Returns:
 *      1               success
 *      < 0             failure
 *
 * Side effects:
 *      this request will be registered into the binding-table and in
 *      Tcl Script parsing phase upon matching the registered entry,
 *      the variable been registerd into binding-table will be
 *      assigned a value, which is specified in Tcl Script.
 *---------------------------------------------------------------------------*/
int vbind(NslObject *obj, const char *name, char **var) {

        *var = 0;
        return(cmd_Interp_.Bind_(obj, name, var));
}


/*---------------------------------------------------------------------------
 * vbind_mac()
 *
 * Bind a variable to Tcl Script. After doing this function call, the
 * variable intented to be assigned value in Tcl Script will be handled
 * automatically.
 *
 * Arguments:
 *      obj             a ptr to a NslObject-type object, which indicates
 *                      which object has intention to bind a variable to
 *                      Tcl Script.
 *      name            a ptr to a space, which stores the variable name
 *                      used in Tcl Script.
 *      var             a ptr to a unsigned char, which will be
 *                      assigned valued by Tcl Script.
 *
 * Returns:
 *      1               success
 *      < 0             failure
 *
 * Side effects:
 *      this request will be registered into the binding-table and in
 *      Tcl Script parsing phase upon matching the registered entry,
 *      the variable been registerd into binding-table will be
 *      assigned a value, which is specified in Tcl Script.
 *---------------------------------------------------------------------------*/
int vbind_mac(NslObject *obj, const char *name, u_char *var) {

        bzero(var, 6);
        return(cmd_Interp_.Bind_mac(obj, name, var));
}



/*---------------------------------------------------------------------------
 * createEvent()
 *
 * Create an event structure. An event structure has the following fields:
 *   (1) timeStamp_     upon matching the system current time, this event
 *                      will be triggered and processed.
 *   (2) perio_         an periodically trigger time. if this field is none
 *                      NULL then this event will be triggered every perio_
 *                      time periodically.
 *   (3) DataInfo_      this is a pointer to "void" type variable, used to
 *                      carry any data if you want.
 *
 * Arguments:
 *      nothing
 *
 * Returns:
 *      ptr to an event                 if success.
 *      NULL                            if failure.
 *
 * Side effects:
 *      no side effect.
 *---------------------------------------------------------------------------*/
Event_ * createEvent() {

        Event_                  *ep;

        ep = (Event_ *)malloc(sizeof(Event_));
        assert(ep);


        /* Initialize the event structure */
        ep->created_nid_   = 10000;
        ep->created_ts_    = 0;
        ep->timeStamp_     = 0;
        ep->perio_         = 0;
        ep->priority_      = 0x40 & 0x3f;
        ep->calloutObj_    = NULL;
        ep->memfun_        = NULL;
        ep->func_          = NULL;
        ep->DataInfo_      = NULL;
        ep->next_ep        = NULL;
        ep->flag           = DEFAULT;
        ep->optval         = 0;

        return(ep);
}


/*---------------------------------------------------------------------------
 * setEventTimeStamp()
 *
 * This API will set the timeStamp field of event structure you created.
 * The timeStamp field should be set in clock tick when you insert an
 * event into the system event scheduler.
 *
 * Arguments:
 *      ep              a pointer to event structure.
 *      timeStamp       the time the event will be triggered and processed,
 *                      the time unit is clock tick.
 *      perio           the periodically time to trigger the event, also
 *                      its time unit is clock tick.
 *
 * Returns:
 *      1               for success
 *      < 0             for failure
 *
 * Side effects:
 *      the timeStamp_ field of event you assign as an argument will
 *      be assigned.
 *---------------------------------------------------------------------------*/
int setEventTimeStamp(Event_ *ep, u_int64_t timeStamp, u_int64_t perio) {

        //SET_EVENT_TIMESTAMP(ep, timeStamp, perio);
        if (ep != NULL) {
                ep->timeStamp_ = timeStamp;
                ep->perio_ = perio;
                return(1);
        }
        return(-1);
}


/*---------------------------------------------------------------------------
 * freeEvent()
 *
 * Upon finishing the use of event, this API should be always called to
 * release the space of event.
 *
 * Arguments:
 *      ep              a pointer to event structure.
 *
 * Returns:
 *      1               success
 *      < 0             failure
 *
 * Side effects:
 *      the space of event will be released, and the field DataInfo_
 *      also will be released if this event is attached a data into
 *      DataInfo_ field.
 *---------------------------------------------------------------------------*/
int freeEvent(Event_ *ep) {

        if (ep != NULL) {
                if (ep->DataInfo_) {
                        free((void *)ep->DataInfo_);
                        ep->DataInfo_ = 0;
                }
                free(ep);
                return(1);
        }
        return(-1);
}


/*---------------------------------------------------------------------------
 * setEventResue()
 *
 * Re-Insert existing event into event scheduler. If you want to use
 * this API, you should make sure that the perio_ field of event structure
 * is a non-zero value; otherwise, thie API call will not work. Whenever
 * an inserted event expires, the event scheduler will call the intended
 * function or member function of Object. Hence in the function or member
 * function of Object if you want to reuse the event without resetting its
 * contents, you can just call the setEventResue() API. But you should make
 * sure that the perio_ field is an non-zero value.
 *
 * Arguments:
 *      ep              a pointer to an event structure.
 *
 * Returns:
 *      1               success
 *      < 0             failure
 *
 * Side effects:
 *      the timeStamp field of event structure will be added by perio
 *      of event structure and re-insert into event scheduler.
 *---------------------------------------------------------------------------*/
int setEventReuse(Event_ *ep ) {

        if ((ep!=NULL)&&(ep->perio_>0)) {
                ep->timeStamp_ = GetCurrentTime() + ep->perio_;
                return(SCHEDULER_INSERT_EVENT(scheduler_, ep));
        }
        return(-1);
}


/*---------------------------------------------------------------------------
 * setEventCallOutFunc()
 *
 * Set the callout function of the event. This API will set a function
 * to be called upcon event time expired. The callout function should
 * of  - int (*func)(Event_ *) - type.
 *
 * Arguments:
 *      ep              a pointer to an event structure.
 *      fun             a function poiner to a function which has
 *                      int (*)(Event_ *) type.
 *      data            a pointer to void type, this arguments will be
 *                      assigned to DataInfo_ field of event structure
 *                      after this API call.
 * Returns:
 *      1               for success.
 *      < 0             for failure.
 *
 * Side effects:
 *      the contents of event will be assigned callout function and
 *      DataInfo_ field will be assigned.
 *---------------------------------------------------------------------------*/
int setEventCallOutFunc(Event_ *ep, int (*fun)(Event_ *), void *data) {

        if ((ep!=NULL)&&(fun!=NULL)) {
                ep->func_ = fun;
                ep->DataInfo_ = data;
                ep->calloutObj_ = NULL;
                return(1);
        }
        return(-1);
}


/*---------------------------------------------------------------------------
 * setEventCallOutObj()
 *
 * Set the callout Object of the event. This API will set the method of
 * Object inherited from NslObject object to be called upon the event
 * time expired. Before calling this API, it is should to declare the
 * variable - BASE_OBJTYPE(type) and call a micro POINTER_TO_MEMBER()
 * to caste the type of method to NslObject type.
 *
 * Arguments:
 *      ep              a pointer to an event structure.
 *      obj             a pointer to a NslObject Instance.
 *      memf            a pointer to a member function of Object, and
 *                      this argument should be in the following form:
 *                      int (NslObject::*memf)(Event_ *, void *)
 *
 * Returns:
 *      1               success
 *      < 0             failure
 *
 * Side effects:
 *      the contents of event structure will be set its callout object
 *      and the DataInfo_ field will be assigned.
 *---------------------------------------------------------------------------*/
int setEventCallOutObj(Event_ *ep, NslObject *obj,
                int (NslObject::*memf)(Event_ *), void *data)
{
        if ((ep!=NULL)&&(obj!=NULL)&&(memf!=NULL)) {
                ep->calloutObj_ = obj;
                ep->memfun_ = memf;
                ep->DataInfo_ = data;
                ep->func_ = NULL;
                return(1);
        }
        return(-1);
}


/*---------------------------------------------------------------------------
 * setFuncEvent()
 *
 * Fill all fields of event structure. This API is equivalent to
 * setEventTimeStamp() + setEventCallOutFunc().
 *
 * Arguments:
 *      ep              a pointer to an event structure.
 *      timeStamp       the time the event be triggered.
 *      perio           the periodical time the event be triggered.
 *      func            a pointer to a callout function, it should be in
 *                      the following form: int (*)(Event_ *)
 *      data            a pointer to void type to carry data if you want.
 *
 * Returns:
 *      1               success
 *      < 0             failure
 *
 * Side effects:
 *      all fields of event structure will be filled with corresponding
 *      arguments.
 *---------------------------------------------------------------------------*/
int setFuncEvent(Event_ *ep, u_int64_t timeStamp, u_int64_t perio,
                int (*func)(Event_ *), void *data)
{
        if ((ep!=NULL)&&(func!=NULL)) {
                ep->timeStamp_ = timeStamp;
                ep->perio_ = perio;
                ep->func_ = func;
                ep->DataInfo_ = data;
                ep->calloutObj_ = NULL;
                return(SCHEDULER_INSERT_EVENT(scheduler_, ep));
        }
        return(-1);
}


/*---------------------------------------------------------------------------
 * setObjEvent()
 *
 * Fill all fields of event structure. This API is equivalent to call
 * setEventTimeStamp() + setEventCallOutObj().
 *
 * Arguments:
 *      ep              a pointer to an event structure.
 *      timeStamp       the time the event be triggered.
 *      perio           the periodical time the event be triggered.
 *      obj             a pointer to a Object Instance inherited from
 *                      NslObject.
 *      memf            a pointer to a method of Object inherited from
 *                      NslObject. this argument should be in the following
 *                      form:  int (NslObject::*)(Event_ *)
 *      data            a pointer to void type to carry data if you want.
 *
 * Returns:
 *      1               success
 *      < 0             failure
 *
 * Side effects:
 *      all fields of event structure will be filled with corresponding
 *      arguments.
 *---------------------------------------------------------------------------*/
int setObjEvent(Event_ *ep, u_int64_t timeStamp, u_int64_t perio,
                NslObject *obj, int (NslObject::*memf)(Event_ *), void *data)
{
        if ((ep!=NULL)&&(obj!=NULL)&&(memf!=NULL)) {
                ep->timeStamp_ = timeStamp;
                ep->perio_ = perio;
                ep->calloutObj_ = obj;
                ep->memfun_ = memf;
                ep->DataInfo_ = data;
                ep->func_ = NULL;
                return(SCHEDULER_INSERT_EVENT(scheduler_, ep));
        }
        return(-1);
}

/*---------------------------------------------------------------------------
 * scheduleInsertEvent()
 *
 * Insert an event into event scheduler. After setting the event, this API
 * should be called to insert event into event scheduler. If you don't want
 * to use this API, you can use setFuncEvent() or setObjEvent() to set all
 * fields of event and automatically insert event into event scheduler.
 *
 * Arguments:
 *      ep              a pointer to an event structure.
 *
 * Returns:
 *      1               success
 *      < 0             failure
 *
 * Side effects:
 *      the event will be inserted into the event scheduler and upon
 *      event time expired, the callout function or callout member
 *      function of Object interited fomr NslObject will be called.
 *---------------------------------------------------------------------------*/
int scheduleInsertEvent(Event_ *ep) {

        if (ep != NULL)
                return(SCHEDULER_INSERT_EVENT(scheduler_, ep));
        else return(-1);
}



/*---------------------------------------------------------------------------
 * set_tuninfo()
 *
 * Set tunnel information. The tunnel information includes following
 * materials :
 *      node ID, port ID, tunnel ID, IPv4 address, netmask,
 *      ieee802 mac address.
 *
 * Arguments:
 *      nid             node ID the tunnel belongs to.
 *      portid          port ID the tunnel belongs to.
 *      tid             the tunnel ID you set.
 *      ip              the IPv4 address this tunnel associates with.
 *      netmask         the netmask of assigned IPv4 address.
 *      mac             the ieee802 mac address this tunnel associates with.
 *
 * Returns:
 *      1               success
 *      < 0             failure
 *
 * Side effects:
 *      the tunnel and above materials will be kept in a table, and you
 *      can query it once you need.
 *---------------------------------------------------------------------------*/
int set_tuninfo(u_int32_t nid, u_int32_t portid, u_int32_t tid,
                u_long *ip, u_long *netmask, u_char *mac)
{

        return(umtbl_add(nid, portid, tid, ip, netmask, mac));
}


/*---------------------------------------------------------------------------
 * RegToMBPoller()
 *
 * Register a polling Module-Binder Request to Module-Binder
 * Poller(MBP). The Module-Binder is a mechanism to bind two
 * module together. We know that the Node NCTUns is composed of modules
 * , and all modules which form a Node should be linked together. The
 * Module-Binder is to do this.
 *
 * Arguments:
 *      mbinder         a pointer to Module-Binder.
 *
 * Returns:
 *      1               success
 *      < 0             failure
 *
 * Side effects:
 *      the Module-Binder you specified will be added into the
 * Module-Binder Queue(MBQ), and will be polled periodically.
 * Upon existing packet in the Module-Binder, the MBP will try
 * to Module-Binder to push the packet to next module.
 *---------------------------------------------------------------------------*/
int RegToMBPoller(MBinder *mbinder) {
        // hwchu
        // Slot 0 of MBH is unused.
        extern MBinder          *MBH[];
        int                     idx = mbinder->bindModule()->get_nid();

        if (MBH[idx] == NULL) {
                // empty hash slot
                MBH[idx] = mbinder;
                mbinder->next_mb = NULL;
        } else {
                // non-empty hash slot
                MBinder *mbp = MBH[idx];

                // insert this mbinder to MBH if necessary
                while (1) {
                        if (mbp == mbinder) {
                                // already exists
                                break;
                        } else if (mbp->next_mb == NULL) {
                                // insert this mbinder
                                mbp->next_mb = mbinder;
                                mbinder->next_mb = NULL;
                                break;
                        } else {
                                mbp = mbp->next_mb;
                        }
                }
        }

        return 1;
}


/*---------------------------------------------------------------------------
 * display_layer3dev_info()
 *
 * Display the interface Information of Layer-3 Device. The Information
 * includes ipv4addr, ieee802 macaddr, port number......etc,.
 *
 * Arguments:
 *      nothing
 *
 * Returns:
 *      nothing
 *
 * Side effects:
 *      no side effects.
 *---------------------------------------------------------------------------*/
void display_layer3dev_info() {

        mtbl_display();
}


/*---------------------------------------------------------------------------
 * nodeid_to_ipv4addr()
 *
 * Use node ID to Query its IPv4 address. If a node has more than one
 * interface with IPv4 address, you should specify which port you want
 * to know its IPv4 address. The port ID is from 1 to n, here n is the
 * number of interfaces the node attached.
 *
 * Arguments:
 *      nid             node ID you want to query its IPv4 address.
 *      port            port ID.
 *
 * Returns:
 *      0               for failure.
 *      otherwise       a 4-bytes IPv4 IP address is returned.
 *
 * Side effects:
 *      no side effects.
 *---------------------------------------------------------------------------*/
u_int32_t nodeid_to_ipv4addr(u_int32_t nid, u_int32_t port) {

        return(mtbl_nidtoip(nid, port));
}


/*---------------------------------------------------------------------------
 * ipv4addr_to_nodeid()
 *
 * Use IPv4 address as a key to query node ID, which the IPv4 address belongs
 * to. Note! A node may have more than one IPv4 addr, but an IPv4 addr. should
 * belong to only one Node.
 *
 * Arguments:
 *      ip              the IPv4 address.
 *
 * Returns:
 *      0               failure
 *      otherwise       node ID
 *
 * Side effects:
 *      no side effect.
 *---------------------------------------------------------------------------*/
u_int32_t ipv4addr_to_nodeid(u_long ip) {

        return(mtbl_iptonid(ip));
}


/*---------------------------------------------------------------------------
 * macaddr_to_ipv4addr()
 *
 * Use ieee802 mac address as a key to query IPv4 address. Note, the
 * relation between ieee802 mac address and IPv4 address is
 * one-to-to mapping.
 *
 * Arguments:
 *      mac             ieee802 mac address
 *
 * Returns:
 *      0               failure
 *      otherwise       IPv4 address
 *
 * Side effects:
 *      no side effect.
 *---------------------------------------------------------------------------*/
u_long macaddr_to_ipv4addr(u_char *mac) {

        return(mtbl_mactoip(mac));
}


/*---------------------------------------------------------------------------
 * ipv4addr_to_macaddr()
 *
 * Use IPv4 address as a key to query ieee802 mac address. Note, the
 * relation between IPv4 address and ieee802 mac address is
 * one-to-one mapping.
 *
 * Arguments:
 *      ip              an IPv4 address
 *
 * Returns:
 *      NULL            failure
 *      otherwise       the ieee802 mac address.
 *
 * Side effects:
 *      no side effect.
 *---------------------------------------------------------------------------*/
u_char *ipv4addr_to_macaddr(u_long ip) {

        return(mtbl_iptomac(ip));
}


/*---------------------------------------------------------------------------
 * is_ipv4_broadcast()
 *
 * Check the given IPv4 address to see if it is a IPv4 broadcast
 * address.
 *
 * Arguments:
 *      id              node ID the IPv4 address belongs to.
 *      ip              IPv4 address you want to check.
 *
 * Returns:
 *      0               not an IPv4 broadcast addr.
 *      1               an IPv4 broadcast addr.
 *
 * Side effects:
 *      no side effect.
 *---------------------------------------------------------------------------*/
u_char is_ipv4_broadcast(u_int32_t id, u_long ip) {

        return(mtbl_isbroadcast(id, ip));
}


char *getifnamebytunid(u_int32_t tid) {

        return(mtbl_getifnamebytunid(tid));
}

u_int32_t getportbytunid(u_int32_t tid) {

        return(mtbl_getportbytunid(tid));
}



/*---------------------------------------------------------------------------
 * GetCurrentTime()
 *
 * Get the whole system current time. The returning time is in tick.
 * The difference between GetCurrentTime() and GetNodeCurrentTime() is
 * that the GetCurrentTime() returns the unique whole system current time,
 * but the GetNodeCurrentTime() only returns the specified node's current
 * time.
 *
 * Arguments:
 *      nothing
 *
 * Returns:
 *      the unique system current time in clock tick.
 *
 * Side effects:
 *      no any side effect.
 *---------------------------------------------------------------------------*/
u_int64_t GetCurrentTime() {

        extern u_int64_t        *currentTime_;
        return(currentTime_[0]);
}


/*---------------------------------------------------------------------------
 * GetNodeCurrentTime()
 *
 * Get the specified node's current time in clock tick. The difference
 * between GetCurrentTime() and GetNodeCurrentTime() is that the
 * GetCurrentTime() returns the unique whole system current time, but
 * the GetNodeCurrentTime() only returns the specified node's current
 * time.
 *
 * Arguments:
 *      nid             node ID of desire node's current time.
 *
 * Returns:
 *      specified node's current time in clock tick.
 *
 * Side effects:
 *      no side effect.
 *---------------------------------------------------------------------------*/
u_int64_t GetNodeCurrentTime(u_int32_t nid) {

        Node                    *node;

        node = (Node *)nodelist[nid];
        if (node == NULL)
                return(0);
        else return(node->getNodeClock());
}


/*---------------------------------------------------------------------------
 * GetSimulationTime()
 *
 * Get the time to run simulation.
 *
 * Arguments:
 *      nothing.
 *
 * Returns:
 *      the simulation time in clock tick.
 *
 * Side effects:
 *      no side effect.
 *---------------------------------------------------------------------------*/
u_int64_t GetSimulationTime() {

        extern u_int64_t        simTime_;
        return(simTime_);
}


/*------------------------------------------------------------------------
 * NCTUNS API: Tcl relative API
 *------------------------------------------------------------------------*/
NslObject *InstanceLookup(u_int32_t id, const char *name) {

        return( cmd_Interp_.table_lookup(id, name) );
}

NslObject *InstanceLookup(u_int32_t Nid, u_int32_t Pid, const char *Mname) {

        return(RegTable_.lookup_Instance(Nid, Pid, Mname));
}

NslObject *InstanceLookup(u_long ip, const char *Mname) {
        u_int32_t               Nid;
        u_int32_t               Pid;

        Nid = mtbl_iptonid(ip);
        Pid = mtbl_iptopid(ip);
        return(RegTable_.lookup_Instance(Nid, Pid, Mname));
}



/*---------------------------------------------------------------------------
 * pkt_copy()
 *
 * Duplicate an exist packet.
 *
 * Arguments:
 *      src             a pointer to a ePacket_ structure(event structure),
 *                      this packet is the packet which desired to be
 *                      duplicated.
 *
 * Returns:
 *      a pointer to a new duplicate-packet.
 *      NULL for replicate fail.
 *
 * Side effects:
 *      a new duplicate packet will be generated.
 *---------------------------------------------------------------------------*/
ePacket_ *pkt_copy(ePacket_ *src) {

        ePacket_                *dst;

        if (src != NULL) {
                //CREATE_EVENT(dst);
                dst = createEvent();
                dst->timeStamp_ = src->timeStamp_;
                dst->perio_ = src->perio_;
                dst->priority_ = src->priority_;
                dst->calloutObj_ = src->calloutObj_;
                dst->func_ = src->func_;
                dst->DataInfo_ = (void *)(((Packet*)(src->DataInfo_))->copy());
                dst->flag = src->flag;

                return(dst);
        }
        return(NULL);
}



/*---------------------------------------------------------------------------
 * createPacket()
 *
 * Generate a Event-Packet and set it's priority to level-2
 *
 * Arguments:
 *      NULL            for failure
 *      otherwise       an event-packet is returned.
 *
 * Returns:
 *
 *
 * Side effects:
 *      an event-packet will be generated.
 *---------------------------------------------------------------------------*/
ePacket_ *createPacket() {

        ePacket_                *epkt;
        Packet                  *pkt;

        /* create an event and a packet-object */
        epkt = createEvent();
        pkt = new Packet;

        /* attach packet-object to event to form a ePacket */
        ATTACH_PKT(pkt, epkt);

        /* set packet prioriry to level 2 */
        SET_MLEVEL_2(epkt);
        return(epkt);
}


/*---------------------------------------------------------------------------
 * freePacket()
 *
 * Release an exist-packet.
 *
 * Arguments:
 *      pkt             a pointer to ePacket_ structure(event structure).
 *
 * Returns:
 *      1               success
 *      < 0             failure
 *
 * Side effects:
 *      an exist-packet will be released.
 *---------------------------------------------------------------------------*/
int freePacket(ePacket_ *pkt) {

        Packet                  *p;

        if (pkt != NULL) {
                p = (Packet *)pkt->DataInfo_;
                if (p) {
//                      p->release();
                        delete p;
                }
                free(pkt);
                return(1);
        }
        return(-1);
}



/*----------------------------------------------------------------------
 * NCTUNS API: Variable Register Table
 *----------------------------------------------------------------------*/
void *get_regvar(u_int32_t nid, u_int32_t portid, const char *vname) {

        Node                    *node;

        node = (Node *)nodelist[nid];
        if (!node) return NULL;
        return(node->getRegVar(portid, vname));
}


void *get_regvar(u_int32_t nid, struct plist *pl, const char *vname) {

        Node                    *node;

        node = (Node *)nodelist[nid];
        if (!node) return NULL;
        return(node->getRegVar(pl, vname));
}

int reg_regvar(NslObject *obj, const char *vname, void *var) {

        Node                    *node;

        node = (Node *)nodelist[obj->get_nid()];
        if (!node) return 0;
        return(node->regVar(obj ,vname ,var));
}


/*---------------------------------------------------------------------------
 * GetNodeLoc()
 *
 * Get the current node's location. Every node in the simulation,
 * always has a location information. And the event scheduler will
 * update it periodically to.
 *
 * Arguments:
 *      nid             node ID which you want.
 *      x, y, z         this is a call by address, these three arguments
 *                      will be assigned node's x, y, z position respectively.
 *
 * Returns:
 *      always 1
 *
 * Side effect:
 *      the parameters you give the GetNodeLoc() as argument will be
 *      assigned node's current position.
 *---------------------------------------------------------------------------*/
int GetNodeLoc(u_int32_t nid, double &x, double &y, double &z) {

        /* Added by C.C. Lin:
         * For cases with the presence of supernodes, the inqueies for the 
         * location of device nodes should be replied with the location of 
         * its supernode. On the other hand, the node instance of device 
         * nodes are not created in such cases, it should be very careful 
         * to redirect the access of device nodes' "NODE" modules to modules
         * inside its correspondent supernode.
         */

        GT_Entry* tmp_gte_p = grp_table.get_entry(nid , SEARCH_BY_DNID );
 
        if (tmp_gte_p) {
		nid = tmp_gte_p->snid;
        }   

        ((Node *)nodelist[nid])->getNodePosition(x, y, z);
        return(1);
}

/*---------------------------------------------------------------------------
 * GetNodeAntenna()
 *
 * Get node's location of Antenna if the node attachs a wireless device;
 * or the results of node's position will be all-zero.
 *
 * Arguments:
 *      nid             node ID which you want.
 *      x, y, z         this is a call by address mechanism. after calling
 *                      this API, the x, y, z arguments will be assigned
 *                      node's x, y, z position.
 *
 * Returns:
 *      alwlays 1
 *
 * Side effects:
 *      the parameters you give the GetNodeAntenna() as argument will
 *      be assigned node's antenna position.
 *---------------------------------------------------------------------------*/
int GetNodeAntenna(u_int32_t nid, double &x, double &y, double &z) {

        ((Node *)nodelist[nid])->getNodeAntenna(x, y, z);
        return(1);
}

int GetNodeSpeed(u_int32_t nid , double & speed){
	((Node*)nodelist[nid])->getNodeSpeed(speed);
	return(1);	
}

int GetNodeAngle(u_int32_t nid , double & angle){
	((Node*)nodelist[nid])->getNodeAngle(angle);
	return(1);
}
/*---------------------------------------------------------------------------
 * GetScriptName()
 *
 * Get the Tcl Script file name.
 *
 * Arguments:
 *      nothing.
 *
 * Returns:
 *      a pointer to Tcl Script file name.
 *
 * Side effects:
 *      no side effect.
 *---------------------------------------------------------------------------*/
char *GetScriptName() {

	if(script_ == NULL) {
		fprintf(stderr, "GetScriptName() error\n");
		exit(0);
	} else {
        	return(script_);
	}
}

/*---------------------------------------------------------------------------
 * GetConfigFileDir()
 *
 * Get the directory of all configure files.
 *
 * Arguments:
 *      nothing.
 *
 * Returns:
 *      a pointer to the directory of all configure files.
 *
 * Side effects:
 *      no side effect.
 *---------------------------------------------------------------------------*/
char *GetConfigFileDir() {

	if(FILEDIR_ == NULL) {
		fprintf(stderr, "GetConfigFileDir() error\n");
		exit(0);
	} else {
        	return FILEDIR_;
	}
}
/*---------------------------------------------------------------------------
 * getNumOfNodes()
 *
 * Get the total number of nodes in a simulation environment.
 *
 * Arguments:
 *      nothing.
 *
 * Returns:
 *      the total number of nodes.
 *
 * Side effects:
 *      no side effect.
 *---------------------------------------------------------------------------*/
extern u_int32_t        __TotalNodes__;

u_int32_t getNumOfNodes() {

        return(__TotalNodes__);
}

/*------------------------------------------------------------------------
 * NCTUNS API: Dispatcher
 *------------------------------------------------------------------------*/
#include <dispatcher.h>

extern Dispatcher       *dispatcher_;

int nctuns_export(NslObject *modu, const char *name, u_char flags) {

        return(dispatcher_->reg_export(modu, name, flags));
}

void export_set_success() {

        dispatcher_->SetSuccess();
}

void export_get_success(struct ExportStr *ExpStr) {

        dispatcher_->GetSuccess();
        dispatcher_->GetExpStr(ExpStr);
}


/*------------------------------------------------------------------------
 * NCTUNS API: Node type
 *------------------------------------------------------------------------*/
#include <nodetype.h>
extern typeTable       *typeTable_;

const char *getTypeName(NslObject *node) {

        return(typeTable_->toName(node->get_type()));
}

const char *getNodeName(u_int32_t nid) {

        if (nid <= MAX_NUM_NODE) {
                if (nodelist[nid])
                        return(nodelist[nid]->get_name());
        }
        return(0);
}

u_char getNodeLayer(u_int32_t nid) {

        if (nid <= MAX_NUM_NODE) {
            if (nodelist[nid])
                return(typeTable_->TypeToLayer(nodelist[nid]->get_type()));
        }
        return(0);
}


/*----------------------------------------------------------------------------
 * getModuleName()
 *
 * Get Instance's Module Name. For every Instance inherited from NslObject
 * object always has a Module Name to indicate that this Instance belongs to
 * which kind of Module.
 *
 * Arguments:
 *      obj             instance interited from NslObject type.
 *
 * Returns:
 *      NULL            failure.
 *      otherwise       a pointer to Module Name.
 *
 * Side effects:
 *      no side effect
 *---------------------------------------------------------------------------*/

const char *getModuleName(const NslObject *obj) {

        struct Instance         *inst;
        struct Module_Info      *minfo;

        inst = RegTable_.get_instanceInfo(obj);
        if (inst == NULL) return(NULL);

        minfo = inst->mInfo_;
        assert(minfo);
        return(minfo->cname_);
}


/*---------------------------------------------------------------------------
 * getConnectNode()
 *
 * Get one node's neighbor node ID, which is directly connected by
 * one port of query node.
 *
 * Arguments:
 *      nid             query node ID.
 *      portid          port ID of query node.
 *
 * Returns:
 *      > 0             neighbor node ID, which is directly connected
 *                      by one port of query node.
 *      = 0             failure.
 *
 * Side effects:
 *      no side effect.
 *---------------------------------------------------------------------------*/
#include <nctuns-dep/link.h>

Link    *__loc__ = new Link(0, 0, 0, " ");

u_int32_t getConnectNode(u_int32_t nid, u_int32_t portid) {

        return(__loc__->getConnectNode(nid, portid));
}


/*---------------------------------------------------------------------------
 * macaddr_to_nodeid()
 *
 * Use ieee802 mac address as a key to find corresponding Node ID.
 * Note that the relation between ieee802 mac address and Node ID
 * is many-to-one mapping.
 *
 * Arguments:
 *      mac             ieee802 mac address
 *
 * Returns:
 *      0               failure
 *      otherwise       node ID
 *
 * Side effects:
 *      no side effect.
 *---------------------------------------------------------------------------*/
u_int32_t macaddr_to_nodeid(u_char *mac) {

        return(__loc__->getNodeIDbymac(mac));
}


/*---------------------------------------------------------------------------
 * reg_IFpolling()
 *
 * Register a tunnel interface to IF-Polling queue to be polled.
 * Every tunnel be used should be registered to IF-Polling so that
 * the packet from kernel can be read into the module which uses
 * that tunnel. This is to simulate a packet is sent by interface.
 *
 * Arguments:
 *      obj             a pointer to an Instance which uses the
 *                      tunnel.
 *      meth            the member function of module. this member
 *                      function will be called when IF-Poller polls
 *                      the tunnel which belongs to that module.
 *      fd              this is a pointer to file descriptor number.
 *                      upon completing register, the IF-Poller will
 *                      create a file descriptor number to fd varivable.
 *                      this fd will be the file descriptor of opened
 *                      tunnel.
 *
 * Returns:
 *      0               failure.
 *      otherwise       the tunnel ID the IF-Poller create for this request.
 *
 * Side effects:
 *      the IF-Polling Queue will be insert a new polling request, and
 *      a tunnel pseudo device will be opened. after completing register, upon
 *      packet coming from kernel, the IF-Poller will callout to the
 *      module which has asked for poller request. and the fd parameter
 *      will be assigned the file descriptor number.
 *---------------------------------------------------------------------------*/
u_long reg_IFpolling(NslObject *obj, int (NslObject::*meth)(Event_ *), int *fd)
{
        return(reg_poller(obj, meth, fd));
}


/*---------------------------------------------------------------------------
 * tun_write()
 *
 * Write a packet into tunnel pseudo device to simulate packet
 * reception. Before using this API, please make sure that you have
 * registered a tunnel pseudo device to IF-Poller.
 *
 * Arguments:
 *      tunfd           tunnel file descriptor. this is assigned by
 *                      IF-Poller when you ask for a polling request.
 *      pkt             a pointer to ePacket_, which encapsulate the
 *                      packet you want to write to tunnel.
 *
 * Returns:
 *      < 0             failure.
 *      otherwise       the number of chars been written to tunnel.
 *
 * Side effects:
 *      the kernel will receive a packet and kernel might do some
 *      corresponding action, eg, reply ACK frame......etc,.
 *---------------------------------------------------------------------------*/
int tun_write(int tunfd, ePacket_ *pkt) {

        int             n;
        char            *ptr;
        Packet          *p;


        if ((tunfd==0)||(pkt==NULL)||(pkt->DataInfo_==NULL))
                return(-1);

        p = (Packet *)pkt->DataInfo_;
        ptr = p->pkt_aggregate();
        n = write(tunfd, ptr, p->pkt_getlen());
        return(n);
}


/*---------------------------------------------------------------------------
 * tun_read()
 *
 * Read a packet from tunnel pseudo-device. This is to simulate
 * transmitting packet. Before using this API, please make sure that
 * you have registered a tunnel pseudo-device to IF-Poller.
 *
 * Arguments:
 *      tunfd           file descriptor to opened tunnel.
 *      pkt             a pointer to ePacket_ structure.
 *
 * Returns:
 *      -1              illegal tunfd or packet.
 *      -2              packet already attached a PT_SDATA.
 *      -3              read error
 *      otherwise       the number of chars read from tunnel.
 *
 * Side effects:
 *      a packet will be read from tunnel pseudo-device.
 *---------------------------------------------------------------------------*/
int tun_read(int tunfd, ePacket_ *pkt) {

        Packet                  *p;
        char                    *buf, *ptr;
        int                     n;
        u_long                  gw;


        if ((pkt==NULL)||(pkt->DataInfo_==NULL)||(tunfd==0))
                return(-1);

        /* try to attach a PT_SDATA pbuf */
        p = (Packet *)pkt->DataInfo_;
        if ((buf = p->pkt_sattach(MAXPACKETSIZE)) == 0)
                return(-2);

        /*
         * Why from buf-sizeof(struct tether_header) !!??
         *     When we use pkt_sattach() method to malloc
         *     memory space, the space always be allocated
         *     (len+PLEN) bytes. And when we read data from
         *     tunnel device, the data is a ether-frame. The
         *     header of this ether-frame is only used to
         *     do routing or arp-request, so we don't take
         *     the ether header into consideration.
         */
        n = read(tunfd, buf-sizeof(struct ether_header), MAXPACKETSIZE);
        if (n>=0) {
                p->pkt_sprepend(buf, n-sizeof(struct ether_header));
                p->pkt_setflow(PF_SEND);

                /*
                 * Because the kernel routing information is
                 * hid in the ether header of read-out data
                 * from tunnel, I should to find this information
                 * and set it in Packet object.
                 */
                ptr = buf-sizeof(struct ether_header);
                (void)memcpy((char *)&gw, ptr, sizeof(u_long));
                p->rt_setgw(gw);

        } else return (-3);
        return(0);
}

/*---------------------------------------------------------------------------
 * pos2angle()
 *
 * The origin is located at (0, 0). The following table shows some
 * example input-output pairs.
 * 
 *   [target located at]  [angle]
 *         (1, 1)            45
 *         (-1, 1)          135
 *         (-1, -1)         225
 *         (1, -1)          315
 *
 * Arguments:
 *	x, y		position of the target
 *
 * Returns:
 *      0 < angle < 360
 *
 * Side effects:
 *      no side effect.
 *---------------------------------------------------------------------------*/
double pos2angle(double x, double y)
{
	double tan;
	double angle;

	if (x == 0)
	{
		return (y >= 0) ? 90.0 : 270.0;
	}

	tan = y / x;
	angle = atan(tan) * 180 / M_PI;

	if (x < 0)
	{
		// 2nd & 3rd quadrants
		angle += 180;
	}
	else if (x >= 0 && y < 0)
	{
		// 4th quadrant
		angle += 360;
	}

	return angle;
}

/*---------------------------------------------------------------------------
 * getAntennaDirection()
 * 
 * Calculate current pointing direction of a specific antenna, given its
 * initial pointing direction and angular speed.
 *
 * Arguments:
 *      pointingDirection	initial pointing direction
 *	angularSpeed		angular speed
 *
 * Returns:
 *      current pointing direction
 *
 * Side effects:
 *      no side effect.
 *---------------------------------------------------------------------------*/
double getAntennaDirection(double pointingDirection, double angularSpeed)
{
	double			currTime;
	double			currDirection;

	currTime = GetCurrentTime();
	TICK_TO_SEC(currTime, GetCurrentTime());

	currDirection = pointingDirection + (currTime * angularSpeed);
	currDirection -= ((int) (currDirection / 360)) * 360;

	return currDirection;
}

/*---------------------------------------------------------------------------
 * gaussian()
 *
 * Gaussian random number generator.
 *
 * Arguments:
 *	min, max
 *
 * Returns:
 *	a Gaussian random number between min and max
 *
 * Side effects:
 *      no side effect.
 *
 * References:
 *	http://www.bearcave.com/misl/misl_tech/wavelets/hurst/random.html
 *---------------------------------------------------------------------------*/
double gaussian(double min, double max)
{
	double x1, x2, w, y, f1, f2;

	while (1)
	{
		do {
			f1 = (random() % 1000) / 1000.0;
			f2 = (random() % 1000) / 1000.0;

			x1 = 2.0 * f1 - 1.0;
			x2 = 2.0 * f2 - 1.0;
			w = x1 * x1 + x2 * x2;
		} while (w >= 1.0);

		w = sqrtf((-2.0 * log(w)) / w);
		y = x2 * w;

		if (y >= min && y <= max)
		{
			return y;
		}
	}
}

double GetNodeDistance(u_int32_t nid1, u_int32_t nid2)
{
	double x1, y1, z1;
	double x2, y2, z2;

	GetNodeLoc(nid1, x1, y1, z1);
	GetNodeLoc(nid2, x2, y2, z2);

	return sqrt((x1-x2)*(x1-x2) + (y1-y2)*(y1-y2) + (z1-z2)*(z1-z2));
}

double gaussian_normal(double avg, double std)
{
	/*
	 * avg = average
	 * std = standard deviation
	 */
	static int parity = 0;
	static double nextresult;
	double x1, x2, w, f1, f2;

	if(std == 0) return avg;
	if(parity == 0){
		while (1)
		{
			do {
				f1 = (random() % 1000) / 1000.0;
				f2 = (random() % 1000) / 1000.0;

				x1 = 2.0 * f1 - 1.0;
				x2 = 2.0 * f2 - 1.0;
				w = x1 * x1 + x2 * x2;
			} while (w >= 1.0);

			w = sqrtf((-2.0 * log(w)) / w);
			nextresult = x2 * w;
			parity = 1;
			return (x1 * w * std + avg);
		}
	}
	else{
		parity = 0;
		return (nextresult * std + avg);
	}
}

void sendRuntimeMsg(u_int32_t type, int nodeID, const char* module, const char* message)
{
	u_int64_t time;

#if IPC
	struct runtimeMsgInfo data;

	SEC_TO_TICK(time, 1);
	data.type = type;
	data.time = (GetCurrentTime()/(float)time);
	data.nodeID = nodeID;
	sprintf(data.module, "%s", module);
	sprintf(data.message, "%s", message);
	sendWarningtoGUI(data);
#else
        const char *type_str[3] = {"Information", "  Warning  ", "Fatal Error"};
        char response;
        extern Dispatcher *dispatcher_;
        extern sysrt *sysrt_;
        extern cmd_server *cmd_server_;

	if(type != RTMSG_INFORMATION &&
		type != RTMSG_WARNING &&
		type != RTMSG_FATAL_ERROR)
		return;

	if(type == RTMSG_WARNING)
		dispatcher_->Pause(NULL);

	SEC_TO_TICK(time, 1);

	printf("*************************%s*************************\n", type_str[type-1]);
	printf("Time:\t\t%0.2f sec\n", GetCurrentTime()/(float)time);
	printf("Node ID:\t%d\n", nodeID);
	printf("Module:\t\t%s\n", module);
	printf("Message:\t%s\n",  message);
	printf("*************************************************************\n");

	switch(type)
	{
		case RTMSG_FATAL_ERROR:
			printf("Simulation halts.\n");
			cmd_server_->closeAllClientSocket();
			delete sysrt_;
			printf("Simulation Engine is going to call exit().\n");
			exit(0);
		case RTMSG_WARNING:
			printf("Press \"y\" to continue the simulation or \"n\" to stop the simulation: ");
			while(response != 'y')
			{
				response = getchar();
				switch(response)
				{
					case 'Y':
					case 'y':
						dispatcher_->Continue(NULL);
						response = 'y';
						break;
					case 'N':
					case 'n':
						printf("Simulation halts.\n");
						cmd_server_->closeAllClientSocket();
						delete sysrt_;
						printf("Simulation Engine is going to call exit().\n");
						exit(0);
					case '\n':
					case '\r':
						break;
					default:
						printf("Press \"y\" to continue the simulation or \"n\" to stop the simulation: ");
				}
			}
			break;
		case RTMSG_INFORMATION:
			break;
		default:
			fprintf(stderr, "There is no handler related to type %d.\n", type);
	}
#endif
}
