#include "pch.h"
#include "AppBase.h"

int main() { 
	DE::AppBase app;

	if (!app.Initialize())
		return -1;

	return app.Run();
}