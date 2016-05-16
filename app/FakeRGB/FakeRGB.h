/**
 * \file FakeRGB.h
 *
 * \ingroup Package_Name
 * 
 * \brief Class def header for a class FakeRGB
 *
 * @author vgenty
 */

/** \addtogroup Package_Name

    @{*/
#ifndef __FAKERGB_H__
#define __FAKERGB_H__

#include "Processor/ProcessBase.h"
#include "Processor/ProcessFactory.h"
namespace larcv {

  /**
     \class ProcessBase
     User defined class FakeRGB ... these comments are used to generate
     doxygen documentation!
  */
  class FakeRGB : public ProcessBase {

  public:
    
    /// Default constructor
    FakeRGB(const std::string name="FakeRGB");
    
    /// Default destructor
    ~FakeRGB(){}

    void configure(const PSet&);

    void initialize();

    bool process(IOManager& mgr);

    void finalize();


  private:
    void color( float ADC, float&r, float&g, float& b );

    float _baseline;
    float _adc_mip;
    float _adc_min;
    float _adc_max;

    float _imin;
    float _imax;
     
    size_t _plane_image;

    std::string _in_img_producer;
    std::string _out_img_producer;


    std::string _in_roi_producer;
    std::string _out_roi_producer;



    
  };

  /**
     \class larcv::FakeRGBFactory
     \brief A concrete factory class for larcv::FakeRGB
  */
  class FakeRGBProcessFactory : public ProcessFactoryBase {
  public:
    /// ctor
    FakeRGBProcessFactory() { ProcessFactory::get().add_factory("FakeRGB",this); }
    /// dtor
    ~FakeRGBProcessFactory() {}
    /// creation method
    ProcessBase* create(const std::string instance_name) { return new FakeRGB(instance_name); }
  };

}

#endif
/** @} */ // end of doxygen group 

