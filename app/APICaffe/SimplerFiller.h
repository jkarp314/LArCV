/**
 * \file SimpleFiller.h
 *
 * \ingroup Package_Name
 * 
 * \brief Class def header for a class SimpleFiller
 *
 * @author kazuhiro
 */

/** \addtogroup Package_Name

    @{*/
#ifndef __SIMPLERFILLER_H__
#define __SIMPLERFILLER_H__

#include "Processor/ProcessFactory.h"
#include "DatumFillerBase.h"
namespace larcv {

  /**
     \class ProcessBase
     User defined class SimplerFiller ... these comments are used to generate
     doxygen documentation!
  */
  class SimplerFiller : public DatumFillerBase {

  public:
    
    /// Default constructor
    SimplerFiller(const std::string name="SimplerFiller");
    
    /// Default destructor
    ~SimplerFiller(){}

    void child_configure(const PSet&);

    void child_initialize();

    void child_batch_begin();

    void child_batch_end();

    void child_finalize();

    void set_dimension(const std::vector<larcv::Image2D>&);

    void fill_entry_data(const std::vector<larcv::Image2D>&,const std::vector<larcv::ROI>&);

  private:
    void assert_dimension(const std::vector<larcv::Image2D>&);

    std::vector<size_t> _slice_v;
    size_t _max_ch;
    std::vector<float> _max_adc_v;
    std::vector<float> _min_adc_v;
    std::vector<size_t> _caffe_idx_to_img_idx;
    std::vector<size_t> _roitype_to_class;
    double _adc_gaus_mean;
    double _adc_gaus_sigma;
    bool _adc_gaus_pixelwise;
  };

  /**
     \class larcv::SimplerFillerFactory
     \brief A concrete factory class for larcv::SimplerFiller
  */
  class SimplerFillerProcessFactory : public ProcessFactoryBase {
  public:
    /// ctor
    SimplerFillerProcessFactory() { ProcessFactory::get().add_factory("SimplerFiller",this); }
    /// dtor
    ~SimplerFillerProcessFactory() {}
    /// creation method
    ProcessBase* create(const std::string instance_name) { return new SimplerFiller(instance_name); }
  };

}

#endif
/** @} */ // end of doxygen group 

