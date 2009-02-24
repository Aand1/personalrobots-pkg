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

#ifndef FILTERS_FILTER_BASE_H_
#define FILTERS_FILTER_BASE_H_

#include <tinyxml/tinyxml.h>
#include <typeinfo>
#include <loki/Factory.h>
#include "ros/assert.h"

typedef std::vector<float> std_vector_float;
typedef std::vector<double> std_vector_double;


namespace filters
{

template <typename T>
std::string getFilterID(const std::string & filter_name)
{
  return filter_name + typeid(T).name();
  
}


/** \brief A Base filter class to provide a standard interface for all filters
 *
 */
template<typename T>
class FilterBase
{
public:
  FilterBase(){};
  virtual ~FilterBase(){};

  virtual bool configure(unsigned int number_of_channels, TiXmlElement *config)=0;


  /** \brief Update the filter and return the data seperately
   */
  virtual bool update(const T& data_in, T& data_out)=0;

  std::string getType() {return typeid(T).name();};
};


template <typename T>
class FilterFactory : public Loki::SingletonHolder < Loki::Factory< filters::FilterBase<T>, std::string >,
                                                     Loki::CreateUsingNew,
                                                     Loki::LongevityLifetime::DieAsSmallObjectChild >
{
  //empty
};
  


#define ROS_REGISTER_FILTER(c,t) \
  filters::FilterBase<t> * Filters_New_##c##__##t() {return new c< t >;}; \
  bool ROS_FILTER_## c ## _ ## t =                                                    \
    filters::FilterFactory<t>::Instance().Register(filters::getFilterID<t>(std::string(#c)), Filters_New_##c##__##t); 
///\todo make this use templating to get the data type, the user doesn't ever set the data type at runtime

}

#endif //#ifndef FILTERS_FILTER_BASE_H_
