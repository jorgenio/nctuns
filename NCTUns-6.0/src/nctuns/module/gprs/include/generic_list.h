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

#ifndef __GENERIC_LIST_H
#define __GENERIC_LIST_H

#include <gprs/include/types.h>

template <class Elem>
class ListElem {
    private:
        Elem* elem;
        ListElem* next;
    public:
        ListElem(Elem* new_elem)            {elem = new_elem; next=NULL;}
        //~ListElem()                       {if (elem) delete elem;}
        int         set_next(ListElem* next_elem)   {next = next_elem; return 1;};
        ListElem*   get_next()                      {return next;}
        Elem*       get_elem()                      {return elem;}
};

template <class Elem>
class ListElem1 {
    private:
        Elem* elem;
    public:
        ListElem1(Elem* new_elem)       {elem = new_elem;}
        //~ListElem1()                    {if (elem) delete elem;}
        Elem* get_elem()                {return elem;}
};

template <class Elem>
class ListElem2 {
    private:
        Elem* elem;
        Elem* prev;
        Elem* next;
    public:
        ListElem2(Elem* new_elem)           {elem = new_elem; prev=NULL; next=NULL;}
        ~ListElem2()                        {if (elem) delete elem;}
        int     set_next(Elem* next_elem)   {next = next_elem; return 1;};
        int     set_prev(Elem* prev_elem)   {prev = prev_elem; return 1;};
        Elem*   get_prev()                  {return prev;}
        Elem*   get_next()                  {return next;}
        Elem*   get_elem()                  {return elem;}
};

template <class Elem>
class SList {
    protected:
        ListElem<Elem>*         head;
        ListElem<Elem>*         tail;
        ulong                   list_num;
    public:
        SList()                 {head=NULL;tail=NULL;list_num=0;}
        virtual ~SList();
        ulong                   get_list_num()          {return list_num;}
        int                     is_empty()              {return (head==NULL)?true:false;}
        int                     insert_tail(Elem* item);
        int                     insert_head(Elem* item);
        int                     remove_head();
        int                     remove_tail();
        int                     flush();
        int                     remove(ListElem<Elem>* elem);
        int                     remove(Elem* elem);
        int                     remove_entry(ListElem<Elem>* elem);
        int                     remove_entry(Elem* elem);
        ListElem<Elem>*         get_head()              {return head;}
        Elem*                   get_head_elem()         {return (head) ? head->get_elem():NULL;}
        Elem*                   get_elem(ulong index);
        virtual ListElem<Elem>* search(Elem* elem);
};

template <class Elem, ulong size>
class Array {
    protected:
        ListElem1<Elem>**   array;
        ulong               array_size;
        char*               indicator;
        ulong               num_of_tbfs;
        bool                range_test(ulong index) { return (index>=0 && index<size)?true:false;}
    public:
        Array();
        ~Array();
        int                 insert(Elem* new_elem, ulong index);
        ulong               get_num_of_tbfs()  {return num_of_tbfs;}
        int                 del(ulong index);
        int                 del_entry(ulong index);
        ListElem1<Elem>*    get(uchar tfi);
        ListElem1<Elem>*    get1();
};

#endif
