#ifndef __JPEGDump_CXX__
#define __JPEGDump_CXX__

#include "JPEGDump.h"

#include "DataFormat/EventImage2D.h"
#include "DataFormat/EventROI.h"

#include "DataFormat/UtilFunc.h"

#include "opencv/cv.h"
#include "opencv2/opencv.hpp"

#include <fstream>

namespace larcv {

  static JPEGDumpProcessFactory __global_JPEGDumpProcessFactory__;

  JPEGDump::JPEGDump(const std::string name)
    : ProcessBase(name)
  {}
    
  void JPEGDump::configure(const PSet& cfg)
  {

    _baseline = cfg.get<float> ( "ADC_Baseline" );
    _adc_mip  = cfg.get<float> ( "ADC_MIP" );
    _adc_min  = cfg.get<float> ( "ADC_MIN" );
    _adc_max  = cfg.get<float> ( "ADC_MAX" );

    _imin = cfg.get<float> ("iMin");
    _imax = cfg.get<float> ("iMax");
     
    _plane_image = cfg.get<size_t>( "Plane" );

    _in_img_producer  = cfg.get<std::string>( "InputImageProducer" );
    _in_roi_producer  = cfg.get<std::string>( "InputROIProducer" );

  }


  void JPEGDump::initialize()
  {
    _pars["Eminus"] = 0;
    _pars["Proton"] = 0;
    _pars["Piminus"] = 0;
    _pars["Gamms"] = 0;
    _pars["Muminus"] = 0;
  }

  bool JPEGDump::process(IOManager& mgr)
  {

      auto hiresimages = (larcv::EventImage2D*)(mgr.get_data(kProductImage2D,_in_img_producer));
      auto hiresrois   = (larcv::EventROI*)(mgr.get_data(kProductROI,_in_roi_producer));
      
      ROIType_t roi_type = kROICosmic;
      ROI roi_v;
      for(auto const& roi : hiresrois->ROIArray()) {
	if(roi.MCSTIndex() != kINVALID_USHORT) continue;
	roi_type = roi.Type();
	roi_v = roi;
	if(roi_type == kROIUnknown) roi_type = PDG2ROIType(roi.PdgCode());
	break;
      }

      auto particle = ROIType2String(roi_type);

      if (_pars.find(particle) == _pars.end()) 
	{ std::cout << particle << std::endl; return false; }

      std::stringstream oss;
      oss << "/stage/vgenty/Singledevkit7/Annotations/" << particle << "_" << _pars[particle] << ".txt";
      
      std::stringstream pss;

      std::ofstream ofile;
      ofile.open(oss.str());
      
      larcv::Image2D const& hiresimage = hiresimages->at(_plane_image);
      auto& imm = hiresimage.meta();
        
      auto& bbox = roi_v.BB(_plane_image);

      auto x = bbox.bl().x - imm.bl().x;
      auto y = bbox.bl().y - imm.bl().y;
        
      auto dw_i = imm.cols() / ( imm.tr().x - imm.bl().x );
      auto dh_i = imm.rows() / ( imm.tr().y - imm.bl().y );
        
      auto w_b = bbox.tr().x - bbox.bl().x;
      auto h_b = bbox.tr().y - bbox.bl().y;

      auto x1 = x*dw_i;
      auto y1 = y*dh_i;

      auto x2 = x1 + w_b*dw_i;
      auto y2 = y1 + h_b*dh_i;
      
      y1 = 576 - y1;
      y2 = 576 - y2;

      pss << particle << " " << x1 << " " << y1 << " " << x2 << " " << y2 << "\n";
      
      ofile << pss.str();
      ofile.close();      
      
      cv::Mat outimg;
      //larcv::Image2D const& hiresimage = hiresimages->at(_plane_image);
      outimg = cv::Mat::zeros( hiresimage.meta().rows(), hiresimage.meta().cols(), CV_8UC3 );

      for (size_t r=0; r<hiresimage.meta().rows(); r++) {
      	for (size_t c=0; c<hiresimage.meta().cols(); c++) {

      	  float adc = hiresimage.pixel( r, c );

      	  float rr,gg,bb;
	  
      	  adc -= _imin;

      	  if ( adc < 0 )     adc = 0;	
      	  if ( adc > _imax ) adc = _imax;
	  
      	  color(adc,rr,gg,bb);
	  
      	  rr *= 255.0;
      	  gg *= 255.0;
      	  bb *= 255.0;

      	  // if ( adc != 0 )
      	  //   std::cout << "adc: " << adc 
      	  // 	      << " b: " << bb 
      	  // 	      << " g: " << gg 
      	  // 	      << " r: " << rr 
      	  // 	      << " r : " << r
      	  // 	      << " c : " << c 
      	  // 	      << "\n";

	  
      	  outimg.at< cv::Vec3b >(r,c)[0] = (int) bb;
      	  outimg.at< cv::Vec3b >(r,c)[1] = (int) gg;
      	  outimg.at< cv::Vec3b >(r,c)[2] = (int) rr;
      	}	  
      }

      
      std::stringstream ss;
      ss << "/stage/vgenty/Singledevkit7/JPEGImages/" << particle << "_" << _pars[particle] << ".JPEG";
      cv::imwrite( ss.str(), outimg );
      _pars[particle]++;      
      return true;
      
  }


  void JPEGDump::color( float ADC, float&r, float&g, float& b ) {
    // out of range                                                                                                             

    if ( ADC<_adc_min ) {
      r = 0; g = 0; b = 1.0;
      return;
    }
    if ( ADC>_adc_max ) {
      r = 1.0; g = 0; b = 0;
      return;
    }
    
    // 0 to 1.0 MIPs: blue to green                                                                                                             
    if ( ADC < _adc_mip ) {
      float colorlen = _adc_mip - _adc_min;
      g = (ADC-_adc_min)/colorlen;
      b = (1 - (ADC-_adc_min)/colorlen);
      r = 0;
    }

    // 1.0 to 2.0 MIPs green to red                                                                                                             
    else if ( ADC>=_adc_mip ) {
      float colorlen = _adc_max - _adc_mip;
      b = 0;
      g = (ADC-_adc_mip)/colorlen;
      r = (1.0 - (ADC-_adc_mip)/colorlen);
    }
  }


  void JPEGDump::finalize() {
  
    for(auto& aho : _pars)
      std::cout << aho.first << " : " << aho.second << "\n";
    
  }



}
#endif
