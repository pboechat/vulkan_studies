#include "SampleApplication.h"

int main(int argc, char **argv)
{
	SampleApplication app;
	app.initialize({"sample"});
	app.run(argc, argv);
	return 0;
}