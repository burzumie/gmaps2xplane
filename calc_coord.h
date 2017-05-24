#pragma once

double trunc(double value, int digits);
void calc_dist(double latitude, double longtitude, int zoom, int distance_px_lat, int distance_px_lon,
      double& res_dist_lat_m, double& res_dist_lon_m, double& res_dist_lat_coord, double& res_dist_lon_coord);
