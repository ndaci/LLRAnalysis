#include "FWCore/FWLite/interface/AutoLibraryLoader.h"

#include <cstdlib>
#include <iostream> 
#include <fstream>
#include <sstream>
#include <map>
#include <string>

#include "TChain.h"
#include "TEntryList.h"
#include "TMath.h"
#include "TFile.h"
#include "TTree.h"
#include "TString.h"
#include "TObjString.h"
#include "TSystem.h"
#include "TROOT.h"
#include "TPluginManager.h"
#include "TH1F.h"
#include "TH2F.h"
#include "TH1.h"
#include "TFile.h"
#include "TCanvas.h"
#include "TPad.h"
#include "TF1.h"
#include "TLegend.h"
#include "THStack.h"
#include "TCut.h"
#include "TArrayF.h"
#include "TStyle.h"

#include "HiggsAnalysis/CombinedLimit/interface/TH1Keys.h"

#define VERBOSE          true
#define DEBUG            true
#define NOLOOP           false
#define USESSBKG         false
#define scaleByBinWidth  false
#define DOSPLIT          false
#define useZDataMC       false

typedef map<TString, TChain* >  mapchain;

///////////////////////////////////////////////////////////////////////////////////////////////
TH1F* blindHistogram(TH1F* h, float xmin, float xmax, TString name) {

  TH1F* hOut = (TH1F*)h->Clone(name);
  int nBins  = hOut->GetNbinsX();
  float x    = 0;

  for(int i=1; i<nBins+1 ; i++) {
    x = hOut->GetBinCenter(i);
    if( x>xmin && x<xmax ) hOut->SetBinContent(i,-10);
  }

  return hOut;

}
///////////////////////////////////////////////////////////////////////////////////////////////

void makeHistoFromDensity(TH1* hDensity, TH1* hHistogram){

  if(hDensity->GetNbinsX() != hHistogram->GetNbinsX()){
    cout << "makeHistoFromDensity: different binning" << endl;
    return;
  }

  for(int k = 1 ; k <= hDensity->GetNbinsX(); k++){
    float bink   = hDensity->GetBinContent(k);
    float widthk = hHistogram->GetBinWidth(k);
    hDensity->SetBinContent(k, bink*widthk );
  }
  hDensity->Scale(hHistogram->Integral()/hDensity->Integral());
}

///////////////////////////////////////////////////////////////////////////////////////////////
void chooseSelection(TString version_, TCut& tiso, TCut& ltiso, TCut& mtiso, TCut& antimu)
{

  // Anti-Mu discriminator //
  if(version_.Contains("AntiMu1"))      antimu = "tightestAntiMuWP>2";
  else if(version_.Contains("AntiMu2")) antimu = "tightestAntiMu2WP>2";
  
  // TauIso1 //
  if(version_.Contains("TauIso1")) {
    tiso   = "tightestHPSMVAWP>=0" ;
    ltiso  = "tightestHPSMVAWP>-99" ;
    mtiso  = "hpsMVA>0.7" ;
  }
  // TauIso2 //
  else if(version_.Contains("TauIso2")) {
    tiso   = "tightestHPSMVA2WP>=0" ;
    ltiso  = "tightestHPSMVA2WP>-99" ;
    mtiso  = "hpsMVA2>0.7" ;
  }
  // TauIso DB3Hits cut-based //
  else if(version_.Contains("HPSDB3H")) {
    tiso   = "tightestHPSDB3HWP>=0" ;
    ltiso  = "tightestHPSDB3HWP>-99" ;
    //mtiso  = "hpsDB3H>0.7" ;
    mtiso  = "hpsDB3H<3" ;
  }

}
///////////////////////////////////////////////////////////////////////////////////////////////

void createStringsIsoFakeRate(TString fileName = "FakeRate.root",
			      string& scaleFactMu     = *( new string()), 
			      string& scaleFactMuUp   = *( new string()),
			      string& scaleFactMuDown = *( new string()),
			      string variableX = "ptL1",
			      string variableY = "FakeRate",
			      string interspace = "_",
			      string selection = "Mu_ptL1_incl",
			      bool verbose = false){

 TFile FakeRate(fileName,"READ");
 if(FakeRate.IsZombie()){
   cout << "Missing FR histos... create dummy ones" << endl;
   scaleFactMu     = "(etaL1<999)";
   scaleFactMuUp   = "(etaL1<999)";
   scaleFactMuDown = "(etaL1<999)";
   return;
 }
 
 TF1*  frMu     = (TF1*)FakeRate.Get(("fit"+interspace+selection).c_str());
 TH1F* frMuUp   = (TH1F*)FakeRate.Get(("h"+variableY+"ErrUp"+selection).c_str());
 TH1F* frMuDown = (TH1F*)FakeRate.Get(("h"+variableY+"ErrDown"+selection).c_str());

 if(!frMu || !frMuUp || !frMuDown){
   cout << "Missing FR histos... exit" << endl;
   return;
 }
 
 vector<int> binsFR;
 binsFR.push_back(17);  binsFR.push_back(20);
 binsFR.push_back(22);  binsFR.push_back(24);
 binsFR.push_back(26);  binsFR.push_back(28);
 binsFR.push_back(30);  binsFR.push_back(32);
 binsFR.push_back(34);  binsFR.push_back(36);
 binsFR.push_back(40);  binsFR.push_back(45);
 binsFR.push_back(50);  binsFR.push_back(60); 
 binsFR.push_back(80);  binsFR.push_back(100);
 binsFR.push_back(9999);

 scaleFactMu     = "( ";
 scaleFactMuUp   = "( ";
 scaleFactMuDown = "( ";

 for(unsigned int i = 0; i < binsFR.size()-1; i++){
   
    float min = binsFR[i];
    float max = binsFR[i+1];
    
    float bin = frMuUp->FindBin((max+min)/2.);
    if( bin == frMuUp->GetNbinsX() + 1) bin--;
    
    float weightBinMu_i     = frMu->Eval( (max+min)/2.)>0 ?       1./frMu->Eval( (max+min)/2.)      : 0.0;
    float weightBinMu_iUp   = frMuUp->GetBinContent( bin )>0 ?    1./frMuUp->GetBinContent( bin )   : 0.0;
    float weightBinMu_iDown = frMuDown->GetBinContent( bin )>0 ?  1./frMuDown->GetBinContent( bin ) : 0.0;
    
    scaleFactMu     += string( Form("(%s>=%f && %s<%f)*%f", variableX.c_str(), min , variableX.c_str(), max, weightBinMu_i ) );
    scaleFactMuUp   += string( Form("(%s>=%f && %s<%f)*%f", variableX.c_str(), min , variableX.c_str(), max, weightBinMu_iUp   ) );
    scaleFactMuDown += string( Form("(%s>=%f && %s<%f)*%f", variableX.c_str(), min , variableX.c_str(), max, weightBinMu_iDown ) );

    if(i < binsFR.size() - 2 ){
      scaleFactMu     += " + ";
      scaleFactMuUp   += " + ";
      scaleFactMuDown += " + ";
    }
 }
 
 scaleFactMu     += " )";
 scaleFactMuUp   += " )";
 scaleFactMuDown += " )";
 
 if(verbose){
   cout << scaleFactMu << endl;
   cout << scaleFactMuUp << endl;
   cout << scaleFactMuDown << endl;
 }

}
///////////////////////////////////////////////////////////////////////////////////////////////
void drawHistogram(TString type = "MC",
		   TString version_ = "",
		   TString RUN = "ABCD",
		   TChain* tree = 0, 
		   TString variable = "", 
		   float& normalization      = *(new float()), 
		   float& normalizationError = *(new float()), 
		   float scaleFactor = 1., 
		   TH1F* h = 0, 
		   TCut cut = TCut(""),
		   int verbose = 0){

  if(DEBUG) cout << "Start drawHistogram : " << type << " " << version_ << " " << RUN << endl;

  // Special case for data
  if(type.Contains("Data")) {
    if(variable=="caloMEtNoHF")    variable="caloMEtNoHFUncorr";
    if(variable=="caloMEtNoHFPhi") variable="caloMEtNoHFUncorrPhi";
  }

  // Start processing
  if(tree!=0 && h!=0){

    if(NOLOOP) { // check AntiMu1_TauIso2 std analysis
      TCut pairIndex="pairIndex[1]<1";
      TCut weight="run>0";
      if(type.Contains("MC")) {
	if(     version_.Contains("SoftABC"))  weight = "(sampleWeight*puWeightHCP*HLTweightTauABC*HLTweightMuABCShift*SFTau*SFMu_ABC*weightHepNup*passL1etmCutABC)";
	else if(version_.Contains("SoftD"))    weight = "(sampleWeight*puWeightDLow*puWeightDHigh*HLTTauD*HLTMuSoft*SFTau*SFMu_D*weightHepNup*passL1etmCut)";
	else if(version_.Contains("SoftLTau")) weight = "(sampleWeight*puWeightDLow*puWeightDHigh*HLTTauD*HLTMuSoft*SFTau*SFMu_D*weightHepNup)";
	else if(!version_.Contains("Soft")) {
	  if(     RUN=="ABC")                  weight = "(sampleWeight*puWeightHCP*HLTweightTauABC*HLTweightMuABC*SFTau*SFMu_ABC*weightHepNup)";
	  else if(RUN=="D")                    weight = "(sampleWeight*puWeightD*HLTweightTauD*HLTweightMuD*SFTau*SFMu_D*weightHepNup)";
	  else                                 weight = "(sampleWeight*puWeight*HLTweightTau*HLTweightMu*SFTau*SFMu*weightHepNup)";
	}      
      }
      else if(type.Contains("Embed")) {
	if(     version_.Contains("SoftABC"))  weight = "(HLTTauABC*HLTMuABCShift*passL1etmCutABC*embeddingWeight)";
	else if(version_.Contains("SoftD"))    weight = "(HLTTauD*HLTMuSoft*passL1etmCut*embeddingWeight)";
	else if(version_.Contains("SoftLTau")) weight = "(HLTTauD*HLTMuSoft*embeddingWeight)";
	else if(!version_.Contains("Soft")) {
	  if(RUN=="ABC")                       weight = "(HLTTauABC*HLTMuABC*embeddingWeight)";
	  else if(RUN=="D")                    weight = "(HLTTauD*HLTMuD*embeddingWeight)";
	  else                                 weight = "(HLTTau*HLTMu*embeddingWeight)";
	}
      }
      tree->Draw(variable+">>"+TString(h->GetName()),cut*weight*pairIndex);
      h->Scale(scaleFactor);
      normalization      = h->Integral();
      normalizationError = TMath::Sqrt(h->GetEntries())*(normalization/h->GetEntries());
      if(verbose==0) h->Reset();
      return;
    }

    // Produce the skim TEntryList from the cut
    if(DEBUG) cout << "-- produce skim" << endl;
    
    tree->Draw(">>+skim",cut,"entrylist");
    TEntryList *skim = (TEntryList*)gDirectory->Get("skim");
    int nEntries = skim->GetN();
    tree->SetEntryList(skim);

    if(DEBUG) cout << "Reset h" << endl;
    h->Reset();

    // Declare tree variables
    int   run  = 0;
    int   event= 0; 
    float var  = 0;
    float sampleWeight   =0;
    float HLTweightTau   =0;
    float HLTweightTauABC=0;
    float SFTau=0;
    float weightHepNup=0;
    float passL1etmCutABC=0;
    float passL1etmCut=0;
    float puWeight=0;
    float puWeightHCP=0;
    float puWeightD=0;
    float puWeightDLow=0;
    float puWeightDHigh=0;
    float HLTweightTauD=0;
    float HLTTauABC=0;
    float HLTTauD=0;
    float HLTTau=0;
    float HLTweightMu=0;
    float HLTweightMuSoft=0;
    float HLTweightMuD=0;
    float HLTMuSoft=0;
    float HLTMuABCShift=0;
    float HLTMuABC=0;
    float HLTMuD=0;
    float HLTMu=0;
    float SFMu=0;
    float SFMu_ABC=0;
    float SFMu_D=0;
    float embeddingWeight=0;
    float fakeRate=0;
    float ptL1=0;
    float HLTweightMuABC=1;
    float HLTweightMuABCShift=0;
    //float diTauNSVfitMass=0;

    fakeRate = 1.0; // to be updated with Artur's tools

    // Declare tree branches
    if(DEBUG) cout << "-- declare tree branches" << endl;
    tree->SetBranchStatus("*",  0);   
    tree->SetBranchStatus(variable, 1); tree->SetBranchAddress(variable, &var   );
    tree->SetBranchStatus("run",    1); tree->SetBranchAddress("run",    &run   );
    tree->SetBranchStatus("event",  1); tree->SetBranchAddress("event",  &event );
    
    if(type.Contains("MC") || type.Contains("Embed")) {
      
      tree->SetBranchStatus("sampleWeight",    1); tree->SetBranchAddress("sampleWeight",    &sampleWeight );
      tree->SetBranchStatus("weightHepNup",    1); tree->SetBranchAddress("weightHepNup",    &weightHepNup );
      tree->SetBranchStatus("SFTau",           1); tree->SetBranchAddress("SFTau",           &SFTau );
      tree->SetBranchStatus("embeddingWeight", 1); tree->SetBranchAddress("embeddingWeight", &embeddingWeight );
      //tree->SetBranchStatus("fakeRate",        1); tree->SetBranchAddress("fakeRate",        &fakeRate   );
      //tree->SetBranchStatus("",1); tree->SetBranchAddress("",  &   );
      
      if(RUN=="ABC") {
	tree->SetBranchStatus("puWeightHCP",     1); tree->SetBranchAddress("puWeightHCP",         &puWeightHCP  );
	tree->SetBranchStatus("HLTTauABC",       1); tree->SetBranchAddress("HLTTauABC",           &HLTTauABC);
	tree->SetBranchStatus("HLTweightTauABC", 1); tree->SetBranchAddress("HLTweightTauABC",     &HLTweightTauABC ); // BUG
	tree->SetBranchStatus("SFMu_ABC",        1); tree->SetBranchAddress("SFMu_ABC",            &SFMu_ABC );
	if(version_.Contains("Soft")) {
	  tree->SetBranchStatus("HLTMuABCShift",      1); tree->SetBranchAddress("HLTMuABCShift",       &HLTMuABCShift);
	  tree->SetBranchStatus("HLTweightMuABCShift",1); tree->SetBranchAddress("HLTweightMuABCShift", &HLTweightMuABCShift );
	  tree->SetBranchStatus("passL1etmCutABC",    1); tree->SetBranchAddress("passL1etmCutABC",     &passL1etmCutABC);
	}
	else {
	  tree->SetBranchStatus("HLTMuABC",      1); tree->SetBranchAddress("HLTMuABC",       &HLTMuABC      );
	  tree->SetBranchStatus("HLTweightMuABC",1); tree->SetBranchAddress("HLTweightMuABC", &HLTweightMuABC ); // BUG
	}
      }
      else if(RUN=="D") {
	tree->SetBranchStatus("puWeightD",         1); tree->SetBranchAddress("puWeightD",      &puWeightD    );
	tree->SetBranchStatus("puWeightDLow",      1); tree->SetBranchAddress("puWeightDLow",   &puWeightDLow );
	tree->SetBranchStatus("puWeightDHigh",     1); tree->SetBranchAddress("puWeightDHigh",  &puWeightDHigh);
	tree->SetBranchStatus("HLTTauD",           1); tree->SetBranchAddress("HLTTauD",        &HLTTauD);
	tree->SetBranchStatus("HLTweightTauD",     1); tree->SetBranchAddress("HLTweightTauD",  &HLTweightTauD); // BUG
	tree->SetBranchStatus("SFMu_D",            1); tree->SetBranchAddress("SFMu_D",         &SFMu_D);
	if(version_.Contains("Soft")) {
	  tree->SetBranchStatus("HLTMuSoft",       1); tree->SetBranchAddress("HLTMuSoft",      &HLTMuSoft);
	  tree->SetBranchStatus("HLTweightMuSoft", 1); tree->SetBranchAddress("HLTweightMuSoft",&HLTweightMuSoft);
	  tree->SetBranchStatus("passL1etmCut",    1); tree->SetBranchAddress("passL1etmCut",   &passL1etmCut);
	}
	else {	    
	  tree->SetBranchStatus("HLTMuD",          1); tree->SetBranchAddress("HLTMuD",         &HLTMuD      );
	  tree->SetBranchStatus("HLTweightMuD",    1); tree->SetBranchAddress("HLTweightMuD",   &HLTweightMuD); // BUG
	}
      }
      else if(RUN=="ABCD") {
	tree->SetBranchStatus("puWeight",          1); tree->SetBranchAddress("puWeight",       &puWeight    );
	tree->SetBranchStatus("HLTTau",            1); tree->SetBranchAddress("HLTTau",         &HLTTau      );
	tree->SetBranchStatus("HLTweightTau",      1); tree->SetBranchAddress("HLTweightTau",   &HLTweightTau);   // BUG
	tree->SetBranchStatus("SFMu",              1); tree->SetBranchAddress("SFMu",           &SFMu        );
	tree->SetBranchStatus("HLTMu",             1); tree->SetBranchAddress("HLTMu",          &HLTMu       );
	tree->SetBranchStatus("HLTweightMu",       1); tree->SetBranchAddress("HLTweightMu",    &HLTweightMu );
      }
    }
    
    if(DEBUG) cout << "-- has set entry list to skim (" << nEntries << " entries)" << endl;

    // Variables for the loop
    int treenum, iEntry, chainEntry, lastRun, lastEvent;
    treenum = iEntry = chainEntry = lastRun = lastEvent = -1;
    float weight=0;

    // Loop over skimmed entries to choose the best pair
    if(DEBUG) cout << "-- loop" << endl;
    for(Long64_t i=0 ; i<nEntries ; i++) {
      iEntry     = skim->GetEntryAndTree(i, treenum);
      chainEntry = iEntry + (tree->GetTreeOffset())[treenum];
      tree->GetEntry(chainEntry);

      if(DEBUG && i<1) cout << "---- var before pairIndex=" << var << endl
			    << endl;

      // reject all pairs of the evt except first passing selection (skim)
      if(run==lastRun && event==lastEvent) continue; 
      else {
	lastRun  = run;
	lastEvent= event;
      }

      if(DEBUG && i<1) cout << "RUN "           << run   
			    << "   EVENT "      << event << endl
			    << "sampleWeight="  << sampleWeight    << endl
			    << "puWeight="      << puWeight        << endl
			    << "puWeightHCP="   << puWeightHCP     << endl
			    << "puWeightD="     << puWeightD       << endl
			    << "puWeightDLow="  << puWeightDLow    << endl
			    << "puWeightDHigh=" << puWeightDHigh   << endl
			    << "---- var before pairIndex=" << var << endl
			    << endl;
      
       // compute the weight
      if(type.Contains("MC")) { // MC //
	weight = sampleWeight*SFTau*weightHepNup;
	if(version_.Contains("Soft")) {
	  if(     RUN=="ABC")  weight *= (puWeightHCP*HLTweightTauABC*HLTweightMuABCShift*SFMu_ABC*passL1etmCutABC);
	  else if(RUN=="D")    weight *= (puWeightDLow*puWeightDHigh*HLTTauD*HLTMuSoft*SFMu_D*passL1etmCut);
	  else if(RUN=="LTau") weight *= (puWeightDLow*puWeightDHigh*HLTTauD*HLTMuSoft*SFMu_D);
	}
	else {
	  if(     RUN=="ABC")  weight *= (puWeightHCP*HLTweightTauABC*HLTweightMuABC*SFMu_ABC);
	  else if(RUN=="D")    weight *= (puWeightD*HLTweightTauD*HLTweightMuD*SFMu_D);
	  else                 weight *= (puWeight*HLTweightTau*HLTweightMu*SFMu);
	}
      }
      else if(type.Contains("Embed")) { // MC //
	weight = embeddingWeight;
	if(version_.Contains("Soft")) {
	  if(     RUN=="ABC")  weight *= (HLTTauABC*HLTMuABCShift*passL1etmCutABC);
	  else if(RUN=="D")    weight *= (HLTTauD*HLTMuSoft*passL1etmCut);
	  else if(RUN=="LTau") weight *= (HLTTauD*HLTMuSoft);
	}
	else {
	  if(     RUN=="ABC")  weight *= (HLTTauABC*HLTMuABC);
	  else if(RUN=="D")    weight *= (HLTTauD*HLTMuD);
	  else                 weight *= (HLTTau*HLTMu);
	}
      }
      else if(type.Contains("Data")) weight = 1.0;
      else { cout << "ERROR : choose type MC, Embed, Data. Exit." << endl; return; }

      // fakeRate case
      if(type.Contains("FR")) weight *= fakeRate ;

      // fill the histogram
      if(DEBUG && i<1) cout << "---- var before fill histo=" << var << endl;

      h->Fill(var,weight);
    }
    if(DEBUG) cout << "-- end loop" << endl;
    // Let all branches be readable again
    tree->SetBranchStatus("*",  1); 

    // Scale the histogram, compute norm and err
    h->Scale(scaleFactor);
    normalization      = h->Integral();
    normalizationError = TMath::Sqrt(h->GetEntries()) * (normalization/h->GetEntries());
    if(verbose==0) h->Reset();

    tree->SetEntryList(0);
    //skim->Delete();
    skim->Reset();
    if(DEBUG) cout << "-- reset skim : " << skim->GetN() << "entries" << endl;
    //delete skim;
    //gDirectory->Delete("skim");
  }
  else{
    cout << "ERROR : drawHistogram => null pointers to tree or histogram. Exit." << endl;
    return;
  }
}

TArrayF createBins(int nBins_ = 80 ,
		   float xMin_ = 0.,
		   float xMax_ = 400.,
		   int& nBins = *(new int()),
		   string selection_   = "inclusive",
		   TString variable_   = "diTauVisMass",
		   //TString location    = "/home/llr/cms/veelken/ArunAnalysis/CMSSW_5_3_4_Sep12/src/Bianchi/Limits/bin/results/bins/"
		   TString location    = "/data_CMS/cms/htautau/HCP12/binning/"
		   ){

  // input txt file with bins
  ifstream is;

  TArrayF dummy(2);
  dummy[0] = xMin_; dummy[1] = xMax_;
  
  char* c = new char[10];
  is.open(Form(location+"/bins_muTau_%s_%s.txt",variable_.Data(), selection_.c_str())); 
  if(nBins_<0 &&  !is.good()){
    cout << "Bins file not found" << endl;
    return dummy;
  }

  int nBinsFromFile = 0;
  while (is.good())     
    {
      is.getline(c,999,',');     
      if (is.good()){
	nBinsFromFile++;
	//cout << c << endl;
      }
    }

  // choose the number of bins
  nBins =  nBins_>0 ? nBins_ : nBinsFromFile-1 ;
  TArrayF bins(nBins+1);
  cout << "Making histograms with " << nBins << " bins:" << endl;

  is.close();
  is.open(Form(location+"/bins_muTau_%s_%s.txt",variable_.Data(), selection_.c_str())); 
  
  nBinsFromFile = 0;

  if(nBins_>0){
    for( ; nBinsFromFile <= nBins ; nBinsFromFile++){
      bins[nBinsFromFile] =  xMin_ + nBinsFromFile*(xMax_-xMin_)/nBins_;
    }
  }
  else{
    while (is.good())  
      {
	is.getline(c,999,',');     
	if (is.good() && nBinsFromFile<=nBins) {
	  bins[nBinsFromFile] = atof(c);
	  cout << bins[nBinsFromFile] << ", " ;
	}
	nBinsFromFile++;
      }
    cout << endl;
  }

  return bins;

}

void evaluateWextrapolation(mapchain mapAllTrees, TString version_ = "",
			    TString RUN = "ABCD",
			    string sign = "OS", bool useFakeRate = false, string selection_ = "",
			    float& scaleFactorOS= *(new float()), 
			    float& OSWinSignalRegionDATA= *(new float()),   float& OSWinSignalRegionMC = *(new float()), 
			    float& OSWinSidebandRegionDATA= *(new float()), float& OSWinSidebandRegionMC = *(new float()), 
			    float& scaleFactorTTOS = *(new float()),
			    TH1F* hWMt=0, TString variable="",
			    float scaleFactor=0., float TTxsectionRatio=0., float lumiCorrFactor=0.,
			    float ExtrapolationFactorSidebandZDataMC = 0., float ExtrapolationFactorZDataMC = 0.,
			    float MutoTauCorrectionFactor = 0., float JtoTauCorrectionFactor = 0.,
			    float antiWsdb = 0., float antiWsgn = 0., bool useMt = true,
			    TCut sbinPZetaRelForWextrapolation = "",
			    TCut sbinPZetaRel = "",  TCut sbinRelPZetaRel = "",
			    TCut pZ="", TCut apZ="", TCut sbinPZetaRelInclusive="",
			    TCut sbinPZetaRelaIsoInclusive = "", TCut sbinPZetaRelaIso = "", 
			    TCut vbf="", TCut boost="", TCut zeroJet = ""){
  
  float Error = 0.; float ErrorW1 = 0.;   float ErrorW2 = 0.;
  drawHistogram("MC", version_, RUN,mapAllTrees["WJets"],variable, OSWinSignalRegionMC,   ErrorW1, scaleFactor, hWMt, sbinPZetaRelForWextrapolation&&pZ);
  drawHistogram("MC", version_, RUN,mapAllTrees["WJets"],variable, OSWinSidebandRegionMC, ErrorW2, scaleFactor, hWMt, sbinPZetaRelForWextrapolation&&apZ);
  scaleFactorOS      = OSWinSignalRegionMC>0 ? OSWinSidebandRegionMC/OSWinSignalRegionMC : 1.0 ;
  float scaleFactorOSError = scaleFactorOS*(ErrorW1/OSWinSignalRegionMC + ErrorW2/OSWinSidebandRegionMC);
  if(useMt)
    cout << "Extrap. factor for W " << sign << " : P(Mt>"     << antiWsdb << ")/P(Mt<"   << antiWsgn << ") ==> " <<OSWinSidebandRegionMC<<"/"<<OSWinSignalRegionMC<<" = "<< scaleFactorOS << " +/- " << scaleFactorOSError << endl;
  else
    cout << "Extrap. factor for W " << sign << " : P(pZeta<- "<< antiWsdb << ")/P(pZeta>"<< antiWsgn << ") ==> " << scaleFactorOS << " +/- " << scaleFactorOSError << endl;    

  // restore with full cut
  drawHistogram("MC", version_, RUN,mapAllTrees["WJets"],variable, OSWinSignalRegionMC,   ErrorW1, scaleFactor, hWMt, sbinPZetaRel&&pZ);
  // restore with full cut
  drawHistogram("MC", version_, RUN,mapAllTrees["WJets"],variable, OSWinSidebandRegionMC, ErrorW2, scaleFactor, hWMt, sbinPZetaRel&&apZ);
 

  float OSTTbarinSidebandRegionMC = 0.;
  drawHistogram("MC", version_, RUN,mapAllTrees["TTbar"],  variable,  OSTTbarinSidebandRegionMC,     Error, scaleFactor*TTxsectionRatio , hWMt, sbinPZetaRel&&apZ);


  TCut bTagCut; TCut bTagCutaIso;
  if(selection_.find("novbf")!=string::npos){
    bTagCut     = sbinPZetaRelInclusive    &&apZ&&TCut("nJets20BTagged>1")&&zeroJet;
    bTagCutaIso = sbinPZetaRelaIsoInclusive&&apZ&&TCut("nJets20BTagged>1")&&zeroJet;
  }
  else if(selection_.find("boost")!=string::npos){
    bTagCut     = sbinPZetaRelInclusive    &&apZ&&TCut("nJets20BTagged>1")&&boost;
    bTagCutaIso = sbinPZetaRelaIsoInclusive&&apZ&&TCut("nJets20BTagged>1")&&boost;
  }
  else if(selection_.find("vbf")!=string::npos && selection_.find("novbf")==string::npos){
    bTagCut     = sbinPZetaRelInclusive    &&apZ&&TCut("nJets20BTagged>0")&&vbf;
    bTagCutaIso = sbinPZetaRelaIsoInclusive&&apZ&&TCut("nJets20BTagged>0")&&vbf;
  }
  else{
    bTagCut     = sbinPZetaRel             &&apZ&&TCut("nJets20BTagged>1");
    bTagCutaIso = sbinPZetaRelaIso         &&apZ&&TCut("nJets20BTagged>1");
  }

  float OSTTbarinSidebandRegionBtagMC = 0.;
  drawHistogram("MC", version_, RUN,mapAllTrees["TTbar"], variable, OSTTbarinSidebandRegionBtagMC,  Error, scaleFactor*TTxsectionRatio, hWMt, bTagCut);
  float OSWinSidebandRegionBtagMC = 0.;
  drawHistogram("MC", version_, RUN,mapAllTrees["WJets"], variable, OSWinSidebandRegionBtagMC,      Error, scaleFactor                , hWMt, bTagCut);
  float OSOthersinSidebandRegionBtagMC = 0.;
  drawHistogram("MC", version_, RUN,mapAllTrees["Others"], variable, OSOthersinSidebandRegionBtagMC,Error, scaleFactor                , hWMt, bTagCut);
  float OSQCDinSidebandRegionBtag = 0.;
  drawHistogram("Data_FR", version_, RUN, mapAllTrees["Data"], variable, OSQCDinSidebandRegionBtag,       Error, 1.0                        , hWMt, bTagCutaIso);
  float OSWinSidebandRegionBtagAIsoMC = 0.;
  drawHistogram("MC_FR", version_, RUN,mapAllTrees["WJets"], variable, OSWinSidebandRegionBtagAIsoMC,  Error, scaleFactor        , hWMt, bTagCutaIso);
  float OSDatainSidebandRegionBtag = 0.;
  drawHistogram("Data", version_, RUN, mapAllTrees["Data"], variable, OSDatainSidebandRegionBtag,  Error, 1.0 , hWMt, bTagCut);

  scaleFactorTTOS = OSTTbarinSidebandRegionBtagMC>0 ? 
    (OSDatainSidebandRegionBtag-
     OSOthersinSidebandRegionBtagMC-
     OSWinSidebandRegionBtagMC-
     (OSQCDinSidebandRegionBtag-OSWinSidebandRegionBtagAIsoMC))/OSTTbarinSidebandRegionBtagMC : 1.0;
  cout << "Normalizing TTbar from sideband: " << OSTTbarinSidebandRegionBtagMC << " events expected from TTbar." << endl 
       << "From WJets " << OSWinSidebandRegionBtagMC <<  ", from QCD " << OSQCDinSidebandRegionBtag << " (of which " 
       << OSWinSidebandRegionBtagAIsoMC << " expected from anti-isolated W)"
       << ", from others " << OSOthersinSidebandRegionBtagMC << endl;
  cout << "Observed " << OSDatainSidebandRegionBtag << endl;
  cout << "====> scale factor for " << sign << " TTbar is " << scaleFactorTTOS << endl;
  if(scaleFactorTTOS<0){
    cout << "!!! scale factor is negative... set it to 1 !!!" << endl;
    scaleFactorTTOS = 1.0;
  }
  OSTTbarinSidebandRegionMC *= scaleFactorTTOS; // to comment
  cout << "Contribution from TTbar in " << sign << " is " << OSTTbarinSidebandRegionMC << endl;

  float OSOthersinSidebandRegionMC   = 0.;
  drawHistogram("MC", version_, RUN,mapAllTrees["Others"],    variable, OSOthersinSidebandRegionMC  ,Error,  scaleFactor , hWMt, sbinPZetaRel&&apZ);
  float OSDYtoTauinSidebandRegionMC  = 0.;
  if(useZDataMC)
    drawHistogram("MC", version_, RUN,mapAllTrees["DYToTauTau"],  variable, OSDYtoTauinSidebandRegionMC ,Error,  scaleFactor*lumiCorrFactor*ExtrapolationFactorSidebandZDataMC*ExtrapolationFactorZDataMC , hWMt, sbinPZetaRel&&apZ);
  else 
    drawHistogram("MC", version_, RUN,mapAllTrees["DYToTauTau"],  variable, OSDYtoTauinSidebandRegionMC ,Error,  scaleFactor*lumiCorrFactor, hWMt, sbinPZetaRel&&apZ);
  float OSDYJtoTauinSidebandRegionMC = 0.;
  drawHistogram("MC", version_, RUN,mapAllTrees["DYJtoTau"],  variable, OSDYJtoTauinSidebandRegionMC ,Error, scaleFactor*lumiCorrFactor*JtoTauCorrectionFactor , hWMt, sbinPZetaRel&&apZ);
  float OSDYMutoTauinSidebandRegionMC = 0.;
  drawHistogram("MC", version_, RUN,mapAllTrees["DYMutoTau"], variable, OSDYMutoTauinSidebandRegionMC ,Error,scaleFactor*lumiCorrFactor*MutoTauCorrectionFactor , hWMt, sbinPZetaRel&&apZ);

  float OSQCDinSidebandRegionData = 0.; float OSAIsoEventsinSidebandRegionData = 0.;
  drawHistogram("Data_FR", version_, RUN, mapAllTrees["Data"],  variable,        OSQCDinSidebandRegionData,   Error, 1.0         , hWMt, sbinPZetaRelaIso&&apZ);
  OSAIsoEventsinSidebandRegionData =  (OSQCDinSidebandRegionData/Error)*(OSQCDinSidebandRegionData/Error);
  float OSWinSidebandRegionAIsoMC  = 0.;float OSAIsoEventsWinSidebandRegionAIsoMC  = 0.;
  drawHistogram("MC_FR", version_, RUN,mapAllTrees["WJets"], variable,OSWinSidebandRegionAIsoMC,   Error, scaleFactor , hWMt, sbinPZetaRelaIso&&apZ);
  OSAIsoEventsWinSidebandRegionAIsoMC = (OSWinSidebandRegionAIsoMC/Error)*(OSWinSidebandRegionAIsoMC/Error)*scaleFactor; 

  drawHistogram("Data", version_, RUN, mapAllTrees["Data"], variable, OSWinSignalRegionDATA ,Error, 1.0 , hWMt, sbinPZetaRel&&apZ);
  cout << "Selected events in " << sign << " data from high Mt sideband " << OSWinSignalRegionDATA << endl;
  OSWinSignalRegionDATA -= OSTTbarinSidebandRegionMC;
  OSWinSignalRegionDATA -= OSOthersinSidebandRegionMC;
  OSWinSignalRegionDATA -= OSDYtoTauinSidebandRegionMC;
  OSWinSignalRegionDATA -= OSDYJtoTauinSidebandRegionMC;
  OSWinSignalRegionDATA -= OSDYMutoTauinSidebandRegionMC;
  if(useFakeRate) OSWinSignalRegionDATA -= (OSQCDinSidebandRegionData-OSWinSidebandRegionAIsoMC);
  OSWinSignalRegionDATA /= scaleFactorOS;
  cout << "- expected from TTbar          " << OSTTbarinSidebandRegionMC << endl;
  cout << "- expected from Others         " << OSOthersinSidebandRegionMC << endl;
  cout << "- expected from DY->tautau     " << OSDYtoTauinSidebandRegionMC << endl;
  cout << "- expected from DY->ll, l->tau " << OSDYMutoTauinSidebandRegionMC << endl;
  cout << "- expected from DY->ll, j->tau " << OSDYJtoTauinSidebandRegionMC  << endl;
  cout << "- expected from QCD " << OSQCDinSidebandRegionData << ", obtained from " << OSAIsoEventsinSidebandRegionData << " anti-isolated events " << endl;
  cout << "  (MC predicts " << OSWinSidebandRegionAIsoMC << " W events in the aIso region, from a total of " << OSAIsoEventsWinSidebandRegionAIsoMC << " events)" << endl;
  if(!useFakeRate) cout << " !!! QCD with fake-rate not subtracted !!!" << endl;
  cout << "W+jets in " << sign << " region is estimated to be " <<  OSWinSignalRegionDATA*scaleFactorOS << "/" << scaleFactorOS
       << " = " <<  OSWinSignalRegionDATA << endl;
  cout << " ==> the MC prediction is " << OSWinSignalRegionMC << " +/- " << ErrorW1 << endl;

  OSWinSidebandRegionDATA = OSWinSignalRegionDATA*scaleFactorOS;
}

void evaluateQCD(mapchain mapAllTrees, TString version_ = "",
		 TString RUN = "ABCD",
		 TH1F* qcdHisto = 0, TH1F* ssHisto = 0, bool evaluateWSS = true, string sign = "SS", bool useFakeRate = false, bool removeMtCut = false, string selection_ = "", 
		 float& SSQCDinSignalRegionDATAIncl_ = *(new float()), float& SSIsoToSSAIsoRatioQCD = *(new float()), float& scaleFactorTTSSIncl = *(new float()),
		 float& extrapFactorWSSIncl = *(new float()), 
		 float& SSWinSignalRegionDATAIncl = *(new float()), float& SSWinSignalRegionMCIncl = *(new float()),
		 float& SSWinSidebandRegionDATAIncl = *(new float()), float& SSWinSidebandRegionMCIncl = *(new float()),
		 TH1F* hExtrap=0, TString variable = "",
		 float scaleFactor=0., float TTxsectionRatio=0., float lumiCorrFactor = 0.,
		 float ExtrapolationFactorSidebandZDataMC = 0., float ExtrapolationFactorZDataMC = 0.,
		 float  MutoTauCorrectionFactor=0. , float JtoTauCorrectionFactor=0.,
		 float OStoSSRatioQCD = 0.,
		 float antiWsdb=0., float antiWsgn=0., bool useMt=true,
		 TCut sbin = "",
		 TCut sbinPZetaRelForWextrapolation = "",
		 TCut sbinPZetaRel ="", TCut pZ="", TCut apZ="", TCut sbinPZetaRelInclusive="", 
		 TCut sbinPZetaRelaIsoInclusive="", TCut sbinPZetaRelaIso="", TCut sbinPZetaRelaIsoSideband = "", 
		 TCut vbf="", TCut boost="", TCut zeroJet="",
		 bool subtractTT=true, bool subtractVV=true){

  if(evaluateWSS)
    evaluateWextrapolation(mapAllTrees, version_, RUN, sign, useFakeRate, selection_ , 
			   extrapFactorWSSIncl, 
			   SSWinSignalRegionDATAIncl,   SSWinSignalRegionMCIncl,
			   SSWinSidebandRegionDATAIncl, SSWinSidebandRegionMCIncl,
			   scaleFactorTTSSIncl,
			   hExtrap, variable,
			   scaleFactor, TTxsectionRatio,lumiCorrFactor,
			   ExtrapolationFactorSidebandZDataMC,ExtrapolationFactorZDataMC, 
			   MutoTauCorrectionFactor, JtoTauCorrectionFactor,
			   antiWsdb, antiWsgn, useMt,
			   sbinPZetaRelForWextrapolation,
			   sbinPZetaRel, sbinPZetaRel,
			   pZ, apZ, sbinPZetaRelInclusive, 
			   sbinPZetaRelaIsoInclusive, sbinPZetaRelaIso, vbf, boost, zeroJet);
  
  float Error = 0.;
  float SSQCDinSignalRegionDATAIncl = 0.;
  drawHistogram("Data", version_, RUN, mapAllTrees["Data"], variable,              SSQCDinSignalRegionDATAIncl,        Error, 1.0,         hExtrap, sbin, 1);
  if(qcdHisto!=0) qcdHisto->Add(hExtrap,  1.0);
  if(ssHisto !=0) ssHisto->Add( hExtrap,  1.0);

  float SSWJetsinSidebandRegionMCIncl    = 0.;
  drawHistogram("MC", version_, RUN,mapAllTrees["WJets"],     variable, SSWJetsinSidebandRegionMCIncl,      Error, scaleFactor*(SSWinSidebandRegionDATAIncl/SSWinSidebandRegionMCIncl), hExtrap, sbin,1);
  if(!removeMtCut){
    hExtrap->Scale(SSWinSignalRegionDATAIncl/hExtrap->Integral());
    SSWJetsinSidebandRegionMCIncl = SSWinSignalRegionDATAIncl;
  }
  if(qcdHisto!=0) qcdHisto->Add(hExtrap, -1.0);

  float SSTTbarinSidebandRegionMCIncl    = 0.;
  if(subtractTT) {
    drawHistogram("MC", version_, RUN,mapAllTrees["TTbar"],     variable, SSTTbarinSidebandRegionMCIncl,      Error, scaleFactor*TTxsectionRatio*scaleFactorTTSSIncl,       hExtrap, sbin,1);
    if(qcdHisto!=0) qcdHisto->Add(hExtrap, -1.0);
    if(ssHisto !=0) ssHisto->Add( hExtrap, -1.0);
  }

  float SSOthersinSidebandRegionMCIncl    = 0.;
  if(subtractVV) {
    drawHistogram("MC", version_, RUN,mapAllTrees["Others"],     variable, SSOthersinSidebandRegionMCIncl,     Error, scaleFactor,       hExtrap, sbin,1);
    if(qcdHisto!=0) qcdHisto->Add(hExtrap, -1.0);
    if(ssHisto !=0) ssHisto->Add( hExtrap, -1.0);
  }

  float SSDYMutoTauinSidebandRegionMCIncl = 0.;
  drawHistogram("MC", version_, RUN,mapAllTrees["DYMutoTau"], variable, SSDYMutoTauinSidebandRegionMCIncl,  Error, lumiCorrFactor*scaleFactor*MutoTauCorrectionFactor,    hExtrap, sbin,1);
  if(qcdHisto!=0) qcdHisto->Add(hExtrap, -1.0);
  if(ssHisto !=0) ssHisto->Add( hExtrap, -1.0);

  float SSDYtoTauinSidebandRegionMCIncl = 0.;
  if(useZDataMC)
    drawHistogram("MC", version_, RUN,mapAllTrees["DYToTauTau"],  variable, SSDYtoTauinSidebandRegionMCIncl,    Error, lumiCorrFactor*scaleFactor*ExtrapolationFactorZDataMC, hExtrap, sbin,1);
  else
    drawHistogram("MC", version_, RUN,mapAllTrees["DYToTauTau"],  variable, SSDYtoTauinSidebandRegionMCIncl,    Error, lumiCorrFactor*scaleFactor, hExtrap, sbin,1);
  if(qcdHisto!=0) qcdHisto->Add(hExtrap, -1.0);
  if(ssHisto !=0) ssHisto->Add( hExtrap, -1.0);

  float SSDYJtoTauinSidebandRegionMCIncl = 0.;
  drawHistogram("MC", version_, RUN,mapAllTrees["DYJtoTau"],  variable, SSDYJtoTauinSidebandRegionMCIncl,   Error, lumiCorrFactor*scaleFactor*JtoTauCorrectionFactor,     hExtrap, sbin,1);
  if(qcdHisto!=0) qcdHisto->Add(hExtrap, -1.0);
  if(ssHisto !=0) ssHisto->Add( hExtrap, -1.0);

  cout << "Selected events in inclusive " << sign << " data " << SSQCDinSignalRegionDATAIncl << endl;
  SSQCDinSignalRegionDATAIncl  -= SSWJetsinSidebandRegionMCIncl;
  SSQCDinSignalRegionDATAIncl  -= SSTTbarinSidebandRegionMCIncl;
  SSQCDinSignalRegionDATAIncl  -= SSOthersinSidebandRegionMCIncl;
  SSQCDinSignalRegionDATAIncl  -= SSDYMutoTauinSidebandRegionMCIncl;
  SSQCDinSignalRegionDATAIncl  -= SSDYJtoTauinSidebandRegionMCIncl;
  SSQCDinSignalRegionDATAIncl  -= SSDYtoTauinSidebandRegionMCIncl;
  SSQCDinSignalRegionDATAIncl *= OStoSSRatioQCD;
  if(qcdHisto!=0) qcdHisto->Scale(OStoSSRatioQCD);

  cout << "- expected from WJets          " << SSWJetsinSidebandRegionMCIncl << endl;
  cout << "- expected from TTbar          " << SSTTbarinSidebandRegionMCIncl << endl;
  cout << "- expected from Others         " << SSOthersinSidebandRegionMCIncl << endl;
  cout << "- expected from DY->tautau     " << SSDYtoTauinSidebandRegionMCIncl << endl;
  cout << "- expected from DY->ll, l->tau " << SSDYMutoTauinSidebandRegionMCIncl << endl;
  cout << "- expected from DY->ll, j->tau " << SSDYJtoTauinSidebandRegionMCIncl  << endl;
  cout << "QCD in inclusive SS region is estimated to be " << SSQCDinSignalRegionDATAIncl/OStoSSRatioQCD  << "*" << OStoSSRatioQCD
       << " = " <<  SSQCDinSignalRegionDATAIncl << endl;
  SSQCDinSignalRegionDATAIncl_ = SSQCDinSignalRegionDATAIncl;

  float SSQCDinSignalRegionDATAInclaIso = 0.;
  drawHistogram("Data", version_, RUN, mapAllTrees["Data"], variable, SSQCDinSignalRegionDATAInclaIso,    Error, 1.0, hExtrap, sbinPZetaRelaIsoSideband&&pZ);
  float SSWinSignalRegionMCInclaIso   = 0.;
  drawHistogram("MC", version_, RUN,mapAllTrees["WJets"], variable, SSWinSignalRegionMCInclaIso,       Error,   scaleFactor*(SSWinSignalRegionDATAIncl/SSWinSignalRegionMCIncl) , hExtrap, sbinPZetaRelaIsoSideband&&pZ);
  float SSTTbarinSignalRegionMCInclaIso   = 0.;
  drawHistogram("MC", version_, RUN,mapAllTrees["TTbar"], variable, SSTTbarinSignalRegionMCInclaIso,   Error,   scaleFactor*scaleFactorTTSSIncl , hExtrap, sbinPZetaRelaIsoSideband&&pZ);
  cout << "Anti-isolated " << sign << " events inclusive = " << SSQCDinSignalRegionDATAInclaIso << ", of which we expect "
       << SSWinSignalRegionMCInclaIso << " from W+jets " << " and " << SSTTbarinSignalRegionMCInclaIso << " from TTbar" << endl;

  SSIsoToSSAIsoRatioQCD = (SSQCDinSignalRegionDATAIncl)/(SSQCDinSignalRegionDATAInclaIso-SSWinSignalRegionMCInclaIso-SSTTbarinSignalRegionMCInclaIso) ;
  cout << "The extrapolation factor Iso<0.1 / Iso>0.2 is " << SSIsoToSSAIsoRatioQCD << endl;

}


void cleanQCDHisto(mapchain mapAllTrees, TString version_ = "",
		   TString RUN = "ABCD",
		   bool removeW = true, TH1F* hCleaner = 0, TH1F* hLooseIso = 0, TString variable = "",
		   float scaleFactor = 0., float WJetsCorrectionFactor=0., float TTbarCorrectionFactor=0.,
		   float MutoTauCorrectionFactor=0., float JtoTauCorrectionFactor=0., float DYtoTauTauCorrectionFactor=0.,
		   TCut sbinSSlIso1 = ""){

  if(hLooseIso==0) return;

  cout << "Cleaning QCD histo with relaxed isolation from backgrounds..." << endl;

  float Error = 0.;
  float totalEvents = hLooseIso->Integral();

  float NormalizationWinLooseRegion = 0.;
  if(removeW){
    drawHistogram("MC", version_, RUN,mapAllTrees["WJets"],variable,      NormalizationWinLooseRegion,     Error, scaleFactor*WJetsCorrectionFactor, hCleaner, sbinSSlIso1 ,1);
    hLooseIso->Add(hCleaner,-1.0);
  }
  float NormalizationTTbarinLooseRegion = 0.;
  drawHistogram("MC", version_, RUN,mapAllTrees["TTbar"], variable,     NormalizationTTbarinLooseRegion, Error, scaleFactor*TTbarCorrectionFactor, hCleaner, sbinSSlIso1,1);
  hLooseIso->Add(hCleaner,-1.0);
  float NormalizationOthersinLooseRegion = 0.;
  drawHistogram("MC", version_, RUN,mapAllTrees["Others"], variable,    NormalizationOthersinLooseRegion,Error, scaleFactor, hCleaner, sbinSSlIso1,1);
  hLooseIso->Add(hCleaner,-1.0);
  float  NormalizationDYMutotauinLooseRegion= 0.;
  drawHistogram("MC", version_, RUN,mapAllTrees["DYMutoTau"], variable, NormalizationDYMutotauinLooseRegion,  Error,  scaleFactor*MutoTauCorrectionFactor,    hCleaner  , sbinSSlIso1,1);
  hLooseIso->Add(hCleaner,-1.0);
  float  NormalizationDYJtotauinLooseRegion= 0.;
  drawHistogram("MC", version_, RUN,mapAllTrees["DYJtoTau"], variable,  NormalizationDYJtotauinLooseRegion,   Error,  scaleFactor*JtoTauCorrectionFactor,     hCleaner  , sbinSSlIso1,1);
  hLooseIso->Add(hCleaner,-1.0);
  float NormalizationDYTauTauinLooseRegion = 0.;
  drawHistogram("MC", version_, RUN,mapAllTrees["DYToTauTau"],  variable, NormalizationDYTauTauinLooseRegion,    Error, scaleFactor*DYtoTauTauCorrectionFactor, hCleaner  , sbinSSlIso1,1);
  hLooseIso->Add(hCleaner,-1.0);

  float totalRemoved = NormalizationWinLooseRegion+NormalizationTTbarinLooseRegion+NormalizationOthersinLooseRegion+
    NormalizationDYMutotauinLooseRegion+NormalizationDYJtotauinLooseRegion+NormalizationDYTauTauinLooseRegion;
  cout << " ==> removed " << totalRemoved << " events from a total of " << totalEvents << " (" << totalRemoved/totalEvents*100 << " %)" << endl;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////7

void plotMuTau( Int_t mH_           = 120,
		Int_t useEmbedding_ = 0,
		string selection_   = "inclusive",
		string analysis_    = "",		  
		TString variable_   = "diTauVisMass",
		TString XTitle_     = "full mass",
		TString Unities_    = "GeV",
		TString outputDir   = "test",
		Int_t nBins_ = 80, Float_t xMin_=0, Float_t xMax_=400,
		Float_t magnifySgn_ = 1.0,
		Float_t hltEff_     = 1.0,
		Int_t logy_         = 0,
		Float_t maxY_       = 1.2,
		TString RUN         = "ABCD",
		TString version_    = "Moriond",
		//TString location    = "/home/llr/cms/veelken/ArunAnalysis/CMSSW_5_3_4_p2_topup/src/Bianchi/Limits/bin/results/"
		//TString location    = "/home/llr/cms/ivo/HTauTauAnalysis/CMSSW_5_3_4_p2_Trees/src/LLRAnalysis/Limits/bin/results/"
		TString location    = "/home/llr/cms/ndaci/WorkArea/HTauTau/Analysis/CMSSW_534p2_Spring13_Trees/src/LLRAnalysis/Limits/bin/results/"
		) 
{   

  cout << endl;
  cout << "@@@@@@@@@@@@@@@@@@ Category  = " << selection_     <<  endl;
  cout << "@@@@@@@@@@@@@@@@@@ Variable  = " << string(variable_.Data()) <<  endl;
  cout << endl;

  ostringstream ossiTmH("");
  ossiTmH << mH_ ;
  TString TmH_ = ossiTmH.str();

  const int nProd=3;
  const int nMasses=15;
  TString nameProd[nProd]={"GGFH","VBFH","VH"};
  TString nameMasses[nMasses]={"90","95","100","105","110","115","120","125","130","135","140","145","150","155","160"};
  int hMasses[nMasses]={90,95,100,105,110,115,120,125,130,135,140,145,150,155,160};

  ofstream out(Form(location+"/%s/yields/yieldsMuTau_mH%d_%s_%s.txt",
		    outputDir.Data(),mH_,selection_.c_str(), analysis_.c_str() ),ios_base::out); 
  out.precision(5);
  int nBins = nBins_;
  TArrayF bins = createBins(nBins_, xMin_, xMax_, nBins, selection_, variable_);

  // LUMINOSITY //
  float Lumi, LumiABC, LumiD, LumiSoftABC;
  LumiABC     = 791.872 + 4434.0 + 495.003 + 6174 + 206.196 ; // 2012ABC 
  LumiSoftABC = LumiABC;                                      // needs modification ?
  LumiD   = 7274;                                             // 2012D 

  if(RUN=="ABC")    Lumi = LumiABC;
  else if(RUN=="D") Lumi = LumiD;
  else              Lumi = LumiABC + LumiD;

  if(version_.Contains("SoftABC"))             Lumi = LumiSoftABC;

  /////////////////

  float lumiCorrFactor                     = 1 ;    //= (1-0.056);
  float TTxsectionRatio                    = 0.98; //lumiCorrFactor*(165.8/157.5) ;
  float OStoSSRatioQCD                     = 1.06;
  float SSIsoToSSAIsoRatioQCD              = 1.0;
  float MutoTauCorrectionFactor            = 0.976;
  float JtoTauCorrectionFactor             = 0.976;
  float ExtrapolationFactorZ               = 1.0;
  float ErrorExtrapolationFactorZ          = 1.0;
  float ExtrapolationFactorZDataMC         = 1.0;
  float ExtrapolationFactorSidebandZDataMC = 1.0;
  float ExtrapolationFactorZFromSideband   = 1.0;
  float scaleFactorTTOS                    = 1.0;
  float scaleFactorTTSS                    = 1.0;
  float scaleFactorTTSSIncl                = 1.0;

  cout << endl;
  cout << "Input: " << endl;
  cout << " > Lumi           = " << Lumi/1000. << " fb-1" << endl;
  cout << " > DY xsection SF = " << lumiCorrFactor << endl;
  cout << " > TTbar SF       = " << TTxsectionRatio << endl;
  cout << " > QCD OS/SS SF   = " << OStoSSRatioQCD << endl;
  cout << " > J->tau SF      = " << JtoTauCorrectionFactor << endl;
  cout << " > Mu->tau SF     = " << MutoTauCorrectionFactor << endl;
  cout << endl;

  /////////////////  change SVfit mass here ///////////////////

  string variableStr = "";
  TString variable(variableStr.c_str());
  variable = variable_;

  //////////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////////

  bool useMt      = true;
  string antiWcut = useMt ? "MtLeg1MVA" : "-(pZetaMVA-1.5*pZetaVisMVA)" ; 
  float antiWsgn  = useMt ? 20. :  20. ;
  float antiWsdb  = useMt ? 70. :  40. ; 

  bool use2Dcut   = false;
  if( use2Dcut ){
    antiWcut = "!(MtLeg1MVA<40 && (pZetaMVA-1.5*pZetaVisMVA)>-20)";
    antiWsgn = 0.5;
    antiWsdb = 0.5;
  }

  //////////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////////

  TCanvas *c1 = new TCanvas("c1","",5,30,650,600);
  c1->SetGrid(0,0);
  c1->SetFillStyle(4000);
  c1->SetFillColor(10);
  c1->SetTicky();
  c1->SetObjectStat(0);
  c1->SetLogy(logy_);

  TPad* pad1 = new TPad("pad1DEta","",0.05,0.22,0.96,0.97);
  TPad* pad2 = new TPad("pad2DEta","",0.05,0.02,0.96,0.20);
 
  pad1->SetFillColor(0);
  pad2->SetFillColor(0);
  pad1->Draw();
  pad2->Draw();

  pad1->cd();
  pad1->SetLogy(logy_);
  gStyle->SetOptStat(0);
  gStyle->SetTitleFillColor(0);
  gStyle->SetCanvasBorderMode(0);
  gStyle->SetCanvasColor(0);
  gStyle->SetPadBorderMode(0);
  gStyle->SetPadColor(0);
  gStyle->SetTitleFillColor(0);
  gStyle->SetTitleBorderSize(0);
  gStyle->SetTitleH(0.07);
  gStyle->SetTitleFontSize(0.1);
  gStyle->SetTitleStyle(0);
  gStyle->SetTitleOffset(1.3,"y");

  TLegend* leg = new TLegend(0.63,0.48,0.85,0.85,NULL,"brNDC");
  leg->SetFillStyle(0);
  leg->SetBorderSize(0);
  leg->SetFillColor(10);
  leg->SetTextSize(0.03);
  leg->SetHeader(Form("#splitline{CMS Preliminary #sqrt{s}=8 TeV}{%.1f fb^{-1} #tau_{#mu}#tau_{had}}", Lumi/1000. ));

  THStack* aStack = new THStack("aStack","");

  TH1F* hSiml     = new TH1F( "hSiml"   ,"all"               , nBins , bins.GetArray());
  TH1F* hSgn      = new TH1F( "hSgn "   ,"vbf+ggf"           , nBins , bins.GetArray());         hSgn->SetFillColor(0); hSgn->SetLineColor(kBlue);hSgn->SetLineWidth(2);hSgn->SetLineStyle(kDashed);
  TH1F* hSgn1     = new TH1F( "hSgn1"   ,"vbf"               , nBins , bins.GetArray());         hSgn1->SetLineWidth(2);
  TH1F* hSgn2     = new TH1F( "hSgn2"   ,"ggf"               , nBins , bins.GetArray());         hSgn2->SetLineWidth(2);
  TH1F* hSgn3     = new TH1F( "hSgn3"   ,"vh"                , nBins , bins.GetArray());         hSgn3->SetLineWidth(2);
  TH1F* hData     = new TH1F( "hData"   ,"        "          , nBins , bins.GetArray());         hData->SetMarkerStyle(20);hData->SetMarkerSize(1.2);hData->SetMarkerColor(kBlack);hData->SetLineColor(kBlack);hData->SetXTitle(XTitle_+" ("+Unities_+")");hData->SetYTitle(Form(" Events/(%.1f %s)", hData->GetBinWidth(1), Unities_.Data() ) );hData->SetTitleSize(0.04,"X");hData->SetTitleSize(0.05,"Y");hData->SetTitleOffset(0.95,"Y");
  TH1F* hDataEmb  = new TH1F( "hDataEmb","Embedded"          , nBins , bins.GetArray());         hDataEmb->SetFillColor(kOrange-4);
  TH1F* hW        = new TH1F( "hW"      ,"W+jets"            , nBins , bins.GetArray());         hW->SetFillColor(kRed+2);
  TH1F* hWSS      = new TH1F( "hWSS"    ,"W+jets SS"         , nBins , bins.GetArray());
  TH1F* hWMinusSS = new TH1F( "hWMinusSS","W+jets - SS"      , nBins , bins.GetArray());         hWMinusSS->SetFillColor(kRed+2);
  TH1F* hW3Jets   = new TH1F( "hW3Jets" ,"W+3jets"           , nBins , bins.GetArray());         hW3Jets->SetFillColor(kRed+2);
  TH1F* hEWK      = new TH1F( "hEWK"    ,"EWK"               , nBins , bins.GetArray());         hEWK->SetFillColor(kRed+2);
  TH1F* hZtt      = new TH1F( "hZtt"    ,"Ztautau"           , nBins , bins.GetArray());         hZtt->SetFillColor(kOrange-4);
  TH1F* hZmm      = new TH1F( "hZmm"    ,"Z+jets, mu->tau"   , nBins , bins.GetArray());         hZmm->SetFillColor(kBlue-2);
  TH1F* hZmmLoose = new TH1F( "hZmmLoose","Z+jets, mu->tau"  , nBins , bins.GetArray());         hZmmLoose->SetFillColor(kBlue-2);
  TH1F* hZmj      = new TH1F( "hZmj"    ,"Z+jets, jet to tau", nBins , bins.GetArray());         hZmj->SetFillColor(kBlue-2);
  TH1F* hZmjLoose = new TH1F( "hZmjLoose","Z+jets, jet->tau" , nBins , bins.GetArray());         hZmjLoose->SetFillColor(kBlue-2);
  TH1F* hZfakes   = new TH1F( "hZfakes" ,"Z+jets, jet to tau", nBins , bins.GetArray());         hZfakes->SetFillColor(kBlue-2);
  TH1F* hTTb      = new TH1F( "hTTb"    ,"ttbar"             , nBins , bins.GetArray());         hTTb->SetFillColor(kBlue-8); 
  TH1F* hSS       = new TH1F( "hSS"     ,"same-sign"         , nBins , bins.GetArray());         hSS->SetFillColor(kMagenta-10);
  TH1F* hSSLooseVBF= new TH1F("hSSLooseVBF" ,"same-sign"     , nBins , bins.GetArray());         hSSLooseVBF->SetFillColor(kMagenta-10);
  TH1F* hLooseIso1= new TH1F( "hLooseIso1","Loose Iso"       , nBins , bins.GetArray());
  TH1F* hLooseIso2= new TH1F( "hLooseIso2","Loose Iso"       , nBins , bins.GetArray());
  TH1F* hLooseIso3= new TH1F( "hLooseIso3","Loose Iso"       , nBins , bins.GetArray());
  TH1F* hAntiIso  = new TH1F( "hAntiIso","Anti Iso"          , nBins , bins.GetArray());
  TH1F* hAntiIsoFR= new TH1F( "hAntiIsoFR","Anti Iso * FR"   , nBins , bins.GetArray());
  TH1F* hVV       = new TH1F( "hVV"     ,"Diboson"           , nBins , bins.GetArray());         hVV->SetFillColor(kRed+2);
  TH1F* hW3JetsMediumTauIso               = new TH1F( "hW3JetsMediumTauIso" ,  "W+3jets (medium tau-iso)"                  , nBins , bins.GetArray());
  TH1F* hW3JetsLooseTauIso                = new TH1F( "hW3JetsLooseTauIso" ,  "W+3jets (loose tau-iso)"                    , nBins , bins.GetArray());
  TH1F* hW3JetsMediumTauIsoRelVBF         = new TH1F( "hW3JetsMediumTauIsoRelVBF" ,  "W+3jets (medium tau-iso)"            , nBins , bins.GetArray());
  TH1F* hW3JetsMediumTauIsoRelVBFMinusSS  = new TH1F( "hW3JetsMediumTauIsoRelVBFMinusSS" ,  "W+3jets (medium tau-iso)-SS"  , nBins , bins.GetArray());
  TH1F* hWLooseBTag                       = new TH1F( "hWLooseBTag" ,  "W+jets (Loose b-Tag)", nBins , bins.GetArray());

  // QCD estimations
  TH1F* hQCD                           = new TH1F( "hQCD"         ,"QCD full vbf"      , nBins , bins.GetArray());                                    hQCD        ->SetFillColor(kMagenta-10);
  TH1F* hQCDrelaxVBF                   = new TH1F( "hQCDRelaxVBF" ,"QCD relax vbf"     , nBins , bins.GetArray());                                    hQCDrelaxVBF->SetFillColor(kMagenta-10); //ND
  //
  TH1F* hDataAntiIsoLooseTauIsoFullVBF = new TH1F( "hDataAntiIsoLooseTauIsoFullVBF","data anti-iso,loose tau-iso,full vbf",  nBins, bins.GetArray()); hDataAntiIsoLooseTauIsoFullVBF ->SetFillColor(kMagenta-10);//ND
  TH1F* hDataAntiIsoLooseTauIsoRelaxVBF= new TH1F( "hDataAntiIsoLooseTauIsoRelaxVBF","data anti-iso,loose tau-iso,relax vbf",nBins, bins.GetArray()); hDataAntiIsoLooseTauIsoRelaxVBF->SetFillColor(kMagenta-10);//ND
  //
  TH1F* hDataAntiIsoLooseTauIso        = new TH1F( "hDataAntiIsoLooseTauIso"   ,"data anti-iso, loose tau-iso,relax vbf", nBins, bins.GetArray()); hDataAntiIsoLooseTauIso   ->SetFillColor(kMagenta-10);
  TH1F* hDataAntiIsoLooseTauIsoQCD     = new TH1F( "hDataAntiIsoLooseTauIsoQCD"   ,"data anti-iso, norm QCD"            , nBins, bins.GetArray()); hDataAntiIsoLooseTauIsoQCD->SetFillColor(kMagenta-10);

  TH1F* hSignal[nProd][nMasses];
  for(int iP=0 ; iP<nProd ; iP++) {
    for(int iM=0 ; iM<nMasses ; iM++) {
      hSignal[iP][iM] = new TH1F("h"+nameProd[iP]+nameMasses[iM], nameProd[iP]+nameMasses[iM], nBins , bins.GetArray());
      hSignal[iP][iM]->SetLineWidth(2);
    }
  }

  vector<string> SUSYhistos;
  //SUSYhistos.push_back("SUSYGG90"); SUSYhistos.push_back("SUSYGG100"); SUSYhistos.push_back("SUSYGG120"); SUSYhistos.push_back("SUSYGG130");
  //SUSYhistos.push_back("SUSYGG140");SUSYhistos.push_back("SUSYGG160"); SUSYhistos.push_back("SUSYGG180"); SUSYhistos.push_back("SUSYGG200");
  //SUSYhistos.push_back("SUSYGG250");SUSYhistos.push_back("SUSYGG300"); SUSYhistos.push_back("SUSYGG350"); SUSYhistos.push_back("SUSYGG400");
  //SUSYhistos.push_back("SUSYGG450");SUSYhistos.push_back("SUSYGG500"); SUSYhistos.push_back("SUSYGG600"); SUSYhistos.push_back("SUSYGG700");
  //SUSYhistos.push_back("SUSYGG800");SUSYhistos.push_back("SUSYGG900"); SUSYhistos.push_back("SUSYBB90");  SUSYhistos.push_back("SUSYBB100");
  //SUSYhistos.push_back("SUSYBB120");SUSYhistos.push_back("SUSYBB130"); SUSYhistos.push_back("SUSYBB140"); SUSYhistos.push_back("SUSYBB160");
  //SUSYhistos.push_back("SUSYBB180");SUSYhistos.push_back("SUSYBB200"); SUSYhistos.push_back("SUSYBB250"); SUSYhistos.push_back("SUSYBB300");
  //SUSYhistos.push_back("SUSYBB350");SUSYhistos.push_back("SUSYBB400"); SUSYhistos.push_back("SUSYBB450"); SUSYhistos.push_back("SUSYBB500");
  //SUSYhistos.push_back("SUSYBB600");SUSYhistos.push_back("SUSYBB700"); SUSYhistos.push_back("SUSYBB800"); SUSYhistos.push_back("SUSYBB900");
  std::map<string,TH1F*> mapSUSYhistos;
  for(unsigned int i = 0; i < SUSYhistos.size() ; i++){
    mapSUSYhistos.insert( make_pair(SUSYhistos[i], 
				    new TH1F(Form("h%s",SUSYhistos[i].c_str()) ,
					     Form("%s", SUSYhistos[i].c_str()), 
					     nBins , bins.GetArray()) ) 
			  );
  }

  TH1F* hParameters   = new TH1F( "hParameters", "" ,30, 0, 30);
 ///////////////////////////////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////////////////////////////
  /*
  // get the FR-file
  string scaleFactMu         = "";
  string scaleFactMuUp       = "";
  string scaleFactMuDown     = "";
  string scaleFactTauQCD     = "";
  string scaleFactTauQCDUp   = "";
  string scaleFactTauQCDDown = "";
  string scaleFactTauW       = "";
  string scaleFactTauWUp     = "";
  string scaleFactTauWDown   = "";

  createStringsIsoFakeRate("FakeRate.root", scaleFactMu,     scaleFactMuUp,     scaleFactMuDown,     "ptL1", "FakeRate",    "_",   "MuTau_Mu_ptL1_incl");
  createStringsIsoFakeRate("FakeRate.root", scaleFactTauQCD, scaleFactTauQCDUp, scaleFactTauQCDDown, "ptL2", "FakeRateQCD", "QCD_","MuTau_Tau_ptL2_QCDSS02_WSS60_incl");
  createStringsIsoFakeRate("FakeRate.root", scaleFactTauW,   scaleFactTauWUp,   scaleFactTauWDown,   "ptL2", "FakeRateW",   "W_",  "MuTau_Tau_ptL2_QCDOS02_WOS60_incl");
  */

  ///////////////////////////////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////////////////////////////

  TString pathToFile = "/data_CMS/cms/htautau/PostMoriond/NTUPLES_JetIdFix/MuTau/temp/";

  TString Tanalysis_(analysis_);
  TString fileAnalysis = Tanalysis_;
  if(Tanalysis_=="") fileAnalysis = "nominal";

  // DATA //
  TChain *data = new TChain("outTreePtOrd");
  if(RUN.Contains("ABC")) {
    data->Add(pathToFile+"/nTupleRun2012A*Data_MuTau.root");
    data->Add(pathToFile+"/nTupleRun2012B*Data_MuTau.root");
    data->Add(pathToFile+"/nTupleRun2012C*Data_MuTau.root");
  }
  if(RUN.Contains("D")) {
    data->Add(pathToFile+"/nTupleRun2012D*Data_MuTau.root");
  }

  if(!data) cout << "### DATA NTUPLE NOT FOUND ###" << endl;

  // EMBEDDED //
  TString treeEmbedded,fileAnalysisEmbedded;
  if(Tanalysis_.Contains("TauUp") || Tanalysis_.Contains("TauDown") ) { //|| Tanalysis_.Contains("MuUp") || Tanalysis_.Contains("MuDown") )
    treeEmbedded = "outTreePtOrd"+Tanalysis_;
    fileAnalysisEmbedded = fileAnalysis ;
  }
  else {
    treeEmbedded = "outTreePtOrd";
    fileAnalysisEmbedded = "nominal";
  }
  TChain *dataEmbedded = new TChain(treeEmbedded);
  //
  if(RUN.Contains("ABC")) {
    dataEmbedded->Add(pathToFile+"/nTupleRun2012A*Embedded_MuTau_"+fileAnalysisEmbedded+".root");
    dataEmbedded->Add(pathToFile+"/nTupleRun2012B*Embedded_MuTau_"+fileAnalysisEmbedded+".root");
    dataEmbedded->Add(pathToFile+"/nTupleRun2012C*Embedded_MuTau_"+fileAnalysisEmbedded+".root");
  }
  if(RUN.Contains("D")) dataEmbedded->Add(pathToFile+"/nTupleRun2012D*Embedded_MuTau_"+fileAnalysisEmbedded+".root");

  if(!dataEmbedded) cout << "### EMBEDDED NTUPLE NOT FOUND ###" << endl;

  // BACKGROUNDS //
  TString treeMC;
  if(Tanalysis_.Contains("TauUp") || Tanalysis_.Contains("TauDown") || Tanalysis_.Contains("JetUp") || Tanalysis_.Contains("JetDown") )
    treeMC = "outTreePtOrd"+Tanalysis_;
  else treeMC = "outTreePtOrd";
  //
  TChain *backgroundDY         = new TChain(treeMC);
  TChain *backgroundDYTauTau   = new TChain(treeMC);
  TChain *backgroundDYMutoTau  = new TChain(treeMC);
  TChain *backgroundDYJtoTau   = new TChain(treeMC);
  TChain *backgroundTTbar      = new TChain(treeMC);
  TChain *backgroundOthers     = new TChain(treeMC);
  TChain *backgroundWJets      = new TChain(treeMC);
  TChain *backgroundW3Jets     = new TChain(treeMC);
  //
  backgroundDY      ->Add(pathToFile+"nTupleDYJets_MuTau_"+fileAnalysis+".root");
  backgroundTTbar   ->Add(pathToFile+"nTupleTTJets_MuTau_"+fileAnalysis+".root");
  //
  backgroundOthers  ->Add(pathToFile+"nTupleT-tW_MuTau_"+fileAnalysis+".root");
  backgroundOthers  ->Add(pathToFile+"nTupleTbar-tW_MuTau_"+fileAnalysis+".root");
  backgroundOthers  ->Add(pathToFile+"nTupleWWJetsTo2L2Nu_MuTau_"+fileAnalysis+".root");
  backgroundOthers  ->Add(pathToFile+"nTupleWZJetsTo2L2Q_MuTau_"+fileAnalysis+".root");
  backgroundOthers  ->Add(pathToFile+"nTupleWZJetsTo3LNu_MuTau_"+fileAnalysis+".root");
  backgroundOthers  ->Add(pathToFile+"nTupleZZJetsTo2L2Nu_MuTau_"+fileAnalysis+".root");
  backgroundOthers  ->Add(pathToFile+"nTupleZZJetsTo2L2Q_MuTau_"+fileAnalysis+".root");
  backgroundOthers  ->Add(pathToFile+"nTupleZZJetsTo4L_MuTau_"+fileAnalysis+".root");
  //
  backgroundWJets   ->Add(pathToFile+"nTupleWJets-p1_MuTau_"+fileAnalysis+".root");
  backgroundWJets   ->Add(pathToFile+"nTupleWJets-p2_MuTau_"+fileAnalysis+".root");
  backgroundWJets   ->Add(pathToFile+"nTupleWJets1Jets_MuTau_"+fileAnalysis+".root");
  backgroundWJets   ->Add(pathToFile+"nTupleWJets2Jets_MuTau_"+fileAnalysis+".root");
  backgroundWJets   ->Add(pathToFile+"nTupleWJets3Jets_MuTau_"+fileAnalysis+".root");
  backgroundWJets   ->Add(pathToFile+"nTupleWJets4Jets_MuTau_"+fileAnalysis+".root");
  //backgroundW3Jets  ->Add(pathToFile+"nTupleWJets3Jets_MuTau_"+fileAnalysis+".root");
  backgroundW3Jets = backgroundWJets;

  if(!backgroundDY)    cout << "###  NTUPLE DY NOT FOUND ###" << endl;  
  if(!backgroundTTbar) cout << "###  NTUPLE TT NOT FOUND ###" << endl;  
  if(!backgroundOthers)cout << "###  NTUPLE VVt NOT FOUND ###" << endl;  
  if(!backgroundWJets) cout << "###  NTUPLE W NOT FOUND ###" << endl;  
  if(!backgroundW3Jets)cout << "###  NTUPLE W3 NOT FOUND ###" << endl;  

  TChain *signal[nProd][nMasses];
  for(int iP=0 ; iP<nProd ; iP++) {
    for(int iM=0 ; iM<nMasses ; iM++) {
      signal[iP][iM] = new TChain(treeMC);
      signal[iP][iM]->Add(pathToFile+"/nTuple"+nameProd[iP]+nameMasses[iM]+"_MuTau_"+fileAnalysis+".root");
      if(!signal[iP][iM])cout << "###  NTUPLE Signal " << nameProd[iP]+nameMasses[iM] << " NOT FOUND ###" << endl;  
    }
  }
  
//   TChain *signalSusy[nProdS][nMassesS];
//   for(int iP=0 ; iP<nProdS ; iP++) {
//     for(int iM=0 ; iM<nMasses ; iM++) {
//       signalSusy[iP][iM] = new TChain(treeMC);
//       signalSusy[iP][iM]->Add(pathToFile+"/nTupleSUSY"+nameProdS[iP]+nameMassesS[iM]+"_MuTau_"+fileAnalysis+".root");
//     }
//   }

  // Split DY into 3 sub-samples (TauTau, MuToTau, JetToTau)
  TFile *dummy1;
  if(DOSPLIT) {
    cout << "SPLIT DY SAMPLE ON THE FLY" << endl;
    //
    dummy1 = new TFile("dummy2.root","RECREATE");
    //
    cout << "Now copying g/Z -> tau+ tau- " << endl;
    backgroundDYTauTau = (TChain*)backgroundDY->CopyTree("abs(genDecay)==(23*15)");                 // g/Z -> tau+ tau-
    //
    cout << "Now copying g/Z -> mu+mu- mu->tau" << endl;
    backgroundDYMutoTau = (TChain*)backgroundDY->CopyTree("abs(genDecay)!=(23*15) &&  leptFakeTau"); // g/Z -> mu+mu- mu->tau
    //
    cout << "Now copying g/Z -> mu+mu- jet->tau" << endl;
    backgroundDYJtoTau  = (TChain*)backgroundDY->CopyTree("abs(genDecay)!=(23*15) && !leptFakeTau"); // g/Z -> mu+mu- jet->tau
  }
  else {
    cout << "USE DY SEPARATE SUB-SAMPLES" << endl;
    //
    backgroundDYTauTau  ->Add(pathToFile+"/nTupleDYJetsTauTau_MuTau_"+fileAnalysis+".root");
    backgroundDYMutoTau ->Add(pathToFile+"/nTupleDYJetsMuToTau_MuTau_"+fileAnalysis+".root");
    backgroundDYJtoTau  ->Add(pathToFile+"/nTupleDYJetsJetToTau_MuTau_"+fileAnalysis+".root");
  }

  cout << backgroundDYTauTau->GetEntries()  << " come from DY->tautau"         << endl;
  cout << backgroundDYMutoTau->GetEntries() << " come from DY->mumu, mu->tau"  << endl;
  cout << backgroundDYJtoTau->GetEntries()  << " come from DY->mumu, jet->tau" << endl;
  
  // MAPPING //
  if(VERBOSE) cout << "-- gather the trees" << endl;

  const int nVarious = 10;
  const int nChains  = nVarious + nProd*nMasses;
  TString treeNamesVarious[nVarious]={"SS","WJets","Data","W3Jets","TTbar","Others","DYToTauTau","DYMutoTau","DYJtoTau","Embedded"};
  TChain* chainsVarious[nVarious]   ={data,backgroundWJets,data,backgroundW3Jets,backgroundTTbar,backgroundOthers,
				      backgroundDYTauTau,backgroundDYMutoTau,backgroundDYJtoTau,dataEmbedded};
  TString  treeNames[nChains];
  TChain*  chains[nChains];
  mapchain mapAllTrees;

  for(int iCh=0 ; iCh<nChains ; iCh++) {
    
    if(iCh<nVarious) { // fill various tree names and trees
      treeNames[iCh] = treeNamesVarious[iCh];
      chains[iCh]    = chainsVarious[iCh];
    }    
    else { // fill signal names and trees
      treeNames[iCh] = nameProd[ int((iCh-nVarious)/nMasses) ] + nameMasses[ int((iCh-nVarious)%nMasses) ];
      chains[iCh]    = signal[ int((iCh-nVarious)/nMasses) ][ int((iCh-nVarious)%nMasses) ];
    }
    mapAllTrees[ treeNames[iCh] ] = chains[iCh]; // create an entry in the map
  }

  ////////////////////////////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////////////////////////////

  ///// MU PT+ID+ISO ///////
  TCut lpt("ptL1>20");
  TCut lptMax("ptL1>-999");
  TCut lID("isPFMuon && isTightMuon");

  if(     version_.Contains("SoftABC"))  {lpt="ptL1>16"; lptMax="ptL1<=20";}
  else if(version_.Contains("SoftD"))    {lpt="ptL1>9";  lptMax="ptL1<=20";}
  else if(version_.Contains("SoftLTau")) {lpt="ptL1>9";  lptMax="ptL1<=20";}
  else if(!version_.Contains("Soft"))    {lpt="ptL1>20"; lptMax="ptL1>-999";}
  if(     version_.Contains("NoMaxPt"))  {lptMax="ptL1>-999";}
  lpt = lpt && lptMax ;

  TCut liso("combRelIsoLeg1DBetav2<0.10");
  TCut laiso("combRelIsoLeg1DBetav2>0.20 && combRelIsoLeg1DBetav2<0.50");
  TCut lliso("combRelIsoLeg1DBetav2<0.30");

  ////// TAU PT+ID+ISO //////
  TCut tpt("ptL2>20");
  TCut antimu("tightestAntiMuWP>2");
  TCut tiso("tightestHPSMVAWP>=0");
  TCut ltiso("tightestHPSMVAWP>-99");
  TCut mtiso("hpsMVA>0.7");

  // Choose tau selection wrt version_
  chooseSelection(version_, tiso, ltiso, mtiso, antimu);

  ////// EVENT WISE //////
  TCut lveto("muFlag!=1 && vetoEvent==0"); //muFlag==0
  TCut SS("diTauCharge!=0");
  TCut OS("diTauCharge==0");
  TCut pZ( Form("((%s)<%f)",antiWcut.c_str(),antiWsgn));
  TCut apZ(Form("((%s)>%f)",antiWcut.c_str(),antiWsdb));
  //TCut apZ2(Form("((%s)>%f && (%s)<120)",antiWcut.c_str(),antiWsdb,antiWcut.c_str()));
  TCut apZ2(Form("((%s)>60 && (%s)<120)",antiWcut.c_str(),antiWcut.c_str()));

  bool removeMtCut     = bool(selection_.find("NoMt")!=string::npos);
  bool invertDiTauSign = bool(selection_.find("SS")!=string::npos);
  TCut MtCut       = removeMtCut     ? "(etaL1<999)" : pZ;
  TCut diTauCharge = invertDiTauSign ? SS : OS; 

  // HLT matching //
  TCut hltevent("HLTx==1 && HLTmatch==1");
  if(     version_.Contains("SoftABC"))  hltevent = "HLTxIsoMu15ETM20==1 && HLTmatchIsoMu15ETM20==1" ;
  else if(version_.Contains("SoftD"))    hltevent = "HLTxSoft==1         && HLTmatchSoft==1" ;
  else if(version_.Contains("SoftLTau")) hltevent = "HLTxIsoMu8Tau20==1  && HLTmatchIsoMu8Tau20==1" ;
  else if(!version_.Contains("Soft"))    hltevent = "HLTx==1             && HLTmatch==1" ;
  else {
    cout << "ERROR : Soft analyses need RUN set to ABC or D, nothing else is allowed." << endl;
    return;
  }

  ////// CATEGORIES ///
  TCut zeroJet("nJets30<1");
  TCut oneJet("nJets30>=1");
  TCut twoJets("nJets30>=2");
  TCut vbf("nJets30>=2 && pt1>30 && pt2>30 && Mjj>500 && Deta>3.5 && isVetoInJets!=1");
  TCut vbfLoose("nJets30>=2 && pt1>30 && pt2>30 && isVetoInJets!=1 && Mjj>200 && Deta>2");
  TCut vbfLooseQCD("nJets20>=2 && isVetoInJets!=1 && Mjj>200 && Deta>2");
  //TCut vbfLoose("nJets30>=2 && pt1>30 && pt2>30 && isVetoInJets!=1 && Mjj>500 && Deta>2");
  //TCut vbfLooseQCD("nJets20>=2 && isVetoInJets!=1 && Mjj>500 && Deta>2");

  if(version_.Contains("Soft")) {
    vbfLooseQCD = vbf;
    vbfLoose = vbf;
  }

  TCut vh("pt1>30 && pt2>30 && Mjj>70 && Mjj<120 && diJetPt>150 && MVAvbf<0.80 && nJets20BTagged<1");
  TCut boost("nJets30>0 && pt1>30 && nJets20BTagged<1");
  boost = boost && !vbf /*&& !vh*/;
  TCut bTag("nJets30<2 && nJets20BTagged>0");
  TCut bTagLoose("nJets30<2 && nJets20BTaggedLoose>0"); //for W shape in b-Category
  //TCut bTagLoose("nJets30<2 && nJets20>=1");
  TCut nobTag("nJets30<2 && nJets20BTagged==0");
  TCut novbf("nJets30<1 && nJets20BTagged==0");

  if(selection_.find("High")!=string::npos) {
    boost = boost&&TCut("ptL2>40"); 
    novbf = novbf&&TCut("ptL2>40");
  }
  else if(selection_.find("Low")!=string::npos){
    boost = boost&&TCut("ptL2<40"); 
    novbf = novbf&&TCut("ptL2<40"); 
  }

  TCut sbinCat("");
  if(     selection_.find("inclusive")!=string::npos) sbinCat = "etaL1<999";
  else if(selection_.find("oneJet")!=string::npos)    sbinCat = oneJet;
  else if(selection_.find("twoJets")!=string::npos)   sbinCat = twoJets;
  else if(selection_.find("vh")!=string::npos)        sbinCat = vh;
  else if(selection_.find("novbf")!=string::npos)     sbinCat = novbf;
  else if(selection_.find("boost")!=string::npos)     sbinCat = boost;
  else if(selection_.find("nobTag")!=string::npos)    sbinCat = nobTag;
  else if(selection_.find("vbf")!=string::npos && selection_.find("novbf")==string::npos)   sbinCat = vbf;
  else if(selection_.find("bTag")!=string::npos && selection_.find("nobTag")==string::npos) sbinCat = bTag;

  // Global TCuts = category && object selection && event selection //

  TCut sbinInclusive                     = lpt && lID && tpt && tiso  && antimu && liso  && lveto && diTauCharge  && MtCut  && hltevent   ;
  TCut sbinEmbeddingInclusive            = lpt && lID && tpt && tiso  && antimu && liso  && lveto && diTauCharge  && MtCut                ;
  TCut sbinPZetaRelEmbeddingInclusive    = lpt && lID && tpt && tiso  && antimu && liso  && lveto && diTauCharge                          ;
  TCut sbinPZetaRelSSInclusive           = lpt && lID && tpt && tiso  && antimu && liso  && lveto && SS                     && hltevent   ;
  TCut sbinPZetaRelInclusive             = lpt && lID && tpt && tiso  && antimu && liso  && lveto && diTauCharge            && hltevent   ;
  TCut sbinSSInclusive                   = lpt && lID && tpt && tiso  && antimu && liso  && lveto && SS           && MtCut  && hltevent   ;
  TCut sbinSSaIsoInclusive               = lpt && lID && tpt && tiso  && antimu && laiso && lveto && SS           && MtCut  && hltevent   ;
  TCut sbinAisoInclusive                 = lpt && lID && tpt && tiso  && antimu && laiso && lveto && diTauCharge  && MtCut  && hltevent   ;
  TCut sbinPZetaRelSSaIsoInclusive       = lpt && lID && tpt && tiso  && antimu && laiso && lveto && SS                     && hltevent   ;
  TCut sbinPZetaRelSSaIsoMtisoInclusive  = lpt && lID && tpt && mtiso && antimu && laiso && lveto && SS                     && hltevent   ;

  TCut sbinSSaIsoLtisoInclusive          = lpt && lID && tpt && mtiso && antimu && laiso && lveto && SS           && MtCut  && hltevent   ;
  TCut sbinSSaIsoMtisoInclusive          = lpt && lID && tpt && mtiso && antimu && laiso && lveto && SS           && MtCut  && hltevent   ;
  TCut sbinPZetaRelaIsoInclusive         = lpt && lID && tpt && tiso  && antimu && laiso && lveto && diTauCharge            && hltevent   ;

  TCut sbinSSltisoInclusive              = lpt && lID && tpt && ltiso && antimu && liso  && lveto && SS           && MtCut  && hltevent   ;
  TCut sbinLtisoInclusive                = lpt && lID && tpt && ltiso && antimu && liso  && lveto && diTauCharge  && MtCut  && hltevent   ;
  TCut sbinMtisoInclusive                = lpt && lID && tpt && mtiso && antimu && liso  && lveto && diTauCharge  && MtCut  && hltevent   ;
  TCut sbinPZetaRelLtisoInclusive        = lpt && lID && tpt && ltiso && antimu && liso  && lveto && diTauCharge            && hltevent   ;

  TCut sbin                   = sbinCat && lpt && lID && tpt && tiso  && antimu && liso  && lveto && diTauCharge  && MtCut  && hltevent   ;
  TCut sbinEmbedding          = sbinCat && lpt && lID && tpt && tiso  && antimu && liso  && lveto && diTauCharge  && MtCut                ;
  TCut sbinEmbeddingPZetaRel  = sbinCat && lpt && lID && tpt && tiso  && antimu && liso  && lveto && diTauCharge                          ;
  TCut sbinPZetaRel           = sbinCat && lpt && lID && tpt && tiso  && antimu && liso  && lveto && diTauCharge            && hltevent   ;
  TCut sbinPZetaRelaIso       = sbinCat && lpt && lID && tpt && tiso  && antimu && laiso && lveto && diTauCharge            && hltevent   ;
  TCut sbinPZetaRelSSaIso     = sbinCat && lpt && lID && tpt && tiso  && antimu && laiso && lveto && SS                     && hltevent   ;
  TCut sbinSS                 = sbinCat && lpt && lID && tpt && tiso  && antimu && liso  && lveto && SS           && MtCut  && hltevent   ;
  TCut sbinPZetaRelSS         = sbinCat && lpt && lID && tpt && tiso  && antimu && liso  && lveto && SS                     && hltevent   ;
  TCut sbinAiso               = sbinCat && lpt && lID && tpt && tiso  && antimu && laiso && lveto && diTauCharge  && MtCut  && hltevent   ;
  TCut sbinSSaIso             = sbinCat && lpt && lID && tpt && tiso  && antimu && laiso && lveto && SS           && MtCut  && hltevent   ;
  TCut sbinSSlIso1            = sbinCat && lpt && lID && tpt && tiso  && antimu && lliso && lveto && SS           && MtCut  && hltevent   ;
  TCut sbinSSlIso2            = sbinCat && lpt && lID && tpt && mtiso && antimu && liso  && lveto && SS           && MtCut  && hltevent   ;
  TCut sbinSSlIso3            = sbinCat && lpt && lID && tpt && mtiso && antimu && lliso && lveto && SS           && MtCut  && hltevent   ;
  TCut sbinSSaIsoLtiso        = sbinCat && lpt && lID && tpt && ltiso && antimu && laiso && lveto && SS           && MtCut  && hltevent   ;
  TCut sbinSSaIsoMtiso        = sbinCat && lpt && lID && tpt && mtiso && antimu && laiso && lveto && SS           && MtCut  && hltevent   ;
  TCut sbinSSltiso            = sbinCat && lpt && lID && tpt && ltiso && antimu && liso  && lveto && SS           && MtCut  && hltevent   ;
  TCut sbinSSmtiso            = sbinCat && lpt && lID && tpt && mtiso && antimu && liso  && lveto && SS           && MtCut  && hltevent   ;
  TCut sbinLtiso              = sbinCat && lpt && lID && tpt && ltiso && antimu && liso  && lveto && diTauCharge  && MtCut  && hltevent   ;
  TCut sbinMtiso              = sbinCat && lpt && lID && tpt && mtiso && antimu && liso  && lveto && diTauCharge  && MtCut  && hltevent   ;
  TCut sbinPZetaRelMtiso      = sbinCat && lpt && lID && tpt && mtiso && antimu && liso  && lveto && diTauCharge            && hltevent   ;
  TCut sbinPZetaRelSSaIsoMtiso= sbinCat && lpt && lID && tpt && mtiso && antimu && laiso && lveto && SS                     && hltevent   ;

  /////////////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////////////

  cout << endl;
  cout << "#############################################################" << endl;
  cout << ">>>>>>>>>>> BEGIN Compute inclusive informations <<<<<<<<<<<<" << endl;
  cout << "#############################################################" << endl;
  cout << endl;

  cout << "******** Extrapolation factors for Z->tautau normalization ********" << endl;
  // inclusive DY->tautau:
  TH1F* hExtrap = new TH1F("hExtrap","",nBins , bins.GetArray());
  float Error = 0.;

  float ExtrapDYInclusive = 0.;
  drawHistogram("MC", version_, RUN,mapAllTrees["DYToTauTau"], variable, ExtrapDYInclusive,   Error,   Lumi*lumiCorrFactor*hltEff_/1000., hExtrap, sbinInclusive);
  cout << "All Z->tautau             = " << ExtrapDYInclusive << " +/- " <<  Error << endl; 

  float ExtrapDYInclusivePZetaRel = 0.;
  drawHistogram("MC", version_, RUN,mapAllTrees["DYToTauTau"], variable, ExtrapDYInclusivePZetaRel,   Error,   Lumi*lumiCorrFactor*hltEff_/1000., hExtrap, sbinPZetaRelInclusive);
  cout << "All Z->tautau (pZeta Rel) = " << ExtrapDYInclusivePZetaRel << " +/- " <<  Error << endl; 

  float ExtrapLFakeInclusive = 0.;
  drawHistogram("MC", version_, RUN,mapAllTrees["DYMutoTau"], variable,ExtrapLFakeInclusive,Error,   Lumi*lumiCorrFactor*hltEff_/1000., hExtrap, sbinInclusive);
  cout << "All Z->mumu, mu->tau      = " << ExtrapLFakeInclusive << " +/- " <<  Error << endl;

  float ExtrapJFakeInclusive = 0.;
  drawHistogram("MC", version_, RUN,mapAllTrees["DYJtoTau"], variable, ExtrapJFakeInclusive,Error,   Lumi*lumiCorrFactor*hltEff_/1000., hExtrap, sbinInclusive);
  cout << "All Z->mumu, j->tau       = " << ExtrapJFakeInclusive << " +/- " <<  Error << endl;
  cout << endl;

  float ExtrapDYNum = 0.;
  drawHistogram("MC", version_, RUN,mapAllTrees["DYToTauTau"], variable, ExtrapDYNum,                  Error,   Lumi*lumiCorrFactor*hltEff_/1000., hExtrap, sbin);
  float ExtrapDYNuminSidebandRegion = 0.;
  drawHistogram("MC", version_, RUN,mapAllTrees["DYToTauTau"], variable, ExtrapDYNuminSidebandRegion,  Error,   Lumi*lumiCorrFactor*hltEff_/1000., hExtrap, sbinPZetaRel&&apZ);
  float ExtrapDYNumPZetaRel = 0.;
  drawHistogram("MC", version_, RUN,mapAllTrees["DYToTauTau"], variable, ExtrapDYNumPZetaRel,          Error,   Lumi*lumiCorrFactor*hltEff_/1000., hExtrap, sbinPZetaRel);
  float ExtrapolationFactorMadGraph      = ExtrapDYNum/ExtrapDYInclusive;
  float ErrorExtrapolationFactorMadGraph = TMath::Sqrt(ExtrapolationFactorMadGraph*(1-ExtrapolationFactorMadGraph)/ExtrapDYInclusive);
  cout << "Extrap. factor using MadGraph            = " << ExtrapolationFactorMadGraph << " +/- " << ErrorExtrapolationFactorMadGraph << endl;

  float ExtrapEmbedNum = 0.;
  drawHistogram("Embed", version_, RUN,mapAllTrees["Embedded"], variable, ExtrapEmbedNum,                 Error, 1.0, hExtrap, sbinEmbedding);
  float ExtrapEmbedNuminSidebandRegion = 0.;
  drawHistogram("Embed", version_, RUN,mapAllTrees["Embedded"], variable, ExtrapEmbedNuminSidebandRegion, Error, 1.0, hExtrap, sbinEmbeddingPZetaRel&&apZ);
  float ExtrapEmbedNumPZetaRel = 0.;
  drawHistogram("Embed", version_, RUN,mapAllTrees["Embedded"], variable, ExtrapEmbedNumPZetaRel,         Error, 1.0, hExtrap, sbinEmbeddingPZetaRel);
  float ExtrapEmbedDen = 0.;
  drawHistogram("Embed", version_, RUN,mapAllTrees["Embedded"], variable, ExtrapEmbedDen,                 Error, 1.0, hExtrap, sbinEmbeddingInclusive);
  float ExtrapEmbedDenPZetaRel = 0.;
  drawHistogram("Embed", version_, RUN,mapAllTrees["Embedded"], variable, ExtrapEmbedDenPZetaRel,         Error, 1.0, hExtrap, sbinPZetaRelEmbeddingInclusive);

  ExtrapolationFactorZ             = ExtrapEmbedNum/ExtrapEmbedDen; 
  ErrorExtrapolationFactorZ        = TMath::Sqrt(ExtrapolationFactorZ*(1-ExtrapolationFactorZ)/ExtrapEmbedDen);
  ExtrapolationFactorZDataMC       = ExtrapolationFactorZ/ExtrapolationFactorMadGraph;
  ExtrapolationFactorZFromSideband = ExtrapEmbedDen/ExtrapEmbedDenPZetaRel;

  float sidebandRatioMadgraph = ExtrapDYNuminSidebandRegion/ExtrapDYNumPZetaRel;
  float sidebandRatioEmbedded = ExtrapEmbedNuminSidebandRegion/ExtrapEmbedNumPZetaRel;
  ExtrapolationFactorSidebandZDataMC = (sidebandRatioMadgraph > 0) ? sidebandRatioEmbedded/sidebandRatioMadgraph : 1.0;
  if(!useEmbedding_) {
    ExtrapolationFactorZ = ExtrapolationFactorZDataMC = ExtrapolationFactorZFromSideband = sidebandRatioEmbedded = ExtrapolationFactorSidebandZDataMC = 1;
  }

  cout << "Extrap. factor using embedded sample     = " << ExtrapolationFactorZ << " +/- " << ErrorExtrapolationFactorZ << endl;
  cout << " ==> data/MC (signal region)             = " << ExtrapolationFactorZDataMC << " +/- " 
       << ExtrapolationFactorZDataMC*(ErrorExtrapolationFactorMadGraph/ExtrapolationFactorMadGraph + 
				      ErrorExtrapolationFactorZ/ExtrapolationFactorZ) 
       << endl;
  cout << " ==> data/MC (sideband-to-signal)        = " << ExtrapolationFactorSidebandZDataMC << endl;
  cout << " ==> data (sideband-to-signal inclusive) = " << ExtrapolationFactorZFromSideband << endl;
  
  /////////////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////////////

  cout << endl;
  cout << "******** Extrapolation factors for QCD normalization ********" << endl;

  float SSQCDinSignalRegionDATAIncl = 0.; 
  float extrapFactorWSSIncl = 0.;
  float SSWinSignalRegionDATAIncl = 0.;
  float SSWinSignalRegionMCIncl = 0.;
  float SSWinSidebandRegionDATAIncl = 0.;
  float SSWinSidebandRegionMCIncl = 0.;      
  if(invertDiTauSign) OStoSSRatioQCD = 1.0;

  evaluateQCD(mapAllTrees, version_, RUN, 0, 0, true, "SS", false, removeMtCut, "inclusive", 
	      SSQCDinSignalRegionDATAIncl , SSIsoToSSAIsoRatioQCD, scaleFactorTTSSIncl,
	      extrapFactorWSSIncl, 
	      SSWinSignalRegionDATAIncl, SSWinSignalRegionMCIncl,
	      SSWinSidebandRegionDATAIncl, SSWinSidebandRegionMCIncl,
 	      hExtrap, variable,
 	      Lumi/1000*hltEff_,  TTxsectionRatio, lumiCorrFactor,
	      ExtrapolationFactorSidebandZDataMC, ExtrapolationFactorZDataMC,
 	      MutoTauCorrectionFactor, JtoTauCorrectionFactor, 
	      OStoSSRatioQCD,
 	      antiWsdb, antiWsgn, useMt,
	      sbinSSInclusive,
	      sbinPZetaRelSSInclusive,
 	      sbinPZetaRelSSInclusive, pZ, apZ, sbinPZetaRelSSInclusive, 
 	      sbinPZetaRelSSaIsoInclusive, sbinPZetaRelSSaIsoInclusive, sbinPZetaRelSSaIsoMtisoInclusive, 
	      vbf, oneJet, zeroJet);

  delete hExtrap;

  cout << endl;
  cout << "#############################################################" << endl;
  cout << ">>>>>>>>>>> END Compute inclusive informations <<<<<<<<<<<<<<" << endl;
  cout << "#############################################################" << endl;

  /////////////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////////////
 
  float SSQCDinSignalRegionDATA = 0.; 
  float extrapFactorWSS = 0.;             
  float SSWinSignalRegionDATA = 0.;       float SSWinSignalRegionMC = 0.; float SSWinSidebandRegionDATA = 0.; float SSWinSidebandRegionMC = 0.;
  float extrapFactorWOSW3Jets = 0.;   
  float OSWinSignalRegionDATAW3Jets = 0.; float OSWinSignalRegionMCW3Jets = 0.; float OSWinSidebandRegionDATAW3Jets = 0.; float OSWinSidebandRegionMCW3Jets = 0.;
  float extrapFactorWOSWJets  = 0.; 
  float OSWinSignalRegionDATAWJets  = 0.; float OSWinSignalRegionMCWJets  = 0.; float OSWinSidebandRegionDATAWJets  = 0.; float OSWinSidebandRegionMCWJets  = 0.; 
  float scaleFactorTTOSW3Jets = 0.;
  float scaleFactorTTOSWJets  = 0.;

  mapchain::iterator it;
  TString currentName, h1Name;
  //TH1F *h1, *hCleaner;
  TChain* currentTree;

  for(int iCh=0 ; iCh<nChains ; iCh++) {

    cout        << endl        << ">>>> Dealing with sample ## " ;
    currentName = treeNames[iCh];
    cout        << currentName << " ##" << endl;

    //if(!currentName.Contains("Data")) continue; // TEST : TO REMOVE !

    it = mapAllTrees.find(currentName);
    if(it!=mapAllTrees.end()) currentTree = it->second;
    else {
      cout << "ERROR : sample not found in the map ! continue;" << endl;
      continue;
    }

    h1Name         = "h1_"+currentName;
    TH1F* h1       = new TH1F( h1Name ,"" , nBins , bins.GetArray());
    TH1F* hCleaner = new TH1F("hCleaner","",nBins , bins.GetArray());
    if ( !h1->GetSumw2N() )       h1->Sumw2(); 
    if ( !hCleaner->GetSumw2N() ) hCleaner->Sumw2();

    if(currentName.Contains("SS")){
      
      cout << "************** BEGIN QCD evaluation using SS events *******************" << endl;

      TH1F* hExtrapSS = new TH1F("hExtrapSS","",nBins , bins.GetArray());
      float dummy1 = 0.;      

      TCut sbinPZetaRelSSForWextrapolation = sbinPZetaRelSS;
      if(selection_.find("vbf")!=string::npos && selection_.find("novbf")==string::npos)
	sbinPZetaRelSSForWextrapolation = (sbinPZetaRelSSInclusive&&vbfLoose);     
      
      // hQCD for relaxed vbf selection
      if(selection_.find("vbf")!=string::npos && selection_.find("novbf")==string::npos) {

	evaluateQCD(mapAllTrees, version_, RUN, hQCDrelaxVBF, hCleaner, true, "SS", false, removeMtCut, selection_, 
		    SSQCDinSignalRegionDATA , dummy1 , scaleFactorTTSS,
		    extrapFactorWSS, 
		    SSWinSignalRegionDATA, SSWinSignalRegionMC,
		    SSWinSidebandRegionDATA, SSWinSidebandRegionMC,
		    hExtrapSS, variable,
		    Lumi/1000*hltEff_,  TTxsectionRatio, lumiCorrFactor,
		    ExtrapolationFactorSidebandZDataMC, ExtrapolationFactorZDataMC,
		    MutoTauCorrectionFactor, JtoTauCorrectionFactor, 
		    OStoSSRatioQCD,
		    antiWsdb, antiWsgn, useMt,
		    sbinSSInclusive && vbfLooseQCD,
		    sbinPZetaRelSSForWextrapolation,
		    sbinPZetaRelSS, pZ, apZ, sbinPZetaRelSSInclusive, 
		    sbinPZetaRelSSaIsoInclusive, sbinPZetaRelSSaIso, sbinPZetaRelSSaIsoMtiso, 
		    vbfLoose, oneJet, zeroJet);

	hCleaner->Reset();
      }

      // Normal hQCD
      evaluateQCD(mapAllTrees, version_, RUN, h1, hCleaner, true, "SS", false, removeMtCut, selection_, 
		  SSQCDinSignalRegionDATA , dummy1 , scaleFactorTTSS,
		  extrapFactorWSS, 
		  SSWinSignalRegionDATA, SSWinSignalRegionMC,
		  SSWinSidebandRegionDATA, SSWinSidebandRegionMC,
		  hExtrapSS, variable,
		  Lumi/1000*hltEff_,  TTxsectionRatio, lumiCorrFactor,
		  ExtrapolationFactorSidebandZDataMC, ExtrapolationFactorZDataMC,
		  MutoTauCorrectionFactor, JtoTauCorrectionFactor, 
		  OStoSSRatioQCD,
		  antiWsdb, antiWsgn, useMt,
		  sbinSS,
		  sbinPZetaRelSSForWextrapolation,
		  sbinPZetaRelSS, pZ, apZ, sbinPZetaRelSSInclusive, 
		  sbinPZetaRelSSaIsoInclusive, sbinPZetaRelSSaIso, sbinPZetaRelSSaIsoMtiso, 
		  vbfLoose, oneJet, zeroJet);

      cout << "************** END QCD evaluation using SS events *******************" << endl;

      delete hExtrapSS;

      hQCD->Add(h1, 1.0); //hQCD->Sumw2();
      hSS->Add(hCleaner, OStoSSRatioQCD); //hSS->Sumw2();
      hCleaner->Reset();

    }
    else{

      if(!currentName.Contains("Embed")){

	float Error = 0.;

	if(currentName.Contains("DYToTauTau")){
	  float NormDYToTauTau = 0.;
	  drawHistogram("MC", version_, RUN,currentTree, variable, NormDYToTauTau, Error,   Lumi*lumiCorrFactor*hltEff_/1000., h1, sbin, 1);
	  hZtt->Add(h1, ExtrapolationFactorZFromSideband);
	}
	if(currentName.Contains("TTbar")){
	  if(selection_.find("vbf")!=string::npos && selection_.find("novbf")==string::npos){ 
	    float NormTTjets = 0.; 
	    TCut sbinVBFLoose = sbinInclusive && vbfLoose; 
	    drawHistogram("MC", version_, RUN,currentTree, variable, NormTTjets,     Error,   Lumi*TTxsectionRatio*scaleFactorTTOSWJets*hltEff_/1000., hCleaner, sbinVBFLoose, 1);
	    hTTb->Add(hCleaner, 1.0);
	    NormTTjets = 0.;
	    drawHistogram("MC", version_, RUN,currentTree, variable, NormTTjets,     Error,   Lumi*TTxsectionRatio*scaleFactorTTOSWJets*hltEff_/1000., h1, sbin, 1);
	    hTTb->Scale( hTTb->Integral()!=0 ? h1->Integral()/hTTb->Integral() : 1.0 ); 
	  } 
	  else{
	    float NormTTjets = 0.;
	    drawHistogram("MC", version_, RUN,currentTree, variable, NormTTjets,     Error,   Lumi*TTxsectionRatio*scaleFactorTTOSWJets*hltEff_/1000., h1, sbin, 1);
	    hTTb->Add(h1, 1.0);
	  }
	}
	else if(currentName.Contains("W3Jets")){

	  TH1F* hExtrapW3Jets = new TH1F("hExtrapW3Jets","",nBins , bins.GetArray());
	 
	  cout << "************** BEGIN W+3jets normalization using high-Mt sideband *******************" << endl;

	  TCut sbinPZetaRelForWextrapolation = sbinPZetaRel;
	  if(selection_.find("vbf")!=string::npos && selection_.find("novbf")==string::npos)
	    sbinPZetaRelForWextrapolation = (sbinPZetaRelInclusive&&vbfLoose);     

	  evaluateWextrapolation(mapAllTrees, version_, RUN, "OS", false, selection_, 
				 extrapFactorWOSW3Jets, 
				 OSWinSignalRegionDATAW3Jets,   OSWinSignalRegionMCW3Jets,
				 OSWinSidebandRegionDATAW3Jets, OSWinSidebandRegionMCW3Jets,
				 scaleFactorTTOSW3Jets,
				 hExtrapW3Jets, variable,
				 Lumi*hltEff_/1000., TTxsectionRatio, lumiCorrFactor,
				 ExtrapolationFactorSidebandZDataMC, ExtrapolationFactorZDataMC,
				 MutoTauCorrectionFactor, JtoTauCorrectionFactor,
				 antiWsdb, antiWsgn, useMt,
				 sbinPZetaRelForWextrapolation,
				 sbinPZetaRel, sbinPZetaRel,
				 pZ, apZ, sbinPZetaRelInclusive, 
				 sbinPZetaRelaIsoInclusive, sbinPZetaRelaIso, vbfLoose, oneJet, zeroJet);

	  cout << "************** END W+3jets normalization using high-Mt sideband *******************" << endl;
	  delete hExtrapW3Jets;

	  float NormW3Jets = 0.;
	  drawHistogram("MC", version_, RUN,currentTree, variable, NormW3Jets, Error,   Lumi*hltEff_/1000., h1, sbin, 1);
	  if(removeMtCut) h1->Scale(OSWinSidebandRegionDATAW3Jets/OSWinSidebandRegionMCW3Jets);
	  else h1->Scale( h1->Integral()!=0 ? OSWinSignalRegionDATAW3Jets/h1->Integral() : 1.0 );
	  hW3Jets->Add(h1, 1.0);

	  drawHistogram("MC", version_, RUN,currentTree, variable, NormW3Jets, Error,   Lumi*hltEff_/1000., hCleaner, sbinMtiso, 1);
	  hW3JetsMediumTauIso->Add(hCleaner, hW3Jets->Integral()/hCleaner->Integral());

	  drawHistogram("MC", version_, RUN,currentTree, variable, NormW3Jets, Error,   Lumi*hltEff_/1000., hCleaner, sbinLtiso, 1);
	  hW3JetsLooseTauIso->Add(hCleaner,  hW3Jets->Integral()/hCleaner->Integral());

	  drawHistogram("MC", version_, RUN,currentTree, variable, NormW3Jets, Error,   Lumi*hltEff_/1000., hCleaner, sbinInclusive&&vbfLoose, 1);
	  hW3JetsMediumTauIsoRelVBF->Add(hCleaner,  hW3Jets->Integral()/hCleaner->Integral());
	  
	  hW3JetsMediumTauIsoRelVBFMinusSS->Add(h1, (1-OStoSSRatioQCD*SSWinSidebandRegionDATA/OSWinSidebandRegionDATAW3Jets));
	  //hW3JetsMediumTauIsoRelVBF->Sumw2(); hW3JetsMediumTauIsoRelVBFMinusSS->Sumw2();
	  if(((selection_.find("vbf")!=string::npos && selection_.find("novbf")==string::npos) || 
	      selection_.find("twoJets")!=string::npos)){
	    if(!USESSBKG) hEWK->Add(hW3JetsMediumTauIsoRelVBF,1.0);
	    else  hEWK->Add(hW3JetsMediumTauIsoRelVBFMinusSS,1.0);
	  }
	}
	else if(currentName.Contains("WJets")){

	  TH1F* hExtrapW = new TH1F("hExtrap","",nBins , bins.GetArray());
	  
	  cout << "************** BEGIN W+jets normalization using high-Mt sideband *******************" << endl;

	  TCut sbinPZetaRelForWextrapolation = sbinPZetaRel;
	  if(selection_.find("vbf")!=string::npos && selection_.find("novbf")==string::npos)
	    sbinPZetaRelForWextrapolation = (sbinPZetaRelInclusive&&vbfLoose); 
	  if(selection_.find("bTag")!=string::npos && selection_.find("nobTag")==string::npos) 
            sbinPZetaRelForWextrapolation = (sbinPZetaRelInclusive&&bTagLoose);

	  evaluateWextrapolation(mapAllTrees, version_, RUN, "OS", false, selection_, 
				 extrapFactorWOSWJets, 
				 OSWinSignalRegionDATAWJets,   OSWinSignalRegionMCWJets,
				 OSWinSidebandRegionDATAWJets, OSWinSidebandRegionMCWJets,
				 scaleFactorTTOSWJets,
				 hExtrapW, variable,
				 Lumi*hltEff_/1000., TTxsectionRatio, lumiCorrFactor,
				 ExtrapolationFactorSidebandZDataMC, ExtrapolationFactorZDataMC,
				 MutoTauCorrectionFactor, JtoTauCorrectionFactor,
				 antiWsdb, antiWsgn, useMt,
				 sbinPZetaRelForWextrapolation,
				 sbinPZetaRel, sbinPZetaRel,
				 pZ, apZ, sbinPZetaRelInclusive, 
				 sbinPZetaRelaIsoInclusive, sbinPZetaRelaIso, vbfLoose, boost, zeroJet);
	  delete hExtrapW;

	  cout << "************** END W+jets normalization using high-Mt sideband *******************" << endl;

	  float NormSSWJets = 0.;
	  drawHistogram("MC", version_, RUN,currentTree, variable, NormSSWJets, Error,   Lumi*hltEff_/1000., h1, sbinSS, 1);
	  if(removeMtCut) h1->Scale(SSWinSidebandRegionDATA/SSWinSidebandRegionMC);
	  else h1->Scale( h1->Integral()!=0 ? SSWinSignalRegionDATA/h1->Integral() : 1.0 );
	  hWSS->Add(h1, 1.0);

	  float NormWJets = 0.;
	  drawHistogram("MC", version_, RUN,currentTree, variable, NormWJets, Error,   Lumi*hltEff_/1000., h1, sbin, 1);
	  if(removeMtCut) h1->Scale(OSWinSidebandRegionDATAWJets/OSWinSidebandRegionMCWJets);
	  else h1->Scale( h1->Integral()!=0 ? OSWinSignalRegionDATAWJets/h1->Integral() : 1.0 );
	  hW->Add(h1, 1.0); //hW->Sumw2();
	  
	  hWMinusSS->Add(h1, (1-OStoSSRatioQCD*SSWinSidebandRegionDATA/OSWinSidebandRegionDATAWJets));

	  if(selection_.find("bTag")!=string::npos && selection_.find("nobTag")==string::npos){
	    float NormWJetsBTag = 0.;
	    drawHistogram("MC", version_, RUN,currentTree, variable, NormWJetsBTag, Error,   Lumi*hltEff_/1000., hCleaner, sbinInclusive&&bTagLoose, 1); 
	    hWLooseBTag->Add(hCleaner,  h1->Integral()/hCleaner->Integral());
	    hEWK->Add(hWLooseBTag,1.0);
	  }
	  else if(!((selection_.find("vbf")!=string::npos && selection_.find("novbf")==string::npos) || 
	       selection_.find("twoJets")!=string::npos)) {
	    if(!USESSBKG) hEWK->Add(h1,1.0);
	    else hEWK->Add(hWMinusSS,1.0);
	  }
	}
	else if(currentName.Contains("DYMutoTau")){
	  if(selection_.find("vbf")!=string::npos && selection_.find("novbf")==string::npos){  
            float NormDYMutoTau = 0.;
            TCut sbinVBFLoose = sbinInclusive && twoJets;  
	    if(useZDataMC)
	      drawHistogram("MC", version_, RUN,currentTree, variable, NormDYMutoTau, Error,   Lumi*lumiCorrFactor*MutoTauCorrectionFactor*ExtrapolationFactorZDataMC*hltEff_/1000., h1, sbinVBFLoose, 1);
	    else
	      drawHistogram("MC", version_, RUN,currentTree, variable, NormDYMutoTau, Error,   Lumi*lumiCorrFactor*MutoTauCorrectionFactor*hltEff_/1000., h1, sbinVBFLoose, 1);
            float NormDYMutoTauEmbdLoose = 0.; 
	    drawHistogram("Embed", version_, RUN,mapAllTrees["Embedded"], variable, NormDYMutoTauEmbdLoose,  Error, 1.0 , hCleaner,  sbinVBFLoose  ,1);
	    hCleaner->Reset();
	    float NormDYMutoTauEmbd = 0.;
	    drawHistogram("Embed", version_, RUN,mapAllTrees["Embedded"], variable, NormDYMutoTauEmbd,  Error, 1.0 , hCleaner,  sbin  ,1);
            h1->Scale( NormDYMutoTauEmbdLoose!=0 ? NormDYMutoTauEmbd/NormDYMutoTauEmbdLoose : 1.0 );
	    if(h1) {
	      hZmm->Add(h1, 1.0); //hZmm->Sumw2(); 
	      hZfakes->Add(h1,1.0); //hZfakes->Sumw2();
	      hEWK->Add(h1,1.0); 
	    }
          }  
	  else if(selection_.find("bTag")!=string::npos && selection_.find("nobTag")==string::npos){
	    float NormDYMutoTau = 0.; 
            TCut sbinbTagLoose = sbinInclusive && oneJet;   
	    if(useZDataMC)
	      drawHistogram("MC", version_, RUN,currentTree, variable, NormDYMutoTau, Error,   Lumi*lumiCorrFactor*MutoTauCorrectionFactor*ExtrapolationFactorZDataMC*hltEff_/1000., hCleaner, sbinbTagLoose, 1); 
	    else
	      drawHistogram("MC", version_, RUN,currentTree, variable, NormDYMutoTau, Error,   Lumi*lumiCorrFactor*MutoTauCorrectionFactor*hltEff_/1000., hCleaner, sbinbTagLoose, 1);
	    NormDYMutoTau = 0.;  
            drawHistogram("MC", version_, RUN,currentTree, variable, NormDYMutoTau, Error,   Lumi*lumiCorrFactor*MutoTauCorrectionFactor*ExtrapolationFactorZ*hltEff_/1000., h1, sbinInclusive, 1);
	    hCleaner->Scale(h1->Integral()/hCleaner->Integral());
	    hZmm->Add(hCleaner, 1.0); //hZmm->Sumw2();  
            hZfakes->Add(hCleaner,1.0); //hZfakes->Sumw2(); 
            hEWK->Add(hCleaner,1.0);
	  }
	  else{
	    float NormDYMutoTau = 0.;
	    if(useZDataMC)
	      drawHistogram("MC", version_, RUN,currentTree, variable, NormDYMutoTau, Error,   Lumi*lumiCorrFactor*MutoTauCorrectionFactor*ExtrapolationFactorZDataMC*hltEff_/1000., h1, sbin, 1);
	    else 
	      drawHistogram("MC", version_, RUN,currentTree, variable, NormDYMutoTau, Error,   Lumi*lumiCorrFactor*MutoTauCorrectionFactor*hltEff_/1000., h1, sbin, 1);
	    hZmm->Add(h1, 1.0); //hZmm->Sumw2();
	    hZfakes->Add(h1,1.0); //hZfakes->Sumw2();
	    hEWK->Add(h1,1.0);
	  }
	}
	else if(currentName.Contains("DYJtoTau")){
	  if(selection_.find("vbf")!=string::npos && selection_.find("novbf")==string::npos){   
            float NormDYJtoTau = 0.; 
            TCut sbinVBFLoose = sbinInclusive && vbfLoose;   
	    drawHistogram("MC", version_, RUN,currentTree, variable, NormDYJtoTau, Error,    Lumi*lumiCorrFactor*JtoTauCorrectionFactor*hltEff_/1000., hCleaner, sbinVBFLoose, 1);
	    NormDYJtoTau = 0.; 
	    if(useZDataMC)
	      drawHistogram("MC", version_, RUN,currentTree, variable, NormDYJtoTau, Error,    Lumi*lumiCorrFactor*JtoTauCorrectionFactor*ExtrapolationFactorZDataMC*hltEff_/1000., h1, sbin, 1); 
	    else
	      drawHistogram("MC", version_, RUN,currentTree, variable, NormDYJtoTau, Error,    Lumi*lumiCorrFactor*JtoTauCorrectionFactor*hltEff_/1000., h1, sbin, 1);
	    hCleaner->Scale( hCleaner->Integral()!=0 ? h1->Integral()/hCleaner->Integral() : 1.0 );
	    if(hCleaner) {
	      hZmj->Add(hCleaner, 1.0); //hZmj->Sumw2();
	      hZfakes->Add(hCleaner,1.0); //hZfakes->Sumw2();
	      hEWK->Add(hCleaner,1.0); 
	    }
	  }
	  else if(selection_.find("bTag")!=string::npos && selection_.find("nobTag")==string::npos){
	    float NormDYJtoTau = 0.;  
            TCut sbinbTagLoose = sbinInclusive && oneJet;    
            drawHistogram("MC", version_, RUN,currentTree, variable, NormDYJtoTau, Error,    Lumi*lumiCorrFactor*JtoTauCorrectionFactor*ExtrapolationFactorZDataMC*hltEff_/1000., hCleaner, sbinbTagLoose, 1); 
            NormDYJtoTau = 0.;  
            drawHistogram("MC", version_, RUN,currentTree, variable, NormDYJtoTau, Error,    Lumi*lumiCorrFactor*JtoTauCorrectionFactor*ExtrapolationFactorZ*hltEff_/1000., h1, sbinInclusive, 1);  
            hCleaner->Scale(h1->Integral()/hCleaner->Integral()); 
            hZmj->Add(hCleaner, 1.0); //hZmj->Sumw2(); 
            hZfakes->Add(hCleaner,1.0); //hZfakes->Sumw2(); 
            hEWK->Add(hCleaner,1.0);  
          } 
	  else{
	    float NormDYJtoTau = 0.;
	    if(useZDataMC)
	      drawHistogram("MC", version_, RUN,currentTree, variable, NormDYJtoTau, Error,    Lumi*lumiCorrFactor*JtoTauCorrectionFactor*ExtrapolationFactorZDataMC*hltEff_/1000., h1, sbin, 1);
	    else
	      drawHistogram("MC", version_, RUN,currentTree, variable, NormDYJtoTau, Error,    Lumi*lumiCorrFactor*JtoTauCorrectionFactor*hltEff_/1000., h1, sbin, 1);
	    hZmj->Add(h1, 1.0); //hZmj->Sumw2();
	    hZfakes->Add(h1,1.0); //hZfakes->Sumw2();
	    hEWK->Add(h1,1.0);
	  }
	}
	else if(currentName.Contains("Others")){
	  if(selection_.find("vbf")!=string::npos && selection_.find("novbf")==string::npos){    
            float NormOthers = 0.;  
            TCut sbinVBFLoose = sbinInclusive && vbfLoose;    
	    drawHistogram("MC", version_, RUN,currentTree, variable, NormOthers, Error,     Lumi*hltEff_/1000., hCleaner, sbinVBFLoose, 1);
	    NormOthers = 0.; 
            drawHistogram("MC", version_, RUN,currentTree, variable, NormOthers , Error,     Lumi*hltEff_/1000., h1, sbin, 1);
            hCleaner->Scale( hCleaner->Integral()!=0 ? h1->Integral()/hCleaner->Integral() : 1.0 ); 
	    if(hCleaner) {
	      hVV->Add(hCleaner, 1.0);  
	      hEWK->Add(hCleaner,1.0);  
	    }
          } 
	  else{
	    float NormOthers = 0.;
	    drawHistogram("MC", version_, RUN,currentTree, variable, NormOthers , Error,     Lumi*hltEff_/1000., h1, sbin, 1);
	    hVV->Add(h1, 1.0);
	    hEWK->Add(h1,1.0);
	  }
	}
	else if(currentName.Contains("Data")){

	  //TFile* ftest = new TFile(Form(location+"/%s/histograms/test_mH%d_%s_%s_%s.root",outputDir.Data(), mH_,selection_.c_str(),analysis_.c_str(),variable_.Data()),"RECREATE");

	  float NormData = 0.;
	  drawHistogram("Data", version_, RUN, currentTree, variable, NormData,  Error, 1.0 , h1, sbin, 1);
	  if(VERBOSE) cout << "DATA YIELD : " << NormData << "  +/- " << Error << endl;
	  hData->Add(h1, 1.0);
	  
	  //ftest->cd();
	  //hData->Write();
	  //h1->Write();
	  //ftest->Write();
 	  //ftest->Close();
	  //return;
	  //continue;

	  if ( !hData->GetSumw2N() )hData->Sumw2();
	  
	  if(selection_.find("vbf")!=string::npos && selection_.find("novbf")==string::npos){

	    drawHistogram("Data", version_, RUN, currentTree, variable, NormData,  Error, 1.0 , hCleaner, sbinSSaIsoMtiso ,1);
	    float tmpNorm = hCleaner->Integral();
	    hDataAntiIsoLooseTauIsoFullVBF->Add(hCleaner); //ND

	    drawHistogram("Data", version_, RUN, currentTree, variable, NormData,  Error, 1.0 , hCleaner, sbinSSaIsoMtisoInclusive&&vbfLooseQCD ,1);
	    hDataAntiIsoLooseTauIsoRelaxVBF->Add(hCleaner); //ND

	    hDataAntiIsoLooseTauIso->Add(hCleaner, SSIsoToSSAIsoRatioQCD*(tmpNorm/hCleaner->Integral()));

	    //get efficiency of events passing QCD selection to pass the category selection 
	    drawHistogram("Data", version_, RUN, currentTree, variable, NormData,  Error, 1.0 , hCleaner, sbinSSaIsoInclusive ,1);
	    float tmpNormQCDSel = hCleaner->Integral();
	    drawHistogram("Data", version_, RUN, currentTree, variable, NormData,  Error, 1.0 , hCleaner, sbinSSaIso, 1); 
            float tmpNormCatSel = hCleaner->Integral();
	    float effQCDToCatSel = tmpNormCatSel/tmpNormQCDSel;
	    //Normalize to Inclusive measured QCD times the above efficiency
	    hDataAntiIsoLooseTauIsoQCD->Add(hDataAntiIsoLooseTauIso, (effQCDToCatSel*SSQCDinSignalRegionDATAIncl)/hDataAntiIsoLooseTauIso->Integral());
	  }
	  else if(selection_.find("novbfHigh")!=string::npos || selection_.find("boost")!=string::npos){
	    drawHistogram("Data", version_, RUN, currentTree, variable, NormData,  Error, 1.0 , hCleaner, sbinSSaIso ,1);
	    hDataAntiIsoLooseTauIso->Add(hCleaner);
	    float NormDYMutoTau = 0;
	    drawHistogram("MC", version_, RUN,mapAllTrees["DYMutoTau"], variable, NormDYMutoTau, Error,   Lumi*lumiCorrFactor*MutoTauCorrectionFactor*hltEff_/1000., hCleaner, sbinSSaIso, 1);
	    hDataAntiIsoLooseTauIso->Add(hCleaner, -1.0);
	    float NormDYJtoTau = 0.; 
            drawHistogram("MC", version_, RUN,mapAllTrees["DYJtoTau"], variable, NormDYJtoTau, Error,    Lumi*lumiCorrFactor*JtoTauCorrectionFactor*hltEff_/1000., hCleaner, sbinSSaIso, 1);
	    hDataAntiIsoLooseTauIso->Add(hCleaner, -1.0);
	    float NormTTjets = 0.; 
            drawHistogram("MC", version_, RUN,mapAllTrees["TTbar"], variable, NormTTjets,     Error,   Lumi*TTxsectionRatio*scaleFactorTTSS*hltEff_/1000., hCleaner, sbinSSaIso, 1);
	    hDataAntiIsoLooseTauIso->Add(hCleaner, -1.0);
	    hDataAntiIsoLooseTauIsoQCD->Add(hDataAntiIsoLooseTauIso, hQCD->Integral()/hDataAntiIsoLooseTauIso->Integral());
	  }

	  else if(selection_.find("novbfLow")!=string::npos) {
	    TH1F* hExtrapSS = new TH1F("hExtrapSS","",nBins , bins.GetArray());
	    TCut sbinPZetaRelSSForWextrapolation = sbinPZetaRelSS;
	    float dummy1 = 0.;      
	    evaluateQCD(mapAllTrees, version_, RUN, hDataAntiIsoLooseTauIso, hCleaner, true, "SS", false, removeMtCut, selection_, 
			SSQCDinSignalRegionDATA , dummy1 , scaleFactorTTSS,
			extrapFactorWSS, 
			SSWinSignalRegionDATA, SSWinSignalRegionMC,
			SSWinSidebandRegionDATA, SSWinSidebandRegionMC,
			hExtrapSS, variable,
			Lumi/1000*hltEff_,  TTxsectionRatio, lumiCorrFactor,
			ExtrapolationFactorSidebandZDataMC, ExtrapolationFactorZDataMC,
			MutoTauCorrectionFactor, JtoTauCorrectionFactor, 
			OStoSSRatioQCD,
			antiWsdb, antiWsgn, useMt,
			sbinSS,
			sbinPZetaRelSSForWextrapolation,
			sbinPZetaRelSS, pZ, apZ, sbinPZetaRelSSInclusive, 
			sbinPZetaRelSSaIsoInclusive, sbinPZetaRelSSaIso, sbinPZetaRelSSaIsoMtiso, 
			vbfLoose, oneJet, zeroJet, true, true);
	    
	    hDataAntiIsoLooseTauIsoQCD->Add(hDataAntiIsoLooseTauIso, hQCD->Integral()/hDataAntiIsoLooseTauIso->Integral());
	    delete hExtrapSS;
	    hCleaner->Reset();
	  }

	  else if(selection_.find("bTag")!=string::npos && selection_.find("nobTag")==string::npos){
	    drawHistogram("Data", version_, RUN, currentTree, variable, NormData,  Error, 1.0 , hCleaner, sbinSSaIso ,1); 
            hDataAntiIsoLooseTauIso->Add(hCleaner); 
            float NormDYMutoTau = 0; 
	    if(useZDataMC)
	      drawHistogram("MC", version_, RUN,mapAllTrees["DYMutoTau"], variable, NormDYMutoTau, Error,   Lumi*lumiCorrFactor*MutoTauCorrectionFactor*ExtrapolationFactorZDataMC*hltEff_/1000., hCleaner, sbinSSaIso, 1); 
	    else
	      drawHistogram("MC", version_, RUN,mapAllTrees["DYMutoTau"], variable, NormDYMutoTau, Error,   Lumi*lumiCorrFactor*MutoTauCorrectionFactor*hltEff_/1000., hCleaner, sbinSSaIso, 1);
            hDataAntiIsoLooseTauIso->Add(hCleaner, -1.0); 
            float NormDYJtoTau = 0.;  
	    if(useZDataMC)
	      drawHistogram("MC", version_, RUN,mapAllTrees["DYJtoTau"], variable, NormDYJtoTau, Error,    Lumi*lumiCorrFactor*JtoTauCorrectionFactor*ExtrapolationFactorZDataMC*hltEff_/1000., hCleaner, sbinSSaIso, 1); 
	    else
	      drawHistogram("MC", version_, RUN,mapAllTrees["DYJtoTau"], variable, NormDYJtoTau, Error,    Lumi*lumiCorrFactor*JtoTauCorrectionFactor*hltEff_/1000., hCleaner, sbinSSaIso, 1);
            hDataAntiIsoLooseTauIso->Add(hCleaner, -1.0); 
            float NormTTjets = 0.;  
            drawHistogram("MC", version_, RUN,mapAllTrees["TTbar"], variable, NormTTjets,     Error,   Lumi*TTxsectionRatio*scaleFactorTTOSWJets*hltEff_/1000., hCleaner, sbinSSaIso, 1); 
            hDataAntiIsoLooseTauIso->Add(hCleaner, -1.0); 
	    if(selection_.find("High")!=string::npos){
	      //get efficiency of events passing QCD selection to pass the category selection  
	      drawHistogram("Data", version_, RUN, currentTree, variable, NormData,  Error, 1.0 , hCleaner, sbinSSaIsoInclusive,1); 
	      float tmpNormQCDSel = hCleaner->Integral(); 
	      drawHistogram("Data", version_, RUN, currentTree, variable, NormData,  Error, 1.0 , hCleaner, sbinSSaIso, 1);  
	      float tmpNormCatSel = hCleaner->Integral(); 
	      float effQCDToCatSel = tmpNormCatSel/tmpNormQCDSel; 
	      //Normalize to Inclusive measured QCD times the above efficiency 
	      hDataAntiIsoLooseTauIsoQCD->Add(hDataAntiIsoLooseTauIso, (effQCDToCatSel*SSQCDinSignalRegionDATAIncl)/hDataAntiIsoLooseTauIso->Integral()); 
	    }
	    else{
	      hDataAntiIsoLooseTauIsoQCD->Add(hDataAntiIsoLooseTauIso, hQCD->Integral()/hDataAntiIsoLooseTauIso->Integral());
	    }
	  }
	  else{
	    drawHistogram("Data", version_, RUN, currentTree, variable, NormData,  Error, 1.0 , hCleaner, sbinSSaIsoMtiso ,1);
	    hDataAntiIsoLooseTauIso->Add(hCleaner, SSIsoToSSAIsoRatioQCD);
	    hDataAntiIsoLooseTauIsoQCD->Add(hDataAntiIsoLooseTauIso, hQCD->Integral()/hDataAntiIsoLooseTauIso->Integral());
	  }
	  //hDataAntiIsoLooseTauIsoQCD->Sumw2();
	  //hDataAntiIsoLooseTauIsoQCD->Add(hDataAntiIsoLooseTauIso, hQCD->Integral()/hDataAntiIsoLooseTauIso->Integral());

	  drawHistogram("Data", version_, RUN, currentTree, variable, NormData,  Error, 1.0 , hCleaner,  sbinSSlIso1 ,1);
	  hLooseIso1->Add(hCleaner, 1.0);
	  drawHistogram("Data", version_, RUN, currentTree, variable, NormData,  Error, 1.0 , hCleaner,  sbinSSlIso2 ,1);
	  hLooseIso2->Add(hCleaner, 1.0);
	  drawHistogram("Data", version_, RUN, currentTree, variable, NormData,  Error, 1.0 , hCleaner,  sbinSSlIso3 ,1);
	  hLooseIso3->Add(hCleaner, 1.0);
	  drawHistogram("Data", version_, RUN, currentTree, variable, NormData,  Error, 1.0 , hCleaner,  sbinSSaIso  ,1);
	  hAntiIso->Add(hCleaner, 1.0);
	  drawHistogram("Data_FR", version_, RUN, currentTree, variable, NormData,  Error, 1.0 , hCleaner,  sbinSSaIso ,1);
	  hAntiIsoFR->Add(hCleaner, 1.0);
	  
	  cleanQCDHisto(mapAllTrees, version_, RUN, true,hCleaner, hLooseIso1, variable, 
			Lumi*hltEff_/1000., (SSWinSidebandRegionDATA/SSWinSidebandRegionMC), 
			TTxsectionRatio*scaleFactorTTSS, MutoTauCorrectionFactor*lumiCorrFactor, 
			JtoTauCorrectionFactor*lumiCorrFactor, lumiCorrFactor*ExtrapolationFactorZDataMC,sbinSSlIso1);
 	  cleanQCDHisto(mapAllTrees, version_, RUN, true,hCleaner, hLooseIso2, variable, 
 			Lumi*hltEff_/1000., SSWinSidebandRegionDATA/SSWinSidebandRegionMC, 
 			TTxsectionRatio*scaleFactorTTSS, MutoTauCorrectionFactor*lumiCorrFactor, 
 			JtoTauCorrectionFactor*lumiCorrFactor, lumiCorrFactor*ExtrapolationFactorZDataMC,sbinSSlIso2);
 	  cleanQCDHisto(mapAllTrees, version_, RUN, true,hCleaner, hLooseIso3, variable, 
 			Lumi*hltEff_/1000., SSWinSidebandRegionDATA/SSWinSidebandRegionMC, 
 			TTxsectionRatio*scaleFactorTTSS, MutoTauCorrectionFactor*lumiCorrFactor, 
 			JtoTauCorrectionFactor*lumiCorrFactor, lumiCorrFactor*ExtrapolationFactorZDataMC,sbinSSlIso3);


	  drawHistogram("Data", version_, RUN, currentTree, variable, NormData,  Error, 1.0 , hCleaner,  sbinSSInclusive&&vbfLoose , 1);
	  hSSLooseVBF->Add(hCleaner, 1.0);
	  cleanQCDHisto(mapAllTrees, version_, RUN, false,hCleaner, hSSLooseVBF, variable, 
 			Lumi*hltEff_/1000., SSWinSidebandRegionDATA/SSWinSidebandRegionMC, 
 			TTxsectionRatio*scaleFactorTTSS, MutoTauCorrectionFactor*lumiCorrFactor, 
 			JtoTauCorrectionFactor*lumiCorrFactor, lumiCorrFactor*ExtrapolationFactorZDataMC,sbinSSInclusive&&vbfLoose);
	  hSSLooseVBF->Scale(hSS->Integral()/hSSLooseVBF->Integral());

	} 
	else if(currentName.Contains("VBFH")  || 
		currentName.Contains("GGFH")  ||
		currentName.Contains("VH")   ){
		//currentName.Contains("SUSY")){

	  float NormSign = 0.;
	  if(VERBOSE) cout << "SIGNAL " << currentName << " ENTRIES " << currentTree->GetEntries() << endl;
	  drawHistogram("MC", version_, RUN,currentTree, variable, NormSign, Error,    Lumi*hltEff_/1000., h1, sbin, 1);

	  if(currentName.Contains("VBFH"+TmH_)){
	    hSgn1->Add(h1,1.0);
	    hSgn1->Scale(magnifySgn_);
	    hSgn->Add(hSgn1,1.0);
	  }
	  else if(currentName.Contains("GGFH"+TmH_)){
	    hSgn2->Add(h1,1.0);
	    hSgn2->Scale(magnifySgn_);
	    hSgn->Add(hSgn2,1.0);
	  }
	  else  if(currentName.Contains("VH"+TmH_)){
	    hSgn3->Add(h1,1.0);
	    hSgn3->Scale(magnifySgn_);
	    hSgn->Add(hSgn3,1.0);
	  }	   

	  for(int iP=0 ; iP<nProd ; iP++)
	    for(int iM=0 ; iM<nMasses ; iM++)
	      if(currentName.Contains(nameProd[iP]+nameMasses[iM]))
		hSignal[iP][iM]->Add(h1,1.0);

// 	  if(currentName.Contains("SUSY")){
// 	    TH1F* histoSusy =  (mapSUSYhistos.find( (it->first) ))->second;
// 	    histoSusy->Add(h1,1.0);
// 	    histoSusy->SetLineWidth(2);
// 	  }

	}

      }

      else{
	if(selection_.find("vbf")!=string::npos && selection_.find("novbf")==string::npos){
	  /* //HCP method 
	    float NormEmbed = 0.;
	  TCut sbinEmbeddingLoose = sbinEmbeddingInclusive && vbfLoose;
	  drawHistogram("Embed", version_, RUN,currentTree, variable, NormEmbed,  Error, 1.0 , hCleaner,  sbinEmbeddingLoose  ,1);
	  hDataEmb->Add(hCleaner, 1.0);
	  NormEmbed = 0.; 
	  drawHistogram("Embed", version_, RUN,currentTree, variable, NormEmbed,  Error, 1.0 , h1,  sbinEmbedding  ,1); 
	  h1->Scale( (ExtrapolationFactorZ*ExtrapDYInclusivePZetaRel*ExtrapolationFactorZFromSideband)/h1->Integral()); 
	  hDataEmb->Scale(h1->Integral()/hDataEmb->Integral());
	  */
	  //Moriond method
	  float NormEmbed = 0.; 
          drawHistogram("Embed", version_, RUN,currentTree, variable, NormEmbed,  Error, 1.0 , h1,  sbinEmbedding  ,1); 
          h1->Scale( (ExtrapolationFactorZ*ExtrapDYInclusivePZetaRel*ExtrapolationFactorZFromSideband)/h1->Integral()); 
          hDataEmb->Add(h1, 1.0); 
	}
	else{
	  float NormEmbed = 0.;
	  drawHistogram("Embed", version_, RUN,currentTree, variable, NormEmbed,  Error, 1.0 , h1,  sbinEmbedding  ,1);
	  h1->Scale( (ExtrapolationFactorZ*ExtrapDYInclusivePZetaRel*ExtrapolationFactorZFromSideband)/h1->Integral());
	  hDataEmb->Add(h1, 1.0);
	}
      }
    }
  
    /////////////////////////////////////////////////////////////////////////////////////

    if(VERBOSE) cout<<(it->first) << " ==> " 
		    << h1->Integral() << " +/- " 
		    << TMath::Sqrt(h1->GetEntries())*(h1->Integral()/h1->GetEntries())
		    << endl;

    //out << (it->first) << "  " << h1->Integral() << " $\\pm$ " <<  TMath::Sqrt(h1->GetEntries())*(h1->Integral()/h1->GetEntries()) << endl;
    char* c = new char[50];
    if(h1->Integral()>=10) 
      sprintf(c,"$%.0f\\pm%.0f$",h1->Integral(),  TMath::Sqrt(h1->GetEntries())*(h1->Integral()/h1->GetEntries()));
    else if(h1->Integral()>=1)
      sprintf(c,"$%.1f\\pm%.1f$",h1->Integral(),  TMath::Sqrt(h1->GetEntries())*(h1->Integral()/h1->GetEntries()));
    else if(h1->Integral()>=0.1)
      sprintf(c,"$%.2f\\pm%.2f$",h1->Integral(),  TMath::Sqrt(h1->GetEntries())*(h1->Integral()/h1->GetEntries()));
    else if(h1->Integral()>=0.01)
      sprintf(c,"$%.3f\\pm%.3f$",h1->Integral(),  TMath::Sqrt(h1->GetEntries())*(h1->Integral()/h1->GetEntries()));
    else
      sprintf(c,"$%.5f\\pm%.5f$",h1->Integral(),  TMath::Sqrt(h1->GetEntries())*(h1->Integral()/h1->GetEntries()));
    out << string(c) << "  //" << (it->first) << endl;
    delete c;

    delete hCleaner;
  }
  out.close();

  //TFile* ftest2 = new TFile(Form(location+"/%s/histograms/test2_mH%d_%s_%s_%s.root",outputDir.Data(), mH_,selection_.c_str(),analysis_.c_str(),variable_.Data()),"RECREATE");
  //ftest2->cd();
  //hData->Write();
  //ftest2->Write();
  //ftest2->Close();
  //return;

  cout << endl;
  cout << "All samples done. Filling hParameters..." << endl;
  hParameters->SetBinContent(1, ExtrapolationFactorZ);               hParameters->GetXaxis()->SetBinLabel(1,"ExtrapolationFactorZ");
  hParameters->SetBinContent(2, ErrorExtrapolationFactorZ);          hParameters->GetXaxis()->SetBinLabel(2,"ErrorExtrapolationFactorZ");
  hParameters->SetBinContent(3, ExtrapolationFactorZDataMC);         hParameters->GetXaxis()->SetBinLabel(3,"ExtrapolationFactorZDataMC");
  hParameters->SetBinContent(4, ExtrapolationFactorZFromSideband);   hParameters->GetXaxis()->SetBinLabel(4,"ExtrapolationFactorZFromSideband");
  hParameters->SetBinContent(5, ExtrapolationFactorSidebandZDataMC); hParameters->GetXaxis()->SetBinLabel(5,"ExtrapolationFactorSidebandZDataMC");
  hParameters->SetBinContent(6, extrapFactorWSSIncl);                hParameters->GetXaxis()->SetBinLabel(6,"extrapFactorWSSIncl");
  hParameters->SetBinContent(7, SSWinSignalRegionDATAIncl);          hParameters->GetXaxis()->SetBinLabel(7,"SSWinSignalRegionDATAIncl");
  hParameters->SetBinContent(8, SSWinSignalRegionMCIncl);            hParameters->GetXaxis()->SetBinLabel(8,"SSWinSignalRegionMCIncl");
  hParameters->SetBinContent(9, SSQCDinSignalRegionDATAIncl);        hParameters->GetXaxis()->SetBinLabel(9,"SSQCDinSignalRegionDATAIncl");
  hParameters->SetBinContent(10,extrapFactorWSS);                    hParameters->GetXaxis()->SetBinLabel(10,"extrapFactorWSS");
  hParameters->SetBinContent(11,SSWinSignalRegionDATA);              hParameters->GetXaxis()->SetBinLabel(11,"SSWinSignalRegionDATA");
  hParameters->SetBinContent(12,SSWinSignalRegionMC);                hParameters->GetXaxis()->SetBinLabel(12,"SSWinSignalRegionMC");
  hParameters->SetBinContent(13,SSQCDinSignalRegionDATA);            hParameters->GetXaxis()->SetBinLabel(13,"SSQCDinSignalRegionDATA");
  hParameters->SetBinContent(14,extrapFactorWOSWJets);               hParameters->GetXaxis()->SetBinLabel(14,"extrapFactorWOSWJets");
  hParameters->SetBinContent(15,OSWinSignalRegionDATAWJets);         hParameters->GetXaxis()->SetBinLabel(15,"OSWinSignalRegionDATAWJets");
  hParameters->SetBinContent(16,OSWinSignalRegionMCWJets );          hParameters->GetXaxis()->SetBinLabel(16,"OSWinSignalRegionMCWJets");   
  hParameters->SetBinContent(17,extrapFactorWOSW3Jets);              hParameters->GetXaxis()->SetBinLabel(17,"extrapFactorWOSW3Jets");
  hParameters->SetBinContent(18,OSWinSignalRegionDATAW3Jets);        hParameters->GetXaxis()->SetBinLabel(18,"OSWinSignalRegionDATAW3Jets");
  hParameters->SetBinContent(19,OSWinSignalRegionMCW3Jets );         hParameters->GetXaxis()->SetBinLabel(19,"OSWinSignalRegionMCW3Jets");
  hParameters->SetBinContent(20,scaleFactorTTOS);                    hParameters->GetXaxis()->SetBinLabel(20,"scaleFactorTTOS");
  hParameters->SetBinContent(21,scaleFactorTTSS);                    hParameters->GetXaxis()->SetBinLabel(21,"scaleFactorTTSS");
  hParameters->SetBinContent(22,scaleFactorTTSSIncl);                hParameters->GetXaxis()->SetBinLabel(22,"scaleFactorTTSSIncl");
  hParameters->SetBinContent(23,SSIsoToSSAIsoRatioQCD);              hParameters->GetXaxis()->SetBinLabel(23,"SSIsoToSSAIsoRatioQCD");
  hParameters->SetBinContent(24,ExtrapDYInclusive);                  hParameters->GetXaxis()->SetBinLabel(24,"ExtrapDYInclusive");
  hParameters->SetBinContent(25,ExtrapDYInclusivePZetaRel);          hParameters->GetXaxis()->SetBinLabel(25,"ExtrapDYInclusivePZetaRel");
  hParameters->SetBinContent(26,ExtrapLFakeInclusive);               hParameters->GetXaxis()->SetBinLabel(26,"ExtrapLFakeInclusive");
  hParameters->SetBinContent(27,ExtrapJFakeInclusive);               hParameters->GetXaxis()->SetBinLabel(27,"ExtrapJFakeInclusive");
  hParameters->SetBinContent(28,ExtrapolationFactorMadGraph);        hParameters->GetXaxis()->SetBinLabel(28,"ExtrapolationFactorMadGraph");
  hParameters->SetBinContent(29,ErrorExtrapolationFactorMadGraph);   hParameters->GetXaxis()->SetBinLabel(29,"ErrorExtrapolationFactorMadGraph");

  hParameters->GetXaxis()->LabelsOption("v");

//YIELDS

  if(selection_.find("vbf")!=string::npos && selection_.find("novbf")==string::npos){
    out<<"Yields for VBF :"<<endl;
    out<<"VBF data : hData -> "<<hData->Integral()<<endl;
    out<<"VBF Ztt : hDataEmb -> "<<hDataEmb->Integral()<<endl;
    out<<"VBF QCD : hDataAntiIsoLooseTauIsoQCD -> "<<hDataAntiIsoLooseTauIsoQCD->Integral()<<endl;
    out<<"VBF W : hW3JetsMediumTauIsoRelVBF -> "<<hW3JetsMediumTauIsoRelVBF->Integral()<<endl;
    out<<"VBF Zee j->t : hZmj -> "<<hZmj->Integral()<<endl;
    out<<"VBF Zee e->t : hZmm -> "<<hZmm->Integral()<<endl;
    out<<"VBF TTb : hTTb -> "<<hTTb->Integral()<<endl;
    out<<"VBF VV : hVV -> "<<hVV->Integral()<<endl;
  }
  else if(selection_.find("boost")!=string::npos){
    out<<"Yields for boost :"<<endl;
    out<<"boost data : hData -> "<<hData->Integral()<<endl;
    out<<"boost Ztt : hDataEmb -> "<<hDataEmb->Integral()<<endl;
    out<<"boost high QCD : hDataAntiIsoLooseTauIsoQCD -> "<<hDataAntiIsoLooseTauIsoQCD->Integral()<<endl;
    out<<"boost low QCD : hQCD -> "<<hQCD->Integral()<<endl;
    out<<"boost W : hW -> "<<hW->Integral()<<endl;
    out<<"boost Zee j->t : hZmj -> "<<hZmj->Integral()<<endl;
    out<<"boost Zee e->t : hZmm -> "<<hZmm->Integral()<<endl;
    out<<"boost TTb : hTTb -> "<<hTTb->Integral()<<endl;
    out<<"boost VV : hVV -> "<<hVV->Integral()<<endl;
  }
  else if(selection_.find("bTag")!=string::npos){
    out<<"Yields for bTag :"<<endl;
    out<<"bTag data : hData -> "<<hData->Integral()<<endl;
    out<<"bTag Ztt : hDataEmb -> "<<hDataEmb->Integral()<<endl;
    out<<"bTag high QCD : hDataAntiIsoLooseTauIso -> "<<hDataAntiIsoLooseTauIso->Integral()<<endl;
    out<<"bTag W : hW -> "<<hW->Integral()<<endl;
    out<<"bTag Zee j->t : hZmj -> "<<hZmj->Integral()<<endl;
    out<<"bTag Zee e->t : hZmm -> "<<hZmm->Integral()<<endl;
    out<<"bTag TTb : hTTb -> "<<hTTb->Integral()<<endl;
    out<<"bTag VV : hVV -> "<<hVV->Integral()<<endl;
  }
  else{
    out<<"Yields for "<<selection_<<" : "<<endl;
    out<<selection_<<" data : hData -> "<<hData->Integral()<<endl;
    out<<selection_<<" Ztt : hDataEmb -> "<<hDataEmb->Integral()<<endl;
    out<<selection_<<" high QCD : hQCD -> "<<hQCD->Integral()<<endl;
    out<<selection_<<" W : hW -> "<<hW->Integral()<<endl;
    out<<selection_<<" Zee j->t : hZmj -> "<<hZmj->Integral()<<endl;
    out<<selection_<<" Zee e->t : hZmm -> "<<hZmm->Integral()<<endl;
    out<<selection_<<" TTb : hTTb -> "<<hTTb->Integral()<<endl;
    out<<selection_<<" VV : hVV -> "<<hVV->Integral()<<endl;
  }
  out.close();

  if(scaleByBinWidth && variable_.Contains("diTauNSVfitMass")){ 
    hData->Scale(1.0, "width");
    hTTb->Scale(1.0, "width");
    hDataEmb->Scale(1.0, "width");
    hZtt->Scale(1.0, "width");
    hDataAntiIsoLooseTauIsoQCD->Scale(1.0, "width");
    hQCD->Scale(1.0, "width");
    hSSLooseVBF->Scale(1.0, "width");
    hSS->Scale(1.0, "width");
    hEWK->Scale(1.0, "width");
    hSgn->Scale(1.0, "width");
  }

  hSiml->Add(hTTb,1.0);
  if(useEmbedding_)
    hSiml->Add(hDataEmb,1.0);
  else 
    hSiml->Add(hZtt);

  if(!USESSBKG){
    if(selection_.find("vbf")!=string::npos && selection_.find("novbf")==string::npos)
      hSiml->Add(hDataAntiIsoLooseTauIsoQCD,1.0);
    else if(selection_.find("novbf")!=string::npos || selection_.find("boost")!=string::npos)
      hSiml->Add(hDataAntiIsoLooseTauIsoQCD,1.0);
    else if(selection_.find("bTag")!=string::npos && selection_.find("nobTag")==string::npos)
      hSiml->Add(hDataAntiIsoLooseTauIsoQCD,1.0);
    else
      hSiml->Add(hQCD,1.0);
  }
  else{
    if(selection_.find("vbf")!=string::npos && selection_.find("novbf")==string::npos){
      hSiml->Add(hSSLooseVBF,1.0);
    }
    else
      hSiml->Add(hSS,1.0);
  }

  hSiml->Add(hEWK,1.0);

  if(!USESSBKG){
    if(selection_.find("vbf")!=string::npos && selection_.find("novbf")==string::npos){
      aStack->Add(hDataAntiIsoLooseTauIsoQCD);
    }
    else if(selection_.find("novbf")!=string::npos || selection_.find("boost")!=string::npos) 
      aStack->Add(hDataAntiIsoLooseTauIsoQCD);
    else if(selection_.find("bTag")!=string::npos && selection_.find("nobTag")==string::npos)
      aStack->Add(hDataAntiIsoLooseTauIsoQCD);
    else
      aStack->Add(hQCD);
  }
  else{
    if(selection_.find("vbf")!=string::npos && selection_.find("novbf")==string::npos){
      aStack->Add(hSSLooseVBF);
    }
    else
      aStack->Add(hSS);
  }
  
  aStack->Add(hEWK);
  aStack->Add(hTTb);
  if(useEmbedding_)
    aStack->Add(hDataEmb);
  else
    aStack->Add(hZtt);
  if(!logy_)
    aStack->Add(hSgn);
  
  leg->AddEntry(hData,"Observed","P");
  leg->AddEntry(hSgn,Form("(%.0fx) H#rightarrow#tau#tau m_{H}=%d",magnifySgn_,mH_),"F");
  if(useEmbedding_)
    leg->AddEntry(hDataEmb,"Z#rightarrow#tau#tau (embedded)","F");
  else
    leg->AddEntry(hZtt,"Z#rightarrow#tau#tau","F"); 
  leg->AddEntry(hTTb,"t#bar{t}","F");
  leg->AddEntry(hEWK,"Electroweak","F");
  leg->AddEntry(hQCD,"QCD","F");
  
  hData->Draw("P");
  aStack->Draw("HISTSAME");
  hData->Draw("PSAME");
  
  TH1F* hStack = (TH1F*)aStack->GetHistogram();
  hStack->SetXTitle(XTitle_+" ("+Unities_+")");
  if(scaleByBinWidth && variable_.Contains("diTauNSVfitMass")){
    hStack->SetYTitle(Form(" Events/(%.0f %s)", 1.0, Unities_.Data() ) );
    hData->SetYTitle(Form(" Events/(%.0f %s)", 1.0, Unities_.Data() ) );
  }
  else 
    hStack->SetYTitle(Form(" Events/(%.0f %s)", hStack->GetBinWidth(1), Unities_.Data() ) );
  hStack->SetTitleSize(0.04,"X");
  hStack->SetTitleSize(0.05,"Y");
  hStack->SetTitleOffset(0.95,"Y");
  if(!logy_)
    hData->SetAxisRange(0.0, TMath::Max( hData->GetMaximum(), hSiml->GetMaximum() )*maxY_ ,"Y");
  else
    hData->SetAxisRange(0.1, TMath::Max( hData->GetMaximum(), hSiml->GetMaximum() )*maxY_ ,"Y");
  aStack->Draw("HISTSAME");
  hData->Draw("PSAME");
  if(logy_)
    hSgn->Draw("HISTSAME");
 
  leg->Draw();

  pad2->cd();
  gStyle->SetOptStat(0);
  gStyle->SetTitleFillColor(0);
  gStyle->SetCanvasBorderMode(0);
  gStyle->SetCanvasColor(0);
  gStyle->SetPadBorderMode(0);
  gStyle->SetPadColor(0);
  gStyle->SetTitleFillColor(0);
  gStyle->SetTitleBorderSize(0);
  gStyle->SetTitleH(0.07);
  gStyle->SetTitleFontSize(0.1);
  gStyle->SetTitleStyle(0);
  gStyle->SetTitleOffset(1.3,"y");

  TH1F* hRatio = new TH1F( "hRatio" ," ; ; #frac{(DATA-MC)}{#sqrt{DATA}}" , nBins , bins.GetArray());
  hRatio->Reset();
  hRatio->SetXTitle("");
  hRatio->SetYTitle("#frac{(DATA-MC)}{MC}");

  hRatio->SetMarkerStyle(kFullCircle);
  hRatio->SetMarkerSize(0.8);
  hRatio->SetLabelSize(0.12,"X");
  hRatio->SetLabelSize(0.10,"Y");
  hRatio->SetTitleSize(0.12,"Y");
  hRatio->SetTitleOffset(0.36,"Y");

  float maxPull = 0.;
  for(int k = 0 ; k < hRatio->GetNbinsX(); k++){
    float pull = hData->GetBinContent(k) - hSiml->GetBinContent(k);
    if(hSiml->GetBinContent(k)>0)
      pull /= hSiml->GetBinContent(k);
    hRatio->SetBinContent(k, pull);
    if(TMath::Abs(pull) > maxPull)
      maxPull = TMath::Abs(pull);
  }
  hRatio->SetAxisRange(-1.2*maxPull,1.2*maxPull,"Y");
  hRatio->Draw("P");

  TF1* line = new TF1("line","0",hRatio->GetXaxis()->GetXmin(),hStack->GetXaxis()->GetXmax());
  line->SetLineStyle(3);
  line->SetLineWidth(1.5);
  line->SetLineColor(kBlack);
  line->Draw("SAME");
  
  //return;

  c1->SaveAs(Form(location+"/%s/plots/plot_muTau_mH%d_%s_%s_%s.png",outputDir.Data(), mH_,selection_.c_str(),analysis_.c_str(),variable_.Data()));
  c1->SaveAs(Form(location+"/%s/plots/plot_muTau_mH%d_%s_%s_%s.pdf",outputDir.Data(), mH_,selection_.c_str(),analysis_.c_str(),variable_.Data()));

  TH1F *hDataBlind, *hRatioBlind;

  // Plot blinded histogram
  if(variable_.Contains("Mass")) {

    hDataBlind  = blindHistogram(hData,  100, 160, "hDataBlind");
    hRatioBlind = blindHistogram(hRatio, 100, 160, "hRatioBlind");

    c1 = new TCanvas("c2","",5,30,650,600);
    c1->SetGrid(0,0);
    c1->SetFillStyle(4000);
    c1->SetFillColor(10);
    c1->SetTicky();
    c1->SetObjectStat(0);
    c1->SetLogy(logy_);

    pad1 = new TPad("pad2_1DEta","",0.05,0.22,0.96,0.97);
    pad2 = new TPad("pad2_2DEta","",0.05,0.02,0.96,0.20);
    
    pad1->SetFillColor(0);
    pad2->SetFillColor(0);
    pad1->Draw();
    pad2->Draw();
    
    pad1->cd();
    pad1->SetLogy(logy_);
    gStyle->SetOptStat(0);
    gStyle->SetTitleFillColor(0);
    gStyle->SetCanvasBorderMode(0);
    gStyle->SetCanvasColor(0);
    gStyle->SetPadBorderMode(0);
    gStyle->SetPadColor(0);
    gStyle->SetTitleFillColor(0);
    gStyle->SetTitleBorderSize(0);
    gStyle->SetTitleH(0.07);
    gStyle->SetTitleFontSize(0.1);
    gStyle->SetTitleStyle(0);
    gStyle->SetTitleOffset(1.3,"y");

    hDataBlind->Draw("P");
    aStack->Draw("HISTSAME");
    hDataBlind->Draw("PSAME");
    if(logy_) hSgn->Draw("HISTSAME");
    leg->Draw();

    pad2->cd();
    gStyle->SetOptStat(0);
    gStyle->SetTitleFillColor(0);
    gStyle->SetCanvasBorderMode(0);
    gStyle->SetCanvasColor(0);
    gStyle->SetPadBorderMode(0);
    gStyle->SetPadColor(0);
    gStyle->SetTitleFillColor(0);
    gStyle->SetTitleBorderSize(0);
    gStyle->SetTitleH(0.07);
    gStyle->SetTitleFontSize(0.1);
    gStyle->SetTitleStyle(0);
    gStyle->SetTitleOffset(1.3,"y");

    hRatioBlind->Draw("P");  

    c1->SaveAs(Form(location+"/%s/plots/plot_muTau_mH%d_%s_%s_%s_blind.png",outputDir.Data(), mH_,selection_.c_str(),analysis_.c_str(),variable_.Data()));
    c1->SaveAs(Form(location+"/%s/plots/plot_muTau_mH%d_%s_%s_%s_blind.pdf",outputDir.Data(), mH_,selection_.c_str(),analysis_.c_str(),variable_.Data()));

  }

  // templates for fitting
  TFile* fout = new TFile(Form(location+"/%s/histograms/muTau_mH%d_%s_%s_%s.root",outputDir.Data(), mH_,selection_.c_str(),analysis_.c_str(),variable_.Data()),"RECREATE");
  fout->cd();

  hSiml->Write();
  hQCD->Write();
  hQCDrelaxVBF->Write();
  hSS->Write();
  hSSLooseVBF->Write();
  hZmm->Write();
  hZmmLoose->Write();
  hZmj->Write();
  hZmjLoose->Write();
  hZfakes->Write();
  hTTb->Write();
  hZtt->Write();
  hDataEmb->Write();
  hLooseIso1->Write();
  hLooseIso2->Write();
  hLooseIso3->Write();
  hAntiIso->Write();
  hAntiIsoFR->Write();
  hW->Write();
  hWSS->Write();
  hWMinusSS->Write();
  hW3Jets->Write();
  hVV->Write();
  hEWK->Write();
  hSgn1->Write();
  hSgn2->Write();
  hSgn3->Write();
  hW3JetsLooseTauIso->Write();
  hW3JetsMediumTauIso->Write();
  hW3JetsMediumTauIsoRelVBF->Write();
  hW3JetsMediumTauIsoRelVBFMinusSS->Write();
  hWLooseBTag->Write();
  hDataAntiIsoLooseTauIso->Write();
  hDataAntiIsoLooseTauIsoFullVBF->Write();
  hDataAntiIsoLooseTauIsoRelaxVBF->Write();
  hDataAntiIsoLooseTauIsoQCD->Write();
  hData->Write();
  hParameters->Write();

  for(int iP=0 ; iP<nProd ; iP++)
    for(int iM=0 ; iM<nMasses ; iM++)
      if(hSignal[iP][iM]) hSignal[iP][iM]->Write();

//   for(unsigned int i = 0; i < SUSYhistos.size() ; i++){
//     ((mapSUSYhistos.find( SUSYhistos[i] ))->second)->Write();
//   }
 

  fout->Write();
  fout->Close();

  delete hQCD; delete hSS; delete hSSLooseVBF; delete hZmm; delete hZmj; delete hZfakes; delete hTTb; delete hZtt; 
  delete hW; delete hWSS; delete hWMinusSS; delete hW3Jets; delete hAntiIso; delete hAntiIsoFR;
  delete hZmmLoose; delete hZmjLoose; delete hLooseIso1; delete hLooseIso2; delete hLooseIso3;
  delete hVV; delete hSgn; delete hSgn1; delete hSgn2; delete hSgn3; delete hData; delete hParameters;
  delete hW3JetsLooseTauIso; delete hW3JetsMediumTauIso; delete hW3JetsMediumTauIsoRelVBF; delete hW3JetsMediumTauIsoRelVBFMinusSS; delete hWLooseBTag;
  delete hDataAntiIsoLooseTauIso; delete hDataAntiIsoLooseTauIsoQCD; delete hDataAntiIsoLooseTauIsoFullVBF;

  for(int iP=0 ; iP<nProd ; iP++)
    for(int iM=0 ; iM<nMasses ; iM++)
      if(hSignal[iP][iM]) delete hSignal[iP][iM];

//   for(unsigned int i = 0; i < SUSYhistos.size() ; i++) delete mapSUSYhistos.find( SUSYhistos[i] )->second ;

      
  delete aStack;  delete hEWK; delete hSiml; delete hDataEmb;  delete hRatio; delete line;
  delete fout;

  for(int iP=0 ; iP<nProd ; iP++) {
    for(int iM=0 ; iM<nMasses ; iM++) {
      //signal[iP][iM]->Close();
      delete signal[iP][iM];
    }
  }

//   for(unsigned int i = 0; i < SUSYhistos.size() ; i++){
//     (mapSUSYfiles.find( SUSYhistos[i] )->second)->Close();
//     delete mapSUSYfiles.find( SUSYhistos[i] )->second ;
//   }

  
}


///////////////////////////////////////////////////////////////////////////////////////////////



void plotMuTauAll( Int_t useEmbedded = 1, TString outputDir = "TempTest"){
      
  vector<string> variables;
  vector<int> mH;

  //variables.push_back("diTauVisMass");
  variables.push_back("diTauNSVfitMass");
 
  //mH.push_back(105);
  //mH.push_back(110);
  //mH.push_back(115);
  //mH.push_back(120);
  mH.push_back(125);
  //mH.push_back(130);
  //mH.push_back(135);
  //mH.push_back(140);
  //mH.push_back(145);
  //mH.push_back(160);
  
  //plotMuTau(125,1,"inclusive",""   ,"decayMode",     "#tau_{h} decay mode","units"   ,outputDir,3,0,3, 5.0,1.0,0,1.4);
  //plotMuTau(125,1,"inclusive",""   ,"visibleTauMass","visible #tau_{h} mass","GeV"   ,outputDir,40,0,2,5.0,1.0,0,1.2);  
  //plotMuTau(125,1,"inclusive",""   ,"MEtMVA","MET","GeV"                        ,outputDir,40,0,100,5.0,1.0,0,1.2);
  //plotMuTau(125,1,"inclusive",""   ,"MEtMVAPhi","MET #phi","units"              ,outputDir,32,-3.2,3.2,   5.0,1.0,0,1.5);
  //plotMuTau(125,1,"inclusiveNoMt",""   ,"MtLeg1MVA","M_{T}(#mu#nu)","GeV" ,                  outputDir,40,0,160,5.0,1.0,0,1.2);
  //plotMuTau(125,1,"inclusive",""   ,"diTauVisMass","visible mass","GeV"      ,outputDir,50,0,200,5.0,1.0,0,1.2); 
  //plotMuTau(125,1,"inclusive",""   ,"diTauNSVfitMass","SVfit mass","GeV"     ,outputDir,60,0,360,5.0,1.0,0,1.2);
  //plotMuTau(125,1,"inclusive",""   ,"etaL1","#mu #eta", "units"              ,outputDir,25,-2.5, 2.5,5.0,1.0,0,2.);
  //plotMuTau(125,1,"inclusive",""   ,"ptL1","#mu p_{T}", "GeV"                ,outputDir,27,11, 92,5.0,1.0,0,1.2);
  //plotMuTau(125,1,"inclusive",""   ,"ptL2","#tau p_{T}","GeV"           ,outputDir,27,11, 92,5.0,1.0,0,1.2);
  //plotMuTau(125,1,"inclusive",""   ,"etaL2","#tau #eta","units"         ,outputDir,25,-2.5, 2.5,5.0,1.0,0,2.);
  //plotMuTau(125,1,"inclusive",""   ,"numPV","reconstructed vertexes","units"             ,outputDir,30,0,30,5.0,1.0,0,1.5);
  //plotMuTau(125,1,"inclusive",""   ,"nJets30","jet multiplicity","units"                 ,outputDir,10,0, 10,5.0,1.0,1,10);
  //plotMuTau(125,1,"bTag",""        ,"ptB1", "leading b-tagged jet p_{T}","GeV"       ,outputDir,50,30, 330,5.0,1.0,1,100);
  //plotMuTau(125,1,"bTag",""        ,"etaB1","leading b-tagged jet #eta","units"      ,outputDir,21,-5, 5,5.0,1.0,0,2.);
  //
  //plotMuTau(125,1,"novbfLow",""   ,"MEtMVA","MET","GeV"                        ,outputDir,10,0,100,5.0,1.0,0,   1.2);
  //plotMuTau(125,1,"novbfLow",""   ,"MEtMVAPhi","MET #phi","units"              ,outputDir,20,-3.2,3.2,   5.0,1.0,0,1.5);
  //plotMuTau(125,1,"novbfLowNoMt",""   ,"MtLeg1MVA","M_{T}(#mu#nu)","GeV" ,                  outputDir,16,0,160,5.0,1.0,0,1.2);
  //plotMuTau(125,1,"novbfLow",""   ,"diTauVisMass","visible mass","GeV"      ,outputDir,20,0,200,5.0,1.0,0,1.2);  
  ////plotMuTau(125,1,"novbfLow",""   ,"diTauNSVfitMass","SVfit mass","GeV"     ,outputDir,-1,0,100,5.0,1.0,0,1.2);
  //plotMuTau(125,1,"novbfLow",""   ,"etaL1","#mu #eta", "units"              ,outputDir,10,-2.5, 2.5,5.0,1.0,0,2.);
  //plotMuTau(125,1,"novbfLow",""   ,"ptL1","#mu p_{T}", "GeV"                ,outputDir,16, 15, 95,5.0,1.0,0,1.2);
  //plotMuTau(125,1,"novbfLow",""   ,"ptL2","#tau p_{T}","GeV"           ,outputDir,16, 15, 95,5.0,1.0,0,1.2);
  //plotMuTau(125,1,"novbfLow",""   ,"etaL2","#tau #eta","units"         ,outputDir,10,-2.5, 2.5,5.0,1.0,0,2.);
  //
  //plotMuTau(125,1,"novbfHigh",""   ,"MEtMVA","MET","GeV"                        ,outputDir,10,0,100,5.0,1.0,0, 1.2);
  //plotMuTau(125,1,"novbfHigh",""   ,"MEtMVAPhi","MET #phi","units"              ,outputDir,10,-3.2,3.2,   5.0,1.0,0,1.5);
  //plotMuTau(125,1,"novbfHighNoMt",""   ,"MtLeg1MVA","M_{T}(#mu#nu)","GeV" ,                  outputDir,16,0,160,5.0,1.0,0,1.2);
  //plotMuTau(125,1,"novbfHigh",""   ,"diTauVisMass","visible mass","GeV"      ,outputDir,10,0,200,5.0,1.0,0,1.2); 
  //plotMuTau(125,1,"novbfHigh",""   ,"diTauNSVfitMass","SVfit mass","GeV"     ,outputDir,-1,0,100,5.0,1.0,0,1.2);
  //plotMuTau(125,1,"novbfHigh",""   ,"etaL1","#mu #eta", "units"              ,outputDir,10,-2.5, 2.5,5.0,1.0,0,2.);
  //plotMuTau(125,1,"novbfHigh",""   ,"ptL1","#mu p_{T}", "GeV"                ,outputDir,16, 15, 95,5.0,1.0,0,1.2);
  //plotMuTau(125,1,"novbfHigh",""   ,"ptL2","#tau p_{T}","GeV"           ,outputDir,16, 15, 95,5.0,1.0,0,1.2);
  //plotMuTau(125,1,"novbfHigh",""   ,"etaL2","#tau #eta","units"         ,outputDir,10,-2.5, 2.5,5.0,1.0,0,2.);
  //
  //
  //plotMuTau(125,1,"boostLow",""   ,"MEtMVA","MET","GeV"                        ,outputDir,10,0,100,5.0,1.0,0, 1.2);
  //plotMuTau(125,1,"boostLow",""   ,"MEtMVAPhi","MET #phi","units"              ,outputDir,10,-3.2,3.2,   5.0,1.0,0,1.5);
  //plotMuTau(125,1,"boostLowNoMt",""   ,"MtLeg1MVA","M_{T}(#mu#nu)","GeV" ,                  outputDir,16,0,160,5.0,1.0,0,1.2);
  //plotMuTau(125,1,"boostLow",""   ,"diTauVisMass","visible mass","GeV"      ,outputDir,20,0,200,5.0,1.0,0,1.2);  
  ////plotMuTau(125,1,"boostLow",""   ,"diTauNSVfitMass","SVfit mass","GeV"     ,outputDir,-1,0,100,5.0,1.0,0,1.2);
  //plotMuTau(125,1,"boostLow",""   ,"etaL1","#mu #eta", "units"              ,outputDir,10,-2.5, 2.5,5.0,1.0,0,2.);
  //plotMuTau(125,1,"boostLow",""   ,"ptL1","#mu p_{T}", "GeV"                ,outputDir,16, 15, 95,5.0,1.0,0,1.2);
  //plotMuTau(125,1,"boostLow",""   ,"ptL2","#tau p_{T}","GeV"           ,outputDir,16, 15, 95,5.0,1.0,0,1.2);
  //plotMuTau(125,1,"boostLow",""   ,"etaL2","#tau #eta","units"         ,outputDir,10,-2.5, 2.5,5.0,1.0,0,2.);
  //
  //
  //plotMuTau(125,1,"boostHigh",""   ,"MEtMVA","MET","GeV"                        ,outputDir,10,0,100,5.0,1.0,0, 1.2);
  //plotMuTau(125,1,"boostHigh",""   ,"MEtMVAPhi","MET #phi","units"              ,outputDir,10,-3.2,3.2,   5.0,1.0,0,1.5);
  //plotMuTau(125,1,"boostHighNoMt",""   ,"MtLeg1MVA","M_{T}(#mu#nu)","GeV" ,                  outputDir,16,0,160,5.0,1.0,0,1.2);
  //plotMuTau(125,1,"boostHigh",""   ,"diTauVisMass","visible mass","GeV"      ,outputDir,10,0,200,5.0,1.0,0,1.2); 
  plotMuTau(125,1,"boostHigh",""   ,"diTauNSVfitMass","SVfit mass","GeV"     ,outputDir,-1,0,100,5.0,1.0,0,1.2);
  //plotMuTau(125,1,"boostHigh",""   ,"etaL1","#mu #eta", "units"              ,outputDir,10,-2.5, 2.5,5.0,1.0,0,2.);
  //plotMuTau(125,1,"boostHigh",""   ,"ptL1","#mu p_{T}", "GeV"                ,outputDir,16, 15, 95,5.0,1.0,0,1.2);
  //plotMuTau(125,1,"boostHigh",""   ,"ptL2","#tau p_{T}","GeV"           ,outputDir,16, 15, 95,5.0,1.0,0,1.2);
  //plotMuTau(125,1,"boostHigh",""   ,"etaL2","#tau #eta","units"         ,outputDir,10,-2.5, 2.5,5.0,1.0,0,2.);
  //
  //plotMuTau(125,1,"vbf",""   ,"MEtMVA","MET","GeV"                        ,outputDir,10,0,100,5.0,1.0,0, 1.2);
  //plotMuTau(125,1,"vbf",""   ,"MEtMVAPhi","MET #phi","units"              ,outputDir,16,-3.2,3.2,   5.0,1.0,0,1.5);
  //plotMuTau(125,1,"vbfNoMt",""   ,"MtLeg1MVA","M_{T}(#mu#nu)","GeV" ,                  outputDir,16,0,160,5.0,1.0,0,1.2);
  //plotMuTau(125,1,"vbf",""   ,"diTauVisMass","visible mass","GeV"      ,outputDir,10,0,200,5.0,1.0,0,1.2);  
  plotMuTau(125,1,"vbf",""   ,"diTauNSVfitMass","SVfit mass","GeV"     ,outputDir,-1,0,100,5.0,1.0,0,1.2);
  //plotMuTau(125,1,"vbf",""   ,"etaL1","#mu #eta", "units"              ,outputDir,10,-2.5, 2.5,5.0,1.0,0, 2.);
  //plotMuTau(125,1,"vbf",""   ,"ptL1","#mu p_{T}", "GeV"                ,outputDir,16, 15, 95,5.0,1.0,0,1.2);
  //plotMuTau(125,1,"vbf",""   ,"ptL2","#tau p_{T}","GeV"           ,outputDir,16, 15, 95,5.0,1.0,0,1.2);
  //plotMuTau(125,1,"vbf",""   ,"etaL2","#tau #eta","units"         ,outputDir,10,-2.5, 2.5,5.0,1.0,0,2.);
  
  return;

  for(unsigned int i = 0 ; i < variables.size(); i++){
    for(unsigned j = 0; j < mH.size(); j++){

        plotMuTau(mH[j],useEmbedded,"novbfLow",""       ,variables[i],"mass","GeV",outputDir,-1,0,100,1.0,1.0,0,1.2);
        plotMuTau(mH[j],useEmbedded,"novbfLow","TauUp"  ,variables[i],"mass","GeV",outputDir,-1,0,100,1.0,1.0,0,1.2);
        plotMuTau(mH[j],useEmbedded,"novbfLow","TauDown",variables[i],"mass","GeV",outputDir,-1,0,100,1.0,1.0,0,1.2);
        plotMuTau(mH[j],useEmbedded,"novbfLow","JetUp"  ,variables[i],"mass","GeV",outputDir,-1,0,100,1.0,1.0,0,1.2);
        plotMuTau(mH[j],useEmbedded,"novbfLow","JetDown",variables[i],"mass","GeV",outputDir,-1,0,100,1.0,1.0,0,1.2);

        plotMuTau(mH[j],useEmbedded,"novbfHigh",""       ,variables[i],"mass","GeV",outputDir,-1,0,100,1.0,1.0,0,1.2);
        plotMuTau(mH[j],useEmbedded,"novbfHigh","TauUp"  ,variables[i],"mass","GeV",outputDir,-1,0,100,1.0,1.0,0,1.2);
        plotMuTau(mH[j],useEmbedded,"novbfHigh","TauDown",variables[i],"mass","GeV",outputDir,-1,0,100,1.0,1.0,0,1.2);
        plotMuTau(mH[j],useEmbedded,"novbfHigh","JetUp"  ,variables[i],"mass","GeV",outputDir,-1,0,100,1.0,1.0,0,1.2);
        plotMuTau(mH[j],useEmbedded,"novbfHigh","JetDown",variables[i],"mass","GeV",outputDir,-1,0,100,1.0,1.0,0,1.2);

        plotMuTau(mH[j],useEmbedded,"boostLow",""       ,variables[i],"mass","GeV",outputDir,-1,0,100,1.0,1.0,0,1.2);
        plotMuTau(mH[j],useEmbedded,"boostLow","TauUp"  ,variables[i],"mass","GeV",outputDir,-1,0,100,1.0,1.0,0,1.2);
        plotMuTau(mH[j],useEmbedded,"boostLow","TauDown",variables[i],"mass","GeV",outputDir,-1,0,100,1.0,1.0,0,1.2);
        plotMuTau(mH[j],useEmbedded,"boostLow","JetUp"  ,variables[i],"mass","GeV",outputDir,-1,0,100,1.0,1.0,0,1.2);
        plotMuTau(mH[j],useEmbedded,"boostLow","JetDown",variables[i],"mass","GeV",outputDir,-1,0,100,1.0,1.0,0,1.2);
      
        plotMuTau(mH[j],useEmbedded,"boostHigh",""       ,variables[i],"mass","GeV",outputDir,-1,0,100,1.0,1.0,0,1.2);
        plotMuTau(mH[j],useEmbedded,"boostHigh","TauUp"  ,variables[i],"mass","GeV",outputDir,-1,0,100,1.0,1.0,0,1.2);
        plotMuTau(mH[j],useEmbedded,"boostHigh","TauDown",variables[i],"mass","GeV",outputDir,-1,0,100,1.0,1.0,0,1.2);
        plotMuTau(mH[j],useEmbedded,"boostHigh","JetUp"  ,variables[i],"mass","GeV",outputDir,-1,0,100,1.0,1.0,0,1.2);
        plotMuTau(mH[j],useEmbedded,"boostHigh","JetDown",variables[i],"mass","GeV",outputDir,-1,0,100,1.0,1.0,0,1.2);

	plotMuTau(mH[j],useEmbedded,"vbf",""       ,variables[i],"mass","GeV",outputDir,-1,0,100,1.0,1.0,0,1.2); 
        plotMuTau(mH[j],useEmbedded,"vbf","TauUp"  ,variables[i],"mass","GeV",outputDir,-1,0,100,1.0,1.0,0,1.2); 
        plotMuTau(mH[j],useEmbedded,"vbf","TauDown",variables[i],"mass","GeV",outputDir,-1,0,100,1.0,1.0,0,1.2); 
        plotMuTau(mH[j],useEmbedded,"vbf","JetUp"  ,variables[i],"mass","GeV",outputDir,-1,0,100,1.0,1.0,0,1.2); 
        plotMuTau(mH[j],useEmbedded,"vbf","JetDown",variables[i],"mass","GeV",outputDir,-1,0,100,1.0,1.0,0,1.2); 

    }
  }
  
  return;

}

int main(int argc, const char* argv[])
{

  std::cout << "plotMuTau()" << std::endl;
  gROOT->SetBatch(true);

  gSystem->Load("libFWCoreFWLite");
  AutoLibraryLoader::enable();

  int mH, nBins, logy, useEmb; 
  float magnify, hltEff, xMin, xMax, maxY;
  string category, analysis, variable, xtitle, unity, outputDir, RUN, version;

  if(argc==1) plotMuTauAll();
  else if(argc>16) { 

    mH         =  (int)atof(argv[1]); 
    category   =  argv[2]; 
    variable   =  argv[3]; 
    xtitle     =  argv[4]; 
    unity      =  argv[5]; 
    nBins      =  (int)atof(argv[6]); 
    xMin       =  atof(argv[7]); 
    xMax       =  atof(argv[8]); 
    magnify    =  atof(argv[9]); 
    hltEff     =  atof(argv[10]); 
    logy       =  (int)atof(argv[11]); 
    maxY       =  atof(argv[12]) ;
    outputDir  =  argv[13]; 
    RUN        =  argv[14] ;
    version    =  argv[15];
    useEmb     =  (int)atof(argv[16]);
    analysis   =  argc>17 ? argv[17] : ""; 

    cout << endl << "RUN : " << RUN << " | VERSION : " << version << " | ANALYSIS : " << analysis << endl << endl;

    plotMuTau(mH,useEmb,category,analysis,variable,xtitle,unity,outputDir,nBins,xMin,xMax,magnify,hltEff,logy,maxY, RUN, version);
  }
  else { cout << "Please put at least 15 arguments" << endl; return 1;}

  cout << "DONE" << endl;
  return 0;
}

// int main(int argc, const char* argv[])
// {

//   std::cout << "plotMuTau()" << std::endl;
//   gROOT->SetBatch(true);

//   gSystem->Load("libFWCoreFWLite");
//   AutoLibraryLoader::enable();

//   int mH, nBins, logy; 
//   float magnify, hltEff, xMin, xMax, maxY;
//   string category, analysis, variable, xtitle, unity, outputDir;

//   //cout << argc << " arguments" << endl < endl;

//   if(argc==1) plotMuTauAll();
//   //else if(argc>14) { 
//   else {
//     mH=(int)atof(argv[1]); category=argv[2]; variable=argv[3]; 

//     int i=4;
//     while(string(argv[i])!="XX") {
//       xtitle += string(argv[i]);
//       xtitle += " "; 
//       i++ ;
//     }
//     i++ ;
//     unity=argv[i]; 

//     nBins=(int)atof(argv[i+1]); xMin=atof(argv[i+2]); xMax=atof(argv[i+3]); 

//     magnify=atof(argv[i+4]); hltEff=atof(argv[i+5]); logy=(int)atof(argv[i+6]); maxY=atof(argv[i+7]) ;

//     outputDir=argv[i+8]; analysis = argc>i+9 ? argv[i+9] : ""; 
    
//     cout << endl
// 	 << "plotMuTau(" << mH << ",1," << category << "," << analysis << "," << variable << "," << xtitle << "," 
// 	 << unity << "," << outputDir << "," << nBins << "," << xMin << "," << xMax << "," << magnify << "," 
// 	 << hltEff << "," << logy << "," << maxY << ")" << endl << endl;

//     plotMuTau(mH,1,category,analysis,variable,xtitle,unity,outputDir,nBins,xMin,xMax,magnify,hltEff,logy,maxY);

//   }
//   //else { cout << "Please put at least 14 arguments" << endl; return 1;}

//   cout << "DONE" << endl;
//   return 0;
// }
