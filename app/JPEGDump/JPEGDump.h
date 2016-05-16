/**
 * \file DumpHiResCropImages.h
 *
 * \ingroup Package_Name
 * 
 * \brief Class def header for a class DumpHiResCropImages
 *
 * @author taritree
 */

/** \addtogroup Package_Name

    @{*/
#ifndef __JPEGDump_H__
#define __JPEGDump_H__

#include "Processor/ProcessBase.h"
#include "Processor/ProcessFactory.h"

namespace larcv {

  /**
     \class ProcessBase
     User defined class JPEGDump ... these comments are used to generate
     doxygen documentation!
  */
  class JPEGDump : public ProcessBase {

  public:
    
    /// Default constructor
    JPEGDump(const std::string name="JPEGDump");
    
    /// Default destructor
    ~JPEGDump(){}

    void configure(const PSet&);

    void initialize();

    bool process(IOManager& mgr);

    void finalize();

  private:
    std::map<std::string,int> _pars;

    void color( float ADC, float&r, float&g, float& b );

    float _baseline;
    float _adc_mip;
    float _adc_min;
    float _adc_max;

    float _imin;
    float _imax;
     
    size_t _plane_image;

    std::string _in_img_producer;
    std::string _in_roi_producer;

    
  };

  /**
     \class larcv::JPEGDumpFactory
     \brief A concrete factory class for larcv::JPEGDump
  */
  class JPEGDumpProcessFactory : public ProcessFactoryBase {
  public:
    /// ctor
    JPEGDumpProcessFactory() { ProcessFactory::get().add_factory("JPEGDump",this); }
    /// dtor
    ~JPEGDumpProcessFactory() {}
    /// creation method
    ProcessBase* create(const std::string instance_name) { return new JPEGDump(instance_name); }
  };

}

#endif
/** @} */ // end of doxygen group 

