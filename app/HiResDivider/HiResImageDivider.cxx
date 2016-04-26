#ifndef __HIRESIMAGEDIVIDER_CXX__
#define __HIRESIMAGEDIVIDER_CXX__

#include "HiResImageDivider.h"
#include "DataFormat/EventImage2D.h"
#include "TFile.h"
#include "TTree.h"

#include <random>
#include <iostream>

#include "opencv/cv.h"
#include "opencv2/opencv.hpp"

namespace larcv {
  namespace hires {
    static HiResImageDividerProcessFactory __global_HiResImageDividerProcessFactory__;
    
    HiResImageDivider::HiResImageDivider(const std::string name)
      : ProcessBase(name)
    {}
    
    void HiResImageDivider::configure(const PSet& cfg)
    {
      fDivisionFile       = cfg.get<std::string>("DivisionFile");
      fNPlanes            = cfg.get<int>( "NPlanes", 3 );
      fTickStart          = cfg.get<int>( "TickStart", 2400 );
      fTickPreCompression = cfg.get<int>( "TickPreCompression", 6 );
      fWirePreCompression = cfg.get<int>( "WirePreCompression", 1 );
      fMaxWireImageWidth  = cfg.get<int>( "MaxWireImageWidth" );
      fInputPMTProducer   = cfg.get<std::string>( "InputPMTProducer" );
      fInputROIProducer   = cfg.get<std::string>( "InputROIProducer" );
      fOutputROIProducer  = cfg.get<std::string>( "OutputROIProducer" );
      fNumNonVertexDivisionsPerEvent = cfg.get<int>( "NumNonVertexDivisionsPerEvent" );
      fInputImageProducer = cfg.get<std::string>( "InputImageProducer" );
      fOutputImageProducer = cfg.get<std::string>( "OutputImageProducer" );
      fInputSegmentationProducer = cfg.get<std::string>( "InputSegmentationProducer" );
      fOutputSegmentationProducer = cfg.get<std::string>( "OutputSegmentationProducer" );
      fInputPMTWeightedProducer = cfg.get<std::string>( "InputPMTWeightedProducer" );
      fOutputPMTWeightedProducer = cfg.get<std::string>( "OutputPMTWeightedProducer" );
      fCropSegmentation = cfg.get<bool>( "CropSegmentation" );
      fCropPMTWeighted  = cfg.get<bool>( "CropPMTWeighted" );
      fNumPixelRedrawThresh_v = cfg.get< std::vector<int> >( "NumPixelRedrawThresh" );
      fInterestingPixelThresh_v = cfg.get< std::vector<float> >( "InterestingPixelThresh" );
      fRedrawOnNEmptyPlanes = cfg.get<int>("RedrawOnNEmptyPlanes",2);
      fMaxRedrawAttempts = cfg.get<int>("MaxRedrawAttempts");
      fDivideWholeImage = cfg.get<bool>("DivideWholeImage");
    }
    
    void HiResImageDivider::initialize()
    {
      // The image divisions are calculated before hand in the fixed grid model
      // we load the prefined region image definitions here
      
      TFile* f = new TFile( Form("%s/app/HiResDivider/dat/%s", getenv("LARCV_BASEDIR"),fDivisionFile.c_str()), "open" );
      TTree* t = (TTree*)f->Get("imagedivider/regionInfo");
      int **planebounds = new int*[fNPlanes];
      int planenwires[fNPlanes];
      for (int p=0; p<fNPlanes; p++) {
	planebounds[p] = new int[2];
	char bname1[100];
	sprintf( bname1, "plane%d_wirebounds", p );
	t->SetBranchAddress( bname1, planebounds[p] );

	char bname2[100];
	sprintf( bname2, "plane%d_nwires", p );
	t->SetBranchAddress( bname2, &(planenwires[p]) );
	//std::cout << "setup plane=" << p << " branches" << std::endl;
      }
      
      float zbounds[2];
      float xbounds[2];
      float ybounds[2];
      int tickbounds[2];

      t->SetBranchAddress( "zbounds", zbounds );
      t->SetBranchAddress( "ybounds", ybounds );
      t->SetBranchAddress( "xbounds", xbounds );
      t->SetBranchAddress( "tickbounds", tickbounds );

      fMaxWireInRegion = 0;
      size_t entry = 0;
      size_t bytes = t->GetEntry(entry);
      while ( bytes>0 ) {
	for (int p=0; p<3; p++) {
	  if ( fMaxWireInRegion<planenwires[p] )
	    fMaxWireInRegion = planenwires[p];
	}
	int plane0[2], plane1[2], plane2[2];
	for (int i=0; i<2; i++) {
	  plane0[i] = (int)planebounds[0][i];
	  plane1[i] = (int)planebounds[1][i];
	  plane2[i] = (int)planebounds[2][i];
	  tickbounds[i] *= fTickPreCompression;
	  tickbounds[i] += fTickStart;
	}
	plane0[1] += fWirePreCompression;
	plane1[1] += fWirePreCompression;
	plane2[1] += fWirePreCompression;
	tickbounds[1] += fTickPreCompression;
 	LARCV_DEBUG() << "division entry " << entry << ": "
		      << " p0: [" << plane0[0] << "," << plane0[1] << "]"
		      << " p1: [" << plane1[0] << "," << plane1[1] << "]"
		      << " p2: [" << plane2[0] << "," << plane2[1] << "]"
		      << " t: ["  << tickbounds[0] << "," << tickbounds[1] << "]"
		      << std::endl;
	
	DivisionDef div( plane0, plane1, plane2, tickbounds, xbounds, ybounds, zbounds );
	
	m_divisions.emplace_back( div );
	entry++;
	bytes = t->GetEntry(entry);
	//std::cout << "Division tree entry:" << entry << " (" << bytes << ")" << std::endl;
      }

      if ( fMaxWireInRegion>fMaxWireImageWidth )
	fMaxWireImageWidth = fMaxWireInRegion;

      LARCV_INFO() << "MaxWireImageWidth: " << fMaxWireImageWidth << std::endl;

      for (int p=0; p<fNPlanes; p++) {
	delete [] planebounds[p];
      }
      delete [] planebounds;
      
      f->Close();

      fProcessedEvent=0;
      fROISkippedEvent=0;
      fProcessedROI=0;
      fROISkipped=0;
    }
    
    bool HiResImageDivider::process(IOManager& mgr)
    {
      ++fProcessedEvent;
      // This processor does the following:
      // 1) read in hi-res images (from producer specified in config)
      // 2) (how to choose which one we clip?)

      // If it exists, we get the ROI which will guide us on how to use the image
      // This does not exist for cosmics, in which case we create
      static const ProducerID_t roi_producer_id = mgr.producer_id(::larcv::kProductROI,fInputROIProducer);
      larcv::ROI roi;
      if(roi_producer_id != kINVALID_PRODUCER) {
	LARCV_INFO() << "ROI by producer " << fInputROIProducer << " found. Searching for MC ROI..." << std::endl;
      }else{
	LARCV_INFO() << "ROI by producer " << fInputROIProducer << " not found. Constructing Cosmic ROI..." << std::endl;
	// Input ROI did not exist. Assume this means cosmics and create one
	roi.Type(kROICosmic);
      }      

      // First we decide what divisions need to be cropped
      std::vector<int> divlist;

      if ( fDivideWholeImage ) {
	generateFitleredWholeImageDivision( divlist, mgr );
      }
      else {
	if ( roi.Type()==kROICosmic )
	  generateSingleCosmicDivision( divlist, mgr, roi );
	else
	  generateSingleMCDivision( divlist, mgr, roi );

	if(!isInteresting(roi)) {
	  LARCV_CRITICAL() << "Did not find any interesting ROI and/or failed to construct Cosmic ROI..." << std::endl;
	  if(roi_producer_id != kINVALID_PRODUCER) {
	    LARCV_ERROR() << "Input ROI does exist. Looping over ROI types and printing out..." << std::endl;
	    auto event_roi = (larcv::EventROI*)(mgr.get_data(roi_producer_id));
	    for(auto const& roi : event_roi->ROIArray()) LARCV_ERROR() << roi.dump();
	    LARCV_ERROR() << "Dump finished..." << std::endl;
	  }
	  // Return false not to store this event in case of filter IO mode
	  return false;
	}
      }

      auto input_event_images = (larcv::EventImage2D*)(mgr.get_data(kProductImage2D,fInputImageProducer));
      
      // now we loop through and make divisions
      for ( auto const& idiv : divlist ) {

	if ( idiv==-1 )
	  continue;

	larcv::hires::DivisionDef const& vertex_div = m_divisions.at( idiv );

	//std::cout << "Vertex in ROI: " << roi.X() << ", " << roi.Y() << ", " << roi.Z() << std::endl;
	
	// now we crop out certain pieces
	// The input images
	auto output_event_images = (larcv::EventImage2D*)(mgr.get_data( kProductImage2D,fOutputImageProducer) );
	output_event_images->clear();
	LARCV_DEBUG() << "Crop " << fInputImageProducer << " Images." << std::endl;
	cropEventImages( *input_event_images, vertex_div, *output_event_images );

	//
	// Image is cropped based on DivisionDef which is found from ROI's vertex
	// However ROI's vertex do not necessarily overlap with the same ROI's 2D bounding box
	// in case of a neutrino interaction because the former is a neutrino interaction vertex
	// while the latter is based on particles' trajectories that deposited energy. An example
	// is a neutron produced at vertex and hit proton far away from the vertex. So, here, we
	// ask, if it is non-cosmic type ROI, created image's meta overlaps with ROI's image meta.
	//
	if(roi.Type() != kROICosmic) {
	  // if not cosmic, we check if neutrino image has enough interesting pixels
	  if ( !isAbovePixelThreshold( *output_event_images ) ) {
	    ++fROISkippedEvent;
	    LARCV_NORMAL() << "Found an event w/ neutrino vertex without enough interesting pixels (" << fROISkippedEvent << " events skipped so far)" << std::endl;
	    auto event_roi = (larcv::EventROI*)(mgr.get_data(roi_producer_id));
	    for(auto const& img : output_event_images->Image2DArray()) LARCV_INFO() << img.meta().dump();
	    for(auto const& aroi : event_roi->ROIArray()) LARCV_INFO() << aroi.dump();
	    output_event_images->clear();
	    continue;
	  }
	  try{
	    for(auto const& img : output_event_images->Image2DArray())
	      roi.BB(img.meta().plane()).overlap(img.meta());
	  }catch(const larbys& err) {
	    ++fROISkippedEvent;
	    LARCV_NORMAL() << "Found an event w/ neutrino vertex not within ROI bounding box (" << fROISkippedEvent << " events so far)" << std::endl;
	    auto event_roi = (larcv::EventROI*)(mgr.get_data(roi_producer_id));
	    for(auto const& img : output_event_images->Image2DArray()) LARCV_INFO() << img.meta().dump();
	    for(auto const& aroi : event_roi->ROIArray()) LARCV_INFO() << aroi.dump();
	    output_event_images->clear();
	    continue;
	  }
	}

	// Output Segmentation
	if ( fCropSegmentation ) {
	  // the semantic segmentation is only filled in the neighboor hood of the interaction
	  // we overlay it into a full image (and then crop out the division)
	  auto input_seg_images = (larcv::EventImage2D*)(mgr.get_data(kProductImage2D,fInputSegmentationProducer));
	  larcv::EventImage2D full_seg_images;
	  for ( unsigned int p=0; p<3; p++ ) {
	    larcv::Image2D const& img = input_event_images->at( p ); 
	    larcv::ImageMeta seg_image_meta( img.meta().width(), img.meta().height(),
					     img.meta().rows(), img.meta().cols(),
					     img.meta().min_x(), img.meta().max_y(),
					     img.meta().plane() );
	    larcv::Image2D seg_image( seg_image_meta );
	    seg_image.paint( 0.0 );
	    seg_image.overlay( input_seg_images->at(p) );
	    full_seg_images.Emplace( std::move(seg_image) );
	  }
	  LARCV_DEBUG() << "Crop " << fInputSegmentationProducer << " Images." << std::endl;
	  auto output_seg_images = (larcv::EventImage2D*)(mgr.get_data(kProductImage2D,fOutputSegmentationProducer));
	  output_seg_images->clear();
	  cropEventImages( full_seg_images, vertex_div, *output_seg_images );
	  
	}// if crop seg
      
	// Output PMT weighted
	if ( fCropPMTWeighted )  {
	  LARCV_DEBUG() << "Load " << fInputPMTWeightedProducer << " Images." << std::endl;
	  auto input_pmtweighted_images = (larcv::EventImage2D*)(mgr.get_data(kProductImage2D,fInputPMTWeightedProducer));
	  LARCV_DEBUG() << "Allocate " << fOutputPMTWeightedProducer << " Images." << std::endl;
	  auto output_pmtweighted_images = (larcv::EventImage2D*)(mgr.get_data(kProductImage2D,fOutputPMTWeightedProducer));
	  output_pmtweighted_images->clear();
	  LARCV_DEBUG() << "Crop " << fInputPMTWeightedProducer << " Images." << std::endl;
	  cropEventImages( *input_pmtweighted_images, vertex_div, *output_pmtweighted_images );	
	}

	// Finally let's store ROI w/ updated ImageMeta arrays
	//
	// 0) Retrieve output image array and input ROI array (for the latter "if exists")
	// 1) Loop over ROI array (or single ROI for "cosmic" = input does not exist), ask overlap in 2D plane ImageMeta with each image
	auto output_pmtweighted_images = (larcv::EventImage2D*)(mgr.get_data(kProductImage2D,fOutputPMTWeightedProducer));
	auto output_rois = (larcv::EventROI*)(mgr.get_data(kProductROI,fOutputROIProducer));
	output_rois->clear();
	
	if(roi_producer_id != kINVALID_PRODUCER) {
	  // Retrieve input ROI array
	  auto event_roi = (larcv::EventROI*)(mgr.get_data(roi_producer_id));
	  // Loop over and store in output
	  
	  for(auto const& aroi : event_roi->ROIArray()) {
	    ++fProcessedROI;
	    std::vector<larcv::ImageMeta> out_meta_v;
	    try {
	      //LARCV_INFO() << "Creating particle ROI for: " << roi.dump() << std::endl;
	      for(auto const& bb : aroi.BB()) {
		auto const& img_meta = output_pmtweighted_images->at(bb.plane()).meta();
		out_meta_v.push_back(img_meta.overlap(bb));
	      }
	    }catch(const larbys& err){
	      LARCV_INFO() << "Found an ROI bounding box that has no overlap with neutrino vertex box. Skipping..." << std::endl;
	      LARCV_INFO() << aroi.dump() << std::endl;
	      out_meta_v.clear();
	      ++fROISkipped;
	    }
	  
	    ::larcv::ROI out_roi(aroi);
	    out_roi.SetBB(out_meta_v);
	    
	    output_rois->Emplace(std::move(out_roi));
	  }
	}else{
	  std::vector<larcv::ImageMeta> out_meta_v;
	  for(auto const& img : output_pmtweighted_images->Image2DArray()) out_meta_v.push_back(img.meta());
	  roi.SetBB(out_meta_v);
	  output_rois->Emplace(std::move(roi));
	}

	mgr.save_entry();
      }//end of divlist loop
      return true;
    }
    
    void HiResImageDivider::finalize(TFile* ana_file)
    {
      LARCV_WARNING() << "Skipped events due to vertex-box not overlapping with ROI: " << fROISkippedEvent << " / " << fProcessedEvent << std::endl;
      LARCV_WARNING() << "Skipped ROI due to not within vertex-box: " << fROISkipped << " / " << fProcessedROI << std::endl;
    }

    // -------------------------------------------------------
    // Division list generators

    void HiResImageDivider::generateSingleCosmicDivision( std::vector< int >& divlist, IOManager& mgr, larcv::ROI& roi ) {
      // we randomly pick a division (by randomly drawing a position in the detector
      // we return a division which satisfies some minimum amount of interesting pixel threshold
      bool viewok = false;
      int ntries = 0;

      roi.Type(kROICosmic);
	
      int idiv = -1;
      auto input_event_images = (larcv::EventImage2D*)(mgr.get_data(kProductImage2D,fInputImageProducer));

      while (viewok && ntries<fMaxRedrawAttempts) {
	ntries++;
	// FIXME: need a way to get detector dimension somehow...
	const double zmin = 0;
	const double zmax = 1036.0;
	const double ymin = -116.;
	const double ymax = 116.;
	const double xmin = 0.;
	const double xmax = 255.;
	const double tmin = 3125.; // in ns
	const double tmax = tmin + 1600.;
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution<> dis(0.,1.);
	roi.Position( dis(gen) * (xmax - xmin) + xmin,
		      dis(gen) * (ymax - ymin) + ymin,
		      dis(gen) * (zmax - zmin) + zmin,
		      dis(gen) * (tmax - tmin) + tmin);
	
	idiv = findVertexDivision( roi );
	if ( idiv==-1 ) {
	  viewok = false;
	  continue;
	}
	larcv::hires::DivisionDef const& vertex_div = m_divisions.at( idiv );
	larcv::EventImage2D cosmic_test;
	cropEventImages( *input_event_images, vertex_div, cosmic_test );
	if ( isAbovePixelThreshold( cosmic_test ) )  {
	  viewok = true;
	  break;
	}
      }
      if ( !viewok ) {
	LARCV_ERROR() << "could not find cosmic roi with enough interesting pixels.\n" << std::endl;
      }
      else {
	divlist.push_back(idiv);
      }
    }

    void HiResImageDivider::generateSingleMCDivision( std::vector< int >& divlist, IOManager& mgr, larcv::ROI& roi ) {
      static const ProducerID_t roi_producer_id = mgr.producer_id(::larcv::kProductROI,fInputROIProducer);

      int idiv = -1;
      if(roi_producer_id != kINVALID_PRODUCER) {
	LARCV_INFO() << "ROI by producer " << fInputROIProducer << " found. Searching for MC ROI..." << std::endl;
	auto event_roi = (larcv::EventROI*)(mgr.get_data(roi_producer_id));
	for ( auto const& aroi : event_roi->ROIArray() ) 
	  if ( isInteresting(aroi) ) { roi = aroi; break; }

	idiv = findVertexDivision( roi );
      }
      
      if ( idiv==-1 ) {
	LARCV_ERROR() << "No divisions were found that contained an event vertex.\n" << roi.dump() << std::endl;
      }
      else {
	divlist.push_back(idiv);
      }
    }
    
    void HiResImageDivider::generateFitleredWholeImageDivision( std::vector< int >& divlist, IOManager& mgr ) {
      // we loop through all divisions and make a test crop. we then test this cropped region if it
      // satisfies the conditions to be deemed interesting enough to save
      auto input_event_images = (larcv::EventImage2D*)(mgr.get_data(kProductImage2D,fInputImageProducer));
      for ( int idiv=0; idiv<(int)m_divisions.size(); idiv++ ) {
	// div is a larcv::hires::DivisionDef
	larcv::hires::DivisionDef const& div = m_divisions.at(idiv);
	larcv::EventImage2D cropped;
	cropEventImages( *input_event_images, div, cropped );
	if ( isAbovePixelThreshold( cropped ) )
	  divlist.push_back( idiv );
      }
    }

    // -------------------------------------------------------

    bool HiResImageDivider::isInteresting( const larcv::ROI& roi ) {
      // Supposed to return the "primary" 
      //return (roi.Type() == kROIBNB || roi.Type() == kROICosmic);
      // Consider if this came from MCTruth: MCSTIndex should be kINVALID_USHORT
      return (roi.MCSTIndex() == kINVALID_USHORT);
    }
    
    bool HiResImageDivider::isAbovePixelThreshold( const larcv::EventImage2D& imgs ) {
      // warning, vectors assume plane ids are sequential and have no gaps
      std::vector<int> npixels(fInterestingPixelThresh_v.size(),0);
      for ( auto const& img: imgs.Image2DArray() ) {
	auto const plane = img.meta().plane();
	auto const& adc_v = img.as_vector();
	for ( auto const& adc : adc_v ) {
	  if ( adc>fInterestingPixelThresh_v.at(plane) ) {
	    npixels.at(plane)++;
	  }
	}
      }
      
      int nempty = 0;
      for ( size_t i=0;i<npixels.size(); i++ ) {
	LARCV_INFO() << "Plane " << i << " has "
		     << npixels[i] << " pixels above "
		     << fInterestingPixelThresh_v[i] << " [ADC] ... Threshold = " 
		     << fNumPixelRedrawThresh_v[i] << std::endl;
	if ( fNumPixelRedrawThresh_v.at(i)>0 && npixels.at(i)<fNumPixelRedrawThresh_v.at(i) )
	  nempty++;
      }
      if ( nempty>=fRedrawOnNEmptyPlanes )
	return false;
      
      return true;
    }
    
    int HiResImageDivider::findVertexDivision( const larcv::ROI& roi ) {
      int regionindex = 0;
      for ( std::vector< larcv::hires::DivisionDef >::iterator it=m_divisions.begin(); it!=m_divisions.end(); it++) {
	DivisionDef const& div = (*it);
	if ( div.isInsideDetRegion( roi.X(), roi.Y(), roi.Z() ) )
	  return regionindex;
	regionindex++;
      }
      return -1;
    }
    
    bool HiResImageDivider::keepNonVertexDivision( const larcv::ROI& roi ) {
      return true;
    }
    
    void HiResImageDivider::cropEventImages( const larcv::EventImage2D& event_images, const larcv::hires::DivisionDef& div, larcv::EventImage2D& output_images ) { 

      // Output Image Container
      std::vector<larcv::Image2D> cropped_images;

      LARCV_DEBUG() << "Images to crop: "<< event_images.Image2DArray().size() << std::endl;

      for ( auto const& img : event_images.Image2DArray() ) {

	int iplane = (int)img.meta().plane();
	larcv::ImageMeta const& divPlaneMeta = div.getPlaneMeta( iplane );

	// divPlaneMeta is based on un-compressed time axis.
	// align the same scaling as img meta

	auto scaled = divPlaneMeta;
	
	scaled.update(scaled.height()/img.meta().pixel_height(),scaled.cols());
	
	auto cropmeta = img.meta().overlap(scaled);

	auto cropped = img.crop(cropmeta);

	cropped.resize(fMaxWireImageWidth,fMaxWireImageWidth,0.);

	LARCV_DEBUG() << "image: " << std::endl << img.meta().dump() ;
	LARCV_DEBUG() << "div: " << std::endl << divPlaneMeta.dump() ;
	LARCV_DEBUG() << "scaled: " << std::endl << scaled.dump() ;
	LARCV_DEBUG() << "to-be-cropped: " << std::endl << cropmeta.dump() ;
	LARCV_DEBUG() << "cropped: " << std::endl << cropped.meta().dump() ;
	
	/*
	// we adjust the actual crop meta
	int tstart = divPlaneMeta.min_y();
	int twidth = fMaxWireImageWidth*fTickDownSample;
	int tmax = std::min( tstart+twidth, (int)img.meta().max_y() );

	larcv::ImageMeta cropmeta( divPlaneMeta.width(), twidth,
				   twidth, divPlaneMeta.width(), 
				   divPlaneMeta.min_x(), tmax );
	
	LARCV_INFO() << "image: " << std::endl << img.meta().dump() << std::endl;
	
	LARCV_INFO() << "div: " << std::endl << divPlaneMeta.dump() << std::endl;
	
	LARCV_INFO() << "crop: " << std::endl << cropmeta.dump() << std::endl;
		
	Image2D cropped = img.crop( cropmeta );
	LARCV_INFO() << "cropped." << std::endl;
	
	//cropped.resize( fMaxWireImageWidth*fTickDownSample, fMaxWireImageWidth, 0.0 );  // resize to final image size (and zero pad extra space)
	LARCV_INFO() << "resized." << std::endl;

	//cropped.compress( (int)cropped.meta().height()/6, fMaxWireImageWidth, larcv::Image2D::kSum );
	LARCV_INFO() << "downsampled. " << cropped.meta().height() << " x " << cropped.meta().width() << std::endl;
	*/	
	cropped_images.emplace_back( cropped );
	LARCV_INFO() << "stored." << std::endl;
      }//end of plane loop

      output_images.Emplace( std::move( cropped_images ) );

    }



  }
}
#endif
