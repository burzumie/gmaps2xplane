#include "gmap_handler.h"
#include "gmap_loader.h"
#include "calc_coord.h"

#include <QString>
#include <QFile>
#include <QFileInfo>
#include <QByteArray>
#include <QImage>
#include <QThread>
#include <QDir>

#include <QDebug>

//
void save_data(const QString& filename, const QByteArray& data, bool append)
{
  QFile file(filename);
  if( (append && file.open(QIODevice::Append)) || file.open(QIODevice::WriteOnly) )
  {
    file.write( data );
    file.flush();
    file.close();
  }
}

//
void save_pol_file(const QString& filename, const QString& img_ext)
{
  // TODO: write valid values for SCALE parameter
  QString pol_file_content = "I\n850\nDRAPED_POLYGON\n\nTEXTURE_NOWRAP		" + filename + img_ext + "\nNO_ALPHA\nSCALE		195 305\nLAYER_GROUP	terrain +1\nSURFACE grass\n";
  save_data("scenario_test/objects/" + filename+".pol", pol_file_content.toLatin1(), false);
}

void save_def_polygon_record(const QString& filename)
{
  QString header_content = "POLYGON_DEF objects/" + filename + ".pol\n";
  save_data("scenario_test/Earth nav data/+50+030/zz_header.txt", header_content.toLatin1(), true);
}

void save_def_polygon_data(double latitude, double longtitude, int zoom, int distance_px_lat, int distance_px_lon)
{
  double dist_lat_m = 0;
  double dist_lon_m = 0;
  double dist_lat_coord = 0;
  double dist_lon_coord = 0;

  calc_dist(latitude, longtitude, zoom, distance_px_lat, distance_px_lon, dist_lat_m, dist_lon_m, dist_lat_coord, dist_lon_coord);

  double new_lat_ne = latitude + dist_lat_coord;
  double new_lon_ne = longtitude + dist_lon_coord;

  double new_lat_sw = latitude - dist_lat_coord;
  double new_lon_sw = longtitude - dist_lon_coord;

  QString lat_ne = QString::number(new_lat_ne, 'f', 6);
  QString lon_ne = QString::number(new_lon_ne, 'f', 6);
  QString lat_sw = QString::number(new_lat_sw, 'f', 6);
  QString lon_sw = QString::number(new_lon_sw, 'f', 6);

  static int cnt = 0;

  QString body_content =
      + "BEGIN_POLYGON " + QString::number(cnt++) + " 65535 4\n"
      + "BEGIN_WINDING\n" +
      + "POLYGON_POINT " + lon_sw + " " + lat_sw + " 0.000000 0.000000\n"
      + "POLYGON_POINT " + lon_ne + " " + lat_sw + " 1.000000 0.000000\n"
      + "POLYGON_POINT " + lon_ne + " " + lat_ne + " 1.000000 1.000000\n"
      + "POLYGON_POINT " + lon_sw + " " + lat_ne + " 0.000000 1.000000\n"
      + "END_WINDING\n"
      + "END_POLYGON\n\n";

  save_data("scenario_test/Earth nav data/+50+030/zz_body.txt", body_content.toLatin1(), true);
}

//
void GMap_Handler::load(double lat, double lon)
{
  if( loader )
  {
    int task_no = loader->get(lat, lon);
    tasks[task_no] = qMakePair(lat, lon);
  }
}

void GMap_Handler::download_progress(int /* n */, qint64 /* current */, qint64 /* total */)
{
}

void GMap_Handler::on_error(int n, int error_code, const QByteArray& response)
{
  qDebug() << "downloaded" << downloads_count << "images, error" << error_code << "in task" << n << ":" << response;
  ::exit(1);
}

void GMap_Handler::handle_downloaded_image(int n, const QByteArray& data)
{
  Tasks::iterator iter = tasks.find(n);
  if( iter == tasks.end() )
  {
    qDebug() << "internal error, no data for task" << n;
    return;
  }

  double latitude = iter->first;
  double longtitude = iter->second;
  tasks.erase(iter);

  // TODO: synchronyze with same code in "GMap_Handler::load_next()"
  QString filename_common = QString::number(latitude, 'f', 6) + "_" + QString::number(longtitude, 'f', 6);
  QString file_ext = ".png";
  QString dir_name = "scenario_test/objects/";
  QString filename_png = dir_name + filename_common + file_ext;
  QFileInfo image_file(filename_png);

  //
  QDir dir(dir_name);
  if( !dir.exists() )
  {
    dir.mkpath(".");
  }

  //
  if( !image_file.exists() || (image_file.size() == 0) )
  {
    qDebug() << "Saving" << filename_png;

    int orig_size = 640;
    int size = 512;
    int offset = (orig_size - size) / 2;

    // also crop image
    QImage image = QImage::fromData(reinterpret_cast<const uchar*>(data.constData()), data.size(), "PNG");
    QImage result = image.copy(offset, offset, size, size);

    result.save(filename_png, 0, -1);
  }

  //
  bool save_additional_data = true;  // TODO: here
  if( save_additional_data )
  {
    // TODO: add directory
    ::save_pol_file(filename_common, file_ext);
    ::save_def_polygon_record(filename_common);
    ::save_def_polygon_data(latitude, longtitude, 18, 256, 256);
  }

  ++downloads_count;

  //
//  QThread::msleep(500);
//  QThread::sleep(1);
  this->load_next();
}


//
void GMap_Handler::gen_sq(int n, int limit, double latitude, double longtitude, bool go_north, bool go_south)
{
  if( n > limit )
  {
    return;
  }

  pending_coords.append( qMakePair(latitude, longtitude) );

  double dist_lat_m = 0;
  double dist_lon_m = 0;
  double dist_lat_coord = 0;
  double dist_lon_coord = 0;

  calc_dist(latitude, longtitude, 18, 512, 512, dist_lat_m, dist_lon_m, dist_lat_coord, dist_lon_coord);

  for(int i = 1; i <= limit; ++i)
  {
    pending_coords.append( qMakePair(latitude, longtitude + (i * dist_lon_coord)) );  // east
  }

  for(int i = 1; i <= limit; ++i)
  {
    pending_coords.append( qMakePair(latitude, longtitude - (i * dist_lon_coord)) );  // west
  }

  if( go_north )
  {
    gen_sq(n+1, limit, latitude + dist_lat_coord, longtitude, true, false);  // north
  }

  if( go_south )
  {
    gen_sq(n+1, limit, latitude - dist_lat_coord, longtitude, false, true);  // south
  }
}

void GMap_Handler::prepare(double latitude, double longtitude, int limit)
{
  this->gen_sq(0, limit, latitude, longtitude, true, true);
}

void GMap_Handler::load_next()
{
  // loop to process already downloaded images
  while( !pending_coords.isEmpty() )
  {
    Coord next_coord = pending_coords.front();
    pending_coords.pop_front();

    double latitude = next_coord.first;
    double longtitude = next_coord.second;

    // TODO: copied from 'GMap_Handler::handle_downloaded_image()', move it to separate function
    QString filename_common = QString::number(latitude, 'f', 6) + "_" + QString::number(longtitude, 'f', 6);
    QString file_ext = ".png";
    QString dir_name = "scenario_test/objects/";
    QString filename_png = dir_name + filename_common + file_ext;
    QFileInfo image_file(filename_png);

    //
    QDir dir(dir_name);
    if( !dir.exists() )
    {
      dir.mkpath(".");
    }
/*
    // move files from one directory to several
    QString src_filename_png = "png/" + filename_common + file_ext;
    QFileInfo src_image_file(src_filename_png);
    if( src_image_file.exists() && dir.exists() )
    {
      qDebug() << src_filename_png << " -> " << filename_png;
      QFile(src_filename_png).rename(filename_png);
    }
*/
    //
    bool do_load = true; // TODO: here
    if( do_load && ( !image_file.exists() || (image_file.size() == 0) ) )
    {
      this->load(latitude, longtitude);
      // break loop when new download started
      break;
    }
    else
    {
      bool save_supplementary_data_in_image_exists = true;
      if( save_supplementary_data_in_image_exists )
      {
        // TODO: add directory
        bool save_additional_data = true; // TODO: here
        if( save_additional_data && image_file.exists() && (image_file.size() != 0) )
        {
          ::save_pol_file(filename_common, file_ext);
          ::save_def_polygon_record(filename_common);
          ::save_def_polygon_data(latitude, longtitude, 18, 256, 256);
        }
      }
    }
  }

  //
  if( pending_coords.isEmpty() && (this->tasks_count() == 0) )
  {
    qDebug() << "all done, downloaded" << downloads_count << "images";
    // TODO: quit
  }
}
