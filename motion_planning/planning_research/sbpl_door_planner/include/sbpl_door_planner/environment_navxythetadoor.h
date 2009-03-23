/*
 * Copyright (c) 2008, Maxim Likhachev
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
 *     * Neither the name of the University of Pennsylvania nor the names of its
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

#include <sbpl/headers.h>

#ifndef __ENVIRONMENT_NAVXYTHETADOOR_H_
#define __ENVIRONMENT_NAVXYTHETADOOR_H_


class EnvironmentNAVXYTHETADOOR : public EnvironmentNAVXYTHETALAT
{
 public:
    EnvironmentNAVXYTHETADOOR() {};
    ~EnvironmentNAVXYTHETADOOR() {};

 protected:

	virtual int GetActionCost(int SourceX, int SourceY, int SourceTheta, EnvNAVXYTHETALATAction_t* action);

    //returns possible doorangle and associated costs (costs are used as multipliers, cost = 0 will be used as a
    //a multiplication factor of 1 (no penalty). Infinite cost should be indicated by INFINITECOST, or better if not
    //returned at all as a possible doorangle.
    void GetValidDoorAngles(EnvNAVXYTHETALAT3Dpt_t worldrobotpose3D, vector<int>* doorangleV, 
                            vector<int>* dooranglecostV);

};

#endif
