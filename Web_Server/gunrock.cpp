#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <signal.h>

#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <sstream>
#include <queue>

#include "HTTPRequest.h"
#include "HTTPResponse.h"
#include "HttpService.h"
#include "HttpUtils.h"
#include "FileService.h"
#include "MySocket.h"
#include "MyServerSocket.h"
#include "dthread.h"

using namespace std;

int PORT = 8080;
int THREAD_POOL_SIZE = 1;
unsigned int BUFFER_SIZE = 1;
string BASEDIR = "static";
string SCHEDALG = "FIFO";
string LOGFILE = "out.txt";

vector<HttpService *> services;

// declare dynamic buffer for requests
// use FIFO scheduling - queue
queue<MySocket*> buffer;

// declare dynamic pool for threads
pthread_t *thread_pool;

// initialize the mutex
pthread_mutex_t mutex_buffer = PTHREAD_MUTEX_INITIALIZER;

// initialize conditional variables
pthread_cond_t empty_buffer = PTHREAD_COND_INITIALIZER;
pthread_cond_t fill_buffer = PTHREAD_COND_INITIALIZER;

HttpService *find_service(HTTPRequest *request) {
   // find a service that is registered for this path prefix
  for (unsigned int idx = 0; idx < services.size(); idx++) {
    if (request->getPath().find(services[idx]->pathPrefix()) == 0) {
      return services[idx];
    }
  }

  return NULL;
}

void invoke_service_method(HttpService *service, HTTPRequest *request, HTTPResponse *response) {
  stringstream payload;
  
  // invoke the service if we found one
  if (service == NULL) {
    // not found status
    response->setStatus(404);
  } else if (request->isHead()) {
    payload << "HEAD " << request->getPath();
    sync_print("invoke_service_method", payload.str());
    cout << payload.str() << endl;
    service->head(request, response);
  } else if (request->isGet()) {
    payload << "GET " << request->getPath();
    sync_print("invoke_service_method", payload.str());
    cout << payload.str() << endl;
    service->get(request, response);
  } else {
    // not implemented status
    response->setStatus(405);
  }
}

void handle_request(MySocket *client) {
  HTTPRequest *request = new HTTPRequest(client, PORT);
  HTTPResponse *response = new HTTPResponse();
  stringstream payload;
  
  // read in the request
  bool readResult = false;
  try {
    payload << "client: " << (void *) client;
    sync_print("read_request_enter", payload.str());
    readResult = request->readRequest();
    sync_print("read_request_return", payload.str());
  } catch (...) {
    // swallow it
  }    
    
  if (!readResult) {
    // there was a problem reading in the request, bail
    delete response;
    delete request;
    sync_print("read_request_error", payload.str());
    return;
  }
  
  HttpService *service = find_service(request);
  invoke_service_method(service, request, response);

  // send data back to the client and clean up
  payload.str(""); payload.clear();
  payload << " RESPONSE " << response->getStatus() << " client: " << (void *) client;
  sync_print("write_response", payload.str());
  cout << payload.str() << endl;
  client->write(response->response());
    
  delete response;
  delete request;

  payload.str(""); payload.clear();
  payload << " client: " << (void *) client;
  sync_print("close_connection", payload.str());
  client->close();
  delete client;
}

// safe lock
void Dthread_mutex_lock(pthread_mutex_t *mutex) {
  if (dthread_mutex_lock(mutex) != 0) {
      perror("Failed to lock");
    }
}

// safe unlock
void Dthread_mutex_unlock(pthread_mutex_t *mutex) {
  if (dthread_mutex_unlock(mutex) != 0) {
      perror("Failed to unlock");
    }
}

//safe wait
void Dthread_cond_wait(pthread_cond_t *condition, pthread_mutex_t *mutex) {
  if (dthread_cond_wait(condition, mutex) != 0) {
      perror("Failed to wait");
    }
}

// safe signal
void Dthread_cond_signal(pthread_cond_t *condition) {
  if (dthread_cond_signal(condition) != 0) {
      perror("Failed to signal");
    }
}

//safe broadcast
void Dthread_cond_broadcast(pthread_cond_t *condition) {
  if (dthread_cond_broadcast(condition) != 0) {
      perror("Failed to broadcast");
    }
}

//concurrent consumer
void *worker(void *args) {
  MySocket *client;
  while (true) {
    Dthread_mutex_lock(&mutex_buffer);
    while ( buffer.empty() ) {
      Dthread_cond_wait(&fill_buffer, &mutex_buffer);
    }
    client = buffer.front();
    buffer.pop();
    Dthread_cond_signal(&empty_buffer);
    Dthread_mutex_unlock(&mutex_buffer);
    handle_request(client);
  }
}

//concurrent producer
void producer(){
  MyServerSocket *server = new MyServerSocket(PORT);
  MySocket *client;
  while(true) {
    sync_print("waiting_to_accept", "");
    client = server->accept();
    sync_print("client_accepted", "");

    Dthread_mutex_lock(&mutex_buffer);
    while ( buffer.size() == BUFFER_SIZE ) {
      Dthread_cond_wait(&empty_buffer, &mutex_buffer);
    }
    buffer.push(client);
    Dthread_cond_signal(&fill_buffer);
    Dthread_mutex_unlock(&mutex_buffer);
  }
}

int main(int argc, char *argv[]) {

  signal(SIGPIPE, SIG_IGN);
  int option;

  while ((option = getopt(argc, argv, "d:p:t:b:s:l:")) != -1) {
    switch (option) {
    case 'd':
      BASEDIR = string(optarg);
      break;
    case 'p':
      PORT = atoi(optarg);
      break;
    case 't':
      THREAD_POOL_SIZE = atoi(optarg);
      break;
    case 'b':
      BUFFER_SIZE = atoi(optarg);
      break;
    case 's':
      SCHEDALG = string(optarg);
      break;
    case 'l':
      LOGFILE = string(optarg);
      break;
    default:
      cerr<< "usage: " << argv[0] << " [-p port] [-t threads] [-b buffers]" << endl;
      exit(1);
    }
  }

  set_log_file(LOGFILE);

  sync_print("init", "");

  services.push_back(new FileService(BASEDIR));

  // initilize the thread_pool
  thread_pool = new pthread_t [THREAD_POOL_SIZE];

  // create threads and detach them
  int i;
  for ( i = 0; i < THREAD_POOL_SIZE; i++ ) {
    if (dthread_create(&thread_pool[i], NULL, worker, NULL ) != 0){
      perror("Failed to create a thread");
    }
    if (dthread_detach(thread_pool[i]) != 0) {
      perror("Failed to detach a thread");
    }
  }
  
  // call producer
  producer();

  // delete dynamically allocated thread_pool array
  delete[] thread_pool;
}
