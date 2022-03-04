#include <vkfw/Application.h>

int main(int argc, char** argv)
{
	vkfw::Application app;
	app.initialize(
		"sample",
		1024,
		768
	);
	app.run();
	return 0;
}