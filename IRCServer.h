
#ifndef IRC_SERVER
#define IRC_SERVER
#include "HashTableVoid.h"
#include <fstream>
#include <string.h>

#define PASSWORD_FILE "password.txt"


class IRCServer {
	// Add any variables you need
	HashTableVoid * userInfo;
	HashTableVoid * roomUsers;
	HashTableVoid * roomMessages;
	std::ofstream myfile;
	std::ifstream myfile2;
	

private:
	int open_server_socket(int port);

public:
	void refresh();
	void initialize();
	bool checkPassword(int fd, const char * user, const char * password);
	void processRequest( int socket );
	void addUser(int fd, const char * user, const char * password);
	void enterRoom(int fd, const char * user, const char * password, const char * args);
	void leaveRoom(int fd, const char * user, const char * password, const char * args);
	void sendMessage(int fd, const char * user, const char * password, const char * args);
	void getMessages(int fd, const char * user, const char * password, const char * args);
	void getUsersInRoom(int fd, const char * user, const char * password, const char * args);
	void getAllUsers(int fd, const char * user, const char * password);
	void runServer(int port);
	void createRoom(int fd, const char * user, const char * password, const char * args);
	void listRooms(int fd, const char * user, const char * password);
	std::string* sortUsers();
};

#endif
