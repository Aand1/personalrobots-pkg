#include <genericControllers/Controller.h>

using namespace CONTROLLER;

//Static function for angle conversion
static double Controller::ModNPi2Pi(double angle)
{
   double theta = angle - ((int)(angle/(2*M_PI))*2*M_PI);
   //double theta = fmod(angle,2*M_PI);
   double result = theta;

   if (theta > M_PI) 
      result = theta - 2*M_PI;
   if(theta < -M_PI)
      result = theta + 2*M_PI;

   return result;
}

Controller::Controller()
{
}

Controller::~Controller()
{
}

//Intended to be overwritten by child classes
virtual void Controller::Update(){

}
