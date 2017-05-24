#pragma once

#include "calc_coord.h"

#include <math.h>

//#include <QDebug>
//#include <QString>

//
double trunc(double value, int digits)
{
  double factor = ::pow(10.0, digits);
  return ::trunc(value * factor) / factor;
}


void calc_dist(double latitude, double longtitude, int zoom, int distance_px_lat, int distance_px_lon,
      double& res_dist_lat_m, double& res_dist_lon_m, double& res_dist_lat_coord, double& res_dist_lon_coord)
{
    static const double earth_radius = 6378.137;  // mean equatorial Earth's radius, km
    static const double pi = 3.14159265358979323846;
    static const double equator_length = 2 * pi * earth_radius;
    static const double meridian_length = 20003.93; // taken from wikipedia, km

    {
/*
      double km_per_degree_eqt = equator_length / 360;   // 113.319 km
      double km_per_degree_mer = meridian_length / 180;  // 113.133 km
*/
      double zoom_factor = ::pow(2.0, zoom-1);
      static const int tile_width_px = 512;
      double tile_width_px_zoom = zoom_factor * tile_width_px;

      double lat_factor = ::cos(latitude * pi/180);

      double m_per_px_lat = 1000 * equator_length * lat_factor / tile_width_px_zoom;
      double m_per_px_lon = 1000 * meridian_length * 2 / tile_width_px_zoom;  // multiply by 2 cause meridian covers half of circle, 180 degrees, not 360

      double minutes_in_circle = 360 * 60; // 1 degree == 60 minutes
      double m_per_min_lat = 1000 * equator_length / minutes_in_circle;
      double m_per_min_lon = 1000 * meridian_length * 2 / minutes_in_circle;

      double dist_lat_m = m_per_px_lat * distance_px_lat;
      double dist_lon_m = m_per_px_lon * distance_px_lon;

      double dist_lat = (m_per_px_lat * distance_px_lat) / (m_per_min_lat * 60);
      double dist_lon = (m_per_px_lon * distance_px_lon) / (m_per_min_lon * 60);
      //km_per_degree_eqt / 60;
      //(m_per_px_lon * distance_px);

      // results
      res_dist_lat_m      = ::trunc(dist_lat_m, 6);
      res_dist_lon_m      = ::trunc(dist_lon_m, 6);
      res_dist_lat_coord  = ::trunc(dist_lat, 6);
      res_dist_lon_coord  = ::trunc(dist_lon, 6);
/*
      double new_lat = latitude + dist_lat;
      double new_lon = longtitude + dist_lon;

      double new_lat_2 = latitude - dist_lat;
      double new_lon_2 = longtitude - dist_lon;

//      qDebug() << km_per_degree_eqt << km_per_degree_mer << minutes_in_circle << m_per_px_lat << m_per_px_lon << m_per_min_lat << m_per_min_lon << dist_lat << dist_lon;
      qDebug() << dist_lat_m << dist_lon_m;
      qDebug() << QString::number(new_lat, 'f', 8) << "|" << QString::number(new_lon, 'f', 8);
      qDebug() << QString::number(new_lat_2, 'f', 8) << "|" << QString::number(new_lon_2, 'f', 8);
*/
    }
}
