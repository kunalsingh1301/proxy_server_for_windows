#include "proxy_parse.hpp"
#include "proxy_parse.cpp"
#include<bits/stdc++.h>
#include <iostream>
#include <string>
#include <ctime>
#include <thread>
#include <tchar.h>
#include <vector>
#include <winsock2.h>
#include <winsock.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <cstring>

using namespace std;

#pragma comment(lib, "Ws2_32.lib")
#define MAX_CLIENTS 10
#define MAX_BYTES 4096
#define MAX_ELEMENT_SIZE 10*(1<<10)
#define MAX_SIZEe 200*(1<<20)

typedef struct cache_element cache_element;

class cache_element { // linked list for LRU cache
public:
    vector<char> data;
    int len;
    string url;
    time_t lru_time_track;
    cache_element* next;

    cache_element(const string& url, const vector<char>& data)
        : url(url), data(data.begin(), data.end()), lru_time_track(time(NULL)), next(nullptr) {}
};

cache_element* find(const string& url);
int add_cache_element(std::vector<char>& data, int size, const std::string& url);
void remove_cache_element();

// Global variables
int port_number = 8080;
int proxy_socketId;
HANDLE tid[MAX_CLIENTS];
HANDLE semaphore;
HANDLE locki;

cache_element* head;
int cache_size;



int checkHTTPversion(char* msg) {
    int version = -1;

    if (strncmp(msg, "HTTP/1.1", 8) == 0) {
        version = 1;
    } else if (strncmp(msg, "HTTP/1.0", 8) == 0) {
        version = 1; // Handling this similar to version 1.1
    } else {
        version = -1;
    }

    return version;
}

int connectRemoteServer(char* host_addr,int port_num){
	int remoteSocket = socket(AF_INET,SOCK_STREAM,0);

	if(remoteSocket <0){
		perror("error in creating of socket")	;
		return -1;
	}

	struct hostent* host = gethostbyname(host_addr);
	if(host == NULL){
		perror("no such hst exist");
		return -1;
	}

	struct sockaddr_in server_addr;
    memset(&server_addr,0,sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_num);


    memcpy((char*)&server_addr.sin_addr, (char*)&host->h_addr_list[0], host->h_length); // bcopy replacement

    if (connect(remoteSocket, (struct sockaddr*)&server_addr, (size_t)sizeof(server_addr)) < 0) {
        return -1;
    }

    return remoteSocket;
}

int sendErrorMessage(SOCKET socket, int status_code)
{
    std::ostringstream oss;
    char currentTime[50];
    time_t now = time(0);

    struct tm data = *gmtime(&now);
    strftime(currentTime, sizeof(currentTime), "%a, %d %b %Y %H:%M:%S %Z", &data);

    switch (status_code)
    {
        case 400: 
            oss << "HTTP/1.1 400 Bad Request\r\nContent-Length: 95\r\nConnection: keep-alive\r\nContent-Type: text/html\r\nDate: " 
                << currentTime << "\r\nServer: VaibhavN/14785\r\n\r\n<HTML><HEAD><TITLE>400 Bad Request</TITLE></HEAD>\n<BODY><H1>400 Bad Request</H1>\n</BODY></HTML>";
            std::cout << "400 Bad Request\n";
            send(socket, oss.str().c_str(), oss.str().length(), 0);
            break;

        case 403: 
            oss.str(""); // Clear the stringstream
            oss << "HTTP/1.1 403 Forbidden\r\nContent-Length: 112\r\nContent-Type: text/html\r\nConnection: keep-alive\r\nDate: " 
                << currentTime << "\r\nServer: VaibhavN/14785\r\n\r\n<HTML><HEAD><TITLE>403 Forbidden</TITLE></HEAD>\n<BODY><H1>403 Forbidden</H1><br>Permission Denied\n</BODY></HTML>";
            std::cout << "403 Forbidden\n";
            send(socket, oss.str().c_str(), oss.str().length(), 0);
            break;

        case 404: 
            oss.str(""); // Clear the stringstream
            oss << "HTTP/1.1 404 Not Found\r\nContent-Length: 91\r\nContent-Type: text/html\r\nConnection: keep-alive\r\nDate: " 
                << currentTime << "\r\nServer: VaibhavN/14785\r\n\r\n<HTML><HEAD><TITLE>404 Not Found</TITLE></HEAD>\n<BODY><H1>404 Not Found</H1>\n</BODY></HTML>";
            std::cout << "404 Not Found\n";
            send(socket, oss.str().c_str(), oss.str().length(), 0);
            break;

        case 500: 
            oss.str(""); // Clear the stringstream
            oss << "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 115\r\nConnection: keep-alive\r\nContent-Type: text/html\r\nDate: " 
                << currentTime << "\r\nServer: VaibhavN/14785\r\n\r\n<HTML><HEAD><TITLE>500 Internal Server Error</TITLE></HEAD>\n<BODY><H1>500 Internal Server Error</H1>\n</BODY></HTML>";
            send(socket, oss.str().c_str(), oss.str().length(), 0);
            break;

        case 501: 
            oss.str(""); // Clear the stringstream
            oss << "HTTP/1.1 501 Not Implemented\r\nContent-Length: 103\r\nConnection: keep-alive\r\nContent-Type: text/html\r\nDate: " 
                << currentTime << "\r\nServer: VaibhavN/14785\r\n\r\n<HTML><HEAD><TITLE>501 Not Implemented</TITLE></HEAD>\n<BODY><H1>501 Not Implemented</H1>\n</BODY></HTML>";
            std::cout << "501 Not Implemented\n";
            send(socket, oss.str().c_str(), oss.str().length(), 0);
            break;

        case 505: 
            oss.str(""); // Clear the stringstream
            oss << "HTTP/1.1 505 HTTP Version Not Supported\r\nContent-Length: 125\r\nConnection: keep-alive\r\nContent-Type: text/html\r\nDate: " 
                << currentTime << "\r\nServer: VaibhavN/14785\r\n\r\n<HTML><HEAD><TITLE>505 HTTP Version Not Supported</TITLE></HEAD>\n<BODY><H1>505 HTTP Version Not Supported</H1>\n</BODY></HTML>";
            std::cout << "505 HTTP Version Not Supported\n";
            send(socket, oss.str().c_str(), oss.str().length(), 0);
            break;

        default:  
            return -1;
    }
    return 1;
}

int handle_request(int clientsocketID, ParsedRequest* request, const std::string& tempReq) {
    std::vector<char> buf(MAX_BYTES);
    strcpy(buf.data(), "GET ");
    strcat(buf.data(), request->path);
    strcat(buf.data(), " ");
    strcat(buf.data(), request->version);
    strcat(buf.data(), "\r\n");

    size_t len = strlen(buf.data());

    if (ParsedHeader_set(request, "Connection", "close") < 0) {
        std::cout << "set header key is not working" << "\n";
    }

    if (ParsedHeader_get(request, "Host") == NULL) {
        if (ParsedHeader_set(request, "Host", request->host) < 0) {
            std::cout << "set Host header key is not working" << "\n";
        }
    }

    if (ParsedRequest_unparse_headers(request, buf.data() + len, MAX_BYTES - len) < 0) {
        std::cout << "unable to parse" << "\n";
    }

    int server_port = 80; // HTTP almost always happens at port 80
    if (request->port != NULL) {
        server_port = atoi(request->port);
    }

    int remoteSocketId = connectRemoteServer(request->host, server_port);
    if (remoteSocketId < 0) {
        return -1;
    }

    int bytes_send = send(remoteSocketId, buf.data(), strlen(buf.data()), 0);
    if (bytes_send < 0) {
        perror("error in sending data to remote server");
        return -1;
    }

    buf.assign(MAX_BYTES, 0);
    std::vector<char> temp_buffer(MAX_BYTES);
    int temp_buffer_size = MAX_BYTES;
    int temp_buffer_index = 0;

    bytes_send = recv(remoteSocketId, buf.data(), MAX_BYTES - 1, 0);
    while (bytes_send > 0) {
        send(clientsocketID, buf.data(), bytes_send, 0);

        if (temp_buffer_index + bytes_send > temp_buffer_size) {
            temp_buffer_size += MAX_BYTES;
            temp_buffer.resize(temp_buffer_size);
        }

        copy(buf.begin(), buf.begin() + bytes_send, temp_buffer.begin() + temp_buffer_index);
        temp_buffer_index += bytes_send;

        memset(buf.data(), 0, MAX_BYTES);
        bytes_send = recv(remoteSocketId, buf.data(), MAX_BYTES - 1, 0);
    }

    temp_buffer.resize(temp_buffer_index);
    add_cache_element(temp_buffer, temp_buffer.size(), tempReq);

    closesocket(remoteSocketId);
    return 0;
}


DWORD WINAPI thread_fn(LPVOID lpParam) {
    int* socketNew = (int*)lpParam;
    WaitForSingleObject(semaphore, INFINITE);
    long p;
    ReleaseSemaphore(semaphore, 1, &p);

    int socket = *socketNew;
    int bytes_send_client, len;
    vector<char> buffer(MAX_BYTES, 0);
    bytes_send_client = recv(socket, buffer.data(), MAX_BYTES, 0);

    while (bytes_send_client > 0) {
        len = strlen(buffer.data());
        if (strstr(buffer.data(), "\r\n\r\n") == NULL) { // \r\n\r\n this is end of any HTTP request
            bytes_send_client = recv(socket, buffer.data() + len, MAX_BYTES - len, 0); // kaha tak mila h waha se lena start karenge
        } else {
            break;
        }
    }

    string tempReq(buffer.begin(), buffer.end()); // not necessary used for searching in cache copy of cache

    cache_element* temp = find(tempReq);
    if (temp) {
        int size = temp->data.size();
        int pos = 0;
        vector<char> response(MAX_BYTES, 0); // we hot response from cache so returned it
        while (pos < size) {
            fill(response.begin(), response.end(), 0);
            for (int i = 0; i < MAX_BYTES && pos < size; ++i, ++pos) {
                response[i] = temp->data[pos];
            }
            send(socket, response.data(), MAX_BYTES, 0);
        }
        cout << "Data retrieved from the cache\n";
        cout << string(response.begin(), response.end()) << "\n";
    } else if (bytes_send_client > 0) {
        len = strlen(buffer.data());
        ParsedRequest* request = ParsedRequest_create();

        if (ParsedRequest_parse(request, buffer.data(), len) < 0) {
            perror("parsing failed");
        } else {
            fill(buffer.begin(), buffer.end(), 0);
            if (!strcmp(request->method, "GET")) {
                if (request->host && request->path && checkHTTPversion(request->version) == 1) {
                    bytes_send_client = handle_request(socket, request, tempReq);
                    if (bytes_send_client == -1) {
                        sendErrorMessage(socket, 500);
                    } else {
                        sendErrorMessage(socket, 500);
                    }
                }
            } else {
                cout << "This code supports only GET";
            }
        }
        ParsedRequest_destroy(request); // free heap all memory
    } else if (bytes_send_client == 0) {
        cout << "client is disconnected" << endl;
    }

    shutdown(socket, SD_BOTH);
    closesocket(socket);
    ReleaseSemaphore(semaphore, 1, NULL); // sem_post equivalent
    return 0;
}

int main(int argc, char* argv[]) {
    int client_socketID, client_len;
    struct sockaddr_in server_addr, client_addr;

    // Create semaphore
    semaphore = CreateSemaphore(NULL, 0, MAX_CLIENTS, NULL);
    locki = CreateMutex(NULL, false, NULL);

    if (argc == 2) port_number = atoi(argv[1]);
    else {
        cout << "too few arguments" << endl;
        exit(1);
    }

    cout << "starting proxy server at " << port_number << endl;
    proxy_socketId = socket(AF_INET, SOCK_STREAM, 0); // handshake for TCP protocol if available then talk
    if (proxy_socketId < 0) { // if socket not formed
        perror("failed to create socket\n");
        exit(1);
    }

    int reuse = 1;
    if (setsockopt(proxy_socketId, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse) < 0)) {
        perror("setSockOpt failed\n");
    } // function to reuse single socket for all request and SO_REUSEADDR is like if google.com hits twice and use same

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET; // AF_INET for IPv4
    server_addr.sin_port = htons(port_number); // convert port to number which internet understand:htons, access using man
    server_addr.sin_addr.s_addr = INADDR_ANY; // give any address to socket which you are going to communicate
    if (bind(proxy_socketId, (struct sockaddr*)&server_addr, sizeof(server_addr) < 0)) {
        perror("Port is not available\n");
        exit(1);
    } // binding

    cout << "Binding to port " << port_number << "\n";
    int listen_status = listen(proxy_socketId, MAX_CLIENTS);
    if (listen_status < 0) {
        perror("Error in listening\n");
        exit(true);
    }

    int i = 0;
    int Connected_socketId[MAX_CLIENTS]; // TO KEEP TRACK of who are connected

    while (1) {
        memset(&client_addr, 0, sizeof(client_addr));
        client_len = sizeof(client_addr);
        client_socketID = accept(proxy_socketId, (struct sockaddr*)&client_addr, &client_len); // accept new connection on a socket
        if (client_socketID < 0) {
            perror("not able to connect on a socket");
            exit(1);
        } else {
            Connected_socketId[i] = client_socketID;
        }

        struct sockaddr_in* client_pt = (struct sockaddr_in*)&client_addr; // making a copy
        struct in_addr ip_addr = client_pt->sin_addr; // extracting IP address of client
        char str[INET_ADDRSTRLEN];
        strcpy(str, inet_ntoa(client_addr.sin_addr)); // convert network address to formatable address
        cout << "client is connected with port no " << ntohs(client_addr.sin_port) << " and IP address " << str << "\n";

        tid[i] = CreateThread(NULL, 0, thread_fn, (void*)&Connected_socketId[i], 0, NULL);
        i++;
    }
    closesocket(proxy_socketId);
    return 0;
}

cache_element* find(string &url){
    cache_element *site = NULL;
    int temp_lock_val = WaitForSingleObject(locki,INFINITE);//pthread_mutex_lock
    cout<< "remove cache lock acquired"<<temp_lock_val<<"\n";
    if(head){
        site = head;
        while(site){
            if(!strcmp(site->url.data(),url.data())){
                cout<<"lru time track before"<<site->lru_time_track<<"\n";
                cout<<"\n url not found \n";
                site->lru_time_track = time(NULL);
                cout<<"lru time track after"<<site->lru_time_track<<"\n";
                break;
            }
            site = site->next;
            
        }
    }else{
        cout<<"url not found\n";
    }
    temp_lock_val = ReleaseMutex(locki);
    cout<<"unlocked\n";
    return site;
}

void remove_cache_element(){
    // If cache is not empty searches for the node which has the least lru_time_track and deletes it
    cache_element * p ;  	// Cache_element Pointer (Prev. Pointer)
	cache_element * q ;		// Cache_element Pointer (Next Pointer)
	cache_element * temp;	// Cache element to remove
    //sem_wait(&cache_lock);
    int temp_lock_val = WaitForSingleObject(locki,INFINITE);
	printf("Remove Cache Lock Acquired %d\n",temp_lock_val); 
	if( head != NULL) { // Cache != empty
		for (q = head, p = head, temp =head ; q -> next != NULL; 
			q = q -> next) { // Iterate through entire cache and search for oldest time track
			if(( (q -> next) -> lru_time_track) < (temp -> lru_time_track)) {
				temp = q -> next;
				p = q;
			}
		}
		if(temp == head) { 
			head = head -> next; /*Handle the base case*/
		} else {
			p->next = temp->next;	
		}
		cache_size = cache_size - (temp -> len) - sizeof(cache_element) - 
		(temp -> url).size() - 1;     //updating the cache size
	} 
	//sem_post(&cache_lock);
    temp_lock_val = ReleaseMutex(locki);
	cout<<"Remove Cache Lock Unlocked"<<temp_lock_val<<"\n"; 
}

int add_cache_element(vector<char>& data, int size, const string& url){
    int temp_lock_val = WaitForSingleObject(locki,INFINITE);//pthread_mutex_lock
    cout<< "remove cache lock acquired"<<temp_lock_val<<"\n";
    int element_size = size + 1 + url.length() + sizeof(cache_element);
    if(element_size < MAX_ELEMENT_SIZE){
        temp_lock_val = ReleaseMutex(locki);
        cout<<"add cache is unlocked";
        return 0;
    }else{
        while(cache_size + element_size > MAX_SIZEe){
            remove_cache_element();
        }
        cache_element *element = new cache_element(url,data);
        element->lru_time_track = time(NULL);
        element->next = head; //set to null
        element->len = size;
        head = element;
        cache_size += element_size;
        temp_lock_val = ReleaseMutex(locki);
        cout<<"add cache lock unlocked"<< temp_lock_val<<"\n";
        return 1;
    }
    return 0;
}