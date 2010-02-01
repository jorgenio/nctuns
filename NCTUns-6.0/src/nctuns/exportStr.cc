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
#include <string.h>
#include "exportStr.h"

ExportStr::ExportStr(u_int32_t num_field_in_a_row) {

	if( num_field_in_a_row <= 0 )
		num_field_in_a_row = 1;

	header = NULL;
	header_length = 0; 

	cmt_list_head = NULL;
	cmt_list_tail = NULL;
	num_cmt = 0;

	row_list_head = NULL;
	row_list_tail = NULL;
	num_row = 0;
	num_column = num_field_in_a_row;
	num_cell = 0;

	total_strlen = 0;
	total_cmtlen = 0;
	total_cellen = 0;
	total_seplen = 0;

	OutputGuiStr = NULL;
	OutputFileStr = NULL;

}


ExportStr::~ExportStr() {
	struct expcmt		*tmpcmt;
	struct expcmt		*precmt;

	struct expfield		*tmp_row_field;
	struct expfield		*pre_row_filed;
	struct expfield		*tmp_col_field;
	struct expfield		*pre_col_field;
	u_int32_t		i,j;

	if( header != NULL )
		free(header);
	if( OutputGuiStr != NULL )
		free(OutputGuiStr);
	if( OutputFileStr != NULL )
		free(OutputFileStr);

	if( num_cmt > 0 ) {

		for ( i = 1, tmpcmt = cmt_list_head; 
		      (i <= num_cmt)&&(tmpcmt != NULL); i++) {

			if( tmpcmt->cmt != NULL )
				free(tmpcmt->cmt);
			precmt = tmpcmt->next_cmt;
			free(tmpcmt);
			tmpcmt = precmt;
		}
	}

	if( num_cell > 0 ) {

		for( i = 1, tmp_row_field = row_list_head; 
		     (i <= num_row)&&(tmp_row_field != NULL); i++) {

			pre_row_filed = tmp_row_field->next_row;

			for( j = 2, tmp_col_field = tmp_row_field;
			     (j <= num_column)&&(tmp_col_field != NULL); j++) {
				if ( tmp_col_field->cell != NULL )
					free(tmp_col_field->cell);
				if ( tmp_col_field->sep != NULL)
					free(tmp_col_field->sep);
				pre_col_field = tmp_col_field->next_column;
				free(tmp_col_field);
				tmp_col_field = pre_col_field;
			}
			
			tmp_row_field = pre_row_filed;			
		}
	}
}


int ExportStr::Insert_header(const char *header_str) {
	
	if( header_str == NULL )
		return 0;
	if( header != NULL ) {
		free(header);
		header = NULL;
		header_length = 0;
	}

	header = strdup(header_str);
	header_length = strlen(header_str);
	return 1;
}


int ExportStr::Insert_comment(const char *cmt) {
	struct expcmt	*tmpcmt;

	if ( cmt == NULL )
		return 0;

	tmpcmt = (struct expcmt *)malloc(sizeof(struct expcmt));
	tmpcmt->cmt_length = strlen(cmt);
	tmpcmt->cmt = strdup(cmt);
	tmpcmt->next_cmt = NULL;

	if ( num_cmt == 0 ) {
		cmt_list_head = tmpcmt;
		cmt_list_tail = tmpcmt;
	}
	else {
		cmt_list_tail->next_cmt = tmpcmt;
		cmt_list_tail = tmpcmt;
	}

	num_cmt++;
	total_cmtlen += tmpcmt->cmt_length;

	return 1;
}

int ExportStr::Add_row() {
	struct expfield		*tmp_field;
	struct expfield		*column_head;
	struct expfield		*column_tail;
	u_int32_t		i;
	
	num_row++;
	for( i = 1; i <= num_column; i++ ) {
		tmp_field = (struct expfield *)malloc(sizeof
					(struct expfield));
		tmp_field->row = num_row;
		tmp_field->column = i;
		tmp_field->cell = NULL;
		tmp_field->cell_length = 0;
		tmp_field->sep = NULL;
		tmp_field->sep_length = 0;
		tmp_field->next_row = NULL;
		tmp_field->next_column = NULL;
	
		if( i == 1 ) {
			column_head = tmp_field;
			column_tail = tmp_field;
		}
		else {
			column_tail->next_column = tmp_field;
			column_tail = tmp_field;
		}
	}

	if( num_row == 1 ) {
		row_list_head = column_head;
		row_list_tail = column_head;
	}
	else {
		row_list_tail->next_row = column_head;
		row_list_tail = column_head;
	}

	return num_row;
}

int ExportStr::Insert_cell(u_int32_t row, u_int32_t column, 
			   const char *cell, const char *separator) {
	const char whitespace[] = "";
	struct expfield		*tmp_field;
	u_int32_t		x,y;

	if ( num_row == 0 )
		return 0;

	if ( cell == NULL && separator == NULL )
		return 0;

	if ( row <= 0 || column <= 0)
		return 0;
	if ( row > num_row || column > num_column)
		return 0;

	tmp_field = row_list_head;
	for( y = 2; y <= row; y++ )
		tmp_field = tmp_field->next_row;

	for( x = 2; x <= column; x++ )
		tmp_field = tmp_field->next_column;

	if ( tmp_field->row != row || tmp_field->column != column)
		return 0;

	if ( tmp_field->cell != NULL ) {
		free(tmp_field->cell);
		num_cell--;
		total_cellen -= tmp_field->cell_length;

		free(tmp_field->sep);
		total_seplen -= tmp_field->sep_length;

		tmp_field->cell = NULL;
		tmp_field->cell_length = 0;
		tmp_field->sep = NULL;
		tmp_field->sep_length = 0;
	}

	tmp_field->cell = (cell) ? strdup(cell) : strdup(whitespace);
	tmp_field->cell_length = strlen(tmp_field->cell);
	num_cell++;
	total_cellen += tmp_field->cell_length;

	tmp_field->sep = (separator) ? strdup(separator) : strdup(whitespace);
	tmp_field->sep_length = strlen(tmp_field->sep);
	total_seplen += tmp_field->sep_length;

	return 1;
}

char * ExportStr::Get_cell(u_int32_t row, u_int32_t column) {
	struct expfield		*tmp_field;
	u_int32_t		x,y;

	if ( num_row == 0 )
		return (NULL);
	if ( row <= 0 || column <= 0 )
		return (NULL);
	if ( row > num_row || column > num_column )
		return (NULL);

	tmp_field = row_list_head;
	for( y = 2; y <= row; y++ )
		tmp_field = tmp_field->next_row;

	for( x = 2; x <= column; x++ )
		tmp_field = tmp_field->next_column;

	if ( tmp_field->row != row || tmp_field->column != column)
		return (NULL);
	
	return(tmp_field->cell);
}


char * ExportStr::ExportStr_to_GUI() {
	struct expfield		*tmp_row_field;
	struct expfield		*tmp_col_field;
	char			*tmpOutputStr = NULL;
	u_int32_t		i,j;
	const char		*sep = "|";
	const char		*default_cell = "-";

	if( OutputGuiStr != NULL )
		free(OutputGuiStr);
	
	total_strlen = header_length + total_cellen + 
		       (2 * strlen(sep) * num_row * num_column);

	OutputGuiStr = (char *)malloc(sizeof(char) * total_strlen);
	tmpOutputStr = OutputGuiStr;

	if( header_length > 0 )
		tmpOutputStr += sprintf(tmpOutputStr, "%s", header);

	if( num_cell > 0 ) {
		for( i = 1, tmp_row_field = row_list_head; 
		     (i <= num_row)&&(tmp_row_field != NULL);
		     i++, tmp_row_field = tmp_row_field->next_row ) {

			for( j = 1, tmp_col_field = tmp_row_field; 
			     (j <= num_column)&&(tmp_col_field != NULL); 
			     j++, tmp_col_field = tmp_col_field->next_column) {
				if( tmp_col_field->cell == NULL ||
				    tmp_col_field->cell_length == 0 )
					tmpOutputStr += sprintf(tmpOutputStr, 
					"%s%s", default_cell, sep);
				else
					tmpOutputStr += sprintf(tmpOutputStr, 
					"%s%s", tmp_col_field->cell, sep);
			}
		}
	}

	return OutputGuiStr;
}


char * ExportStr::ExportStr_to_FILE() {
	struct expcmt		*tmpcmt;
	struct expfield		*tmp_row_field;
	struct expfield		*tmp_col_field;
	char			*tmpOutputStr = NULL;
	u_int32_t		i,j;
	const char		*default_str = "";

	if( OutputFileStr != NULL )
		free(OutputFileStr);

	total_strlen = total_cmtlen + total_cellen + total_seplen;

	OutputFileStr = (char *)malloc(sizeof(char) * total_strlen);
	tmpOutputStr = OutputFileStr;

	if( num_cmt > 0 ) {

		for ( i = 1, tmpcmt = cmt_list_head; 
		      (i <= num_cmt)&&(tmpcmt != NULL); 
	      	      i++, tmpcmt = tmpcmt->next_cmt ) {
			tmpOutputStr += 
				sprintf(tmpOutputStr, "%s", tmpcmt->cmt);
		}
	}

	if( num_cell > 0 ) {

		for( i = 1, tmp_row_field = row_list_head; 
		     (i <= num_row)&&(tmp_row_field != NULL);
		     i++, tmp_row_field = tmp_row_field->next_row ) {

			for( j = 1, tmp_col_field = tmp_row_field; 
			     (j <= num_column)&&(tmp_col_field != NULL); 
			     j++, tmp_col_field = tmp_col_field->next_column) {
				if( tmp_col_field->cell == NULL )
					tmpOutputStr += sprintf(tmpOutputStr, 
					"%s%s", default_str, default_str);
				else
					tmpOutputStr += sprintf(tmpOutputStr, 
					"%s%s", tmp_col_field->cell, 
					tmp_col_field->sep);
			}
		}

				
	}

	return OutputFileStr;
}

u_int32_t ExportStr::Get_ExportGUI_Strlen() {
		return(strlen(ExportStr_to_GUI()));
}

u_int32_t ExportStr::Get_ExportFILE_Strlen() {
		return(strlen(ExportStr_to_FILE()));
}
