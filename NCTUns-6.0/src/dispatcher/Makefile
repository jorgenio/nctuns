CC 	=  g++ -g  
LINK 	=  $(CC) -lcrypt 
LDOUT 	=  -o 
FLAGS	=  $(INCLUDES) $(DEFINE)
RM	=  rm -f

TARGET   = dispatcher

SRC = 	Dispatcher.C Mediator.C SocketManager.H ServerSocket.C IOStream.C \
		ServerList.C SessionList.C BackGndJob.C JobQueue.C

OBJ = 	Dispatcher.o Mediator.o SocketManager.o ServerSocket.o IOStream.o \
		ServerList.o SessionList.o BackGndJob.o JobQueue.o

HDR = 	Dispatcher.H Mediator.H SocketManager.H ServerSocket.H IOStream.H \
		ServerList.H  ServerNode.H Variables.H SessionList.H SessionNode.H \
		JobQueue.H JobNode.H Global.H

.C.o:
	$(CC) -c $(FLAGS) $(DEBUG) -o $@ $*.C 

	
all:	$(TARGET) 


$(TARGET):	$(OBJ)
			$(LINK) $(FLAGS) $(LDOUT) $@ $(OBJ)  $(LIB)
	 	
clean:	
	$(RM) *.o *~ *.core $(TARGET) 
