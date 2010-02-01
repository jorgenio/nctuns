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

#include "Global.H"
#include "Mediator.H"


int Mediator::putfile(char* filename, long fileSize, int cmd_fd, int data_fd){
	char outfile[256];
	strcpy(outfile, currDir.c_str());
	strcat(outfile, filename);
	cout << "[Saving file...] " << outfile << endl << flush; 
	const char* reply = "please\n";
	FILE* out = fopen(outfile, "w");
	if (out == NULL){
		cerr << "Can't Write File to:" << outfile << endl << flush;
		return -1;
	}
   try{
	ioStream -> writen(cmd_fd, reply, strlen(reply));
	//int data_fd = clientList.getActiveClient()->data_fd;
	int rd;
	long buf_size = 256;
	char buf[buf_size];
	long toRead = fileSize;	
	int readBuf = min(toRead, buf_size);
	
	while ((toRead >0) && (ioStream->readn(data_fd, buf, readBuf) >0)){
		fwrite (buf, 1, readBuf, out);
		toRead = toRead - readBuf;
		readBuf = min(toRead, buf_size);
		//cout << buf << flush;
		memset(buf, 0, buf_size);
	}
	fclose(out);
	reply="File get\n";
	ioStream -> writen(cmd_fd, reply, strlen(reply));
	cout << "[Got it!]\n" << flush;
	return 1;
   }	
   catch(IOStream::IOException){
   	 cerr << "IOException! Put file\n" << flush;
	 return -1;
   }

}

bool isLogFile(char* filename){
	if(strstr(filename, ".log") == NULL)
		return false;
	else
		return true;
}

long fileSize(FILE *fileFD){
	fseek (fileFD , 0 , SEEK_END);
  	long fileSize = ftell (fileFD);
 	rewind (fileFD);
	return fileSize;
}

void Mediator::getResults(){
	int command_fd = clientList.getActiveClient()->command_fd;

	DIR* dp = opendir(currDir.c_str());
	struct dirent* dirEnt;
	string prim_name;
	while ((dirEnt = readdir(dp)) != NULL){
		if ((strcmp(dirEnt->d_name, ".") != 0) && (strcmp(dirEnt->d_name, "..") != 0)){
			prim_name = string(strtok(dirEnt->d_name, "."));	
			break;
		}
	}
	string tgz = prim_name + ".tgz";
	chdir(currDir.c_str());
	/* tar files */
	string command = "tar zcvf " + tgz + " *";
	system (command.c_str());
	/* count fileSize */
	FILE* fileFD = fopen(tgz.c_str(), "r");
	long fsize = fileSize(fileFD);
	fclose(fileFD);
	chdir("..");
	char fileInfo[512];
	sprintf(fileInfo, "%s|%ld\n", tgz.c_str(), fsize);
	//cout << fileInfo << endl;
	try{
		cout << "FILE_INFO=" << fileInfo << flush;
		ioStream->writen(command_fd, fileInfo, strlen(fileInfo));
	}
	catch(IOStream::IOException){
		cerr << "IOException! Get result...Write to GUI error!" << flush;
		return;
	}
	return;
}


void Mediator::getFile(char* filename){
	int data_fd = clientList.getActiveClient()->data_fd;
	char currentDir[512];
	strcpy(currentDir, currDir.c_str());
	char fullpath[512];
	strcpy(fullpath, currentDir);
	strcat(fullpath, filename);
	//cout << "getfile\t" << fullpath<< endl;;	
	FILE* fileFD = fopen(fullpath, "r");
	if (fileFD == NULL){
		cerr << "Can't open File:" << filename << endl << flush;
		return;
	}
	long fsize = fileSize(fileFD);
	long toSend = fsize;
	long send_buf;
	long buf_size = 1024;
	char buf[buf_size];
	send_buf = min(toSend, buf_size);
	cout << "DataFD: " << data_fd << endl << flush;
	while((fread(buf, 1, send_buf, fileFD) >0) && (toSend >0)){
		try{
			ioStream->writen(data_fd, buf, send_buf);
		}
		catch(IOStream::IOException){
			fclose(fileFD);
			cerr << "IOException, Write file to GUI Error, filename=" << filename << endl << flush;
			return;
		}
		toSend = toSend-send_buf;
		send_buf = min(toSend, buf_size);
	}
	fclose(fileFD);	
	cout << "[File Sent!]\n" << endl << flush;
	return;
}


int
Mediator::BG_put_file(char* file_name, long file_size, int fd){
	putfile(file_name, file_size, fd, fd);
	
}

/*
void Mediator::getFileList(){

	char currentDir[512];
	strcpy(currentDir, currDir.c_str());
		
	int command_fd = clientList.getActiveClient()->command_fd;

	DIR* dp = opendir(currentDir);
	struct dirent* dirEnt;
	while ((dirEnt = readdir(dp)) != NULL){
		char* filename = dirEnt->d_name;
		
		//if(isLogFile(filename)){
			char fullpath[512];
			strcpy(fullpath, currentDir);
			strcat(fullpath, filename);
			FILE* fileFD = fopen(fullpath, "r");
			long fsize = fileSize(fileFD);
			fclose(fileFD);
			char fileInfo[512];
			sprintf(fileInfo, "%s|%ld\n", filename, fsize);
	//		cout << fileInfo ;
			ioStream->writen(command_fd, fileInfo, strlen(fileInfo));			
		//}
	}
		
	ioStream->writen(command_fd, "\n", 1);	
	cout << "-->Get file list done!\n";	
}
*/
