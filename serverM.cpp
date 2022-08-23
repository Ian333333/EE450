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
#include <pthread.h>
#include <vector>
#include <map>
#include <fstream>
#include <iostream>
#include <algorithm>
using namespace std;

int sock_udp = 0;
struct sockaddr_in serverA_addr, serverB_addr, serverC_addr;
enum Operation {CHECK_WALLET = 1, TXCOINS, TXLIST, stats};

struct transaction {
    int id;
    char username1[100];
    char username2[100];
    int amount;
};

struct st {
	int number;
	int amount;
};

vector <struct transaction> filter_username(vector <struct transaction> t, char *username) {
	vector <struct transaction> t2;
	for (int i = 0; i < t.size(); i++) {
		// cout << t[i].username2 << " " << username << endl;
		if (strcmp(t[i].username1, username) == 0 || strcmp(t[i].username2, username) == 0) {
			t2.push_back(t[i]);
		}
	}
	return t2;
}

// handle query request
vector<struct transaction> request_query(struct sockaddr_in server_addr) {
	int server_addr_len = sizeof(server_addr);
	int operation = CHECK_WALLET;
	sendto(sock_udp, &operation, sizeof(operation), MSG_CONFIRM, (struct sockaddr *)&server_addr, sizeof(server_addr));
   int num = 0;
	recvfrom(sock_udp, &num, 1024, MSG_WAITALL, (struct sockaddr *)&server_addr, (socklen_t *)&server_addr_len);
	vector<struct transaction> t;
	struct transaction temp;
	for (int i = 0; i < num; i++) {
		memset(temp.username1, 0, sizeof(temp.username1));
		memset(temp.username2, 0, sizeof(temp.username2));
		recvfrom(sock_udp, &temp, 1024, MSG_WAITALL, (struct sockaddr *)&server_addr, (socklen_t *)&server_addr_len);
		t.push_back(temp);
		// cout << temp.id << " " << temp.username1 << " " << temp.username2 << " " << temp.amount << endl;
	}
	return t;
}

vector <struct transaction> combine_data() {
	cout << "The main server sent a request to server A." << endl;
	vector<struct transaction> t1 = request_query(serverA_addr);	// request data from serverA
	cout << "The main server received transactions from Server A using UDP over port 21431." << endl;
	cout << "The main server sent a request to server B." << endl;
    vector<struct transaction> t2 = request_query(serverB_addr);    // request data from serverB
    cout << "The main server received transactions from Server B using UDP over port 21431." << endl;
	cout << "The main server sent a request to server C." << endl;
    vector<struct transaction> t3 = request_query(serverC_addr);    // request data from serverC
    cout << "The main server received transactions from Server C using UDP over port 21431." << endl;
	vector<struct transaction> t;
	t.insert(t.end(), t1.begin(), t1.end());	// combine data
	t.insert(t.end(), t2.begin(), t2.end());
	t.insert(t.end(), t3.begin(), t3.end());
	// cout << t.size() << endl;
	return t;
}

int check_wallet(vector <struct transaction> t, char *username) {
	t = filter_username(t, username);
	int balance = 1000;
	if (t.size() == 0) {	// no record found
		cout << "Unable to proceed with the request as " << username << " is not part of the network." << endl;
		balance = -1;
	}
	for (int i = 0; i < t.size(); i++) {
		if (strcmp(t[i].username1, username) == 0) {
			balance -= t[i].amount;
		} else {
			balance += t[i].amount;
		}
	}
	/*
	transaction = request_query(serverB_addr, username);
	for (int i = 0; i < transaction.size(); i++) {
		balance += transaction[i];
	}*/
	return balance;
}

int find_id(vector <struct transaction> t) {
   int max_id = -1;
   for (int i = 0; i < t.size(); i++) {
      if (max_id < t[i].id) {
         max_id = t[i].id;
      }
   }
   return max_id + 1;
}

// store the transaction record to server
void store(vector <struct transaction> t, char username1[1024], char username2[1024], int amount, struct sockaddr_in server_addr) {
	int id = find_id(t);
	int operation = TXCOINS;
	int server_addr_len = sizeof(server_addr);
	sendto(sock_udp, &operation, sizeof(operation), MSG_CONFIRM, (struct sockaddr *)&server_addr, sizeof(server_addr));
	sendto(sock_udp, &id, sizeof(id), MSG_CONFIRM, (struct sockaddr *)&server_addr, sizeof(server_addr));
	sendto(sock_udp, username1, strlen(username1), MSG_CONFIRM, (struct sockaddr *)&server_addr, sizeof(server_addr));
	sendto(sock_udp, username2, strlen(username2), MSG_CONFIRM, (struct sockaddr *)&server_addr, sizeof(server_addr));
	sendto(sock_udp, &amount, sizeof(amount), MSG_CONFIRM, (struct sockaddr *)&server_addr, sizeof(server_addr));
	bool succ;
	recvfrom(sock_udp, &succ, 1024, MSG_WAITALL, (struct sockaddr *)&server_addr, (socklen_t *)&server_addr_len);
	// cout << succ << endl;
}

int cmp(const void *a, const void *b) {
	return (*(struct transaction *)a).id - (*(struct transaction *)b).id;
}

bool cmp_map(const pair<string, struct st> &a, const pair<string, struct st> &b) {
    return a.second.number > b.second.number;
}

vector<pair<string, struct st> > check_stats(vector <struct transaction> t, char username[1024]) {
	map <string, struct st> m;
	for (int i = 0; i < t.size(); i++) {
		if (strcmp(username, t[i].username1) == 0) {
			string s = t[i].username2;
			map<string, struct st>::iterator it = m.find(s);
			if (it != m.end()) {
				it->second.number++;
				it->second.amount -= t[i].amount;
			} else {
				struct st record;
				record.number = 1;
				record.amount = -t[i].amount;
				m[s] = record;
			}
		} else if (strcmp(username, t[i].username2) == 0) {
			string s = t[i].username1;
            map<string, struct st>::iterator it = m.find(s);
            if (it != m.end()) {
                it->second.number++;
                it->second.amount += t[i].amount;
            } else {
                struct st record;
                record.number = 1;
                record.amount = t[i].amount;
                m[s] = record;
            }
		}
	}

	vector<pair<string, struct st> > v(m.begin(), m.end());
	sort(v.begin(), v.end(), cmp_map);	// sort the map by number of transaction made by users
	return v;
}

// multithread to create TCP socket with clientA and clientB
void *thread_tcp(void *arg) {
	int PORT = *(int *)arg;

    int sock, child_socket;
    struct sockaddr_in server_addr, client_addr;
	int client_addr_len = sizeof(client_addr);
	char buffer[1024] = {0};
    char message[1024];
	char LOCALHOST[] = "127.0.0.1";

	srand((unsigned)time(NULL));

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == 0) {   // create tcp socket and error checking
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
	
	if (listen(sock, 3) < 0 ) {
		perror("Failed to listen");
		exit(EXIT_FAILURE);
	}
   
    while (true) {
        if ((child_socket = accept(sock, (struct sockaddr *)&client_addr, (socklen_t *)&client_addr_len)) < 0) {	// accept client connection
		    perror("Failed to accept");
		    exit(EXIT_FAILURE);
	    }
		int operation = 0;
	    recv(child_socket, &operation, 1024, 0);	// receive operation
		switch (operation) {
			case CHECK_WALLET: {
				char username[1024] = {0};
				char j = 0;	// client j
				recv(child_socket, &j, 1024, 0);
				memset(username, 0, sizeof(username));
				recv(child_socket, username, 1024, 0);	// recieve username from client
				cout << "The main server received input=\"" << username << "\" from the client using TCP over port " << client_addr.sin_port << "." << endl;
				vector <struct transaction> t = combine_data();
				int balance = check_wallet(t, username);
				send(child_socket, &balance, sizeof(balance), 0);
				cout << "The main server sent the current balance to client " << j << "." << endl;
				break;
			}
			case TXCOINS: {
				char username1[1024] = {0};
				char username2[1024] = {0};
				int amount = 0;
				char j = 0;	// client j
				memset(username1, 0, sizeof(username1));
				memset(username2, 0, sizeof(username2));
				recv(child_socket, &j, 1024, 0);
				recv(child_socket, username1, 1024, 0);
				cout << "The main server received input=\"" << username1 << "\" from the client using TCP over port " << client_addr.sin_port << "." << endl;
				recv(child_socket, username2, 1024, 0);
				recv(child_socket, &amount, 1024, 0);
				cout << "The main server received from \"" << username1 << "\" to transfer " << amount << " coins to \"" << username2 << "\" using TCP over port " << client_addr.sin_port << "." << endl;
				
				vector <struct transaction> t = combine_data();
				int balance1 = check_wallet(t, username1);
				int balance2 = check_wallet(t, username2);
				
				int indicator = 0;
				if (balance1 < 0) {
					indicator += 1;
				}
				if (balance2 < 0) {
					indicator += 2;
				}
				send(child_socket, &indicator, sizeof(indicator), 0);

				if (indicator == 0) {
					send(child_socket, &balance1, sizeof(balance1), 0);
					balance1 -= amount;
					// cout << balance << " " << amount << endl;
					if (balance1 >= 0) {
						int r = rand() % 3;
						// int r = 0;
						char i = 'A' + r;
						
						switch (r) {
						   case 0:
						      store(t, username1, username2, amount, serverA_addr);
						      break;
						   case 1:
						      store(t, username1, username2, amount, serverB_addr);
						      break;
						   case 2:
						      store(t, username1, username2, amount, serverC_addr);
						      break;
						}

						cout << "The main server sent a request to server " << i << "." << endl;
						cout << "The main server received the feedback from server " << i << " using UDP over port " << 21431 + r * 1000 << "." << endl;				
						// cout << balance1 << endl;
						send(child_socket, &balance1, sizeof(balance1), 0);
					}
					cout << "The main server sent the current balance to client " << j << "." << endl;
					cout << "The main server sent the result of the transaction to client " << j << "." << endl;
				}
				break;
			}
			case TXLIST: {
				vector <struct transaction> t = combine_data();
				qsort(&t[0], t.size(), sizeof(struct transaction), cmp);
				ofstream fout("alichain.txt");

				for (int i = 0; i < t.size(); i++) {
					fout << t[i].id << " " << t[i].username1 << " " << t[i].username2 << " " << t[i].amount << endl;
				}

				fout.close();

				break;
			}
			case stats: {
				char username[1024];
				memset(username, 0, sizeof(username));

				vector <struct transaction> t = combine_data();
				recv(child_socket, username, 1024, 0);

				vector<pair<string, struct st> > v = check_stats(t, username);

				int num = v.size();
				send(child_socket, &num, sizeof(num), 0);
				char number[1024] = {0};
				char amount[1024] = {0};
				for (int i = 0; i < v.size(); i++) {
					char *username2 = (char *)v[i].first.data();
					sprintf(number, "%d", v[i].second.number);
					sprintf(amount, "%d", v[i].second.amount);
					// cout << username2 << " " << number << " " << amount << endl;
					send(child_socket, username2, 1024, 0);
					send(child_socket, number, 1024, 0);
					send(child_socket, amount, 1024, 0);
				}
				break;
			}
		}
	close(child_socket);
    }
}

int init_udp() {
    int sock;
    struct sockaddr_in server_addr;
	char buffer[1024] = {0};
    char message[1024];
	char LOCALHOST[] = "127.0.0.1";

	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == 0) {	// create udp socket
		exit(EXIT_FAILURE);
	}
	
	server_addr.sin_family = AF_INET;   // host byte order
    server_addr.sin_port = htons(24431);   // short, network byte order
    server_addr.sin_addr.s_addr = inet_addr(LOCALHOST);
    memset(&(server_addr.sin_zero), '\0', 8);   // zero the rest of the struct

	if (bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
		perror("Fail to bind");
		exit(EXIT_FAILURE);
	}

	return sock;
}

int main(int argc, char *argv[]) {
	cout << "The main server is up and running." << endl;

	pthread_t thread_tcp_A, thread_tcp_B;
	int PORTA = 25431, PORTB = 26431;
	sock_udp = init_udp();

	char LOCALHOST[] = "127.0.0.1";
	char message[] = "message from serverM.";

	// serverA configuration
	serverA_addr.sin_family = AF_INET;   // host byte order
    serverA_addr.sin_port = htons(21431);   // short, network byte order
    serverA_addr.sin_addr.s_addr = inet_addr(LOCALHOST);
    memset(&(serverA_addr.sin_zero), '\0', 8);   // zero the rest of the struct
	
	// serverB configuration
	serverB_addr.sin_family = AF_INET;
	serverB_addr.sin_port = htons(22431);
	serverB_addr.sin_addr.s_addr = inet_addr(LOCALHOST);
	memset(&(serverB_addr.sin_zero), '\0', 8);

	// serverC configuration
    serverC_addr.sin_family = AF_INET;
    serverC_addr.sin_port = htons(23431);
    serverC_addr.sin_addr.s_addr = inet_addr(LOCALHOST);
    memset(&(serverC_addr.sin_zero), '\0', 8);

	// sendto(sock_udp, message, strlen(message), MSG_CONFIRM, (struct sockaddr *)&serverA_addr, sizeof(serverA_addr));

	// cout << "sent to serverA." << endl;

    int ret_tcp_A = pthread_create(&thread_tcp_A, NULL, thread_tcp, &PORTA);
    int ret_tcp_B = pthread_create(&thread_tcp_B, NULL, thread_tcp, &PORTB);
    
	pthread_join(thread_tcp_A, NULL);
    pthread_join(thread_tcp_B, NULL);
 	if (ret_tcp_A || ret_tcp_B) {
	   exit(EXIT_FAILURE);
	}
	
	return 0;
}
