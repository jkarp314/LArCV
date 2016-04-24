#ifndef __NUCROPPER_CXX__
#define __NUCROPPER_CXX__

#include "NuCropper.h"

#include "DataFormat/Image2D.h"

#include "DataFormat/EventImage2D.h"
#include "DataFormat/EventROI.h"

namespace larcv {
  
  static NuCropperProcessFactory __global_NuCropperProcessFactory__;
  
  NuCropper::NuCropper(const std::string name)
    : ProcessBase(name)
  {}

  void NuCropper::configure(const PSet& cfg)
  {

    _plane_image = cfg.get<int> ("Plane");
    
    _in_img_producer  = cfg.get<std::string>( "InputImageProducer" );
    _out_img_producer = cfg.get<std::string>( "OutputImageProducer" );

    _in_roi_producer  = cfg.get<std::string>( "InputROIProducer" );
    _out_roi_producer = cfg.get<std::string>( "OutputROIProducer" );
  }

  void NuCropper::initialize()
  {}

  bool NuCropper::process(IOManager& mgr)
  {

    //input
    auto tpc_event_image_v = (EventImage2D*)(mgr.get_data(kProductImage2D,_in_img_producer));
    auto tpc_event_roi_v   = (EventROI*)(mgr.get_data(kProductROI,_in_roi_producer));

    //output
    auto out_image_v = (EventImage2D*)(mgr.get_data(kProductImage2D,_out_img_producer));
    auto out_roi_v   = (EventROI*)(mgr.get_data(kProductROI,_out_roi_producer));
    
    // let's take just the Y plane again
    const auto& plane_img   = tpc_event_image_v->Image2DArray()[ _plane_image ];
    const auto& img_meta    = plane_img.meta();

    // get the Y-plane ROI
    
    if ( tpc_event_roi_v->ROIArray().size() == 0 )
      { LARCV_NORMAL() << "No neutrino here\n"; return false;}
    
    const auto& ROI       = tpc_event_roi_v->ROIArray()[ 0 ];
    const auto& plane_bb  = ROI.BB( _plane_image );

    LARCV_DEBUG() << "image meta dump\n";
    LARCV_DEBUG() << img_meta.dump();
    LARCV_DEBUG() << "plane meta dump\n";
    LARCV_DEBUG() << plane_bb.dump();

    //sometimes bbox is out of bounds... fix that?

    auto x1 = plane_bb.min_x();
    auto y1 = plane_bb.min_y();
    auto x2 = plane_bb.max_x();
    auto y2 = plane_bb.max_y();
    
    if ( x1 < img_meta.min_x() ) x1 = img_meta.min_x();
    if ( x1 > img_meta.max_x() ) x1 = img_meta.max_x();

    if ( y1 < img_meta.min_y() ) y1 = img_meta.min_y();
    if ( y1 > img_meta.max_y() ) y1 = img_meta.max_y();

    if ( x2 < img_meta.min_x() ) x2 = img_meta.min_x();
    if ( x2 > img_meta.max_x() ) x2 = img_meta.max_x();

    if ( y2 < img_meta.min_y() ) y2 = img_meta.min_y();
    if ( y2 > img_meta.max_y() ) y2 = img_meta.max_y();


    auto row_shrink = y2 == img_meta.max_y() ? (plane_bb.max_y() - img_meta.max_y()) / img_meta.pixel_height() : 0;
    auto col_shrink = x2 == img_meta.max_x() ? (plane_bb.max_x() - img_meta.max_x()) / img_meta.pixel_width()  : 0;


    if ( row_shrink > 0 ) return false;
    if ( col_shrink > 0 ) return false;
    
    LARCV_DEBUG() << "row_shrink " << row_shrink << " col_shrink: " << col_shrink << "\n";
    ImageMeta cropped_bb(x2-x1,y2-y1,
			 plane_bb.rows() - row_shrink,
			 plane_bb.cols() - col_shrink,
			 x1,y1,
			 _plane_image);
    
    LARCV_DEBUG() << "cropped meta dump\n";
    LARCV_DEBUG() << cropped_bb.dump();
    //crop it

    LARCV_DEBUG() << "Cropping\n";
    auto cropped_img = plane_img.crop( plane_bb );

    //    ROI cropped_roi
    
    out_image_v->Emplace(std::move(cropped_img));
    //out_roi_v->Emplace(std::move(cropped_roi));

    LARCV_INFO() << "Cropped\n";
    
    return true;
  }
  
  void NuCropper::finalize(TFile* ana_file)
  {}

}
#endif



    // //ROI is full resolution so I have to shrink it, right?
    
    // auto x1 = plane_bb.min_x() - img_meta.min_x();
    // auto y1 = plane_bb.min_y() - img_meta.min_y();

    // auto dw_i = img_meta.cols() / ( img_meta.max_x() - img_meta.min_x() );
    // auto dh_i = img_meta.rows() / ( img_meta.max_y() - img_meta.min_y() );
		  
    // auto w_b = plane_bb.max_x() - plane_bb.min_x();
    // auto h_b = plane_bb.max_y() - plane_bb.min_y();

    // x1 *= dw_i;
    // y1 *= dh_i;

    // auto x2 = x1 + w_b * dw_i;
    // auto y2 = y1 + h_b * dh_i;
    
    // //make sure to clip it to fit inside the compressed image
    // if ( x1 < img_meta.min_x() ) x1 = img_meta.min_x();
    // if ( x1 > img_meta.max_x() ) x1 = img_meta.max_x();

    // if ( y1 < img_meta.min_y() ) y1 = img_meta.min_y();
    // if ( y1 > img_meta.max_y() ) y1 = img_meta.max_y();

    // if ( x2 < img_meta.min_x() ) x2 = img_meta.min_x();
    // if ( x2 > img_meta.max_x() ) x2 = img_meta.max_x();

    // if ( y2 < img_meta.min_y() ) y2 = img_meta.min_y();
    // if ( y2 > img_meta.max_y() ) y2 = img_meta.max_y();
    

    // ImageMeta cropped_meta(x2 - x1 , y2 - y1,
    // 			   x2 - x1 , y2 - y1,
    // 			   x1      ,      y1,
    // 			   _plane_image);
