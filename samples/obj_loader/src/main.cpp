#include "ObjLoaderApplication.h"

int main(int argc, char **argv)
{
    ObjLoaderApplication app;
    app.initialize({"obj_loader"});
    app.run(argc, argv);
    return 0;
}