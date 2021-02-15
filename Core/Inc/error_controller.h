#pragma once
#include <stdint.h>

enum DEBUG_LEVEL_
{
	DEBUG_LEVEL_ALL,  	//All levels including custom levels.
	DEBUG_LEVEL_DEBUG,	//Designates fine - grained informational events that are most useful to debug an application.
	DEBUG_LEVEL_INFO,	//Designates informational messages that highlight the progress of the application at coarse - grained level.
	DEBUG_LEVEL_WARN,	//Designates potentially harmful situations.
	DEBUG_LEVEL_ERROR,	//Designates error events that might still allow the application to continue running.
	DEBUG_LEVEL_FATAL,	//Designates very severe error events that will presumably lead the application to abort.
	DEBUG_LEVEL_OFF,	//The highest possible rank and is intended to turn off logging.
};

class error_controller
{
		
public:
	DEBUG_LEVEL_ error_level_output;
	error_controller();
	~error_controller();
	void log(DEBUG_LEVEL_, const char*, uint16_t size);
	void on_fatal();
};
