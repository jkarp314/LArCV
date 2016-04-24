/**
 * \file NuCropper.h
 *
 * \ingroup Package_Name
 * 
 * \brief Class def header for a class NuCropper
 *
 * @author vgenty
 */

/** \addtogroup Package_Name

    @{*/
#ifndef __NUCROPPER_H__
#define __NUCROPPER_H__

#include "Processor/ProcessBase.h"
#include "Processor/ProcessFactory.h"
namespace larcv {

  /**
     \class ProcessBase
     User defined class NuCropper ... these comments are used to generate
     doxygen documentation!
  */
  class NuCropper : public ProcessBase {

  public:
    
    /// Default constructor
    NuCropper(const std::string name="NuCropper");
    
    /// Default destructor
    ~NuCropper(){}

    void configure(const PSet&);

    void initialize();

    bool process(IOManager& mgr);

    void finalize(TFile* ana_file);


  private:

    int _plane_image;
    
    std::string _in_img_producer;
    std::string _out_img_producer;


    std::string _in_roi_producer;
    std::string _out_roi_producer;



    
  };

  /**
     \class larcv::NuCropperFactory
     \brief A concrete factory class for larcv::NuCropper
  */
  class NuCropperProcessFactory : public ProcessFactoryBase {
  public:
    /// ctor
    NuCropperProcessFactory() { ProcessFactory::get().add_factory("NuCropper",this); }
    /// dtor
    ~NuCropperProcessFactory() {}
    /// creation method
    ProcessBase* create(const std::string instance_name) { return new NuCropper(instance_name); }
  };

}

#endif
/** @} */ // end of doxygen group 

