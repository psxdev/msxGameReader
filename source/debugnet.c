#include <stdio.h>
#include <stdarg.h>
#include <debugnet.h>


void debugNetPrintf_(const char* fmt, ...)
{
	char buffer[0x800];
	va_list arg;
	va_start(arg, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, arg);
	va_end(arg);
	printf("%s",buffer);
	fflush(stdout);
}
/**
 * Log Level printf for debugnet library 
 *
 * @par Example:
 * @code
 * debugNetPrintf(INFO,"This is a test\n");
 * @endcode
 *
 * @param level - NONE,INFO,ERROR or DEBUG
 */
int logLevel=DEBUG;
void debugNetPrintf(int level, char* format, ...) 
{
	char msgbuf[0x800];
	va_list args;
	
		if (level>logLevel)
		return;
       
	va_start(args, format);
	vsnprintf(msgbuf,2048, format, args);	
	msgbuf[2047] = 0;
	va_end(args);
	switch(level)
	{
		case INFO:
	    	debugNetPrintf_("[HOST][INFO]:%s",msgbuf);  
	        break;
	   	case ERROR: 
	    	debugNetPrintf_("[HOST][ERROR]:%s",msgbuf);
	        break;
		case DEBUG:
	        debugNetPrintf_("[HOST][DEBUG]:%s",msgbuf);
	        break;
		case NONE:
			break;
	    default:
		    debugNetPrintf_("%s",msgbuf);
       
	}
	
}
