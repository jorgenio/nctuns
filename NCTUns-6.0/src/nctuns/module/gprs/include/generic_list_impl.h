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

#ifndef __GENERIC_LIST_IMPL_H
#define __GENERIC_LIST_IMPL_H
#include <exception>
#include <stdio.h>
#include <iostream>
#include <assert.h>
#include "generic_list.h"

    template <class Elem>
    SList<Elem>::~SList() {
        if (!head)
            return;

        ListElem<Elem>* ptr  = head;
        ListElem<Elem>* ptr1;

        while (ptr) {
            ptr1 = ptr;
            ptr = ptr->get_next();
            delete ptr1;
        }
    }

   template <class Elem>
   int SList<Elem>::flush() {
       if (!head)
           return -1;

       ListElem<Elem>* ptr  = head;
       ListElem<Elem>* ptr1;

       while (ptr) {
           ptr1 = ptr;
           ptr = ptr->get_next();
           delete ptr1;
       }
       head = tail = NULL;
       list_num = 0;
       return 1;
   }

   template <class Elem>
   int SList<Elem>::insert_tail(Elem* item) {
        if (!item)
            return -1;
        try {
            ListElem<Elem>* new_elem = new ListElem<Elem>(item);

        if ( !list_num )
            head = tail = NULL;

        if (!head) {
                head = tail = new_elem;
                tail->set_next(static_cast< ListElem<Elem>*> (NULL) );
            }
            else {
                tail->set_next(new_elem);
                tail = new_elem;
            }
            ++list_num;
            return 1;
        }
        catch(std::exception& e) {
            //cout << "Slist insert():" << e.what() << endl;
            printf("SList insertion failed. \n");
            return -1;
        }
   }

   template <class Elem>
   int SList<Elem>::insert_head(Elem* item) {
        if (!item)
            return -1;
        try {
            ListElem<Elem>* new_elem = new ListElem<Elem>(item);

        if ( !list_num )
            head = tail = NULL;

            if (!head) {
                head = tail = new_elem;
                tail->set_next( reinterpret_cast<ListElem<Elem>*> (NULL) );
            }
            else {
                new_elem->set_next(head);
                head = new_elem;
            }
            ++list_num;
            return 1;
        }
        catch(std::exception& e) {
            //cout << "Slist insert():" << e.what() << endl;
            printf("SList insertion failed. \n");
            return -1;
        }
    }

    template <class Elem>
    int SList<Elem>::remove_head() {
        if (!head)
            return -1;
        ListElem<Elem>* ptr = head->get_next();
        if ( head == tail )
            tail = NULL;
        delete head;
        head = ptr;
        --list_num;
        return 1;
    }

    template <class Elem>
    int SList<Elem>::remove_tail() {
        if (is_empty())
            return 1;
        if ( head == tail ) { /* only one elem */
            delete head;
            head = tail = NULL;
            --list_num;
            return 1;
        }
        ListElem<Elem>* ptr1;
        for ( ptr1=head ; ptr1 ; ptr1=(ptr1->get_next()) ) {
            if ( (ptr1->get_next() )==tail) {
                delete tail;
                ptr1->set_next( reinterpret_cast<ListElem<Elem>*> (NULL));
                tail = ptr1;
                --list_num;
                return 1;
            }
        }
        return -1;
    }

    template <class Elem>
    int SList<Elem>::remove(Elem* elem) {

        if (is_empty())
            return -1;

        ListElem<Elem>* ptr1, *last = NULL;
        for ( ptr1=head ; ptr1 ; ptr1=(ptr1->get_next()) ) {

		if (ptr1->get_elem() == elem) {
			if (head == ptr1)
				head = head->get_next();
			else {
				last->set_next(ptr1->get_next());
			}

			if (tail == ptr1)
				tail = last;

			delete ptr1;
			--list_num;
			return 1;
		}

		last = ptr1;

#if 0
            if (!(ptr1->get_next())) {
                /* reach the end of the list */
                if ( (elem) == ((ptr1)->get_elem()) ) {
                    if ( ptr1 == head ) {
                        if ( head == tail )
                            tail = NULL;
                        head = NULL;
                        delete ptr1;
                        delete elem;
                        --list_num;
                        return 1;
                    }
                }
            }

            else if ( (elem) == ((ptr1->get_next())->get_elem()) ) {

                ListElem<Elem>* ptr2 = (ptr1->get_next());
                ptr1->set_next( ptr2->get_next() );
                if ( ptr2 == head )
                    head = ptr1->get_next();
                if ( ptr2 == tail )
                    tail = ptr1;
                //delete elem->get_elem();
                delete ptr2;
                delete elem;
                --list_num;
                return 1;
            }

            else
                ;
#endif
        }
        return -1;

    }


    template <class Elem>
    int SList<Elem>::remove(ListElem<Elem>* elem) {
        if (is_empty())
            return -1;
        ListElem<Elem>* ptr1, *last = NULL;
        for ( ptr1=head ; ptr1 ; ptr1=(ptr1->get_next()) ) {

		if (ptr1->get_elem() == elem->get_elem()) {
			if (head == ptr1)
				head = head->get_next();
			else {
				last->set_next(ptr1->get_next());
			}

			if (tail == ptr1)
				tail = last;

			delete ptr1;
			--list_num;
			return 1;
		}

		last = ptr1;
#if 0
            if (!(ptr1->get_next())) {
                /* reach the end of the list */
                if ( (elem->get_elem()) == ((ptr1)->get_elem()) ) {
                    if ( ptr1 == head ) {
                        if ( head == tail )
                            tail = NULL;
                        head = NULL;
                        delete elem;
                        --list_num;
                        return 1;
                    }
                }
            }

            else if ( (elem->get_elem()) == ((ptr1->get_next())->get_elem()) ) {

                ListElem<Elem>* ptr2 = (ptr1->get_next());
                ptr1->set_next( ptr2->get_next() );

                if ( ptr2 == head )
                    head = NULL;
                if ( ptr2 == tail )
                    tail = ptr1;
                //delete elem->get_elem();
                delete elem;
                --list_num;
                return 1;
            }

            else
                ;
#endif
        }
        return -1;
    }

    template <class Elem>
    int SList<Elem>::remove_entry(ListElem<Elem>* elem) {
        if (is_empty())
            return -1;
        ListElem<Elem>* ptr1, *last = NULL;
        for ( ptr1=head ; ptr1 ; ptr1=(ptr1->get_next()) ) {

		if (ptr1->get_elem() == elem->get_elem()) {
			if (head == ptr1)
				head = head->get_next();
			else {
				last->set_next(ptr1->get_next());
			}

			if (tail == ptr1)
				tail = last;

			delete ptr1;
			--list_num;
			return 1;
		}

		last = ptr1;
#if 0
            if (!(ptr1->get_next())) {
                /* reach the end of the list */
                if ( (elem->get_elem()) == ((ptr1)->get_elem()) ) {
                    if ( ptr1 == head ) {
                        if ( head == tail )
                            tail = NULL;
                        head = NULL;
                        delete elem;
                        --list_num;
                        return 1;
                    }
                }
            }

            else if ( (elem->get_elem()) == ((ptr1->get_next())->get_elem()) ) {
                ListElem<Elem>* ptr2 = (ptr1->get_next());
                ptr1->set_next( ptr2->get_next() );

                if ( ptr2 == head )
                    head = NULL;
                if ( ptr2 == tail )
                    tail = ptr1;

                delete elem;
                --list_num;
                return 1;
            }
            else
                ;
#endif
        }
        return -1;
    }

    template <class Elem>
    int SList<Elem>::remove_entry( Elem* elem) {
        if (is_empty())
            return -1;


#if 0
        ListElem<Elem>* ptr1;

        if ( (head->get_elem()) == elem ) {

            ptr1 = head->get_next();
            delete head;
            head = ptr1;
            --list_num;
            if ( list_num  <= 0 ) {

                head = tail = NULL;

            }

            return 1;

        }
#endif

        ListElem<Elem>* ptr1, *last = NULL;
        for ( ptr1=head ; ptr1 ; ptr1=(ptr1->get_next()) ) {

		if (ptr1->get_elem() == elem) {
			if (head == ptr1)
				head = head->get_next();
			else {
				last->set_next(ptr1->get_next());
			}

			if (tail == ptr1)
				tail = last;

			delete ptr1;
			--list_num;
			return 1;
		}

		last = ptr1;
#if 0
            if (!(ptr1->get_next())) {
                /* reach the end of the list */
                if ( (elem == (ptr1->get_elem()) ) ) {
                    if ( ptr1 == head ) {
                        if ( head == tail )
                            tail = NULL;
                        head = NULL;
                        delete ptr1;
                        --list_num;
                        return 1;
                    }
                }
            }

            else if ( (elem == ((ptr1->get_next())->get_elem())) ) {
                ListElem<Elem>* ptr2 = (ptr1->get_next());
                ptr1->set_next( ptr2->get_next() );

                if ( ptr2 == head )
                    head = NULL;
                if ( ptr2 == tail )
                    tail = ptr1;
                delete ptr2;
                --list_num;
                return 1;
            }
            else
                ;
#endif
        }
        return -1;
    }

    template <class Elem>
    ListElem<Elem>* SList<Elem>::search(Elem* elem) {
        ListElem<Elem>* ptr;
        for (ptr=head; ptr ; ptr=(ptr->get_next()) ) {
            if ( elem ==(ptr->get_elem()) ) {
                return ptr;
            }
        }
        return NULL;
    }

    template <class Elem>
    Elem* SList<Elem>::get_elem(ulong index) {

        if ( index >= list_num ) {

            //printf("SList get_elem(): index is out of range. index value = %ld \n" , index );
            return NULL;

        }

        ListElem<Elem>* ptr = head;

        for ( ulong i=0; i<list_num ; ++i ) {

            if ( i == index ) {

                ASSERTION( ptr , "Slist get_elem(): the required listelem is null.\n" );
                return (ptr->get_elem());
            }

            ptr = ptr->get_next();
        }

        return NULL;

    }

/**************************************************************/

    template <class Elem, ulong size>
    Array<Elem,size>::Array() {
        array_size = size;
        array = new ListElem1<Elem>*[array_size];
        indicator= new char[array_size];
        num_of_tbfs = 0;
        if (array) {
            for (ulong i=0 ; i<array_size;++i)
                array[i] = NULL;
        }
        if ( indicator ) {
            bzero(indicator,array_size*sizeof(char));
        }
    }

    template <class Elem, ulong size>
    Array<Elem,size>::~Array() {
        for (ulong i=0 ; i<array_size ; ++i ) {
            if (array[i])
               delete array[i];
        }
        delete array;
        if (indicator)
               delete indicator;

    }

    template <class Elem,ulong size>
    int Array<Elem,size>::insert(Elem* new_elem, ulong index) {
        if (!new_elem)
            return -1;
        if (!range_test(index) )
            return -2;
        if ( array[index] ) {
            printf("Array insert(): array[%ld] has already been used\n",index);
            return -3;
        }

        ListElem1<Elem>* new_listelem = new ListElem1<Elem>(new_elem);
        if (!new_listelem)
            return -4;

        array[index] = new_listelem;
        indicator[index] = 1;
        ++num_of_tbfs;
        return 1;
    }


    template <class Elem,ulong size>
    int Array<Elem,size>::del(ulong index) {
        if (!range_test(index))
            return -1;
        if (array[index]) {
            delete array[index];
            array[index] = NULL;
            --num_of_tbfs;
            return 1;
        }
        else {
            printf( "Array del(): array[%lu] is not used \n",index);
            return -1;
        }
    }

    template <class Elem,ulong size>
    int Array<Elem,size>::del_entry(ulong index) {
        if (!range_test(index))
            return -1;
        if (array[index]) {
            delete array[index];
            array[index] = NULL;
            --num_of_tbfs;
            return 1;
        }
        else {
            printf( "Array del(): array[%lu] is not used \n",index);
            return -1;
        }
    }

    template <class Elem,ulong size>
    ListElem1<Elem>* Array<Elem,size>::get(uchar tfi) {
        if (!range_test(tfi))
           return NULL;
        return array[tfi];
    }

    template <class Elem,ulong size>
    ListElem1<Elem>* Array<Elem,size>::get1() {
        for ( ulong i=0 ; i<array_size ; ++i)
            if (indicator[i])
                return array[i];
        return NULL;
    }



#endif
