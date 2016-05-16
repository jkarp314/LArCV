#ifndef __FAKERGB_CXX__
#define __FAKERGB_CXX__

#include "FakeRGB.h"

#include "DataFormat/Image2D.h"

#include "DataFormat/EventImage2D.h"
#include "DataFormat/EventROI.h"

namespace larcv {
  
  static FakeRGBProcessFactory __global_FakeRGBProcessFactory__;
  
  FakeRGB::FakeRGB(const std::string name)
    : ProcessBase(name)
  {}

  void FakeRGB::configure(const PSet& cfg)
  {
    _baseline = cfg.get<float> ( "ADC_Baseline" );
    _adc_mip  = cfg.get<float> ( "ADC_MIP" );
    _adc_min  = cfg.get<float> ( "ADC_MIN" );
    _adc_max  = cfg.get<float> ( "ADC_MAX" );

    _imin = cfg.get<float> ("iMin");
    _imax = cfg.get<float> ("iMax");
     
    _plane_image = cfg.get<size_t>( "Plane" );

    _in_img_producer  = cfg.get<std::string>( "InputImageProducer" );
    _out_img_producer = cfg.get<std::string>( "OutputImageProducer" );

    _in_roi_producer  = cfg.get<std::string>( "InputROIProducer" );
    _out_roi_producer = cfg.get<std::string>( "OutputROIProducer" );
  }

  void FakeRGB::initialize()
  {}

  bool FakeRGB::process(IOManager& mgr)
  {

    //input
    auto tpc_event_image_v = (EventImage2D*)(mgr.get_data(kProductImage2D,_in_img_producer));
    auto tpc_event_roi_v = (EventROI*)(mgr.get_data(kProductROI,_in_roi_producer));

    //output
    auto out_image_v = (EventImage2D*)(mgr.get_data(kProductImage2D,_out_img_producer));
    auto out_roi_v   = (EventROI*)(mgr.get_data(kProductROI,_out_roi_producer));
    
    // let's take just the Y plane
    const auto& plane_img = tpc_event_image_v->Image2DArray()[ _plane_image ];
    const auto& plane_meta = plane_img.meta();

    //allocate the space for 3 fake planes
    std::vector<Image2D> fake_color;
    fake_color.reserve(3);
    
    for(unsigned i=0; i<3; ++i) {
      ImageMeta im(plane_meta.width(), plane_meta.height(),
		   plane_meta.rows() , plane_meta.cols(),
		   plane_meta.tl().x , plane_meta.tl().y,
		   i);
      Image2D p(im);
      fake_color.emplace_back(std::move(p));
    }
    
    for(unsigned i = 0; i < plane_meta.rows(); ++i) {
      for(unsigned j = 0; j < plane_meta.cols(); ++j) {
	
	float r,g,b;
	
	auto px = plane_img.pixel(i,j);
	px -= _imin;
	
	if ( px < 0 )     px = 0;	
	if ( px > _imax ) px = _imax;

	
	color(px,r,g,b);

	r *= 255.0;
	g *= 255.0;
	b *= 255.0;
			
	
	fake_color[0].set_pixel(i,j,b);
	fake_color[1].set_pixel(i,j,g);
	fake_color[2].set_pixel(i,j,r);
	
      }
    }
    
    out_image_v->Emplace(std::move(fake_color));
    
    //have to make a copy right
    out_roi_v->Set(tpc_event_roi_v->ROIArray());

    return true;
  }
  
  void FakeRGB::finalize() {}

  //thanks kazu
  void FakeRGB::color( float ADC, float&r, float&g, float& b ) {

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

}
#endif
