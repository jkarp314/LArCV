#include "DataFormat/IOManager.h"
#include "DataFormat/EventImage2D.h"
#include "DataFormat/Image2D.h"
#include "DataFormat/EventROI.h"

using namespace larcv;


float BASELINE = 0;
float ADC_MIP = 20.0;
float ADC_MIN = -10.0;
float ADC_MAX = 190.0;

void getRGB( float ADC, float&r, float&g, float& b ) {
  // out of range                                                                                                                             
  if ( ADC<ADC_MIN ) {
    r = 0; g = 0; b = 1.0;
    return;
  }
  if ( ADC>ADC_MAX ) {
    r = 1.0; g = 0; b = 0;
    return;
  }

  // 0 to 1.0 MIPs: blue to green                                                                                                             
  if ( ADC < ADC_MIP ) {
    float colorlen = ADC_MIP - ADC_MIN;
    g = (ADC-ADC_MIN)/colorlen;
    b = (1 - (ADC-ADC_MIN)/colorlen);
    r = 0;
  }
  // 1.0 to 2.0 MIPs green to red                                                                                                             
  else if ( ADC>=ADC_MIP ) {
    float colorlen = ADC_MAX - ADC_MIP;
    b = 0;
    g = (ADC-ADC_MIP)/colorlen;
    r = (1.0 - (ADC-ADC_MIP)/colorlen);
  }
}


int main() {
  
  auto iom1 = IOManager(IOManager::kREAD);
  auto iom2 = IOManager(IOManager::kWRITE);

  iom1.add_in_file("supera_rcnn_test.root");
  iom2.set_out_file("supera_rcnn_test_fake.root");

  iom1.set_verbosity(msg::kDEBUG);
  iom2.set_verbosity(msg::kDEBUG);

  iom1.initialize();
  iom2.initialize();
  
  auto ne = iom1.get_n_entries();
  
  for(int entry=0; entry<ne; ++entry) {

    iom1.read_entry(entry);
    
    auto ev_image     = (EventImage2D*)iom1.get_data(kProductImage2D,"bnb_mc");
    auto ev_image_out = (EventImage2D*)iom2.get_data(kProductImage2D,"bnb_mc_fake");  
    auto ev_roi_out   = (EventROI*)iom2.get_data(kProductROI,"bnb_mc_fake");  
    
    if ( ! ev_image->Image2DArray().size() ) continue;
    
    auto& img = ev_image->Image2DArray()[0]; // plane 0
    
    int nx = 448;
    
    Image2D img0(nx,nx);
    Image2D img1(nx,nx);
    Image2D img2(nx,nx);
    
    float r,b,g;
    
    for(short i=0;i<nx;++i) {
      for(short j=0;j<nx;++j) {
	
	getRGB(img.pixel(j,i),r,g,b);
	img0.set_pixel(j,i,b*255.0);
	img1.set_pixel(j,i,g*255.0);
	img2.set_pixel(j,i,r*255.0);
	
      }
    }   
    
    iom2.set_id(1,0,entry);

    ev_image_out->Emplace(std::move(img0));
    ev_image_out->Emplace(std::move(img1));
    ev_image_out->Emplace(std::move(img2));
    
    iom2.save_entry();

  }
  
  iom1.finalize();
  iom2.finalize();
    
  return 0;

}
