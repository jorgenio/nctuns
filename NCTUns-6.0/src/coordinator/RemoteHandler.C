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

#include <pwd.h>

#include "Global.H"
#include "RemoteHandler.H"



RemoteHandler::
RemoteHandler(ClientNode cli_node, string uname, SocketManager* manager){
	c_node = cli_node;
	username = uname;
	socketManager = manager;
	struct passwd* pw = getpwnam(username.c_str());
	userhome =  string(pw->pw_dir);
	curr_dir = userhome + "/.nctuns/coordinator/workdir/";
	if (chdir(curr_dir.c_str())<0){
		string cmd = "mkdir -p " + curr_dir;
		system(cmd.c_str());
		chdir(curr_dir.c_str());
	}
		cout << userhome << " | " << curr_dir << endl << flush;
}

void
RemoteHandler::
run(){
	IOStream* ioStream;
	for(;;){
			char command[256];
			int rd = ioStream->readLine(c_node.command_fd, command, 256);
			if (rd >0)
				dispatch(command);
			else {
				cout << "client closed" << endl;
				exit(0);
			}
	}
}

void
RemoteHandler::
dispatch(char* command){
	char* opcode = strtok(command, "|\n");
	if (strcmp(opcode, "getFileList") == 0)
		getFileList();
	else if (strcmp(opcode, "getFile") == 0)
		getFile(strtok(NULL, "|\n"));
	else if (strcmp(opcode, "changeDir") == 0)
		changeDir(strtok(NULL, "|\n"));
	else if (strcmp(opcode, "deleteFile") == 0)
		deleteFile(strtok(NULL, "|\n"));
	else if (strcmp(opcode, "closeHandler") == 0)
		closeHandler();
	else
		cout << "else?? " << opcode;

}

void 
RemoteHandler::
deleteFile(char* filename){
    	cout << curr_dir << filename << endl;
	string full_path = curr_dir + "/" + filename;
	remove(full_path.c_str());
}

void 
RemoteHandler::
changeDir(char* path){
	int  size = 1024;
	char ptr[size];

	string temp = string(getcwd(ptr, size));
	if ((temp == userhome) && (strcmp(path, "..") == 0))
	    return;
	
	chdir(path);
	char* curr_path = getcwd(ptr, size);
	if (curr_path != NULL)
		curr_dir = string(curr_path);
}

extern long fileSize(FILE *fileFD);

void
RemoteHandler::
getFile(char* filename){

	string fullpath;
	int pos = curr_dir.find_last_of("/");	
	if(pos == curr_dir.length()-1)
		fullpath = curr_dir + string(filename);
	else
		fullpath = curr_dir + "/" + string(filename);
			
	int cmd_fd  = c_node.command_fd;
	int data_fd = c_node.data_fd;
	IOStream* ioStream;
	
	FILE* fileFD = fopen(fullpath.c_str(), "r");
	long fsize = fileSize(fileFD);
	
	char reply[128];
	sprintf(reply, "size|%ld\n", fsize);
  
  try{
	ioStream->writen(cmd_fd, reply, strlen(reply));
	
	long toSend = fsize;
	long send_buf;
	long buf_size = 1024;
	char buf[buf_size];
	send_buf = min(toSend, buf_size);
	while((fread(buf, 1, send_buf, fileFD) >0) && (toSend >0)){
		ioStream->writen(data_fd, buf, send_buf);
		toSend = toSend-send_buf;
		send_buf = min(toSend, buf_size);
	}
	fclose(fileFD);	
	cout << "[File Sent!]\n" << endl << flush;
	return;
  }
  catch(IOStream::IOException){
  	cerr << "IOException! in get file, write to GUI Error!\n" << flush;
	return;
  }

}

void 
RemoteHandler::
closeHandler(){
	exit(0);
}

void add_row (string filename, struct stat* buf, string& flist);

void
RemoteHandler::
getFileList(){

	string f_list;
	
	DIR* dirp = opendir(curr_dir.c_str());
	struct dirent *dp;	
    while ((dp = readdir(dirp)) != NULL){	
		struct stat buf;
		lstat(dp->d_name, &buf);
		string fname(dp->d_name);
		if (fname != ".")
			add_row(fname, &buf, f_list);	
	}
	f_list += "\n";
	//cout << f_list;
	int cmd_fd = c_node.command_fd;
	IOStream* ioStream;
	try{
		ioStream->writen(cmd_fd, f_list.c_str(), f_list.length());
	}
	catch(IOStream::IOException){
		cerr << "IOException! Get File List Error!\n" << flush;
		return;
	}
	return;
}


void add_row (string filename, struct stat* buf, string& flist){
	string type;
	if (S_ISDIR(buf->st_mode))
		type = "directory";
	else
		type = "regular";
	string temp_time = string(ctime(&(buf->st_mtime)));
	string m_time = temp_time.substr(0, temp_time.length()-1);
	ostringstream s_out;
	s_out << filename << "|" << buf->st_size << "|" << type 
		  << "|" << m_time <<"\t";

	flist += s_out.str();
}

