#include "LLRAnalysis/TauTauStudies/plugins/CompositePtrCandidateT1T2toMEtAssociationProducer.h"

#include "FWCore/Utilities/interface/Exception.h"

#include "FWCore/ParameterSet/interface/ParameterSet.h"

#include "DataFormats/Candidate/interface/Candidate.h"
#include "DataFormats/PatCandidates/interface/MET.h"
#include "DataFormats/Common/interface/RefToPtr.h"
#include "DataFormats/Common/interface/Handle.h"

#include <TMath.h>

#include <string>

const std::string instNameDiTaus = "diTaus";
const std::string instNameMEtObjects = "met";
const std::string instNameDiTauToMEtAssociations = "diTauToMEtAssociations";

template <typename T1, typename T2>
CompositePtrCandidateT1T2toMEtAssociationProducer<T1,T2>::CompositePtrCandidateT1T2toMEtAssociationProducer(const edm::ParameterSet& cfg)
  : moduleLabel_(cfg.getParameter<std::string>("@module_label"))
{
  edm::VParameterSet cfgInputs = cfg.getParameter<edm::VParameterSet>("inputs");
  for ( edm::VParameterSet::const_iterator cfgInput = cfgInputs.begin();
	cfgInput != cfgInputs.end(); ++cfgInput ) {
    inputEntryType input;
    input.srcDiTau_ = cfgInput->getParameter<edm::InputTag>("srcDiTau");
    input.srcMET_ = cfgInput->getParameter<edm::InputTag>("srcMET");
    inputs_.push_back(input);
  }

  produces<CompositePtrCandidateCollection>(instNameDiTaus);
  produces<pat::METCollection>(instNameMEtObjects);
  produces<diTauToMEtAssociation>(instNameDiTauToMEtAssociations);
}

template <typename T1, typename T2>
CompositePtrCandidateT1T2toMEtAssociationProducer<T1,T2>::~CompositePtrCandidateT1T2toMEtAssociationProducer()
{
// nothing to be done yet...
}

template <typename T1, typename T2>
void CompositePtrCandidateT1T2toMEtAssociationProducer<T1,T2>::produce(edm::Event& evt, const edm::EventSetup& es)
{
  std::auto_ptr<CompositePtrCandidateCollection> outputDiTaus(new CompositePtrCandidateCollection());
  edm::RefProd<CompositePtrCandidateCollection> outputDiTauRefProd = evt.getRefBeforePut<CompositePtrCandidateCollection>(instNameDiTaus);

  std::auto_ptr<pat::METCollection> outputMETs(new pat::METCollection());
  edm::RefProd<pat::METCollection> outputMEtRefProd = evt.getRefBeforePut<pat::METCollection>(instNameMEtObjects);

  std::auto_ptr<diTauToMEtAssociation> outputMEtAssociations(new diTauToMEtAssociation(outputDiTauRefProd));

  int idxDiTau = 0;
  for ( typename std::vector<inputEntryType>::const_iterator input = inputs_.begin();
	input != inputs_.end(); ++input ) {
    edm::Handle<CompositePtrCandidateCollection> inputDiTaus;    
    evt.getByLabel(input->srcDiTau_, inputDiTaus);
    if ( !inputDiTaus.isValid() ) continue;
    
    edm::Handle<pat::METCollection> inputMETs;
    evt.getByLabel(input->srcMET_, inputMETs);
    if ( inputMETs->size() != 1 ) 
      throw cms::Exception("CompositePtrCandidateT1T2toMEtAssociationProducer")
	<< "Failed to find unique MET object for input collection = " << input->srcMET_.label() << " !!\n";
    const pat::MET& inputMET = inputMETs->front();

    for ( typename CompositePtrCandidateCollection::const_iterator inputDiTau = inputDiTaus->begin();
	  inputDiTau != inputDiTaus->end(); ++inputDiTau ) {
      outputDiTaus->push_back(*inputDiTau);
      outputMETs->push_back(inputMET);
      pat::METRef outputMEtRef(outputMEtRefProd, idxDiTau);
      int idxMET = outputMEtRef.index();
      outputMEtAssociations->setValue(idxDiTau, idxMET);
      ++idxDiTau;
    }
  }

  evt.put(outputDiTaus, instNameDiTaus);
  evt.put(outputMETs, instNameMEtObjects);
  evt.put(outputMEtAssociations, instNameDiTauToMEtAssociations);  
}

#include "DataFormats/PatCandidates/interface/Electron.h"
#include "DataFormats/PatCandidates/interface/Muon.h"

typedef CompositePtrCandidateT1T2toMEtAssociationProducer<pat::Electron, pat::Tau> PATElecTauPairToMEtAssociationProducer;
typedef CompositePtrCandidateT1T2toMEtAssociationProducer<pat::Muon, pat::Tau> PATMuTauPairToMEtAssociationProducer;

#include "FWCore/Framework/interface/MakerMacros.h"

DEFINE_FWK_MODULE(PATElecTauPairToMEtAssociationProducer);
DEFINE_FWK_MODULE(PATMuTauPairToMEtAssociationProducer);