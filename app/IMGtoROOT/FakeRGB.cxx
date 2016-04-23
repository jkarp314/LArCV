#ifndef __FAKERGB_CXX__
#define __FAKERGB_CXX__

#include "FakeRGB.h"

#include <ifstream>

#include "DataFormat/Image2D.h"
#include "DataFormat/EventImage2D.h"

#include "CVUtil/CVUtil.h"

namespace larcv {
  
  static FakeRGBProcessFactory __global_FakeRGBProcessFactory__;
  
  FakeRGB::FakeRGB(const std::string name)
    : ProcessBase(name)
  {}

  void FakeRGB::configure(const PSet& cfg)
  {
    _file_list          = cfg.get<std::string>( "FileList" );
    _out_image_producer = cfg.get<std::string>( "OutputImageProducer" );
  }

  void FakeRGB::initialize()
  {

    LARCV_INFO << "Loading " << _file_list;
    std::ifstream source_file(_file_list.c_str());
    
    if (source_file.is_open()) {
      std::string line;
      while (source_file >> line) {
	LARCV_INFO << "Got" << line;
	_image_v.emplace_back(line);
      }
    } else {
      LARCV_CRITICAL << "Failed to open source file: " << source;
    }
    source_file.close();
  }

  bool FakeRGB::process(IOManager& mgr)
  {

    //output
    auto out_image_v = (EventImage2D*)(mgr.get_data(kProductImage2D,_out_image_producer));
    
    for(unsigned i=0;i<_image_v.size();++i) {

      mgr.set_id(1,0,i);

      auto image2d_v = imread_v( _image_v[i] );
      
      out_image_v->Emplace(std::move(image2d_v));
    }
  
    return true;
  }
  
  void FakeRGB::finalize(TFile* ana_file)
  {   
    mgr.finalize();
  }


}
#endif
