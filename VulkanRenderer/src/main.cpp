#include "Logger.h"

int main()
{
	Logger::Init();

	LOG(trace, "trace");
	LOGVk(info, "info");

	return 0;
}