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
#include <fstream>
#include <vector>
using namespace std;

struct transaction {
	int id;
	char username1[100];
	char username2[100];
	int amount;
};

vector<struct transaction> read_block() {

	vector<struct transaction> t;

	ifstream fin("block1.txt");
	if (fin.fail()) {
		cout << "Fail to open block1.txt" << endl;
		exit(0);
	}

	struct transaction temp;

	while (!fin.eof()) {
		memset(temp.username1, 0, sizeof(temp.username1));
		memset(temp.username2, 0, sizeof(temp.username2));
		fin >> temp.id >> temp.username1 >> temp.username2 >> temp.amount;
		// cout << temp.id << " " << temp.username1 << " " << temp.username2 << " " << temp.amount << endl;
		t.push_back(temp);
	}
	t.pop_back();

	fin.close();

	return t;
}

bool write_block(int id, char username1[1024], char username2[1024], int amount) {
	vector<struct transaction> t = read_block();

	ofstream fout("block1.txt");

	for (int i = 0; i < t.size(); i++) {
		fout << t[i].id << " " << t[i].username1 << " " << t[i].username2 << " " << t[i].amount << endl;
	}
	
	fout << id << " " << username1 << " " << username2 << " " << amount << endl;

	fout.close();

	return true;
}

int main(int argc, char *argv[]) {
    int sock;
    struct sockaddr_in server_addr, serverM_addr;
	char LOCALHOST[] = "127.0.0.1";
    int PORT = 21431;
	int PORT_M = 24431;

	enum Operation {CHECK_WALLET = 1, TXCOINS, TXLIST, stats};

    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == 0) {   // error checking
        perror("Failed to create socket");
        exit(EXIT_FAILURE);
    }

	server_addr.sin_family = AF_INET;   // host byte order
    server_addr.sin_port = htons(PORT);   // short, network byte order
    server_addr.sin_addr.s_addr = inet_addr(LOCALHOST);
    memset(&(server_addr.sin_zero), '\0', 8);   // zero the rest of the struct

	if (bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Failed to bind");
        exit(EXIT_FAILURE);
    }

	cout << "The ServerA is up and running using UDP on port " << PORT << "." << endl;

    serverM_addr.sin_family = AF_INET;   // host byte order
    serverM_addr.sin_port = htons(PORT_M);   // short, network byte order
    serverM_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    memset(&(serverM_addr.sin_zero), '\0', 8);   // zero the rest of the struct

	int serverM_addr_len = sizeof(serverM_addr);

	while (true) {
		// receive operation
		int operation = 0;
		recvfrom(sock, &operation, 1024, MSG_WAITALL, (struct sockaddr *)&serverM_addr, (socklen_t *)&serverM_addr_len);

		cout << "The ServerA received a request from the Main Server." << endl;

		vector <struct transaction> t = read_block();
		switch (operation) {
			case CHECK_WALLET: {
				// cout << "CHECK WALLET" << endl;
				int num = t.size();
				sendto(sock, &num, sizeof(num), MSG_CONFIRM, (struct sockaddr *)&serverM_addr, sizeof(serverM_addr));
				for (int i = 0; i < num; i++) {
					sendto(sock, &t[i], sizeof(t[i]), MSG_CONFIRM, (struct sockaddr *)&serverM_addr, sizeof(serverM_addr));
				}
				break;
			}
			case TXCOINS: {
				int id;
				char username1[1024];
				char username2[1024];
				int amount;
				memset(username1, 0, sizeof(username1));
				memset(username2, 0, sizeof(username2));
				recvfrom(sock, &id, 1024, MSG_WAITALL, (struct sockaddr *)&serverM_addr, (socklen_t *)&serverM_addr_len);
				recvfrom(sock, username1, 1024, MSG_WAITALL, (struct sockaddr *)&serverM_addr, (socklen_t *)&serverM_addr_len);
				recvfrom(sock, username2, 1024, MSG_WAITALL, (struct sockaddr *)&serverM_addr, (socklen_t *)&serverM_addr_len);
				recvfrom(sock, &amount, 1024, MSG_WAITALL, (struct sockaddr *)&serverM_addr, (socklen_t *)&serverM_addr_len);
	
				bool succ = write_block(id, username1, username2, amount);
				sendto(sock, &succ, sizeof(succ), MSG_CONFIRM, (struct sockaddr *)&serverM_addr, sizeof(serverM_addr));
				break;
			}
			case TXLIST: {
				break;
			}
			case stats: {
				break;
			}
		}

		cout << "The ServerA finished sending the response to the Main Server." << endl;
	}
	
	// sendto(sock, message, strlen(message), MSG_CONFIRM, (struct sockaddr *)&serverM_addr, sizeof(serverM_addr));


	return 0;
}
