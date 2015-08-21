#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#include <Neptune.h>

enum Function
{
	Fun_ControlPoint = 0,
	Fun_MediaServer,
	Fun_MediaRender
};

enum FlushMode
{
	FM_ALL = 0,
	FM_MediaServer,
	FM_MediaRender
};

#endif	// __GLOBAL_H__