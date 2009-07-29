#ifndef DESCRIPTORS_2D_H
#define DESCRIPTORS_2D_H
 
#include <iostream>
#include "opencv/cxcore.h"
#include "opencv/cv.h"
#include "opencv/highgui.h"
#include "opencv/cvaux.hpp"
#include <string>
#include <Eigen/Core>
#include <math.h>
#include <list>
#include <vector>
#include "ros/console.h"
#include "ros/assert.h"
#include <chamfer_matching/chamfer_matching.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>

namespace cv
{
    typedef KeyPoint Keypoint;
}

typedef cv::Vector< cv::Vector<float> > vvf;
#define CVSHOW(name, img) cvNamedWindow(name); cvShowImage(name, img)
  
/***************************************************************************
***********  Misc. useful classes.
****************************************************************************/


/**
 * @class Histogram
 * @brief 1D histogram class.  Deprecated.  
 */
class Histogram {
public:
  std::vector<float> bins_;
  Histogram(int nBins, float min, float max);
  int nInsertions_;
  bool insert(float val);
  void normalize();
  void print();
  void printGraph();
  void printBoundaries();
  void clear();

private:
  std::vector<float> boundaries_;
  int nBins_;
  float min_;
  float max_;
  float bin_size_;
};

  
/***************************************************************************
***********  ImageDescriptor
****************************************************************************/

/**
 * @class ImageDescriptor
 * @brief Abstract base class for all descriptors.
 */
class ImageDescriptor {
 public:
  //! Whether to visualize feature computation.
  bool debug_;

  ImageDescriptor();
  virtual ~ImageDescriptor() {};
  std::string getName();
  //! Returns result_size_.
  unsigned int getSize();
  //! Vectorized feature computation call.
  virtual void compute(IplImage* img, const cv::Vector<cv::Keypoint>& points, vvf& results) {};
  //! Clean up any data specific to computation at a point.
  virtual void clearPointCache() {}
  //! Clean up any data specific to computation on a particular image.
  virtual void clearImageCache() {}

 protected:
  //! Name of the descriptor.  Should be unique for any parameter setting.
  std::string name_;
  //! Length of the vector that results from computing the feature at a point.
  unsigned int result_size_;
  //! The image that we are computing descriptors on.
  IplImage* img_;

  //! Show the input image and a red + at the point at which the descriptor is being computed.
  void commonDebug(cv::Keypoint kp, IplImage* vis = NULL);
  //! Sets the img_ pointer and clears the image cache.
  virtual void setImage(IplImage* img);
};



/***************************************************************************
***********  SURF
****************************************************************************/

class SurfWrapper : public ImageDescriptor {
 public:
  bool extended_;
  int size_;

  SurfWrapper(bool extended = true, int size = 100);
  ~SurfWrapper();
  void compute(IplImage* img, const cv::Vector<cv::Keypoint>& points, vvf& results);
};

/***************************************************************************
***********  Hog
****************************************************************************/

/**
 * @class HogWrapper
 * @brief Histogram of oriented gradients.  Wraps the opencv descriptor.
 */
class HogWrapper : public ImageDescriptor {
 public:
  HogWrapper();
  HogWrapper(cv::Size winSize, cv::Size blockSize, cv::Size blockStride, cv::Size cellSize,
	     int nbins, int derivAperture=1, double winSigma=-1,
	     int histogramNormType=0, double L2HysThreshold=0.2, bool gammaCorrection=false); //0=L2Hys
  void compute(IplImage* img, const cv::Vector<cv::Keypoint>& points, vvf& results);
  
 protected:
  cv::HOGDescriptor hog_;
};


/***************************************************************************
***********  Integral Image-based descriptors.
****************************************************************************/

/**
 * @class IntegralImageDescriptor
 * @brief Abstract base class for descriptors that use integral images.
 */
class IntegralImageDescriptor : public ImageDescriptor {
  
 public:
  //! The integral image.
  IplImage* ii_;
  //! The 45 degree tilt integral image.
  IplImage* ii_tilt_;
  //! The grayscale image used to compute the integral images.
  IplImage* gray_;

  IntegralImageDescriptor(IntegralImageDescriptor* ii_provider);
  ~IntegralImageDescriptor();

 protected:
  IntegralImageDescriptor* ii_provider_;

  bool integrateRect(float* result, int row_offset, int col_offset, int half_height, int half_width, const cv::Keypoint& kp, float* area = NULL);
  bool integrateRect(float* result, const cv::Keypoint& kp, const CvRect& rect);
  void integrate();
  virtual void clearImageCache();
};

/**
 * @class HaarDescriptor
 * @brief Haar descriptor like those from Viola-Jones.
 */
class HaarDescriptor : public IntegralImageDescriptor {
 public:
  HaarDescriptor(cv::Vector<CvRect> rects, cv::Vector<int> weights, IntegralImageDescriptor* ii_provider = NULL);
  //! Scale of the window is determined by cv::Keypoint.
  void compute(IplImage* img, const cv::Vector<cv::Keypoint>& points, vvf& results);

 protected:
  cv::Vector<CvRect> rects_;
  //! e.g. weights_[i] == -1 if the sum of values in rects_[i] should be subtracted.
  cv::Vector<int> weights_;
};

vector<ImageDescriptor*> setupDefaultHaarDescriptors();

/**
 * @class IntegralImageTexture
 * @brief Experimental texture descriptor based on integral images.  TODO: Add more textures, make scale be determined by keypoint.
 */
class IntegralImageTexture : public IntegralImageDescriptor {
 public:
  IntegralImageTexture(int scale = 1, IntegralImageDescriptor* ii_provider = NULL);
  void compute(IplImage* img, const cv::Vector<cv::Keypoint>& points, vvf& results);
  void compute(IplImage* img, const cv::Keypoint& point, cv::Vector<float>& result);

 protected:
  int scale_;
};


/***************************************************************************
***********  Contour Fragments
****************************************************************************/

/**
 * @class ContourFragmentManager
 * @brief Class to load, save, and extract contour fragments. 
 */
class ContourFragmentManager {
 public:
  ContourFragmentManager(int num_templates_per_label = 10, bool debug = false, int min_area = 30, 
			   float min_density = 0.01, int min_side = 5, int min_edge_pix = 10, int min_edge_pix_besides_line = 5);
  ~ContourFragmentManager();
  void learnContours(std::vector<IplImage*> imgs, std::vector<IplImage*> masks);
  void saveContours(string dir);
  void loadContours(string dir);
  
  std::vector<IplImage*> contours_;
  

 private:
  int num_templates_per_label_;
  bool debug_;
  int min_area_;
  float min_density_;
  int min_side_;
  int min_edge_pix_;
  //! Min number of edge pixels AFTER removing the primary line from the template.
  int min_edge_pix_besides_line_; 

  
  bool contourTest(IplImage* cf);
};

/**
 * @class ContourFragmentDescriptor
 * @brief Descriptor based on chamfer matching of contour fragments.  Under construction.
 */
class ContourFragmentDescriptor : public ImageDescriptor {
 public:
  ContourFragmentDescriptor* chamfer_provider_;
  
  //! Loads contours from a dir.
  ContourFragmentDescriptor(int cf_id, string dir);
  //! Uses another ContourFragmentDescriptor object to get its data.  
  ContourFragmentDescriptor(int cf_id, ContourFragmentDescriptor* chamfer_provider);
  void compute(IplImage* img, const cv::Vector<cv::Keypoint>& points, vvf& results);

 private:
  int cf_id_;
  ContourFragmentManager cfc_;
  ChamferMatching* chamfer_;
  ChamferMatch* matches_;
};



/***************************************************************************
***********  Superpixel Statistic
****************************************************************************/

/**
 * @class SuperpixelStatistic
 * @brief Abstract base class for all descriptors based on superpixels.
 */
class SuperpixelStatistic : public ImageDescriptor {
 public:
  SuperpixelStatistic(int seed_spacing, float scale, SuperpixelStatistic* provider);
  ~SuperpixelStatistic();
  void segment(IplImage* img);
  void clearImageCache();
  int getSeedSpacing();
  float getScale();
  SuperpixelStatistic* getSegProvider();
  IplImage* getSegmentation();
  std::vector< std::vector<CvPoint> >* getIndex();

 protected:
  //! (*index_)[i] returns the vector of CvPoints for segment i of the image.
  std::vector< std::vector<CvPoint> > *index_;
  //! The segmentation.
  IplImage* seg_;
  //! Number of pixels between each seed.
  int seed_spacing_;
  //! Scaling factor to apply to the image when computing the segmentation.
  float scale_;
  //! Pointer to an object from which the segmentation can be gotten.  
  SuperpixelStatistic* seg_provider_;  

  //! Computes superpixels and puts into seg_, and computes the superpixel to pixel index.  Is called automatically, if necessary, by the compute(.) function.
  void segment();
  IplImage* createSegmentMask(int label, CvRect* rect);

};


/**
 * @class SuperpixelColorHistogram
 * @brief Descriptor that segments the image, then computes a histogram of hue and saturation values for each segment.
 */
class SuperpixelColorHistogram : public SuperpixelStatistic {
 public:
  std::vector<CvHistogram*> histograms_cv_;
  IplImage* hsv_;
  IplImage* hue_;
  IplImage* sat_;
  IplImage* val_;
  IplImage* channel_;

  SuperpixelColorHistogram(int seed_spacing, float scale, int nBins, std::string type, SuperpixelStatistic* seg_provider=NULL, SuperpixelColorHistogram* hsv_provider_=NULL);
  ~SuperpixelColorHistogram();
  void compute(IplImage* img, const cv::Vector<cv::Keypoint>& points, vvf& results);

 protected:
  int nBins_;
  //! Not used right now.
  std::string type_;
  SuperpixelColorHistogram* hsv_provider_;
  float max_val_;
  bool hists_reserved_;

  void computeHistogram(int label);
  void computeHistogramCV(int label); 
  void compute(IplImage* img, const cv::Keypoint& point, cv::Vector<float>& result);
  void clearImageCache();
};

std::vector<ImageDescriptor*> setupImageDescriptors();
void whiten(Eigen::MatrixXf* m);
int getdir (string dir, vector<string> &files);


/***************************************************************************
***********  Patch-based.  
****************************************************************************/
//Not converted to standard yet.  Only will be if wanted.

/* class Patch : public ImageDescriptor { */
/*  public: */
/*   //! Length of the sides of the patch, before scaling. */
/*   int raw_size_; */
/*   //! Scaling to apply to the raw patch before computing the feature. */
/*   float scale_; */
/*   //! The final patch, made available for other descriptors to re-use computation. */
/*   IplImage* final_patch_; */
/*   //! Length of the sides of the patch, after scaling. */
/*   int size_; */
/*   //! The scaled color image patch. */
/*   IplImage* scaled_patch_; */


/*   Patch(int raw_size, float scale); */
/*   //! Common patch constructor computation. */
/*   bool preCompute(); */

/*   void clearPointCache(); */
/*   ~Patch() {}   */
/* }; */

/* class IntensityPatch : public Patch { */
/*  public: */
/*   //! If true, the resulting feature vector has its mean set to 0 and variance set to 1.  This might give some lighting invariance for intensity patches, for example. */
/*   bool whiten_; */

/*   IntensityPatch(int raw_size, float scale, bool whiten); */
/*   bool compute(Eigen::MatrixXf** result); */
/*   void compute(IplImage* img, const cv::Vector<cv::Keypoint>& points, vvf& results); */
/*   void clearImageCache() {} */
/* }; */

/* class PatchStatistic : public ImageDescriptor { */
/*  public: */
/*   //! "variance" */
/*   std::string type_; */
/*   //! Pointer to Patch object which will contain the final_patch_ to compute the statistic on. */
/*   Patch* patch_; */

/*   PatchStatistic(std::string type, Patch* patch); */
/*   bool compute(Eigen::MatrixXf** result); */

/*   void clearPointCache() {} */
/*   void clearImageCache() {} */
/* }; */

#endif