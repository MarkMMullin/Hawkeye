#ifndef Hawkeye_h
#define Hawkeye_h
#include <iostream>
#include <unordered_map>
#include <cstdio>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <unistd.h>

#include "server.hpp"
#include "request.hpp"
#include "response.hpp"
#include "constants.hpp"
#include "file_resources.hpp"

#include "configuru.hpp"
#include "Arducam.h"
#include "CameraConfig.h"
#include "CameraManager.h"
typedef std::vector<std::string> strvector;

class Hawkeye
{
public:
  enum ESubsystem {
    CAMERA,
    CAMERABANK,
    MCP,
    NO_SUBSYSTEM};

  enum ECameraCommand {
    IMAGE,     // GET
    NEXTIMAGE, //GET
    RESOLUTION, // GET/PUT
    PERFORMANCE, // GET
    ON,   // GET/SET
    SEE,   // GET
    PROGRAM,
    REBOOT,
    QUANTIZATION,
    IMAGECOUNTER,     //GET
    NO_CAMERACOMMAND
  };
  enum EMCPCommand {
    TIME,            //GET,PUT
    IMAGESET,         // GET
    IMAGESETITEM,            // GET
    PURGE,           // DELETE
    STAMP,           // PUT
    STAMPS,          // GET
    USED,            // GET
    FREE,            // GET
    NO_MCPCOMMAND
  };

  Hawkeye();
  virtual ~Hawkeye();
  strvector getTokens(std::string url);
  ESubsystem getSubsystem(const strvector& tokens);
  bool getCameraCommand(const strvector& tokens,int& cameraNumber,ECameraCommand& command);

  void startCameraDriver();
  void stopCameraDriver();
  bool dispatchCommand(const Request_data& theRequest,Html_file& responseDocument);
private:
  void generateAutoRefreshImageFile(Html_file& theFile,const std::string& imageUrl,int msecInterval);
  bool dispatchCameraCommand(const Request_data& theRequest,int cameraNumber,ECameraCommand command,Html_file& responseDocument);
  strvector m_commandTokens;
  pthread_t m_cameraThread;
  bool m_cameraRunning;
};
#endif
