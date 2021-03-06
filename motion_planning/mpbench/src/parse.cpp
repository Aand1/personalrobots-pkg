/*********************************************************************
 * Software License Agreement (BSD License)
 * 
 *  Copyright (c) 2008, Willow Garage, Inc.
 *  All rights reserved.
 * 
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 * 
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of the Willow Garage nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 * 
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *********************************************************************/

#include "parse.h"
#include <sfl/util/strutil.hpp>

extern "C" {
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <err.h>
}

using namespace std;

extern "C" {
  
  static void start_element_handler(void * user_data, const XML_Char * name,
				    const XML_Char ** atts)
    throw(std::runtime_error);
  
  static void end_element_handler(void * user_data, const XML_Char * name)
    throw(std::runtime_error);
  
  static void character_data_handler(void * user_data, const XML_Char * s,
				     int len);
  
}

namespace mpbench {
  
  StringBuffer::
  StringBuffer()
    : m_lastchar(' ')
  {
  }
  
  
  void StringBuffer::
  Append(const XML_Char * s, int len)
  {
    for (int ii(0); ii < len; ++ii) {
      if (isspace(s[ii])) {
	if (' ' != m_lastchar) {
	  m_string += ' ';
	  m_lastchar = ' ';
	}
	// else skip
      }
      else {
	m_string += s[ii];
	m_lastchar = s[ii];
      }
    }
  }
  
  
  string const & StringBuffer::
  GetString() const
  {
    return m_string;
  }
  
  
  bool StringBuffer::
  Empty() const
  {
    return m_string.empty();
  }
  
  
  File::
  File(char const * path, int flags) throw (std::runtime_error)
    : fd(open(path, flags))
  {
    if (fd < 0)
      throw runtime_error(string("open(") + path + "): " + strerror(errno));
  }
  
  
  File::
  File(char const * path, int flags, mode_t mode) throw (std::runtime_error)
    : fd(open(path, flags, mode))
  {
    if (fd < 0)
      throw runtime_error(string("open(") + path + "): " + strerror(errno));
  }
  
  
  File::
  ~File()
  {
    if (fd >= 0)
      close(fd);
  }
  
  
  SetupParser::
  SetupParser()
    : parser(0),
      filename("/dev/null"),
      bufsize(128),
      setup(0),
      progress_os(0),
      def_start(false, false, true, numeric_limits<double>::max(), 0, 0, 0),
      def_goal(0, 0, 0, 1, M_PI),
      tmp_start(def_start),
      tmp_goal(def_goal),
      tmp_task("", def_goal)
  {
  }
  
  
  SetupParser::
  ~SetupParser()
  {
    if (parser)
      XML_ParserFree(parser);
  }
  
  
  void SetupParser::
  Parse(std::string xml_filename, Setup * setup,
	std::ostream * progress_os, std::ostream * debug_os)
    throw(std::runtime_error)
  {
    if (0 == parser)
      parser = XML_ParserCreate(NULL);
    XML_SetElementHandler(parser, start_element_handler, end_element_handler);
    XML_SetCharacterDataHandler(parser, character_data_handler);
    XML_SetUserData(parser, this);
    
    filename = xml_filename;
    this->setup = setup;
    this->world = setup->getWorld().get();
    this->progress_os = progress_os;
    this->debug_os = debug_os;
    file.reset(new File(filename.c_str(), O_RDONLY));
    
    // clear tag_stack
    while ( ! tag_stack.empty())
      tag_stack.pop();
    
    while (true) {
      void * buf(XML_GetBuffer(parser, bufsize));
      if (NULL == buf)
	throw runtime_error("mpbench::SetupParser::Parse(): XML_GetBuffer() failed");
      ssize_t const bytes_read(read(file->fd, buf, bufsize));
      if (bytes_read < 0) {
	ostringstream os;
	os << "mpbench::SetupParser::Parse(): " << filename << ": read(): " << strerror(errno);
	throw runtime_error(os.str());
      }
      if ( ! XML_ParseBuffer(parser, bytes_read, bytes_read == 0)) {
	ostringstream os;
	os << "mpbench::SetupParser::Parse(): " << filename
	   << ": " << XML_GetCurrentLineNumber(parser)
	   << ": parse error: " << XML_ErrorString(XML_GetErrorCode(parser));
	throw runtime_error(os.str());
      }
      if (bytes_read == 0)
	break;
    }
  }
  
}


using namespace mpbench;

static void throwme(SetupParser * sp, string const & complaint) throw(runtime_error)
{
  ostringstream os;
  os << sp->filename.c_str() << ":" << XML_GetCurrentLineNumber(sp->parser) << ": " << complaint;
  throw runtime_error(os.str());
}


void start_element_handler(void * user_data,
			   const XML_Char * name,
			   const XML_Char ** atts)
    throw(std::runtime_error)
{
  mpbench::SetupParser * sp(reinterpret_cast<mpbench::SetupParser *>(user_data));

  string prevtag("NONE");
  if ( ! sp->tag_stack.empty())
    prevtag = sp->tag_stack.top();
  string const tag(name);
  
  if (("addline" == tag) || ("rmline" == tag)) {
    if ("world" != prevtag)
      throwme(sp, "<" + tag + "> only allowed in <world>, not in <" + prevtag + ">");
    sp->buffer.reset(new mpbench::StringBuffer());
    sp->tmp_episode = 0;
    while ((0 != atts) && (0 != *atts)) {
      string const name(*atts);
      ++atts;
      if (0 == atts)
	throwme(sp, "bug in expat? while parsing attribute " + name + "of <" + tag
		+ ">: attribute value is null");
      if ("episode" == name) {
	string value(*atts);
	if ( ! value.empty()) {
	  if ('"' == value[0])
	    value[0] = ' ';
	  istringstream is(value);
	  int ep;
	  is >> ep;
	  if ( ! is)
	    throwme(sp, "could not read episode attribute value from " + value);
	  if (0 < ep)
	    sp->tmp_episode = ep;
	}
      }
      else if (sp->debug_os)
	*sp->debug_os << "mpbench::parse: ignoring attribute " << name << " in <" << tag << ">\n";
      ++atts;
    }
  }
  
  else if ("description" == tag) {
    if ("task" != prevtag)
      throwme(sp, "<description> only allowed in <task>, not in <" + prevtag + ">");
    sp->buffer.reset(new mpbench::StringBuffer());
  }
  
  else if (("from_scratch" == tag) || ("use_initial_solution" == tag)
	   || ("allow_iteration" == tag) || ("alloc_time" == tag)) {
    if ("start" != prevtag)
      throwme(sp, "<" + tag + "> only allowed in <start>, not in <" + prevtag + ">");
    sp->buffer.reset(new mpbench::StringBuffer());
  }
  
  else if ("tolerance" == tag) {
    if ("goal" != prevtag)
      throwme(sp, "<tolerance> only allowed in <goal>, not in <" + prevtag + ">");
    sp->buffer.reset(new mpbench::StringBuffer());
  }
  
  else if ("pose" == tag) {
    if (("start" != prevtag) && ("goal" != prevtag))
      throwme(sp, "<pose> only allowed in <start> or <goal>, not in <" + prevtag + ">");
    sp->buffer.reset(new mpbench::StringBuffer());
  }
  
  else if ("door" == tag) {
    if ("task" != prevtag)
      throwme(sp, "<door> only allowed in <task>, not in <" + prevtag + ">");
    sp->buffer.reset(new mpbench::StringBuffer());
  }
  
  else if ("task" == tag) {
    sp->tmp_task.description = "none";
    sp->tmp_task.start.clear();
    sp->tmp_task.door.reset();
  }
  
  else if ("start" == tag) {
    sp->tmp_start = sp->def_start;
  }
  
  else if ("goal" == tag) {
    sp->tmp_goal = sp->def_goal;
  }
  
  else if (sp->debug_os)
    *sp->debug_os << "mpbench::parse: ignoring start tag <" << tag << ">\n";
  
  sp->tag_stack.push(tag);
}


void end_element_handler(void * user_data,
			 const XML_Char * name)
    throw(std::runtime_error)
{
  mpbench::SetupParser * sp(reinterpret_cast<mpbench::SetupParser *>(user_data));
  
  sp->tag_stack.pop();
  string prevtag("NONE");
  if ( ! sp->tag_stack.empty())
    prevtag = sp->tag_stack.top();
  string const tag(name);
  
  if (("addline" == tag) || ("rmline" == tag)){
    if ( ! sp->buffer)
      throwme(sp, "BUG: no string buffer for <" + tag + ">");
    else {
      istringstream is(sp->buffer->GetString());
      double x0, y0, x1, y1;
      is >> x0 >> y0 >> x1 >> y1;
      if ( ! is)
	throwme(sp, "could not read x0 y0 x1 y1 from \"" + sp->buffer->GetString() + "\"");
      if ("addline" == tag)
	sp->world->drawLine(sp->tmp_episode, true, x0, y0, x1, y1);
      else
	sp->world->drawLine(sp->tmp_episode, false, x0, y0, x1, y1);
    }
  }
  
  else if ("description" == tag) {
    if ( ! sp->buffer)
      throwme(sp, "BUG: no string buffer for <description>");
    sp->tmp_task.description = sp->buffer->GetString();
  }
  
  else if ("from_scratch" == tag) {
    if ( ! sp->buffer)
      throwme(sp, "BUG: no string buffer for <from_scratch>");
    if ( ! sfl::string_to(sp->buffer->GetString(), sp->tmp_start.from_scratch))
      throwme(sp, "from_scratch: expected boolean, not " + sp->buffer->GetString());
  }
  
  else if ("use_initial_solution" == tag) {
    if ( ! sp->buffer)
      throwme(sp, "BUG: no string buffer for <use_initial_solution>");
    if ( ! sfl::string_to(sp->buffer->GetString(), sp->tmp_start.use_initial_solution))
      throwme(sp, "use_initial_solution: expected boolean, not " + sp->buffer->GetString());
  }
  
  else if ("allow_iteration" == tag) {
    if ( ! sp->buffer)
      throwme(sp, "BUG: no string buffer for <allow_iteration>");
    if ( ! sfl::string_to(sp->buffer->GetString(), sp->tmp_start.allow_iteration))
      throwme(sp, "allow_iteration: expected boolean, not " + sp->buffer->GetString());
  }
  
  else if ("alloc_time" == tag) {
    if ( ! sp->buffer)
      throwme(sp, "BUG: no string buffer for <alloc_time>");
    if (("inf" == sp->buffer->GetString()) || ("max" == sp->buffer->GetString()))
      sp->tmp_start.alloc_time = numeric_limits<double>::max();
    else {
      if ( ! sfl::string_to(sp->buffer->GetString(), sp->tmp_start.alloc_time))
	throwme(sp, "alloc_time: expected number, not " + sp->buffer->GetString());
      if (0 > sp->tmp_start.alloc_time)
	throwme(sp, "alloc_time: expected non-negative number, not " + sp->buffer->GetString());
    }
  }
  
  else if ("tolerance" == tag) {
    if ( ! sp->buffer)
      throwme(sp, "BUG: no string buffer for <tolerance>");
    istringstream is(sp->buffer->GetString());
    is >> sp->tmp_goal.tol_xy
       >> sp->tmp_goal.tol_th;
    if ( ! is)
      throwme(sp, "could not read tolerance from \"" + sp->buffer->GetString() + "\"");
  }
  
  else if ("pose" == tag) {
    if ( ! sp->buffer)
      throwme(sp, "BUG: no string buffer for <pose>");
    istringstream is(sp->buffer->GetString());
    if ("start" == prevtag) {
      is >> sp->tmp_start.px
	 >> sp->tmp_start.py
	 >> sp->tmp_start.pth;
      if ( ! is)
	throwme(sp, "could not read start pose from \"" + sp->buffer->GetString() + "\"");
    }
    else if ("goal" == prevtag) {
      is >> sp->tmp_goal.px
	 >> sp->tmp_goal.py
	 >> sp->tmp_goal.pth;
      if ( ! is)
	throwme(sp, "could not read goal pose from \"" + sp->buffer->GetString() + "\"");
    }
    else
      throwme(sp, "<pose> only allowed in <start> or <goal>, not in <" + prevtag + ">");
  }
  
  else if ("door" == tag) {
    if ( ! sp->buffer)
      throwme(sp, "BUG: no string buffer for <door>");
    istringstream is(sp->buffer->GetString());
    is >> sp->tmp_hinge_x
       >> sp->tmp_hinge_y
       >> sp->tmp_door_x
       >> sp->tmp_door_y
       >> sp->tmp_handle_distance
       >> sp->tmp_angle_range;
    if ( ! is)
      throwme(sp, "could not read door spec from \"" + sp->buffer->GetString() + "\"");
    sp->tmp_task.door = mpglue::doorspec::convert(sp->tmp_hinge_x,
						   sp->tmp_hinge_y,
						   sp->tmp_door_x,
						   sp->tmp_door_y,
						   sp->tmp_handle_distance,
						   sp->tmp_angle_range);
  }
  
  else if ("goal" == tag) {
    if ("defaults" == prevtag)
      sp->def_goal = sp->tmp_goal;
    else if ("task" == prevtag)
      sp->tmp_task.goal = sp->tmp_goal;
    else
      throwme(sp, "<goal> only allowed in <defaults> or <task>, not in <" + prevtag + ">");
  }
  
  else if ("start" == tag) {
    if ("defaults" == prevtag)
      sp->def_start = sp->tmp_start;
    else if ("task" == prevtag)
      sp->tmp_task.start.push_back(sp->tmp_start);
    else
      throwme(sp, "<start> only allowed in <defaults> or <task>, not in <" + prevtag + ">");
  }
  
  else if ("task" == tag) {
    sp->setup->addTask(sp->tmp_task);
  }
  
  else if (sp->debug_os)
    *sp->debug_os << "mpbench::parse: ignoring end tag <" << tag << ">\n";
  
  sp->buffer.reset();
}


void character_data_handler(void * user_data,
			    const XML_Char * s,
			    int len)
{
  mpbench::SetupParser * sp(reinterpret_cast<mpbench::SetupParser *>(user_data));
  if (sp->buffer)
    sp->buffer->Append(s, len);
}
