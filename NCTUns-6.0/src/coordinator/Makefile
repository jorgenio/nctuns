CC 	=  g++ -g  
LINK 	=  $(CC) -lcrypt -pthread
LDOUT 	=  -o
FLAGS	=  $(INCLUDES) $(DEFINE)
RM	=  rm -f

TARGET  =  coordinator

SRC =	Coordinator.C Mediator.C SocketManager.C DatagramSocket.C		\
		UnixSocket.C Socket.C ServerSocket.C IOStream.C ProcessList.C	\
		JobControl.C FileManager.C SessionList.C SessionControl.C	\
		ClientList.C RunTime.C RemoteHandler.C

OBJ =	Coordinator.o Mediator.o SocketManager.o DatagramSocket.o		\
		UnixSocket.o Socket.o ServerSocket.o IOStream.o ProcessList.o	\
		JobControl.o FileManager.o SessionList.o SessionControl.o 	\
		ClientList.o RunTime.o RemoteHandler.o

HDR =	Coordinator.H Mediator.H SocketManager.H DatagramSocket.H		\
		UnixSocket.H Socket.H ServerSocket.H IOStream.H ProcessList.H	\
		SessionList.H ClientList.H ClientNode.H SessionNode.H RemoteHandler.H

.C.o:
		$(CC) -c $(FLAGS) $(DEBUG) -o $@ $*.C 

	
all:	$(TARGET) 


Mediator.o: INCLUDES += -I../nctuns
$(TARGET):	$(OBJ)
			$(LINK) $(FLAGS) $(LDOUT) $@ $(OBJ)  $(LIB)
	 	
clean:	
	$(RM) *.o *~ *.core $(TARGET)
