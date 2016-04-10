/**
 * \file ChStatus.h
 *
 * \ingroup DataFormat
 * 
 * \brief Class def header for a class ChStatus
 *
 * @author kazuhiro
 */

/** \addtogroup DataFormat

    @{*/
#ifndef CHSTATUS_H
#define CHSTATUS_H

#include <iostream>
#include "DataFormatTypes.h"
namespace larcv {
  /**
     \class ChStatus
     User defined class ChStatus ... these comments are used to generate
     doxygen documentation!
  */
  class ChStatus {
    
  public:
    
    /// Default constructor
    ChStatus(){}
    
    /// Default destructor
    ~ChStatus(){}

    PlaneID_t Plane() const { return _plane; }

    void  Reset(size_t nwires, short status);

    void Status(size_t wire, short status);

    short Status(size_t wire) const;

    const std::vector<short>& as_vector() const { return _status_v; }

  private:
    std::vector<short> _status_v;
    PlaneID_t _plane;
  };
}

#endif
/** @} */ // end of doxygen group 

