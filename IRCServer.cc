
const char * usage =
"                                                               \n"
"IRCServer:                                                   \n"
"                                                               \n"
"Simple server program used to communicate multiple users       \n"
"                                                               \n"
"To use it in one window type:                                  \n"
"                                                               \n"
"   IRCServer <port>                                          \n"
"                                                               \n"
"Where 1024 < port < 65536.                                     \n"
"                                                               \n"
"In another window type:                                        \n"
"                                                               \n"
"   telnet <host> <port>                                        \n"
"                                                               \n"
"where <host> is the name of the machine where talk-server      \n"
"is running. <port> is the port number you used when you run    \n"
"daytime-server.                                                \n"
"                                                               \n";

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <fstream>
#include <string>
#include <iostream>
#include "slist.h"


#include "IRCServer.h"

using namespace std;

int totalUsers = 0;

int QueueLength = 5;
// SLList * userInfo; //initialize username and password info
// SLList * roomMessages; //initialize messages for each room
// SLList * roomUsers; //initialize room array for the users
// int users = 0;
// int rooms = 0;
// int maxMessages = 100;
// int messages = 0;


class UserInfoStruct {
public:
	string username;
	string password;
	UserInfoStruct(char *user, char *pass);
	UserInfoStruct(string user, string pass);
	UserInfoStruct(const char *user, const char *pass);
};


class MessageClass {
public:
	int messageNumber;
	string username;
	string message;

	MessageClass();
};

class RoomMessagesClass {
public:
	int messageCount;
	int currentIndex;
	int gMessageNumber;

	MessageClass messages[100];

	RoomMessagesClass();
	void addMessage(string username, string message);
};

RoomMessagesClass::RoomMessagesClass() {
	messageCount = currentIndex = gMessageNumber = 0;
}


MessageClass::MessageClass() {
	messageNumber = 0;
}

UserInfoStruct::UserInfoStruct(char *user, char *pass) {
	username = string(user);
	password = string(pass);
}

UserInfoStruct::UserInfoStruct(const char *user, const char *pass) {
	username = string(user);
	password = string(pass);
}

UserInfoStruct::UserInfoStruct(string user, string pass) {
	username = user;
	password = pass;
}

void RoomMessagesClass::addMessage(string username, string message) {
	MessageClass m;

	m.messageNumber = gMessageNumber++;
	m.username = username;
	m.message = message;

	messages[currentIndex] = m;

	currentIndex = (currentIndex + 1) % 100;

	if(messageCount < 100) {
		messageCount++;
	}
}

int
IRCServer::open_server_socket(int port) {

	// Set the IP address and port for this server
	struct sockaddr_in serverIPAddress; 
	memset( &serverIPAddress, 0, sizeof(serverIPAddress) );
	serverIPAddress.sin_family = AF_INET;
	serverIPAddress.sin_addr.s_addr = INADDR_ANY;
	serverIPAddress.sin_port = htons((u_short) port);
  
	// Allocate a socket
	int masterSocket =  socket(PF_INET, SOCK_STREAM, 0);
	if ( masterSocket < 0) {
		perror("socket");
		exit( -1 );
	}

	// Set socket options to reuse port. Otherwise we will
	// have to wait about 2 minutes before reusing the sae port number
	int optval = 1; 
	int err = setsockopt(masterSocket, SOL_SOCKET, SO_REUSEADDR, 
			     (char *) &optval, sizeof( int ) );
	
	// Bind the socket to the IP address and port
	int error = bind( masterSocket,
			  (struct sockaddr *)&serverIPAddress,
			  sizeof(serverIPAddress) );
	if ( error ) {
		perror("bind");
		exit( -1 );
	}
	
	// Put socket in listening mode and set the 
	// size of the queue of unprocessed connections
	error = listen( masterSocket, QueueLength);
	if ( error ) {
		perror("listen");
		exit( -1 );
	}

	return masterSocket;
}

void
IRCServer::runServer(int port)
{
	int masterSocket = open_server_socket(port);

	initialize();
	
	while ( 1 ) {
		
		// Accept incoming connections
		struct sockaddr_in clientIPAddress;
		int alen = sizeof( clientIPAddress );
		int slaveSocket = accept( masterSocket,
					  (struct sockaddr *)&clientIPAddress,
					  (socklen_t*)&alen);
		
		if ( slaveSocket < 0 ) {
			perror( "accept" );
			exit( -1 );
		}
		
		// Process request.
		processRequest( slaveSocket );		
	}
}

int
main( int argc, char ** argv )
{
	// Print usage if not enough arguments
	if ( argc < 2 ) {
		fprintf( stderr, "%s", usage );
		exit( -1 );
	}
	
	// Get the port from the arguments
	int port = atoi( argv[1] );

	IRCServer ircServer;

	// It will never return
	ircServer.runServer(port);
	
}

//
// Commands:
//   Commands are started by the client.
//
//   Request: ADD-USER <USER> <PASSWD>\r\n
//   Answer: OK\r\n or DENIED\r\n
//
//   REQUEST: GET-ALL-USERS <USER> <PASSWD>\r\n
//   Answer: USER1\r\n
//            USER2\r\n
//            ...
//            \r\n
//
//   REQUEST: CREATE-ROOM <USER> <PASSWD> <ROOM>\r\n
//   Answer: OK\n or DENIED\r\n
//
//   Request: LIST-ROOMS <USER> <PASSWD>\r\n
//   Answer: room1\r\n
//           room2\r\n
//           ...
//           \r\n
//
//   Request: ENTER-ROOM <USER> <PASSWD> <ROOM>\r\n
//   Answer: OK\n or DENIED\r\n
//
//   Request: LEAVE-ROOM <USER> <PASSWD>\r\n
//   Answer: OK\n or DENIED\r\n
//
//   Request: SEND-MESSAGE <USER> <PASSWD> <MESSAGE> <ROOM>\n
//   Answer: OK\n or DENIED\n
//
//   Request: GET-MESSAGES <USER> <PASSWD> <LAST-MESSAGE-NUM> <ROOM>\r\n
//   Answer: MSGNUM1 USER1 MESSAGE1\r\n
//           MSGNUM2 USER2 MESSAGE2\r\n
//           MSGNUM3 USER2 MESSAGE2\r\n
//           ...\r\n
//           \r\n
//
//    REQUEST: GET-USERS-IN-ROOM <USER> <PASSWD> <ROOM>\r\n
//    Answer: USER1\r\n
//            USER2\r\n
//            ...
//            \r\n
//

void
IRCServer::processRequest( int fd )
{
	// Buffer used to store the comand received from the client
	const int MaxCommandLine = 1024;
	char commandLine[ MaxCommandLine + 1 ];
	int commandLineLength = 0;
	int n;
	
	// Currently character read
	unsigned char prevChar = 0;
	unsigned char newChar = 0;
	
	//
	// The client should send COMMAND-LINE\n
	// Read the name of the client character by character until a
	// \n is found.
	//

	// Read character by character until a \n is found or the command string is full.
	while ( commandLineLength < MaxCommandLine &&
		read( fd, &newChar, 1) > 0 ) {

		if (newChar == '\n' && prevChar == '\r') {
			break;
		}
		
		commandLine[ commandLineLength ] = newChar;
		commandLineLength++;

		prevChar = newChar;
	}
	
	// Add null character at the end of the string
	// Eliminate last \r
	commandLineLength--;
        commandLine[ commandLineLength ] = 0;

	printf("RECEIVED: %s\n", commandLine);

	// printf("The commandLine has the following format:\n");
	// printf("COMMAND <user> <password> <arguments>. See below.\n");
	// printf("You need to separate the commandLine into those components\n");
	// printf("For now, command, user, and password are hardwired.\n");

	const char * command;
	const char * user;
	const char * password;
	const char * args = "";

	//PARSE AND BREAK UP COMPONENTS

	int count = 0;
	unsigned char currentChar;
	int argnum = 0;
	int i = 0;
	char tempArgs[MaxCommandLine];
	int argsCount = 0;

	char * nextStartPosition = commandLine;

	int error = 0;

	while(true) {
		if(argnum == 0) {
			char * space = strstr(nextStartPosition, " ");
			if(space != NULL) {
				*space = '\0';
				command = strdup(nextStartPosition);
				argnum++;
				nextStartPosition = space + 1;
			}
			else {
				error = 1;
				break;
			}
		}
		else if(argnum == 1) {
			char * space = strstr(nextStartPosition, " ");
			if(space != NULL) {
				*space = '\0';
				user = strdup(nextStartPosition);
				argnum++;
				nextStartPosition = space + 1;
			}
			else {
				error = 1;
				break;
			}
		}
		else if(argnum == 2) {
			char * space = strstr(nextStartPosition, " ");
			if(space != NULL) {
				*space = '\0';
				password = strdup(nextStartPosition);
				argnum++;
				nextStartPosition = space + 1;
			}
			else {
				space = nextStartPosition;
				password = strdup(nextStartPosition);
				break;
			}
		}
		else if(argnum == 3) {
			//char * space = strstr(nextStartPosition, " ");
			if(*nextStartPosition != '\0') {
				args = strdup(nextStartPosition);
			}
			break;
		}

		else {
			break;
		}
	}

	if(error) {
		const char * msg = "Error: improper arguments\r\n";
		write(fd, msg, strlen(msg));
		close(fd);
		return;
	}

	printf("command=%s\n", command);
	printf("user=%s\n", user);
	printf( "password=%s\n", password );
	printf("args=%s\n", args);

	if (!strcmp(command, "ADD-USER")) {
		addUser(fd, user, password);
	}
	else if (!strcmp(command, "ENTER-ROOM")) {
		enterRoom(fd, user, password, args);
	}
	else if (!strcmp(command, "LEAVE-ROOM")) {
		leaveRoom(fd, user, password, args);
	}
	else if (!strcmp(command, "SEND-MESSAGE")) {
		sendMessage(fd, user, password, args);
	}
	else if (!strcmp(command, "GET-MESSAGES")) {
		getMessages(fd, user, password, args);
	}
	else if (!strcmp(command, "GET-USERS-IN-ROOM")) {
		getUsersInRoom(fd, user, password, args);
	}
	else if (!strcmp(command, "GET-ALL-USERS")) {
		getAllUsers(fd, user, password);
	}
	else if(!strcmp(command, "LIST-ROOMS")) {
		listRooms(fd, user, password);
	}
	else if(!strcmp(command, "CREATE-ROOM")) {
		createRoom(fd, user, password, args);
	}
	else {
		const char * msg =  "UNKNOWN COMMAND\r\n";
		write(fd, msg, strlen(msg));
	}

	// Send OK answer
	//const char * msg =  "OK\n";
	//write(fd, msg, strlen(msg));

	close(fd);	
}

void
IRCServer::initialize()
{

	// Open password file
	
	myfile.open(PASSWORD_FILE, ofstream::out | ofstream::app);
	
	ifstream myfile2 (PASSWORD_FILE);

	// Take info from txt file and put it in data structure

	string line ("temp:orary");
	string line2 ();
	
	userInfo = new HashTableVoid();

	while(!myfile.eof()) {
		getline (myfile2, line);
		int index = line.find(":");
		if(index != -1) {
			string user = line.substr(0, index);
			string password = line.substr(index + 1);

			// now store the user info into the HashTable

			UserInfoStruct * info = new UserInfoStruct(user, password);

			const char * key = user.c_str();

			userInfo->insertItem(key, info);

			totalUsers++;
		}
		else {
			break;
		}

	}


	// Initialize users in room
	
	roomUsers = new HashTableVoid();

	// Initalize message list
	
	roomMessages = new HashTableVoid();

}

bool
IRCServer::checkPassword(int fd, const char * user, const char * password) {
	// Here check the password

	// loop through the hashtable and check if the password matches the name
	
	UserInfoStruct * info;

	if(userInfo->find(user, (void **)&info)) {
		// found the username and password


		if(strcmp(password, info->password.c_str()) == 0) {
				return true;
		}

		else {
			return false;
		}

	}

	else {
		return false;
	}
	
}

void
IRCServer::addUser(int fd, const char * user, const char * password)
{
	// Here add a new user. For now always return OK.


	UserInfoStruct * info = new UserInfoStruct(user, password);

	if(userInfo->find(user, (void **)&info)) {
		// User already exists
		const char * msg = "DENIED\r\n";
		write(fd, msg, strlen(msg));
	}

	else {
		// add user to the user info table and file

		myfile << user;
		myfile << ":";
		myfile << password;
		myfile << "\n";
		myfile.flush();

		info->username = user;
		info->password = password;

		userInfo->insertItem(user, info);

		totalUsers++;

		const char * msg = "OK\r\n";
		write(fd, msg, strlen(msg));

	}

	return;		
}

void
IRCServer::enterRoom(int fd, const char * user, const char * password, const char * args)
{
	if(checkPassword(fd, user, password)) {
		// password is correct
		
		// check if room exists
		SLList * info = (SLList *) malloc(sizeof(SLList));

		if(roomUsers->find(args, (void **)&info)) {
			// room exists

			// check if user already exists, don't want to have repeats

			if(llist_exists(info, user)) {
				const char * msg = "OK\r\n";
				write(fd, msg, strlen(msg));
				return;
			}

			sllist_add_end(info, (char *) user);

			const char * msg = "OK\r\n";
			write(fd, msg, strlen(msg));
		}
		else {
			const char * msg = "ERROR (No room)\r\n";
			write(fd, msg, strlen(msg));
		}

		// add user to the room in the hashtable

		
	}
	else {
		const char * msg = "ERROR (Wrong password)\r\n";
		write(fd, msg, strlen(msg));
	}

}



void
IRCServer::leaveRoom(int fd, const char * user, const char * password, const char * args)
{
	if(checkPassword(fd, user, password)) {
		// password is correct

		//remove user from the room 

		SLList * info = (SLList *) malloc(sizeof(SLList));

		if(roomUsers->find(args, (void **)&info)) {
			// room exists
			//
			// Now check if user was orignally in the room, if not print ERROR (No user in room)

			if(llist_exists(info, user)) {
				// User exists
				sllist_remove(info, (char *) user);

				const char * msg = "OK\r\n";
				write(fd, msg, strlen(msg));

				return;
			
			}
			const char * msg = "ERROR (No user in room)\r\n";
			write(fd, msg, strlen(msg));
		}
		else {
			const char * msg = "DENIED\r\n";
			write(fd, msg, strlen(msg));
		}

	}

	else {
		const char * msg = "ERROR (Wrong password)\r\n";
		write(fd, msg, strlen(msg));
	}
}

void
IRCServer::sendMessage(int fd, const char * user, const char * password, const char * args)
{
	if(checkPassword(fd, user, password)) {
		// password is correct

		// parse args to check if room is correct and then add the rest onto the messages

		string arg(args);

		int index = arg.find(" ");
		if(index != -1) {
			string room0 = arg.substr(0, index);
			string msg0 = arg.substr(index + 1);

			// Now check if user is in the room

			SLList * info = (SLList *) malloc(sizeof(SLList));

			if(roomUsers->find((const char *) room0.c_str(), (void **)&info)) {
				// room exists

				SLEntry *e = info->head;
				while(e != NULL) {
					if(strcmp(e->value, user) == 0) {
						// user exists in room

						RoomMessagesClass *rmc;

						roomMessages->find((const char *)room0.c_str(), (void **)&rmc);
						rmc->addMessage(string(user), msg0);

						dprintf(fd, "OK\r\n");
						return;
					}
					e = e->next;
				}

				const char * msg = "ERROR (user not in room)\r\n";
				write(fd, msg, strlen(msg));
				return;
			}

			
		}

		const char * msg = "ERROR (user not in room)\r\n";
		write(fd, msg, strlen(msg));
		return;
		// make sure messages does not exceed 100
	}

	const char * msg = "ERROR (Wrong password)\r\n";
	write(fd, msg, strlen(msg));
}

// TODO: UPDATE THIS METHOD
void
IRCServer::getMessages(int fd, const char * user, const char * password, const char * args)
{
	if(checkPassword(fd, user, password)) {
		// password is correct


		string arg(args);

		int index = arg.find(" ");
		if(index != -1) {
			string lastMessageNumberStr = arg.substr(0, index);
			string room0 = arg.substr(index + 1);

			int lastMessageNumber = atoi(lastMessageNumberStr.c_str());

			// Now check if user is in the room

			SLList * info = (SLList *) malloc(sizeof(SLList));


			RoomMessagesClass *rmc;

			if(roomMessages->find((const char *)room0.c_str(), (void **)&rmc)) {
				// room exists

				// TODO: Check if user exists in the room, if not in room print "ERROR (User not in room)"

				// TODO: If last-message-num is >= most recent message print "NO-NEW-MESSAGES"

				// TODO: make sure it prints the messages starting the number AFTER last-message-number

				if(roomUsers->find((const char *) room0.c_str(), (void **)&info)) {
					if(llist_exists(info, user)) {
						// user exists
					}
					else {
						const char * msg = "ERROR (User not in room)\r\n";
						write(fd, msg, strlen(msg));
						return;
					}
				}

				if(lastMessageNumber >= rmc->messageCount) {
					const char * msg = "NO-NEW-MESSAGES\r\n";
					write(fd, msg, strlen(msg));
					return;
				}			

				for(int i = rmc->currentIndex; i < rmc->messageCount; i++) {
					MessageClass m = rmc->messages[i];
					if(m.messageNumber >= lastMessageNumber) {
						dprintf(fd, "%d %s %s\r\n", m.messageNumber, m.username.c_str(), m.message.c_str());
					}
				}

				for(int i = 0; i < rmc->currentIndex; i++) {
					MessageClass m = rmc->messages[i];
					if(m.messageNumber >= lastMessageNumber) {
						dprintf(fd, "%d %s %s\r\n", m.messageNumber, m.username.c_str(), m.message.c_str());
					}
				}

				// just added
				const char * msg = "\r\n";
				write(fd, msg, strlen(msg));
			}
			return;
		}
	}

	const char * msg = "ERROR (Wrong password)\r\n";
	write(fd, msg, strlen(msg));
}

// TODO: UPDATE THIS METHOD
void
IRCServer::getUsersInRoom(int fd, const char * user, const char * password, const char * args)
{
	if(checkPassword(fd, user, password)) {
		// password is correct

		// check if room is correct

		SLList * info = (SLList *) malloc(sizeof(SLList));

		if(roomUsers->find(args, (void **)&info)) {
			// room exists

			if(info->head == NULL) {
				const char * msg = "\r\n";
				write(fd, msg, strlen(msg));
			}

			else {			
				llist_sort(info);
				sllist_print(info, fd);
				const char * msg = "\r\n";
				write(fd, msg, strlen(msg));
			}

			// TODO: print out list of users in the room in ALPHABETICAL ORDER
		}
		else {
			const char * msg = "DENIED\r\n";
			write(fd, msg, strlen(msg));
		}


	}
	else {
		const char * msg = "ERROR (Wrong password)\r\n";
		write(fd, msg, strlen(msg));
	}
	
}

// TODO: UPDATE THIS METHOD
void
IRCServer::getAllUsers(int fd, const char * user, const char * password)
{
	if(checkPassword(fd, user, password)) {
		// password is correct
		
		string * allUsers = sortUsers();

		for(int i = 0; i < totalUsers; i++) {
			dprintf(fd, "%s\r\n", allUsers[i].c_str());

		}
		const char * msg = "\r\n";
		write(fd, msg, strlen(msg));

	}
	else {
		const char * msg = "ERROR (Wrong password)\r\n";
		write(fd, msg, strlen(msg));
	}
}

void
IRCServer::createRoom(int fd, const char * user, const char * password, const char * args) {
	if(checkPassword(fd, user, password)) {
		// password is correct

		// check if room doesn't exist

		// add code

		SLList * info = (SLList *) malloc(sizeof(SLList));

		if(roomUsers->find(args, (void **)&info)) {
			// User already exists
			const char * msg = "DENIED\r\n";
			write(fd, msg, strlen(msg));
		}
		else {
			sllist_init(info);
			roomUsers->insertItem(args, info);

			RoomMessagesClass *h = new RoomMessagesClass();
			roomMessages->insertItem(args, h);

			const char * msg = "OK\r\n";
			write(fd, msg, strlen(msg));
		}
	}
	else {
		const char * msg = "ERROR (Wrong password)\r\n";
		write(fd, msg, strlen(msg));
	}
}

// TODO: UPDATE THIS METHOD
void
IRCServer::listRooms(int fd, const char * user, const char * password) {

	// TODO: check if it needs to be alphabetized when it prints

	if(checkPassword(fd, user, password)) {
		// password is correct

		// list out every room name

		HashTableVoidIterator it(roomUsers);

		const char * key;
		void * data;

		while(it.next(key, data)) {
			dprintf(fd, "%s\r\n", key);
		}

	}
	else {
		const char * msg = "ERROR (Wrong password)\r\n";
		write(fd, msg, strlen(msg));
	}
}

// TODO: UPDATE THIS METHOD


std::string* IRCServer::sortUsers() {
	// sort users alphabetically in hash table

	// create an array the size of total users

	// TODO: Fix error in the sort method

	string *allNames = new string[totalUsers];

	HashTableVoidIterator it(userInfo);

	const char * key;
	void * data;
	int i;
	i = 0;

	while(it.next(key, data)) {
		
		UserInfoStruct *data2 = (UserInfoStruct *)data;
		string msg = data2->username;
		allNames[i] = msg;
		i++;
	}

	// then use bubble sort on them

	int swap;

	do {
		swap = 0;



		for(int count = 0; count < totalUsers - 1; count++) {
			const char * current = allNames[count].c_str();
			const char * next = allNames[count + 1].c_str();

			if(strcasecmp(current, next) > 0) {
				string temp = current;
				allNames[count] = next;
				allNames[count + 1] = temp;
				swap = 1;
			}
		}

	}while(swap);

	return allNames;

}