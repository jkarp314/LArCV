#ifndef __GETRGB_CXX__
#define __GETRGB_CXX__

#include "GetRGB.h"
#include "DataFormat/Image2D.h"
#include "DataFormat/EventImage2D.h"

namespace larcv {

  static GetRGBProcessFactory __global_GetRGBProcessFactory__;

  GetRGB::GetRGB(const std::string name)
    : ProcessBase(name)
  {}

  void GetRGB::configure(const PSet& cfg)
  {
    _baseline = cfg.get<float> ( "ADC_Baseline" );
    _adc_mip  = cfg.get<float> ( "ADC_MIP" );
    _adc_min  = cfg.get<float> ( "ADC_MIN" );
    _adc_max  = cfg.get<float> ( "ADC_MAX" );

    _imin = cfg.get<float> ("iMin");
    _imax = cfg.get<float> ("iMin");
     
    _plane_image = cfg.get<size_t>( "Plane" );

    _in_img_producer  = cfg.get<std::string>( "InputImageProducer" );
    _out_img_producer = cfg.get<std::string>( "OutputImageProducer" );
  }

  void GetRGB::initialize()
  {}

  bool GetRGB::process(IOManager& mgr)
  {

    //input
    auto tpc_event_image_v = (EventImage2D*)(mgr.get_data(kProductImage2D,_in_img_producer));
    if(!tpc_event_image_v || tpc_event_image_v->Image2DArray().empty()) return false;

    //output
    auto out_image_v = (EventImage2D*)(mgr.get_data(kProductImage2D,_out_img_producer));
    
    // let's take just the Y plane
    const auto& plane_img = tpc_event_image_v->Image2DArray()[ _plane_image ];
    const auto& plane_meta = plane_img.meta();
    //allocate the space for 3 fake planes
    std::vector<Image2D> fake_color;
    fake_color.reserve(3);

    for(unsigned i=0; i<3; ++i) {
	Image2D p(plane_meta.rows(),plane_meta.cols());
	fake_color.emplace_back(std::move(p));
      }
    
    for(unsigned i = 0; i < plane_meta.rows(); ++i) {
      for(unsigned j = 0; j < plane_meta.cols(); ++j) {
	float r,g,b;

	auto px = plane_img.pixel(i,j);
	
	if ( px > 400) px = 400;
	if ( px < 5  ) px = 0;
	
	color(px,r,g,b);

	r *= 255;
	g *= 255;
	b *= 255;
			
	
	fake_color[0].set_pixel(i,j,b);
	fake_color[1].set_pixel(i,j,g);
	fake_color[2].set_pixel(i,j,r);
      }
    }

    out_image_v->Emplace(std::move(fake_color));

    return true;
  }

  void GetRGB::finalize(TFile* ana_file)
  {}


  //thanks kazu
  void GetRGB::color( float ADC, float&r, float&g, float& b ) {

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
