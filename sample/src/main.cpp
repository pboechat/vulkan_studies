#include <framework/Application.h>

int main(int argc, char** argv)
{
	framework::Application app("sample");
	app.initialize(
		framework::eQueueCap_Transfer | framework::eQueueCap_Graphics | framework::eQueueCap_Presentation,
		{ "VK_LAYER_KHRONOS_validation" },
		{ "VK_KHR_surface" }
	);
	return 0;
}