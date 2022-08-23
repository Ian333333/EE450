#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <iostream>
using namespace std;

struct transaction {
    int id;
    char username1[100];
    char username2[100];
    int amount;
};

int create_socket() {
    int sock;
	struct sockaddr_in server_addr;
	char LOCALHOST[] = "127.0.0.1";
	int PORT = 25431;	

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == 0) {	// error checking
		perror("Socket failed to create");
		exit(EXIT_FAILURE);
    }

	cout << "The client B is up and running." << endl;
	
	server_addr.sin_family = AF_INET;	// host byte order
	server_addr.sin_addr.s_addr = inet_addr(LOCALHOST);
	server_addr.sin_port = htons(PORT);	// short, network byte order
	memset(&(server_addr.sin_zero), '\0', 8);	// zero the rest of the struct

	if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
		perror("Connection failed");
		exit(EXIT_FAILURE);
	}

	return sock;
}

int main(int argc, char *argv[]) {
	int sock;
	char buffer[1024] = {0};
	char j = 'B';

	enum Operation {CHECK_WALLET = 1, TXCOINS, TXLIST, stats};

	if (argc == 2) {
		if (strcmp(argv[1], "TXLIST") == 0) {	// request full text version of all the transactions
			int operation;
			operation = TXLIST;
			sock = create_socket();
			send(sock, &operation, sizeof(operation), 0);
			cout << "ClientB sent a sorted list request to the main server." << endl;
		} else {	// username1
			int operation;
			operation =  CHECK_WALLET;
			char *username = argv[1];
			sock = create_socket();
			send(sock, &operation, sizeof(operation), 0);
			send(sock, &j, sizeof(j), 0);
			send(sock, username, strlen(username), 0);
			cout << "\"" << username << "\" sent a balance enquiry request to the main server." << endl;

			int balance = 0;
			recv(sock, &balance, 1024, 0);
			if (balance >= 0) {
				cout << "The current balance of \"" << username << "\" is : " << balance << " alicoins." << endl;
			} else {
				cout << "Unable to proceed with the request as " << username << " is not part of the network." << endl;
			}
		}
	} else if (argc == 3) {	// check stats
		int operation;
		operation = stats;
		char *username = argv[1];
		
		// send request
		sock = create_socket();
		send(sock, &operation, sizeof(operation), 0);
		send(sock, username, strlen(username), 0);

		char buffer[1024] = {0};
		memset(buffer, 0, sizeof(buffer));
		// recv(sock, buffer, 1024, 0);
		// cout << buffer << endl;

		int num = 0;
		char username2[1024] = {0};
		char number[1024] = {0};
		char amount[1024] = {0};
		recv(sock, &num, sizeof(num), 0);
		cout << "Rank\tUsername\tNumber of transactions made with user\tTransfer Amount" << endl;
		
		for (int i = 0; i < num; i++) {
			memset(username2, 0, sizeof(username2));
			memset(number, 0, sizeof(number));
			memset(amount, 0, sizeof(amount));
			recv(sock, username2, 1024, 0);
			recv(sock, number, 1024, 0);
			recv(sock, amount, 1024, 0);
			cout << i + 1 << "\t" << username2 << "\t\t" << number << "\t\t\t\t\t" << amount << endl;
		}
	} else if (argc == 4) {	// transfer coins
		int operation;
		operation = TXCOINS;
		char *username1 = argv[1];
		char *username2 = argv[2];
		int amount = atoi(argv[3]);
		
		// send request
		sock = create_socket();
		send(sock, &operation, sizeof(operation), 0);
		send(sock, &j, sizeof(j), 0);
		send(sock, username1, strlen(username1), 0);
		send(sock, username2, strlen(username2), 0);
		send(sock, &amount, sizeof(amount), 0);
		cout << "\"" << username1 << "\" has requested to transfer " << amount << " coins to \"" << username2 << "\"." << endl;
		
		// receive response
		int indicator = 0;
		recv(sock, &indicator, 1024, 0);
		int balance, remain;
		if (indicator == 0) {
			recv(sock, &balance, 1024, 0);
			cout << "The current balance of \"" << username1 << "\" is : " << balance << " alicoins." << endl;
			if (balance - amount >= 0) {
				recv(sock, &remain, 1024, 0);
				cout << "\"" << username1 << "\" successfully transferred " << amount << " alicoins to \"" << username2 << "\"." << endl;
				cout << "The current balance of \"" << username1 << "\" is : " << remain << " alicoins." << endl;
			} else {
				cout << "\"" << username1 << "\" was unable to transfer " << amount << " alicoins to \"" << username2 << "\" because of insufficient balance." << endl;
				cout << "The current balance of \"" << username1 << "\" is : " << balance << " alicoins." << endl;
			}
		} else if (indicator == 3) {
			cout << "Unable to proceed with the transaction as \"" << username1 << "\" and \"" << username2 << "\" are not part of the network." << endl;
		} else {
			cout << "Unable to proceed with the transaction as \"" << username1 << "\"/\"" << username2 << "\" is not part of the network." << endl;
		}
	
	}
	close(sock);
	return 0;
}
