#pragma once

#include <QMap>
#include <QPair>
#include <QList>

class QByteArray;

class GMap_Loader;

//
class GMap_Handler
{
private:
  GMap_Loader* loader;

  typedef QPair<double, double> Coord;
  typedef QMap<int, Coord> Tasks;

  int downloads_count;
  Tasks tasks;
  QList<Coord> pending_coords;

  //
  void gen_sq(int n, int limit, double latitude, double longtitude, bool go_north, bool go_south);

public:
  GMap_Handler()
    : loader(NULL), downloads_count(0)
  {
  }

  void set_loader(GMap_Loader* l)
  {
    loader = l;
  }

  int tasks_count()
  {
    return tasks.size();
  }

  void prepare(double latitude, double longtitude, int limit);
  void load_next();

  void load(double lat, double lon);
  void handle_downloaded_image(int n, const QByteArray& data);

  void download_progress(int n, qint64 current, qint64 total);
  void on_error(int n, int error_code, const QByteArray& response);
};
