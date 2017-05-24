#include "gmap_loader.h"
#include "gmap_handler.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QByteArray>

#include <QDebug>

//
GMap_Loader::GMap_Loader(Network::Core* _nc, GMap_Handler* _nh)
  : net_core(_nc), net_manager(_nc, this), nh(_nh)
{
}

int GMap_Loader::get(double lat, double lon, HANDLER_Finished h, HANDLER_Progress p)
{
  QString url = "http://maps.googleapis.com/maps/api/staticmap?center=" + QString::number(lat, 'f', 6) + "," + QString::number(lon, 'f', 6) + "&zoom=18&scale=1&size=640x640&maptype=satellite&sensor=false&format=png";

  return net_manager.download(url, h, p);
}

int GMap_Loader::get(double lat, double lon)
{
//  return this->get(50.450700, 30.523123, &GMap_Loader::download_finished, &GMap_Loader::download_progress);
//  return this->get(50.452448, 30.523123, &GMap_Loader::download_finished, &GMap_Loader::download_progress);
  return this->get(lat, lon, &GMap_Loader::download_finished, &GMap_Loader::download_progress);
}

void GMap_Loader::download_finished(int n, const QByteArray& data)
{
//  qDebug() << "downloaded" << data.size() << "bytes";
  nh->handle_downloaded_image(n, data);
}

void GMap_Loader::download_progress(int n, qint64 current, qint64 total)
{
//  qDebug() << "download progress" << current << "of" << total << "bytes";
  nh->download_progress(n, current, total);
}

void GMap_Loader::on_error(int n, int error_code, const QByteArray& response)
{
//  qDebug() << "download error" << error_code << ":" << response;
  nh->on_error(n, error_code, response);
}
