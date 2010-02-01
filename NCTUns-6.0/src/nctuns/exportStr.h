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

#ifndef __NCTUNS_exportStr_h__
#define __NCTUNS_exportStr_h__

#include <sys/types.h>

/* define export comment structure */
struct expcmt {

	char			*cmt;
	u_int32_t		cmt_length;
	struct expcmt		*next_cmt;

};

/* define export field structure */
struct expfield {

	u_int32_t		row;
	u_int32_t		column;
	char			*cell;
	u_int32_t		cell_length;
	char			*sep;
	u_int32_t		sep_length;
	struct expfield		*next_row;
	struct expfield		*next_column;

};



class ExportStr {

 private:
 	char			*header;
 	u_int32_t		header_length;			
 
 	struct expcmt		*cmt_list_head;
 	struct expcmt		*cmt_list_tail;
 	u_int32_t		num_cmt;
 	
 	struct expfield		*row_list_head;
 	struct expfield		*row_list_tail;
	u_int32_t		num_row;
	u_int32_t		num_column;
	u_int32_t		num_cell;

	u_int32_t		total_strlen;
	u_int32_t		total_cmtlen;	// comment length
	u_int32_t		total_cellen;	// cell length
	u_int32_t		total_seplen;	// separator length

	char			*OutputGuiStr;
	char			*OutputFileStr;
			
 public:
 
 	ExportStr(u_int32_t);
 	~ExportStr();

	int			Insert_header(const char *);
	int			Insert_comment(const char *);
	int			Add_row();
	int			Insert_cell(u_int32_t, u_int32_t, const char *, const char *);
	char			*Get_cell(u_int32_t, u_int32_t);
	char			*ExportStr_to_GUI();
	char			*ExportStr_to_FILE();		
	u_int32_t		Get_ExportGUI_Strlen();
	u_int32_t		Get_ExportFILE_Strlen();
};


#endif /* __NCTUNS_exportStr_h__ */
