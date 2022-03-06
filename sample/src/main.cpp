#include "SampleApplication.h"

int main(int argc, char** argv)
{
	SampleApplication app;
	app.initialize(
		"sample",
		1024,
		768
	);
	app.run();
	return 0;
}