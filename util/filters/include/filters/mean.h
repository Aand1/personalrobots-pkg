/*
 * Copyright (c) 2008, Willow Garage, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Willow Garage, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived from
 *       this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef FILTERS_MEAN_H_
#define FILTERS_MEAN_H_

#include <stdint.h>
#include <cstring>
#include <stdio.h>

#include "filters/filter_base.h"
#include "ros/assert.h"


namespace filters
{

/** \brief A median filter which works on double arrays.
 *
 */
template <typename T>
class MeanFilter: public FilterBase <T>
{
public:
  /** \brief Construct the filter with the expected width and height */
  MeanFilter();

  /** \brief Destructor to clean up
   */
  ~MeanFilter();

  virtual bool configure(unsigned int width, const std::string& number_of_observations);

  /** \brief Update the filter and return the data seperately
   * \param data_in T array with length width
   * \param data_out T array with length width
   */
  virtual bool update( const T & data_in, T& data_out);

protected:
  T data_storage_;                       ///< Storage for data between updates
  uint32_t last_updated_row_;                   ///< The last row to have been updated by the filter
  uint32_t iterations_;                         ///< Number of iterations up to number of observations

  uint32_t number_of_observations_;             ///< Number of observations over which to filter
  uint32_t width_;           ///< Number of elements per observation

  bool configured_;
};


ROS_REGISTER_FILTER(MeanFilter, std_vector_double)
ROS_REGISTER_FILTER(MeanFilter, std_vector_float)


template <typename T>
MeanFilter<T>::MeanFilter():
  last_updated_row_(0),
  iterations_(0),
  number_of_observations_(0),
  width_(0),
  configured_(false)
{
}

template <typename T>
bool MeanFilter<T>::configure(unsigned int width, const std::string& number_of_observations)
{
  if (configured_)
    return false;
  width_ = width;
  std::stringstream ss;
  ss << number_of_observations;
  ss >> number_of_observations_;
  last_updated_row_ = number_of_observations_;

  data_storage_.resize(number_of_observations_ * width_);
  configured_ = true;
  return true;
}

template <typename T>
MeanFilter<T>::~MeanFilter()
{
}


template <typename T>
bool MeanFilter<T>::update(const T & data_in, T& data_out)
{
  //  ROS_ASSERT(data_in.size() == width_);
  //ROS_ASSERT(data_out.size() == width_);
  if (data_in.size() != width_ || data_out.size() != width_)
    return false;

  //update active row
  if (last_updated_row_ >= number_of_observations_ - 1)
    last_updated_row_ = 0;
  else
    last_updated_row_++;

  //copy incoming data into perminant storage
  memcpy(&data_storage_[width_ * last_updated_row_],
         &data_in[0],
         sizeof(data_in[0]) * width_);

  //Return values

  //keep track of number of rows used while starting up
  uint32_t length;
  if (iterations_ < number_of_observations_ )
  {
    iterations_++;
    length = iterations_;
  }
  else //all rows are allocated
  {
    length = number_of_observations_;
  }

  //Return each value
  for (uint32_t i = 0; i < width_; i++)
  {
    data_out[i] = 0;
    for (uint32_t row = 0; row < length; row ++)
    {
      data_out[i] += data_storage_[i + row * width_];
    }
    data_out[i] /= length;
  }

  return true;
}

}

#endif //#ifndef FILTERS_MEDIAN_H_
