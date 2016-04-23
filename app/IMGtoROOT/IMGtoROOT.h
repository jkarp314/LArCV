/**
 * \file IMGtoROOT.h
 *
 * \ingroup Package_Name
 * 
 * \brief Class def header for a class IMGtoROOT
 *
 * @author vgenty
 */

/** \addtogroup Package_Name

    @{*/
#ifndef __IMGTOROOT_H__
#define __IMGTOROOT_H__

#include "Processor/ProcessBase.h"
#include "Processor/ProcessFactory.h"
namespace larcv {

  /**
     \class ProcessBase
     User defined class IMGtoROOT ... these comments are used to generate
     doxygen documentation!
  */
  class IMGtoROOT : public ProcessBase {

  public:
    
    /// Default constructor
    IMGtoROOT(const std::string name="IMGtoROOT");
    
    /// Default destructor
    ~IMGtoROOT(){}

    void configure(const PSet&);

    void initialize();

    bool process(IOManager& mgr);

    void finalize(TFile* ana_file);


  private:

    std::string _file_list;
    std::string _out_image_producer;

    std::vector<std::string> _image_v;

    
  };

  /**
     \class larcv::IMGtoROOTFactory
     \brief A concrete factory class for larcv::IMGtoROOT
  */
  class IMGtoROOTProcessFactory : public ProcessFactoryBase {
  public:
    /// ctor
    IMGtoROOTProcessFactory() { ProcessFactory::get().add_factory("IMGtoROOT",this); }
    /// dtor
    ~IMGtoROOTProcessFactory() {}
    /// creation method
    ProcessBase* create(const std::string instance_name) { return new IMGtoROOT(instance_name); }
  };

}

#endif
/** @} */ // end of doxygen group 

