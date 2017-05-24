#pragma once

#include "network.h"

class QHttpMultiPart;

class GMap_Handler;

//
class GMap_Loader
{
private:
  typedef void (GMap_Loader::*HANDLER_Progress)(int, qint64, qint64);
  typedef void (GMap_Loader::*HANDLER_Finished)(int, const QByteArray&);
  typedef void (GMap_Loader::*HANDLER_Error)(int, int, const QByteArray&);

  Network::Core* net_core;
  Network::Manager<GMap_Loader> net_manager;

  GMap_Handler* nh;

  int get(double lat, double lon, HANDLER_Finished h, HANDLER_Progress p);

public:
  GMap_Loader(Network::Core* _nc, GMap_Handler* _nh);

  static HANDLER_Error get_error_handler()
  {
    return &GMap_Loader::on_error;
  }

  int get(double lat, double lon);

  void download_finished(int n, const QByteArray& data);
  void download_progress(int n, qint64 current, qint64 total);
  void on_error(int n, int /* error_code */, const QByteArray& response);
};
