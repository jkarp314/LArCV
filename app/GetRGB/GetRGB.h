/**
 * \file GetRGB.h
 *
 * \ingroup Package_Name
 * 
 * \brief Class def header for a class GetRGB
 *
 * @author vgenty
 */

/** \addtogroup Package_Name

    @{*/
#ifndef __GETRGB_H__
#define __GETRGB_H__

#include "Processor/ProcessBase.h"
#include "Processor/ProcessFactory.h"
namespace larcv {

  /**
     \class ProcessBase
     User defined class GetRGB ... these comments are used to generate
     doxygen documentation!
  */
  class GetRGB : public ProcessBase {

  public:
    
    /// Default constructor
    GetRGB(const std::string name="GetRGB");
    
    /// Default destructor
    ~GetRGB(){}

    void configure(const PSet&);

    void initialize();

    bool process(IOManager& mgr);

    void finalize(TFile* ana_file);


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



    
  };

  /**
     \class larcv::GetRGBFactory
     \brief A concrete factory class for larcv::GetRGB
  */
  class GetRGBProcessFactory : public ProcessFactoryBase {
  public:
    /// ctor
    GetRGBProcessFactory() { ProcessFactory::get().add_factory("GetRGB",this); }
    /// dtor
    ~GetRGBProcessFactory() {}
    /// creation method
    ProcessBase* create(const std::string instance_name) { return new GetRGB(instance_name); }
  };

}

#endif
/** @} */ // end of doxygen group 

