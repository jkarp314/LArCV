#ifndef __IMGTOROOT_CXX__
#define __IMGTOROOT_CXX__

#include "IMGtoROOT.h"

#include <fstream>

#include "DataFormat/Image2D.h"
#include "DataFormat/EventImage2D.h"

#include "CVUtil/CVUtil.h"

namespace larcv {
  
  static IMGtoROOTProcessFactory __global_IMGtoROOTProcessFactory__;
  
  IMGtoROOT::IMGtoROOT(const std::string name)
    : ProcessBase(name)
  {}

  void IMGtoROOT::configure(const PSet& cfg)
  {
    _file_list          = cfg.get<std::string>( "FileList" );
    _out_image_producer = cfg.get<std::string>( "OutputImageProducer" );
  }

  void IMGtoROOT::initialize()
  {
    
    LARCV_DEBUG() << "Loading " << _file_list << "\n";
    std::ifstream source_file(_file_list.c_str());
    
    if (source_file.is_open()) {
      std::string line;
      while (source_file >> line) {
	LARCV_INFO() << "Got " << line << "\n";
	_image_v.emplace_back(line);
      }
    } else {
      LARCV_CRITICAL() << "Failed to open source file: " << _file_list;
    }
    source_file.close();
  }

  bool IMGtoROOT::process(IOManager& mgr)
  {

    //output
    LARCV_INFO() << "Processing " << _image_v.size() << " images\n";
    for(unsigned i=0;i<_image_v.size();++i) {
      auto out_image_v = (EventImage2D*)(mgr.get_data(kProductImage2D,_out_image_producer));    

      mgr.set_id(1,0,i);

      LARCV_DEBUG() << "Processing image " << _image_v[i] << "\n";

      auto image2d_v = imread_v( _image_v[i] );
      
      out_image_v->Emplace(std::move(image2d_v));
      
      //this is annoying, ProcessDriver calles save_entry no matter what, don't call on last image
      if ( i != _image_v.size() - 1 ) mgr.save_entry(); 
    }

    LARCV_INFO() << "Done processing\n";

    return true;
  }
  
  void IMGtoROOT::finalize(TFile* ana_file)
  {}

}
#endif
