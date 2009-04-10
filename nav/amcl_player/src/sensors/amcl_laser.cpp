/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000  Brian Gerkey   &  Kasper Stoy
 *                      gerkey@usc.edu    kaspers@robotics.usc.edu
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
///////////////////////////////////////////////////////////////////////////
//
// Desc: AMCL laser routines
// Author: Andrew Howard
// Date: 6 Feb 2003
// CVS: $Id: amcl_laser.cc 7057 2008-10-02 00:44:06Z gbiggs $
//
///////////////////////////////////////////////////////////////////////////

#include <sys/types.h> // required by Darwin
#include <math.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>

#include "amcl_laser.h"

using namespace amcl;

////////////////////////////////////////////////////////////////////////////////
// Default constructor
AMCLLaser::AMCLLaser(size_t max_beams, 
                     double range_var,
                     double range_bad,
                     pf_vector_t& laser_pose,
                     map_t* map) : AMCLSensor()
{
  this->time = 0.0;

  this->max_beams = max_beams;
  this->range_var = range_var;
  this->range_bad = range_bad;
  this->laser_pose = laser_pose;
  this->map = map;

  return;
}

#if 0
// TODO: should Unsubscribe from the map on error returns in the function
int
AMCLLaser::SetupMap(void)
{
  Device* mapdev;

  // Subscribe to the map device
  if(!(mapdev = deviceTable->GetDevice(this->map_addr)))
  {
    PLAYER_ERROR("unable to locate suitable map device");
    return -1;
  }
  if(mapdev->Subscribe(AMCL.InQueue) != 0)
  {
    PLAYER_ERROR("unable to subscribe to map device");
    return -1;
  }

  // Create the map
  this->map = map_alloc();
  PLAYER_MSG1(2, "AMCL loading map from map:%d...", this->map_addr.index);

  // Fill in the map structure (I'm doing it here instead of in libmap,
  // because libmap is written in C, so it'd be a pain to invoke the internal
  // device API from there)

  // first, get the map info
  Message* msg;
  if(!(msg = mapdev->Request(AMCL.InQueue,
                             PLAYER_MSGTYPE_REQ,
                             PLAYER_MAP_REQ_GET_INFO,
                             NULL, 0, NULL, false)))
  {
    PLAYER_ERROR("failed to get map info");
    return(-1);
  }

  PLAYER_MSG1(2, "AMCL loading map from map:%d...Done", this->map_addr.index);

  player_map_info_t* info = (player_map_info_t*)msg->GetPayload();

  // copy in the map info
  this->map->origin_x = info->origin.px + (info->scale * info->width) / 2.0;
  this->map->origin_y = info->origin.py + (info->scale * info->height) / 2.0;
  this->map->scale = info->scale;
  this->map->size_x = info->width;
  this->map->size_y = info->height;

  delete msg;

  // allocate space for map cells
  this->map->cells = (map_cell_t*)malloc(sizeof(map_cell_t) *
                                                  this->map->size_x *
                                                  this->map->size_y);
  assert(this->map->cells);

  // now, get the map data
  player_map_data_t* data_req;
  int i,j;
  int oi,oj;
  int sx,sy;
  int si,sj;

  data_req = (player_map_data_t*) malloc(sizeof(player_map_data_t));
  assert(data_req);

  // Tile size, limit to sensible maximum of 640x640 tiles
  sy = sx = 640;
  oi=oj=0;
  while((oi < this->map->size_x) && (oj < this->map->size_y))
  {
    si = MIN(sx, this->map->size_x - oi);
    sj = MIN(sy, this->map->size_y - oj);

    data_req->col = oi;
    data_req->row = oj;
    data_req->width = si;
    data_req->height = sj;
    data_req->data_count = 0;

    if(!(msg = mapdev->Request(AMCL.InQueue,
                               PLAYER_MSGTYPE_REQ,
                               PLAYER_MAP_REQ_GET_DATA,
                               (void*)data_req,0,NULL,false)))
    {
      PLAYER_ERROR("failed to get map info");
      free(data_req);
      free(this->map->cells);
      return(-1);
    }

    player_map_data_t* mapdata = (player_map_data_t*)msg->GetPayload();

    // copy the map data
    for(j=0;j<sj;j++)
    {
      for(i=0;i<si;i++)
      {
        this->map->cells[MAP_INDEX(this->map,oi+i,oj+j)].occ_state =
                mapdata->data[j*si + i];
        this->map->cells[MAP_INDEX(this->map,oi+i,oj+j)].occ_dist = 0;
      }
    }

    delete msg;

    oi += si;
    if(oi >= this->map->size_x)
    {
      oi = 0;
      oj += sj;
    }
  }

  free(data_req);

  // we're done with the map device now
  if(mapdev->Unsubscribe(AMCL.InQueue) != 0)
    PLAYER_WARN("unable to unsubscribe from map device");

  PLAYER_MSG0(2, "Done");

  return(0);
}
#endif

#if 0
////////////////////////////////////////////////////////////////////////////////
// Get the current laser reading
//AMCLSensorData *AMCLLaser::GetData(void)
// Process message for this interface
int AMCLLaser::ProcessMessage(QueuePointer &resp_queue,
                                     player_msghdr * hdr,
                                     void * idata)
{
  int i;
  double r, b, db;
  AMCLLaserData *ndata;

  if(!Message::MatchMessage(hdr, PLAYER_MSGTYPE_DATA,
                            PLAYER_LASER_DATA_SCAN, this->laser_addr))
  	return -1;

  this->time = hdr->timestamp;
  player_laser_data_t* data = reinterpret_cast<player_laser_data_t*> (idata);

  b = data->min_angle;
  db = data->resolution;

  ndata = new AMCLLaserData;

  ndata->sensor = this;
  ndata->time = hdr->timestamp;

  ndata->range_count = data->ranges_count;
  ndata->range_max = data->max_range;
  ndata->ranges = new double [ndata->range_count][2];

  // Read the range data
  for (i = 0; i < ndata->range_count; i++)
  {
    r = data->ranges[i];
    ndata->ranges[i][0] = r;
    ndata->ranges[i][1] = b;
    b += db;
  }

  AMCL.Push(ndata);

  return 0;
}
#endif

////////////////////////////////////////////////////////////////////////////////
// Apply the laser sensor model
bool AMCLLaser::UpdateSensor(pf_t *pf, AMCLSensorData *data)
{
  AMCLLaserData *ndata;

  ndata = (AMCLLaserData*) data;
  if (this->max_beams < 2)
    return false;

  // Apply the laser sensor model
  pf_update_sensor(pf, (pf_sensor_model_fn_t) SensorModel, data);

  return true;
}


////////////////////////////////////////////////////////////////////////////////
// Determine the probability for the given pose
double AMCLLaser::SensorModel(AMCLLaserData *data, pf_sample_set_t* set)
{
  AMCLLaser *self;
  int i, j, step;
  double z, c, pz;
  double p;
  double map_range;
  double obs_range, obs_bearing;
  double total_weight;
  pf_sample_t *sample;
  pf_vector_t pose;

  self = (AMCLLaser*) data->sensor;

  total_weight = 0.0;

  double z_hit = 0.25;
  double z_short = 0.25;
  double z_max = 0.25;
  double z_rand = 0.25;

  double sigma_hit = 0.1;
  double lambda_short = 0.1;
  double chi_outlier = 0.5;

  // Compute the sample weights
  for (j = 0; j < set->sample_count; j++)
  {
    sample = set->samples + j;
    pose = sample->pose;

    // Take account of the laser pose relative to the robot
    pose = pf_vector_coord_add(self->laser_pose, pose);

    p = 1.0;

    step = (data->range_count - 1) / (self->max_beams - 1);
    for (i = 0; i < data->range_count; i += step)
    {
      obs_range = data->ranges[i][0];
      obs_bearing = data->ranges[i][1];

      // Compute the range according to the map
      map_range = map_calc_range(self->map, pose.v[0], pose.v[1],
                                 pose.v[2] + obs_bearing, data->range_max + 1.0);
#if 1
      if (obs_range >= data->range_max && map_range >= data->range_max)
      {
        pz = 1.0;
      }
      else
      {
        // TODO: proper sensor model (using Kolmagorov?)
        // Simple gaussian model
        c = self->range_var;
        z = obs_range - map_range;
        pz = self->range_bad + (1 - self->range_bad) * exp(-(z * z) / (2 * c * c));
      }
#else

      pz = 0.0;

      // Part 1: good, but noisy, hit
      z = obs_range - map_range;
      pz += z_hit * exp(-(z * z) / (2 * sigma_hit * sigma_hit));

      // Part 2: short reading from unexpected obstacle (e.g., a person)
      if(z < 0)
        pz += z_short * lambda_short * exp(-lambda_short*obs_range);

      // Part 3: Failure to detect obstacle, reported as max-range
      if(obs_range == data->range_max)
        pz += z_max * 1.0;

      // Part 4: Random measurements
      if(obs_range < data->range_max)
        pz += z_rand * 1.0/data->range_max;

      // TODO: outlier rejection for short readings
#endif

      assert(pz <= 1.0);
      assert(pz >= 0.0);
      p *= pz;
    }

    //printf("%e\n", p);
    //assert(p >= 0);

    sample->weight *= p;
    total_weight += sample->weight;
  }

  return(total_weight);
}



#ifdef INCLUDE_RTKGUI

////////////////////////////////////////////////////////////////////////////////
// Setup the GUI
void AMCLLaser::SetupGUI(rtk_canvas_t *canvas, rtk_fig_t *robot_fig)
{
  this->fig = rtk_fig_create(canvas, robot_fig, 0);

  // Draw the laser map
  this->map_fig = rtk_fig_create(canvas, NULL, -50);
  map_draw_occ(this->map, this->map_fig);

  return;
}


////////////////////////////////////////////////////////////////////////////////
// Shutdown the GUI
void AMCLLaser::ShutdownGUI(rtk_canvas_t *canvas, rtk_fig_t *robot_fig)
{
  rtk_fig_destroy(this->map_fig);
  rtk_fig_destroy(this->fig);
  this->map_fig = NULL;
  this->fig = NULL;

  return;
}


////////////////////////////////////////////////////////////////////////////////
// Draw the laser values
void AMCLLaser::UpdateGUI(rtk_canvas_t *canvas, rtk_fig_t *robot_fig, AMCLSensorData *data)
{
  int i, step;
  double r, b, ax, ay, bx, by;
  AMCLLaserData *ndata;

  ndata = (AMCLLaserData*) data;

  rtk_fig_clear(this->fig);

  // Draw the complete scan
  rtk_fig_color_rgb32(this->fig, 0x8080FF);
  for (i = 0; i < ndata->range_count; i++)
  {
    r = ndata->ranges[i][0];
    b = ndata->ranges[i][1];

    ax = 0;
    ay = 0;
    bx = ax + r * cos(b);
    by = ay + r * sin(b);

    rtk_fig_line(this->fig, ax, ay, bx, by);
  }

  if (this->max_beams < 2)
    return;

  // Draw the significant part of the scan
  step = (ndata->range_count - 1) / (this->max_beams - 1);
  for (i = 0; i < ndata->range_count; i += step)
  {
    r = ndata->ranges[i][0];
    b = ndata->ranges[i][1];
    //m = map_calc_range(this->map, pose.v[0], pose.v[1], pose.v[2] + b, 8.0);

    ax = 0;
    ay = 0;

    bx = ax + r * cos(b);
    by = ay + r * sin(b);
    rtk_fig_color_rgb32(this->fig, 0xFF0000);
    rtk_fig_line(this->fig, ax, ay, bx, by);
  }

  return;
}

#endif



