#ifndef __NUCROPPER_CXX__
#define __NUCROPPER_CXX__

#include "NuCropper.h"

#include "DataFormat/Image2D.h"
#include "DataFormat/EventImage2D.h"

#include "DataFormat/ROI.h"
#include "DataFormat/EventROI.h"



namespace larcv {
  
  static NuCropperProcessFactory __global_NuCropperProcessFactory__;
  
  NuCropper::NuCropper(const std::string name)
    : ProcessBase(name)
    , _gen(_rd())
  
  {}

  void NuCropper::configure(const PSet& cfg)
  {

    _plane = cfg.get<int> ("Plane");
    
    _in_img_producer  = cfg.get<std::string>( "InputImageProducer" );
    _out_img_producer = cfg.get<std::string>( "OutputImageProducer" );

    _in_roi_producer  = cfg.get<std::string>( "InputROIProducer" );
    _out_roi_producer = cfg.get<std::string>( "OutputROIProducer" );

    _crop_size = cfg.get<int>( "CropSize" );
  }

  void NuCropper::initialize()
  {

    std::uniform_real_distribution<> dis(1, 2);
    for (int n = 0; n < 10; ++n) {
      std::cout << dis(_gen) << ' ';
    }



  }

  bool NuCropper::process(IOManager& mgr)
  {

    //input
    auto input_image_v = (EventImage2D*)(mgr.get_data(kProductImage2D,_in_img_producer));
    auto input_roi_v   = (EventROI*)(mgr.get_data(kProductROI,_in_roi_producer));

    //output
    auto out_image_v = (EventImage2D*)(mgr.get_data(kProductImage2D,_out_img_producer));
    auto out_roi_v   = (EventROI*)(mgr.get_data(kProductROI,_out_roi_producer));
    
    // let's take just the Y plane again
    const auto& plane_img   = input_image_v->Image2DArray()[ _plane ];
    const auto& plane_meta  = plane_img.meta();

    // get the Y-plane ROI
    if ( input_roi_v->ROIArray().size() == 0 ) {

      auto random_cropped_meta = random_crop(_crop_size,plane_meta);
      
      auto cropped_img = plane_img.crop( random_cropped_meta );

      out_image_v->Append(cropped_img);

      std::vector<ROI> roi_v;
      out_roi_v->Emplace(std::move(roi_v));

      return true;
    }

    const auto& ROI_p     = input_roi_v->ROIArray()[ 0 ];
    const auto& plane_bb  = ROI_p.BB( _plane );

    LARCV_DEBUG() << "image meta dump\n";
    LARCV_DEBUG() << plane_meta.dump();
    LARCV_DEBUG() << "plane meta dump\n";
    LARCV_DEBUG() << plane_bb.dump();

    // Sometimes bbox is out of bounds... fix that? forget it

    if ( plane_bb.min_x() > plane_meta.max_x() ) return false;
    if ( plane_bb.min_y() > plane_meta.max_y() ) return false;
	
    auto cropped_bb = plane_bb.overlap(plane_meta);

    LARCV_DEBUG() << "cropped meta dump\n";
    LARCV_DEBUG() << cropped_bb.dump();
    
    //max width and height
    auto dx = _crop_size * plane_meta.pixel_width();
    auto dy = _crop_size * plane_meta.pixel_height();
    LARCV_DEBUG() << " dx: " << dx << " dy: " << dy << "\n";

    //get the center
    auto cx = cropped_bb.min_x() + ( cropped_bb.max_x() - cropped_bb.min_x() ) / 2.0;
    auto cy = cropped_bb.min_y() + ( cropped_bb.max_y() - cropped_bb.min_y() ) / 2.0;
    LARCV_DEBUG() << "cx: " << cx << " cy: " << cy << "\n";
    
    auto minx = cx - dx / 2.0 > plane_meta.min_x() ? cx - dx / 2.0 : plane_meta.min_x();
    auto maxy = cy + dy / 2.0 < plane_meta.max_y() ? cy + dy / 2.0 : plane_meta.max_y();
    LARCV_DEBUG() << "minx : " << minx << " maxy: " << maxy << "\n";

    //make sure we aren't fucked on the other side
    minx = minx + dx > plane_meta.max_x() ?  plane_meta.max_x() - dx  : minx;
    maxy = maxy - dy < plane_meta.min_y() ?  plane_meta.min_y() + dy  : maxy;
    
    LARCV_DEBUG() << "minx : " << minx << " maxy: " << maxy << "\n";    
    ImageMeta square_bb(dx,
			dy,
			_crop_size,
			_crop_size,
			minx,
			maxy,
			plane_meta.plane());
    
    LARCV_DEBUG() << square_bb.dump();
    //crop it
    LARCV_DEBUG() << "Cropping\n";
    auto cropped_img = plane_img.crop( square_bb );

    ROI cropped_roi(kROIBNB,kShapeUnknown);
    cropped_roi.AppendBB( square_bb );
    
    out_image_v->Emplace(std::move(cropped_img));
    out_roi_v->Emplace(std::move(cropped_roi));

    LARCV_INFO() << "Cropped\n";
    
    return true;
  }
  
  void NuCropper::finalize(TFile* ana_file)
  {}


  ImageMeta NuCropper::random_crop(int crop_size, const ImageMeta& img_meta) {

      //lets take a random crop
    std::uniform_real_distribution<> dis1(img_meta.min_x(), img_meta.max_x() - crop_size * img_meta.pixel_width());
    
    //give me another number
    std::uniform_real_distribution<> dis2(crop_size * img_meta.pixel_height(),img_meta.max_y());

    auto x1 = dis1(_gen);
    auto y1 = dis2(_gen);

    LARCV_DEBUG() << "x1: " << x1 << " y1: " << y1 << " img_meta.pixel_width() " << img_meta.pixel_width() << "\n";
      
    return ImageMeta(crop_size * img_meta.pixel_width(),
		     crop_size * img_meta.pixel_height(),
		     crop_size,
		     crop_size,
		     x1,
		     y1,
		     img_meta.plane());
      
  }

  
}
#endif
