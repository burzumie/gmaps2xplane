#include "calc_coord.h"
#include "gmap_handler.h"
#include "gmap_loader.h"

#include <QApplication>

//
int main(int argc, char* argv[])
{
  QApplication qapp(argc, argv);

  // starting coordinates
  double latitude = 50.450700;
  double longtitude = 30.523123;

  //
  Network::Core* net_core = new Network::Core;
  GMap_Handler* gmap_handler = new GMap_Handler;
  GMap_Loader* gmap_loader = new GMap_Loader(net_core, gmap_handler);
  gmap_handler->set_loader(gmap_loader);

  gmap_handler->prepare(latitude, longtitude, 16);  // try to make this count of steps to north, south, east and west
  gmap_handler->load_next();

  return qapp.exec();
}
