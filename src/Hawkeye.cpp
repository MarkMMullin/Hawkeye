#include "Hawkeye.h"
#include <chrono>
#include <thread>
#include "configuru.hpp"

static uint8_t* emitBuffer;
Hawkeye::Hawkeye()
{
  m_cameraRunning = false;
  emitBuffer = new uint8_t[2048 + Arducam::getImageBufferSize()];
}
Hawkeye::~Hawkeye()
{
  if(m_cameraRunning)
    stopCameraDriver();
}
void Hawkeye::generateAutoRefreshImageFile(Html_file& theFile,const std::string& imageUrl,int msecInterval)
{

  std::string body =
    "<html>"
    "<head>"
    " <script language=\"JavaScript\">"
    " function refreshIt() {"
    " e = document.getElementById(\"myCam\");"
    " e.src = e.src.split('?')[0] + '?' + new Date().getTime();"
    "}"
    " setInterval(refreshIt,300) "
    " </script>"
    " </head>"
    " <body>"
    "     <img src=\""+ imageUrl +"\" id=\"myCam\" >"
    " </body>"
    "</html>";

  initializeTextFile(&theFile,
                     "autorefresh",
                     "img/99/see",
                     body,
		     "text/html; charset=UTF-8");
  
}
strvector Hawkeye::getTokens(std::string url)
{
  strvector results;
  while(true)
    {
      std::size_t found = url.find("/");
      if(found == std::string::npos)
	{
	  if(url.size() > 0)
	    results.push_back(url);
           
	  return results;
	}
      std::string item = url.substr(0,found);
      if(item.size() > 0)
	results.push_back(item);
      url = url.substr(found + 1,url.size());
    }
        
}

Hawkeye::ESubsystem Hawkeye::getSubsystem(const strvector& tokens)
{
  if(tokens.size() == 0)
    return ESubsystem::NO_SUBSYSTEM;
  if(!tokens.at(0).compare("cam"))
    return ESubsystem::CAMERA;
  if(!tokens.at(0).compare("bnk"))
    return ESubsystem::CAMERABANK;
  if(!tokens.at(0).compare("mcp"))
    return ESubsystem::MCP;
  return ESubsystem::NO_SUBSYSTEM;
}
bool Hawkeye::getCameraCommand(const strvector& tokens,int& cameraNumber,ECameraCommand& command)
{
  
  if(tokens.size() < 3)
    {
      command = ECameraCommand::NO_CAMERACOMMAND;
      return false;
    }
  
  cameraNumber = ::atoi(tokens.at(1).c_str());
  std::string test = tokens.at(2);
  if(!tokens.at(2).compare("img"))
    command = ECameraCommand::IMAGE;
  else  if(!tokens.at(2).compare("rez"))
    command = ECameraCommand::RESOLUTION;
  else  if(!tokens.at(2).compare("prf"))
    command = ECameraCommand::PERFORMANCE;
  else  if(!tokens.at(2).compare("on"))
    command = ECameraCommand::ON;
  else  if(!tokens.at(2).compare("see"))
    command = ECameraCommand::SEE;
  else if(!tokens.at(2).compare("pgm"))
    command = ECameraCommand::PROGRAM;
  else if(!tokens.at(2).compare("reboot"))
    command = ECameraCommand::REBOOT;
  else if(!tokens.at(2).compare("qtz"))
    command = ECameraCommand::QUANTIZATION;
  else if(!tokens.at(2).compare("ctr"))
    command = ECameraCommand::IMAGECOUNTER;
  else
    command = ECameraCommand::NO_CAMERACOMMAND;
  return true;
}



void* cameraDriver(void*)
{
  // start the CameraManager
  CameraManager* cm = CameraManager::GetSingleton();
  cm->StartCapture();
  return NULL;
}

void Hawkeye::startCameraDriver()
{
  // spin the camera hardware up to speed first
  CameraManager::GetSingleton();
  pthread_create(&m_cameraThread,NULL,cameraDriver,NULL);
  m_cameraRunning = true;
}

void Hawkeye::stopCameraDriver()
{
  pthread_cancel(m_cameraThread);
  m_cameraRunning = false;
}

bool Hawkeye::dispatchCommand(const Request_data& theRequest,Html_file& responseDocument)
{
  // load the URL into the command buffer
  m_commandTokens = getTokens(theRequest.url);

  Hawkeye::ESubsystem ss = Hawkeye::getSubsystem(m_commandTokens);
  if(ss == ESubsystem::NO_SUBSYSTEM)
    return false;
  int cameraNumber;
  ECameraCommand camCmd;

  bool state;
  switch(ss)
    {
    case Hawkeye::ESubsystem::CAMERA:
      state = getCameraCommand(m_commandTokens,cameraNumber,camCmd);
      if(state)
	return dispatchCameraCommand(theRequest,cameraNumber,camCmd,responseDocument);
      break;
    case Hawkeye::ESubsystem::CAMERABANK:
    case Hawkeye::ESubsystem::MCP:
    case Hawkeye::ESubsystem::NO_SUBSYSTEM:
      break;
    }
  return false;
}
bool Hawkeye::dispatchCameraCommand(const Request_data& theRequest,int cameraNumber,
				    ECameraCommand command,Html_file& responseDocument)
{
  char db[32];
  char rsp[64];
  std::string jsonContent;
  std::map<std::string,std::string> kvp;
  Arducam* camera = CameraManager::GetSingleton()->getMappedCamera(cameraNumber);
#if LOG_INFO
  fprintf(stderr,"camera indirect from %d (%d)\n",cameraNumber,camera != NULL ? camera->CameraNumber() : -1);
#endif
  if(camera == NULL)
    return false;
  std::string camfile = camera->getLastSave();

  
  switch(command)
    {
    case Hawkeye::ECameraCommand::IMAGECOUNTER:
       sprintf(db,"cam/%d/ctr",cameraNumber);
       fprintf(stderr,"info,Camera,%d,shot,%d\n",camera->CameraNumber(),camera->getShotCounter());
        sprintf(rsp,"{ \"image-counter\" : %d } ",camera->getShotCounter());
      initializeTextFile(&responseDocument,"CameraShotCounter",db,rsp,"application/json");
      return true;
    case Hawkeye::ECameraCommand::NEXTIMAGE:
      {
	if(theRequest.req_type == Request::type::GET || m_commandTokens.size() == 4)
	{
	  uint32_t objId = (uint32_t)  ::atoi(m_commandTokens.at(3).c_str());
	  if(objId >= camera->getShotCounter())
	    {
	      // wait for the next image to arrive
	      pthread_cond_wait(camera->getImageUpdateConditionVariable(),camera->getLastImageBufferMutex());
	    }
	  }
	else
	  return false;
	}
      // fall through is deliberate - nextimage is basically a potential wait on image
    case Hawkeye::ECameraCommand::IMAGE:
      {
	uint32_t imageSize;
	uint8_t* imageBuffer = camera->getLastImageBuffer(emitBuffer,imageSize);
	if(imageSize > 0) {
	  initializeRawMemory(&responseDocument,
			    camfile,
			    "/",
			      imageBuffer,imageSize,
			      camera->getShotCounter());

	  return true;
	}
      }
      return false;
     
    case Hawkeye::ECameraCommand::RESOLUTION:
      if(theRequest.req_type == Request::type::PUT || m_commandTokens.size() == 4)
	{
	  int resid = ::atoi(m_commandTokens.at(3).c_str());
	  Arducam::EResolution rez = (Arducam::EResolution) resid;
	  Arducam* camera = CameraManager::GetSingleton()->getCamera(cameraNumber);
	  if(camera == NULL)
	    return false;
	  camera->changeResolution(rez);
  
	}
      sprintf(db,"cam/%d/rez",cameraNumber);
      sprintf(rsp,"{ \"resolution\" : %d } ",camera->getResolution());
      initializeTextFile(&responseDocument,"CameraResolution",db,rsp,"application/json");
      return true;
    case Hawkeye::ECameraCommand::PERFORMANCE:
      int completed,attempted,timeouts;
      double avgTime,completionRatio;
      CameraManager::GetSingleton()->getPerformanceStats(cameraNumber,completed,attempted,timeouts,avgTime,completionRatio);
      sprintf(db,"%d",completed);
      kvp["completed"] = db;
      sprintf(db,"%d",attempted);
      kvp["attempted"] = db;
      sprintf(db,"%d",timeouts);
      kvp["timeouts"] = db;
      sprintf(db,"%lf",avgTime);
      kvp["avgTime"] = db;
      sprintf(db,"%lf",completionRatio);
      kvp["completionRatio"] = db;
      jsonContent = createJSON(kvp);
      sprintf(db,"cam/%d/prf",cameraNumber);
      initializeTextFile(&responseDocument,"CameraPerformance",db,jsonContent,"application/json");
      return true;

    case Hawkeye::ECameraCommand::ON:
      break;
    case Hawkeye::ECameraCommand::SEE :
      {
	sprintf(db,"/cam/%d/img/dummy?0",cameraNumber);
	int msi = 750;
	if(m_commandTokens.size() == 4)
	  msi = ::atoi(m_commandTokens.at(3).c_str());
	std::string dbx(db);
	generateAutoRefreshImageFile(responseDocument,dbx,msi);
      }
      return true;
    case Hawkeye::ECameraCommand::PROGRAM :
      {
	std::string pxdata;
	if(theRequest.req_type == Request::type::PUT)
	  pxdata = theRequest.data;
	else if(m_commandTokens.size() == 4)
	  pxdata = m_commandTokens.at(3);
	else
	  return false;
	// the last url element is actually a json program
	std::cout << "Parsing " << pxdata.c_str();
	configuru::Config cfg = configuru::parse_stringEx(pxdata, configuru::JSON);
	std::cout << "parsed\n";
	// an array of array
	std::vector<sensor_reg> items;
	for(const configuru::Config& element: cfg["pgm"].as_array()) {
	  // each element is a two item array [address,value]
	  sensor_reg newItem;
	  // bit of a dance cause its hex
	  std::string regval = (std::string) element[0];
	  std::string valval = (std::string) element[1];
	  sscanf(regval.c_str(),"%hx",&newItem.reg);
	  sscanf(valval.c_str(),"%hx",&newItem.val);
	  std::cout << "Reg:" << newItem.reg << ", val: " << newItem.val << std::endl;
	  items.push_back(newItem);
	}
	// ok, we've got ourselves a program, lets go break the camera
	camera->programSensor(&items[0]);
      }
      return true;
    case Hawkeye::ECameraCommand::REBOOT:
      {
	camera->initializeSensor();
	return true;
      }
    case Hawkeye::ECameraCommand::QUANTIZATION:
      {
	if(m_commandTokens.size() == 4)
	  {
	    std::string qd = m_commandTokens.at(3);
	    uint32_t rqv;
	    sscanf(qd.c_str(),"%x",&rqv);
	    uint8_t fqv = (uint8_t) rqv;
	    fprintf(stderr,"info,camera,%d,SET,quantize , %02x\n",camera->CameraNumber(),fqv);
	    camera->setQuantization(fqv);
	  }
	  return true;
      }
    case Hawkeye::ECameraCommand::NO_CAMERACOMMAND:
      break;
    }
  return false;
}
