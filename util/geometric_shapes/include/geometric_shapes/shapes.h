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

#ifndef GEOMETRIC_SHAPES_SHAPES_
#define GEOMETRIC_SHAPES_SHAPES_

#include <cstdlib>
#include <vector>
#include <LinearMath/btVector3.h>

/** Definition of various shapes. No properties such as position are
    included. These are simply the descriptions and dimensions of
    shapes. */

namespace shapes
{
    
    /** \brief A basic definition of a shape. Shapes are considered centered at origin */
    class Shape
    {		    
    public:	    
	Shape(void)
	{
	    type = UNKNOWN;
	}
	
	virtual ~Shape(void)
	{
	}
	
	enum { UNKNOWN, SPHERE, CYLINDER, BOX, MESH } 
	    type;
	
    };
    
    /** \brief Definition of a sphere */
    class Sphere : public Shape
    {
    public:
	Sphere(void) : Shape()
	{
	    type   = SPHERE;
	    radius = 0.0;
	}
	
	Sphere(double r) : Shape()
	{
	    type   = SPHERE;
	    radius = r;
	}
	
	double radius; 
    };
    
    /** \brief Definition of a cylinder */
    class Cylinder : public Shape
    {
    public:
	Cylinder(void) : Shape()
	{
	    type   = CYLINDER;
	    length = radius = 0.0;
	}
	
	Cylinder(double r, double l) : Shape()
	{
	    type   = CYLINDER;
	    length = l;
	    radius = r;
	}
	
	double length, radius; 
    };
    
    /** \brief Definition of a box */
    class Box : public Shape
    {
    public:
	Box(void) : Shape()
	{
	    type = BOX;
	    size[0] = size[1] = size[2] = 0.0;
	}
	
	Box(double x, double y, double z) : Shape()
	{
	    type = BOX;
	    size[0] = x;
	    size[1] = y;
	    size[2] = z;
	}
	
	/** \brief x, y, z */
	double size[3]; 
    };
    
    /** \brief Definition of a mesh */
    class Mesh : public Shape
    {
    public:
	Mesh(void) : Shape()
	{
	    type = MESH;
	    vertexCount = 0;
	    vertices = NULL;
	    triangleCount = 0;
	    triangles = NULL;
	    normals = NULL;
	}
	
	Mesh(unsigned int vCount, unsigned int tCount) : Shape()
	{
	    type = MESH;
	    vertexCount = vCount;
	    vertices = new double[vCount * 3];
	    triangleCount = tCount;
	    triangles = new unsigned int[tCount * 3];
	    normals = new double[tCount * 3];
	}
	
	virtual ~Mesh(void)
	{
	    if (vertices)
		delete[] vertices;
	    if (triangles)
		delete[] triangles;
	    if (normals)
		delete[] normals;
	}
	
	/** \brief the number of available vertices */
	unsigned int  vertexCount;       

	/** \brief the position for each vertex vertex k has values at
	 * index (3k, 3k+1, 3k+2) = (x,y,z) */
	double       *vertices;          

	/** \brief the number of triangles formed with the vertices */
	unsigned int  triangleCount;     

	/** \brief the vertex indices for each triangle
	 * triangle k has vertices at index (3k, 3k+1, 3k+2) = (v1, v2, v3) */
	unsigned int *triangles;
	
	/** \brief the normal to each triangle unit vector represented
	    as (x,y,z)  */
	double       *normals;       
    };
    
    /** \brief Load a mesh from a set of vertices. Every 3 vertices
	are considered a triangle. Repeating vertices are identified
	and the set of triangle indices is constructed. The normal at
	each triangle is also computed */
    Mesh* create_mesh_from_vertices(const std::vector<btVector3> &source);

    /** \brief Load a mesh from a binary STL file. Normals are
	recomputed and repeating vertices are identified. */
    Mesh* create_mesh_from_binary_stl(const char *filename);
}

#endif
