#include <iostream>
#include <unordered_map>
#include <cstdio>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <unistd.h>

#include <stdio.h>
#include <execinfo.h>
#include <signal.h>
#include <stdlib.h>

#include "server.hpp"
#include "request.hpp"
#include "response.hpp"
#include "constants.hpp"
#include "file_resources.hpp"

#include "configuru.hpp"
#include "Arducam.h"
#include "CameraConfig.h"
#include "CameraManager.h"

#include "Hawkeye.h"

void interrupt_handler(int);


bool serverRunning = false;

long numConnections = 0;
int main(int argc, char *argv[])
{
  // get the configuration file
  configuru::Config cfg = configuru::parse_file("/home/mark/Hawkeye/Hawkeye.config", configuru::JSON);
  CameraConfig::Load(cfg);

  Hawkeye hawkeye;
  hawkeye.startCameraDriver();
  struct sigaction sigIntHandler;
  sigIntHandler.sa_handler = interrupt_handler;
  sigemptyset(&sigIntHandler.sa_mask);
  sigIntHandler.sa_flags = 0;
  sigaction(SIGINT, &sigIntHandler, NULL);

  initializeFiles();
  std::unordered_map<std::string, Html_file*> file_umap = populate_file_umap();

  int serverPort = (int) cfg["server-port"];
  Server::create("127.0.0.10", serverPort);
  serverRunning = true;
  std::cout << "Listening...\n";
  Server::listen();
  try {
    while (1) {
      // calculate wait time here
      int clientid = Server::accept();
      ++numConnections;

      std::string request = Server::receive(clientid);
      fprintf(stderr,"webservice,receive\n");
      Html_file responseDocument;
      std::vector<uint8_t> response;
      if (!request.empty()) {
	fprintf(stderr,"webservice,decode\n");
        Request_data req_data = Request::handle_request(request);
	fprintf(stderr,"webservice,dispatch\n");
        bool success = hawkeye.dispatchCommand(req_data,responseDocument);
        if(!success)
          response = Response::generate_response(*file_umap["/404.html"], Response::type::NOT_FOUND);
	else if(responseDocument.content.size() == 0)
          response = Response::generate_response(*file_umap["/OK.html"], Response::type::OK);
	else
	  response = Response::generate_response(responseDocument,Response::type::OK);

        Server::send(clientid, response);
	fprintf(stderr,"webservice,respond %s\n",success ? "SUCCESS" : "FAILURE");
      }

    }
  }
  catch(...)
  {
    void *array[32];
    size_t size;
    fprintf(stderr,"crash,Something bad happened!\n");
    // get void*'s for all entries on the stack
    size = backtrace(array, 32);

    // print out all the frames to stderr
    backtrace_symbols_fd(array, size, STDERR_FILENO);
  }
  if(serverRunning) {
    Server::remove();
  }
  return EXIT_SUCCESS;
}


void interrupt_handler(int s)
{
  std::cout << "Caught signal " << s << "\n";
  Server::remove();
  serverRunning = false;
  std::cout << "Httpserver has been shutdown successfully.\n";
  exit(EXIT_SUCCESS);
}
