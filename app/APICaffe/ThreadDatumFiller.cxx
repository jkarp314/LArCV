#ifndef THREADDATUMFILLER_CXX
#define THREADDATUMFILLER_CXX

#include "ThreadDatumFiller.h"
#include "Base/LArCVBaseUtilFunc.h"
#include <random>
#include <sstream>
#include <unistd.h>

namespace larcv {
  ThreadDatumFiller::ThreadDatumFiller(std::string name)
    : larcv_base(name)
    , _processing(false)
    , _thread_running(false)
    , _random_access(false)	
    , _configured(false)
    , _enable_filter(false)
    , _num_processed(0)
    , _dim_v(4,0)
    , _driver(name + "ProcessDriver")
    , _filler(nullptr)
    , _th()
    {}

    ThreadDatumFiller::~ThreadDatumFiller()
    {
   		if(_th.joinable()) _th.join();
   		if(_processing) _driver.finalize();
    }

	void ThreadDatumFiller::reset() 
 	{
 		if(_processing) {
 			LARCV_NORMAL() << "Finalizing..." << std::endl;
 			_driver.finalize();
 		}
 		_filler = nullptr;
 		_driver.reset();
 		_configured = false;
 		_processing = false;
 		_num_processed = 0;
 	}

	void ThreadDatumFiller::configure(const std::string config_file)
  	{
    	LARCV_DEBUG() << "Called" << std::endl;
    	// check state
    	if(_processing) {
      		LARCV_CRITICAL() << "Must call finalize() before calling initialize() after starting to process..." << std::endl;
      		throw larbys();
    	}
	    // check cfg file
    	if(config_file.empty()) {
      		LARCV_CRITICAL() << "Config file not set!" << std::endl;
      		throw larbys();
    	}

    	// check cfg content top level
    	auto main_cfg = CreatePSetFromFile(config_file);
    	if(!main_cfg.contains_pset(name())) {
      		LARCV_CRITICAL() << "ThreadDatumFiller configuration (" << name() << ") not found in the config file (dump below)" << std::endl
		       << main_cfg.dump()
		       << std::endl;
      		throw larbys();
    	}
    	auto const cfg = main_cfg.get<larcv::PSet>(name());
    	configure(cfg);
  	}

  	void ThreadDatumFiller::configure(const PSet& orig_cfg) 
  	{

	  reset();
	  PSet cfg(_driver.name());
	  for(auto const& value_key : orig_cfg.value_keys())
	    cfg.add_value(value_key,orig_cfg.get<std::string>(value_key));
	  
	  for(auto const& pset_key : orig_cfg.pset_keys()) {
	    if(pset_key == "IOManager"){
	      LARCV_WARNING() << "IOManager configuration will be ignored..." << std::endl;
	    }else{
	      cfg.add_pset(orig_cfg.get_pset(pset_key));
	    }
	  }
	  set_verbosity( (msg::Level_t)(cfg.get<unsigned short>("Verbosity",2)) );
	  _enable_filter = cfg.get<bool>("EnableFilter");
	  _random_access = cfg.get<bool>("RandomAccess");
	  _input_fname_v = cfg.get<std::vector<std::string> >("InputFiles");
	  // Brew read-only configuration
	  PSet io_cfg("IOManager");
	  std::stringstream ss;
	  ss << logger().level();
	  io_cfg.add_value("Verbosity",ss.str());
	  io_cfg.add_value("Name",name() + "IOManager");
	  io_cfg.add_value("IOMode","0");
	  io_cfg.add_value("OutFileName","");
	  io_cfg.add_value("StoreOnlyType","[]");
	  io_cfg.add_value("StoreOnlyName","[]");
	  
	  cfg.add_pset(io_cfg);

 		// Configure the driver
 		_driver.configure(cfg);

 		// override input file
 		_driver.override_input_file(_input_fname_v);

 		// Make sure event_creator does not exist
		ProcessID_t last_process_id=0;
		ProcessID_t datum_filler_id=kINVALID_SIZE;
		for(auto const& process_name : _driver.process_names()) {
			ProcessID_t id = _driver.process_id(process_name);
			if(id > last_process_id) last_process_id = id;
				auto ptr = _driver.process_ptr(id);
			if(ptr->is("DatumFiller")) {
				if(datum_filler_id != kINVALID_SIZE) {
					LARCV_CRITICAL() << "Duplicate DatumFillers: id=" << datum_filler_id
						<< " vs. id=" << id << std::endl;
					throw larbys();
				}
				datum_filler_id = id;
			}
		}
		if(datum_filler_id == kINVALID_SIZE) {
			LARCV_CRITICAL() << "DatumFiller not found in process list..." << std::endl;
			throw larbys();
		}
		if(datum_filler_id != last_process_id){
			LARCV_CRITICAL() << "DatumFiller not the last process..." << std::endl;
			throw larbys();
		}
		// Retrieve the filler ptr
		_filler = (DatumFillerBase*)(_driver.process_ptr(datum_filler_id));
		_configured = true;
  	}

    const std::vector<int>& ThreadDatumFiller::dim()
    {
    	if(!_processing) {
    		LARCV_CRITICAL() << "Dimension is not known before start processing!" << std::endl;
    		throw larbys();
	   	}
	   	if(_thread_running) {
	   		LARCV_CRITICAL() << "Thread is currently running (cannot retrieve data)" << std::endl;
	   		throw larbys();
	   	}
	   	_dim_v[0] = _filler->_nentries;
	   	_dim_v[1] = _filler->_num_channels;
	   	_dim_v[2] = _filler->_rows;
	   	_dim_v[3] = _filler->_cols;
	   	return _dim_v;
    }

    const std::vector<float>& ThreadDatumFiller::data() const
    {
    	if(!_processing) {
    		LARCV_CRITICAL() << "Dimension is not known before start processing!" << std::endl;
    		throw larbys();
	   	}
	   	if(_thread_running) {
	   		LARCV_CRITICAL() << "Thread is currently running (cannot retrieve data)" << std::endl;
	   		throw larbys();
	   	}
	   	return _filler->data();    	
    }

    const std::vector<float>& ThreadDatumFiller::labels() const
    {
    	if(!_processing) {
    		LARCV_CRITICAL() << "Dimension is not known before start processing!" << std::endl;
    		throw larbys();
	   	}
	   	if(_thread_running) {
	   		LARCV_CRITICAL() << "Thread is currently running (cannot retrieve data)" << std::endl;
	   		throw larbys();
	   	}
	   	return _filler->labels();
    }

   	bool ThreadDatumFiller::batch_process(size_t nentries)
   	{
   		LARCV_DEBUG() << " start" << std::endl;
   		if(_thread_running) return false;
   		if(_th.joinable()) {
   			LARCV_INFO() << "Thread has finished running but not joined. "
	   			<< "You might want to retrieve data?" << std::endl;
	   		_th.join();
   		}
   		if(!_processing && !_configured) {
   			LARCV_CRITICAL() << "Must call configure() before run process!" << std::endl;
   			throw larbys();
   		}
 		LARCV_INFO() << "Instantiating thread..." << std::endl;
		std::thread t(&ThreadDatumFiller::_batch_process_,this,nentries);
    	_th = std::move(t);
    	usleep(100);
    	return true;
   	}

    bool ThreadDatumFiller::_batch_process_(size_t nentries)
    {
    	LARCV_DEBUG() << " start" << std::endl;
    	_thread_running = true;
    	_filler->_nentries = nentries;
	   	if(!_processing) {
	   		LARCV_INFO() << "Initializing for 1st time processing" << std::endl;
			_driver.initialize();
			_processing = true;
	   	}

    	size_t last_entry = kINVALID_SIZE-1;
    	if(_batch_entries.size()) last_entry = _batch_entries.back();
    		_batch_entries.resize(nentries,0);

    	_filler->_nentries = nentries;
    	_filler->batch_begin();

    	size_t valid_ctr = 0;
    	std::random_device rd;
    	std::mt19937 gen(rd());
    	std::uniform_int_distribution<> dis(0,_driver.io().get_n_entries()-1);
	if(_random_access) 
	  LARCV_INFO() << "Generating random numbers from 0 to " << _driver.io().get_n_entries() << std::endl;

	LARCV_INFO() << "Entering process loop" << std::endl;
    	while(valid_ctr < nentries) {
      		size_t entry = last_entry+1;
      		if(entry == kINVALID_SIZE) entry = 0;

      		if(_random_access) {
      			entry = dis(gen);
      			while(entry == last_entry) entry = dis(gen);
      		}
      		else if(entry >= _driver.io().get_n_entries()) entry -= _driver.io().get_n_entries(); 

      		LARCV_INFO() << "Processing entry: " << entry << std::endl; 

      		last_entry = entry;
      		bool good_status = _driver.process_entry(entry);
      		if(_enable_filter && !good_status) {
      			LARCV_INFO() << "Filter enabled: bad event found" << std::endl;
      			continue;
			}

      		_batch_entries[valid_ctr] = entry;
      		++valid_ctr;
			LARCV_INFO() << "Processed good event: valid entry counter = " << valid_ctr << std::endl;
    	}
    	_num_processed += valid_ctr;

    	_filler->batch_end();
    	_thread_running = false;
    	LARCV_DEBUG() << " end" << std::endl;
    	return true;
    }
}

#endif
