#ifndef __PHANTOM_HELPER_H__
#define __PHANTOM_HELPER_H__

#include <HD/hd.h>
#include <HL/hl.h>
#include <HDU/hduError.h>
#include <HDU/hduVector.h>

#include <iostream>
#include <fstream>
#include <string>



namespace PHANTOM_TOOLS
{
	void adjust_force(unsigned char);
	void initHD();
	void exitHandler();
	void printOutput(double);
	void change_direction();
	static HDboolean bRenderForce;
	static HDboolean bool_RighToLeft;
	static HDdouble kStiffness;
}

HDCallbackCode HDCALLBACK DeviceStateCallback(void* data);


#endif	//__PHANTOM_HELPER_H__