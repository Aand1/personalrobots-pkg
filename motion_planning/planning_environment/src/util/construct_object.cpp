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

/** \author Ioan Sucan */

#include "planning_environment/util/construct_object.h"
#include <ros/console.h>

shapes::Shape* planning_environment::constructObject(const mapping_msgs::Object &obj)
{
    shapes::Shape *shape = NULL;
    if (obj.type == mapping_msgs::Object::SPHERE)
    {
	if (obj.dimensions.size() != 1)
	    ROS_ERROR("Unexpected number of dimensions in sphere definition");
	else
	    shape = new shapes::Sphere(obj.dimensions[0]);
    }
    else
    if (obj.type == mapping_msgs::Object::BOX)
    {
	if (obj.dimensions.size() != 3)
	    ROS_ERROR("Unexpected number of dimensions in box definition");
	else
	    shape = new shapes::Box(obj.dimensions[0], obj.dimensions[1], obj.dimensions[2]);
    }
    else
    if (obj.type == mapping_msgs::Object::CYLINDER)
    {
	if (obj.dimensions.size() != 2)
	    ROS_ERROR("Unexpected number of dimensions in cylinder definition");
	else
	    shape = new shapes::Cylinder(obj.dimensions[0], obj.dimensions[1]);
    }   
    else
    if (obj.type == mapping_msgs::Object::MESH)
    {
	if (obj.dimensions.size() != 0)
	    ROS_ERROR("Unexpected number of dimensions in mesh definition");
	else
	{
	    if (obj.triangles.size() % 3 != 0)
		ROS_ERROR("Number of triangle indices is not divisible by 3");
	    else
	    {
		if (obj.triangles.empty() || obj.vertices.empty())
		    ROS_ERROR("Mesh definition is empty");
		else
		{
		    std::vector<btVector3>    vertices(obj.vertices.size());
		    std::vector<unsigned int> triangles(obj.triangles.size());
		    for (unsigned int i = 0 ; i < obj.vertices.size() ; ++i)
			vertices[i].setValue(obj.vertices[i].x, obj.vertices[i].y, obj.vertices[i].z);
		    for (unsigned int i = 0 ; i < obj.triangles.size() ; ++i)
			triangles[i] = obj.triangles[i];
		    shape = shapes::createMeshFromVertices(vertices, triangles);
		}
	    }
	}
    }
    
    if (shape == NULL)
	ROS_ERROR("Unable to construct shape corresponding to object of type %d", (int)obj.type);
    
    return shape;
}

bool planning_environment::constructObjectMsg(const shapes::Shape* shape, mapping_msgs::Object &obj)
{
    obj.dimensions.clear();
    obj.vertices.clear();
    obj.triangles.clear();
    if (shape->type == shapes::SPHERE)
    {
	obj.type = mapping_msgs::Object::SPHERE;
	obj.dimensions.push_back(static_cast<const shapes::Sphere*>(shape)->radius);
    }
    else
    if (shape->type == shapes::BOX)
    {
	obj.type = mapping_msgs::Object::BOX;
	const double* sz = static_cast<const shapes::Box*>(shape)->size;	
	obj.dimensions.push_back(sz[0]);
	obj.dimensions.push_back(sz[1]);
	obj.dimensions.push_back(sz[2]);
    }
    else
    if (shape->type == shapes::CYLINDER)
    {	
	obj.type = mapping_msgs::Object::CYLINDER;
	obj.dimensions.push_back(static_cast<const shapes::Cylinder*>(shape)->radius);
	obj.dimensions.push_back(static_cast<const shapes::Cylinder*>(shape)->length);
    }
    else
    if (shape->type == shapes::MESH)
    {
	const shapes::Mesh *mesh = static_cast<const shapes::Mesh*>(shape);
	const unsigned int t3 = mesh->triangleCount * 3;

	obj.vertices.resize(mesh->vertexCount);
	obj.triangles.resize(t3);
	
	for (unsigned int i = 0 ; i < mesh->vertexCount ; ++i)
	{
	    obj.vertices[i].x = mesh->vertices[3 * i    ];
	    obj.vertices[i].y = mesh->vertices[3 * i + 1];
	    obj.vertices[i].z = mesh->vertices[3 * i + 2];
	}
	
	for (unsigned int i = 0 ; i < t3  ; ++i)
	    obj.triangles[i] = mesh->triangles[i];
    }
    else
    {
	ROS_ERROR("Unable to construct object message for shape of type %d", (int)shape->type);
	return false;
    }
    
    return true;
}


