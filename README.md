<h1>Multi Threaded Proxy Server with Cache</h1>

This project is implemented using `C++` and Parsing of HTTP referred from <a href = "https://github.com/vaibhavnaagar/proxy-server"> Proxy Server </a>


## Index

- [Project Theory]
- [How to Run]
- [Demo]

## Project Theory

##### Introduction

##### Basic Working Flow of the Proxy Server:
![](https://github.com/kunalsingh1301/proxy_server_for_windows/blob/main/resources/uml2.png)

##### How did we implement Multi-threading?
- Used Semaphore instead of Condition Variables and `WaitForSingleObject()` and `ExitThread()` function. 
- `WaitForSingleObject()` requires us to pass the thread id of the the thread to wait for. 
- Semaphore’s `WaitForSingleObject()` and `ReleaseSemaphore()` doesn’t need any parameter. So it is a better option. 


##### Motivation/Need of Project
- To Understand → 
  - The working of requests from our local computer to the server.
  - The handling of multiple client requests from various clients.
  - Locking procedure for concurrency.
  - The concept of cache and its different functions that might be used by browsers.
- Proxy Server do → 
  - It speeds up the process and reduces the traffic on the server side.
  - It can be used to restrict user from accessing specific websites.
  - A good proxy will change the IP such that the server wouldn’t know about the client who sent the request.
  - Changes can be made in Proxy to encrypt the requests, to stop anyone accessing the request illegally from your client.
 
##### OS Component Used ​
- Threading
- Locks 
- Semaphore
- Cache (LRU algorithm is used in it)

##### Limitations ​
- If a URL opens multiple clients itself, then our cache will store each client’s response as a separate element in the linked list. So, during retrieval from the cache, only a chunk of response will be send and the website will not open
- Fixed size of cache element, so big websites may not be stored in cache. 

##### How this project can be extended?
- We can implement requests like POST with this code.


## How to Run

```bash
$ git clone https://github.com/Lovepreet-Singh-LPSK/MultiThreadedProxyServerClient.git

# Note:
- This code can only be run in Windows Machine. Please disable your browser cache

## Demo
