import FWCore.ParameterSet.Config as cms

process = cms.Process("MUTAUANA")

process.load('Configuration.StandardSequences.Services_cff')
process.load('Configuration.StandardSequences.FrontierConditions_GlobalTag_cff')
process.load("Configuration.Geometry.GeometryIdeal_cff")
process.load("Configuration.StandardSequences.MagneticField_cff")

#from Configuration.PyReleaseValidation.autoCond import autoCond
#process.GlobalTag.globaltag = cms.string( autoCond[ 'startup' ] )

process.load('JetMETCorrections.Configuration.DefaultJEC_cff')

runOnMC     = False
runOnEmbed  = True
embedType   = "RhEmbedMuTauHighPt" #"PfEmbed" or "RhEmbed","MuTau" or "EleTau","LowPt","HighPt","FullRange"
reRunPatJets = True
applyTauESCorr= True 
doSVFitReco = True
usePFMEtMVA = True
useRecoil   = True
useMarkov   = True
runMoriond = True

#if runOnEmbed and runOnMC:
#    print "Running on Embedded, runOnMC should be switched off"
#    runOnMC=False

if runOnMC:
    print "Running on MC"
else:
    print "Running on Data"

if (not runOnMC) and (not runOnEmbed) and applyTauESCorr:
    print "Running on Data, Tau ESCorr should be switched off"
    applyTauESCorr=False 

if useMarkov:
    print "Use SVFit with Markov chain integration"
else:
    print "Use SVFit with VEGAS integration"
    
if runOnMC:
    process.GlobalTag.globaltag = cms.string('START53_V23::All')
else:
    process.GlobalTag.globaltag = cms.string('FT_53_V21_AN4::All')
    
    
process.load("FWCore.MessageLogger.MessageLogger_cfi")
process.MessageLogger.cerr.FwkReport.reportEvery = 10
process.maxEvents = cms.untracked.PSet( input = cms.untracked.int32(-1) )
process.options = cms.untracked.PSet( wantSummary = cms.untracked.bool(True) )

process.source = cms.Source(
    "PoolSource",
    fileNames = cms.untracked.vstring(
        #'file:patTuples_LepTauStream.root'
        #'file:VBFH125.root'
        #'file:data2012D.root'        
        #'file:/afs/cern.ch/work/a/anayak/public/HTauTau/Spring2013/patTuples_LepTauStream.root'
        #'file:/data_CMS/cms/htautau/PostMoriond/pat/MC/file_GG125_patTuples_LepTauStream_10_1_Rs2.root'
        #'file:/data_CMS/cms/htautau/PostMoriond/pat/Data/file_Data_2012D_PRV1_HTT_06Mar2013_PAT_v1_p2_patTuples_LepTauStream_78_1_2KS.root'    
        #'file:/data_CMS/cms/htautau/PostMoriond/PAT/MC/patTuples_LepTauStream_VBFH125_PAT_v2.root'
        #'file:/data_CMS/cms/htautau/PostMoriond/PAT/MC/GGFH125/patTuples_LepTauStream_100_1_ztz.root'
        #'file:/data_CMS/cms/htautau/PostMoriond/PAT/MC/VBFH125/patTuples_LepTauStream_100_1_qMY.root'
        'root://polgrid4.in2p3.fr//dpm/in2p3.fr/home/cms/trivcat/store/user/bluj/TauPlusX/Data_2012D_PRV1_HTT_06Mar13_PAT_v2_p1/633d9a9cc3632fa03920e1c68550a01b/patTuples_LepTauStream_9_2_XRa.root'
        )
    )

#process.source.skipEvents = cms.untracked.uint32(90)

#process.source.eventsToProcess = cms.untracked.VEventRange(
#    '1:69216'
#    )

process.allEventsFilter = cms.EDFilter(
    "AllEventsFilter"
    )

#######################################################################
#quark/gluon jets
process.load('QuarkGluonTagger.EightTeV.QGTagger_RecoJets_cff')  
process.QGTagger.srcJets = cms.InputTag("selectedPatJets")
process.QGTagger.isPatJet = cms.untracked.bool(True) 
#Switch for when PFJets with CHS are used (only supported for LD):
#process.QGTagger.useCHS  = cms.untracked.bool(True) 
#If an uncorrected jet source is used as input, you can correct the pt on the fly inside the QGTagger:
#process.QGTagger.jec     = cms.untracked.string('ak5PFL1FastL2L3')

###################################################################################
'''
#----------------------------------------------------------------------------------
# produce No-PU MET

process.load("JetMETCorrections.METPUSubtraction.noPileUpPFMET_cff")

doSmearJets = None
if runOnMC:
    doSmearJets = True
else:
    doSmearJets = False

from PhysicsTools.PatUtils.tools.runNoPileUpMEtUncertainties import runNoPileUpMEtUncertainties
runNoPileUpMEtUncertainties(
    process,
    electronCollection = '',
    photonCollection = '',
    muonCollection = cms.InputTag('UserIsoMuons'),
    tauCollection = tausForPFMEtMVA,
    jetCollection = cms.InputTag('selectedPatJets'),     
    doApplyChargedHadronSubtraction = False,
    doSmearJets = doSmearJets,
    jecUncertaintyTag = "SubTotalMC",
    addToPatDefaultSequence = False
)

if runOnMC:
    process.calibratedAK5PFJetsForNoPileUpPFMEt.correctors = cms.vstring("ak5PFL1FastL2L3")
else:
    process.calibratedAK5PFJetsForNoPileUpPFMEt.correctors = cms.vstring("ak5PFL1FastL2L3Residual")

process.producePFMEtNoPileUp = cms.Sequence(process.pfNoPileUpMEtUncertaintySequence)
#----------------------------------------------------------------------------------

#----------------------------------------------------------------------------------
# produce Type-1 corrected PFMET

process.load("PhysicsTools.PatUtils.patPFMETCorrections_cff")

process.load("JetMETCorrections.Type1MET.pfMETsysShiftCorrections_cfi")
sysShiftCorrParameter = None
if runOnMC:
    sysShiftCorrParameter = process.pfMEtSysShiftCorrParameters_2012runABCvsNvtx_mc
else:
    sysShiftCorrParameter = process.pfMEtSysShiftCorrParameters_2012runABCvsNvtx_data

from PhysicsTools.PatUtils.tools.runType1PFMEtUncertainties import runType1PFMEtUncertainties
runType1PFMEtUncertainties(
    process,
    electronCollection = '',
    photonCollection = '',
    muonCollection = cms.InputTag('UserIsoMuons'),
    tauCollection = tausForPFMEtMVA,
    jetCollection = cms.InputTag('selectedPatJets'),        
    doSmearJets = doSmearJets,
    jecUncertaintyTag = "SubTotalMC",
    makeType1corrPFMEt = True,
    makeType1p2corrPFMEt = False,
    doApplyType0corr = True,
    sysShiftCorrParameter = sysShiftCorrParameter,
    doApplySysShiftCorr = True,
    addToPatDefaultSequence = False
)

# CV: collections of MET and jets used as input for SysShiftMETcorrInputProducer
#     are not fully consistent, but we anyway use parametrization of MET x/y shift as function of Nvtx
process.pfMEtSysShiftCorr.srcMEt = cms.InputTag('patMETs')
process.pfMEtSysShiftCorr.srcJets = cms.InputTag('selectedPatJets')

##from PhysicsTools.PatAlgos.tools.helpers import massSearchReplaceAnyInputTag
##massSearchReplaceAnyInputTag(process.producePatPFMETCorrections, cms.InputTag('patPFMet'), cms.InputTag('patMETs'))
process.patPFMet.addGenMET = cms.bool(runOnMC)
process.patPFMetJetEnUp.addGenMET = cms.bool(runOnMC)
process.patPFMetJetEnDown.addGenMET = cms.bool(runOnMC)
process.patPFMetMuonEnUp.addGenMET = cms.bool(runOnMC)
process.patPFMetMuonEnDown.addGenMET = cms.bool(runOnMC)
process.patPFMetTauEnUp.addGenMET = cms.bool(runOnMC)
process.patPFMetTauEnDown.addGenMET = cms.bool(runOnMC)
process.patPFMetNoPileUp.addGenMET = cms.bool(runOnMC)
process.patPFMetNoPileUpJetEnUp.addGenMET = cms.bool(runOnMC)
process.patPFMetNoPileUpJetEnDown.addGenMET = cms.bool(runOnMC)
process.patPFMetNoPileUpMuonEnUp.addGenMET = cms.bool(runOnMC)
process.patPFMetNoPileUpMuonEnDown.addGenMET = cms.bool(runOnMC)
process.patPFMetNoPileUpTauEnUp.addGenMET = cms.bool(runOnMC)
process.patPFMetNoPileUpTauEnDown.addGenMET = cms.bool(runOnMC)
process.patPFMetNoPileUpUnclusteredEnUp.addGenMET = cms.bool(runOnMC)
process.patPFMetNoPileUpUnclusteredEnDown.addGenMET = cms.bool(runOnMC)
##process.producePatPFMETCorrections.remove(process.patPFMet)

process.produceType1corrPFMEt = cms.Sequence()
if runOnMC:
    process.patPFJetMETtype1p2Corr.jetCorrLabel = cms.string("L3Absolute")
    process.produceType1corrPFMEt += process.pfType1MEtUncertaintySequence
else:
    # CV: apply data/MC residual correction to "unclustered energy"
    process.calibratedPFCandidates = cms.EDProducer("PFCandResidualCorrProducer",
        src = cms.InputTag('particleFlow'),
        residualCorrLabel = cms.string("ak5PFResidual"),
        residualCorrEtaMax = cms.double(9.9),
        extraCorrFactor = cms.double(1.05)
    )
    process.produceType1corrPFMEt += process.calibratedPFCandidates
    process.pfCandidateToVertexAssociation.PFCandidateCollection = cms.InputTag('calibratedPFCandidates')
    process.patPFJetMETtype1p2Corr.type2ResidualCorrLabel = cms.string("ak5PFResidual")
    process.patPFJetMETtype1p2Corr.type2ResidualCorrEtaMax = cms.double(9.9)
    process.pfCandMETresidualCorr = process.pfCandMETcorr.clone(                
        residualCorrLabel = cms.string("ak5PFResidual"),
        residualCorrEtaMax = cms.double(9.9),
        residualCorrOffset = cms.double(1.),
        extraCorrFactor = cms.double(1.05)
    )
    process.producePatPFMETCorrections.replace(process.pfCandMETcorr, process.pfCandMETcorr + process.pfCandMETresidualCorr)
    process.patType1CorrectedPFMet.srcType1Corrections.append(cms.InputTag('pfCandMETresidualCorr'))
    process.produceType1corrPFMEt += process.pfType1MEtUncertaintySequence
#----------------------------------------------------------------------------------
'''
#----------------------------------------------------------------------------------
# produce CaloMEtNoHF (MC corrected by data/MC difference in CaloMET response)
process.load("LLRAnalysis.TauTauStudies.calibrateCaloMETandL1ETMforEmbedded_cff")
process.load("LLRAnalysis.TauTauStudies.sumCaloTowersInEtaSlices_cfi")

process.produceCaloMEtNoHF = cms.Sequence(process.uncorrectedL1ETM)

if runOnEmbed:
    process.produceCaloMEtNoHF += process.calibrateCaloMETandL1ETMforEmbedded
if runOnMC:
    process.metNoHFresidualCorrected.residualCorrLabel = cms.string("ak5CaloResidual")
    process.metNoHFresidualCorrected.extraCorrFactor = cms.double(1.05)
    process.metNoHFresidualCorrected.isMC = cms.bool(True)
    process.produceCaloMEtNoHF += process.metNoHFresidualCorrected
    process.metNoHFresidualCorrectedUp = process.metNoHFresidualCorrected.clone(
        extraCorrFactor = cms.double(1.10)
    )
    process.produceCaloMEtNoHF += process.metNoHFresidualCorrectedUp
    process.metNoHFresidualCorrectedDown = process.metNoHFresidualCorrected.clone(
        extraCorrFactor = cms.double(1.0)
    )
    process.produceCaloMEtNoHF += process.metNoHFresidualCorrectedDown
else:
    process.metNoHFresidualCorrected.residualCorrLabel = cms.string("")
    process.metNoHFresidualCorrected.extraCorrFactor = cms.double(1.0)
    process.metNoHFresidualCorrected.isMC = cms.bool(False)
    process.produceCaloMEtNoHF += process.metNoHFresidualCorrected
#----------------------------------------------------------------------------------

#----------------------------------------------------------------------------------
# produce PU Jet Ids

from RecoJets.JetProducers.PileupJetID_cfi import *
stdalgos = cms.VPSet(full_53x,cutbased,PhilV1)
process.puJetMva = cms.EDProducer('PileupJetIdProducer',
                                  produceJetIds = cms.bool(True),
                                  jetids = cms.InputTag(""),
                                  runMvas = cms.bool(True),
                                  jets = cms.InputTag("ak5PFJets"),
                                  vertexes = cms.InputTag("offlinePrimaryVertices"),
                                  algos = cms.VPSet(stdalgos),
                                  rho     = cms.InputTag("kt6PFJets","rho"),
                                  jec     = cms.string("AK5PF"),
                                  applyJec = cms.bool(True),
                                  inputIsCorrected = cms.bool(False),
                                  residualsFromTxt = cms.bool(False),
                                  residualsTxt     = cms.FileInPath("RecoJets/JetProducers/data/dummy.txt"),
                                  )

process.puJetIdSequence = cms.Sequence(process.puJetMva)

#----------------------------------------------------------------------------------

#process.puJetIdAndMvaMet = cms.Sequence(process.puJetIdSequence *
#                                        (process.pfMEtMVAsequence*process.patPFMetByMVA))

######################################################
#ReRun patJet
process.runPatJets = cms.Sequence()

if reRunPatJets:
    #process.load("PhysicsTools.PatAlgos.producersLayer1.jetProducer_cff")
    process.load("PhysicsTools.PatAlgos.patSequences_cff")
    from PhysicsTools.PatAlgos.tools.jetTools import *

    switchJetCollection(process,cms.InputTag('ak5PFJets'),
                        doJTA        = True,
                        doBTagging   = True,
                        jetCorrLabel = ('AK5PF', ['L2Relative', 'L3Absolute',]),
                        doType1MET   = False,
                        genJetCollection=cms.InputTag("ak5GenJets"),
                        doJetID      = True,
                        jetIdLabel   = 'ak5',
                        outputModules = []
                        )

    JEClevels = cms.vstring(['L2Relative', 'L3Absolute'])
    if runOnMC:
        JEClevels = ['L1FastJet', 'L2Relative', 'L3Absolute']
    else:
        JEClevels = ['L1FastJet', 'L2Relative', 'L3Absolute', 'L2L3Residual']
        process.makePatJets.remove(process.patJetPartonMatch)
        process.makePatJets.remove(process.patJetGenJetMatch)
        process.makePatJets.remove(process.patJetFlavourId)
        process.patJets.addGenJetMatch = False
        process.patJets.addGenPartonMatch = False
        process.patJets.addPartonJetMatch = False
        process.patJets.getJetMCFlavour = False
        
    process.patJetCorrFactors.levels = JEClevels
    process.patJetCorrFactors.rho    = cms.InputTag('kt6PFJets','rho')
    process.patJetCorrFactors.useRho = True
    
    #process.patJetCorrFactorsL1Offset = process.patJetCorrFactors.clone(
    #    levels = cms.vstring('L1Offset',
    #                         'L2Relative',
    #                         'L3Absolute')
    #    )
    #process.patJetCorrFactorsL1Offset.useRho = False #MB
    #if runOnMC:
    #    process.patJetCorrFactorsL1Offset.levels = ['L1Offset', 'L2Relative', 'L3Absolute']
    #else:
    #    process.patJetCorrFactorsL1Offset.levels = ['L1Offset', 'L2Relative', 'L3Absolute', 'L2L3Residual']
    #    
    #process.patJets.jetCorrFactorsSource = cms.VInputTag(cms.InputTag("patJetCorrFactors"),
    #                                                     cms.InputTag("patJetCorrFactorsL1Offset"))
    #process.makePatJets.replace(process.patJetCorrFactors,
    #                            process.patJetCorrFactors+process.patJetCorrFactorsL1Offset)
    
    process.runPatJets = cms.Sequence(process.makePatJets*process.selectedPatJets)

###################################################################################
process.rescaledTaus = cms.EDProducer(
    "TauRescalerProducer",
    inputCollection = cms.InputTag("tauPtEtaIDAgMuAgElecIsoPtRel"),
    shift           = cms.vdouble(0.03,0.03),
    numOfSigmas     = cms.double(1.0),
    #verbose         = cms.bool(True)
    )
process.rescaledMuons = cms.EDProducer(
    "MuonRescalerProducer",
    inputCollection = cms.InputTag("muPtEtaIDIsoPtRel"),
    shift           = cms.vdouble(0.01,0.01),
    numOfSigmas     = cms.double(1.0),
    #verbose         = cms.bool(True)
    )
process.rescaledMuonsRel = cms.EDProducer(
    "MuonRescalerProducer",
    inputCollection = cms.InputTag("muPtEtaRelID"),
    shift           = cms.vdouble(0.01,0.01),
    numOfSigmas     = cms.double(1.0),
    #verbose         = cms.bool(True)
    )

#######################################################################
from LLRAnalysis.TauTauStudies.runMETByLeptonPairs_cfi import *
getDiTauMassByLeptonPair(process, "muPtEtaIDIso", "", "tauPtEtaIDAgMuAgElecIso", runOnMC, useMarkov, useRecoil, doSVFitReco)

process.selectedDiTau = cms.EDFilter(
    "MuTauPairSelector",
    src = cms.InputTag("MergedDiTaus:diTaus"),
    cut = cms.string("dR12>0.5")
    )
process.selectedDiTauCounter = cms.EDFilter(
    "CandViewCountFilter",
    src = cms.InputTag("selectedDiTau"),
    minNumber = cms.uint32(1),
    maxNumber = cms.uint32(999),
)

#######################################################################
getDiTauMassByLeptonPair(process, "muPtEtaIDIso", "", "tauPtEtaIDAgMuAgElecIsoTauUp", runOnMC, useMarkov, useRecoil, doSVFitReco, "TauUp")

process.selectedDiTauTauUp = process.selectedDiTau.clone(src = cms.InputTag("MergedDiTausTauUp:diTaus") )
process.selectedDiTauTauUpCounter = process.selectedDiTauCounter.clone(src =  cms.InputTag("selectedDiTauTauUp"))

#######################################################################
getDiTauMassByLeptonPair(process, "muPtEtaIDIso", "", "tauPtEtaIDAgMuAgElecIsoTauDown", runOnMC, useMarkov, useRecoil, doSVFitReco, "TauDown")

process.selectedDiTauTauDown = process.selectedDiTau.clone(src = cms.InputTag("MergedDiTausTauDown:diTaus") )
process.selectedDiTauTauDownCounter = process.selectedDiTauCounter.clone(src =  cms.InputTag("selectedDiTauTauDown"))
#######################################################################
getDiTauMassByLeptonPair(process, "muPtEtaIDIsoMuUp", "", "tauPtEtaIDAgMuAgElecIso", runOnMC, useMarkov, useRecoil, doSVFitReco, "MuUp")

process.selectedDiTauMuUp = process.selectedDiTau.clone(src = cms.InputTag("MergedDiTausMuUp:diTaus") )
process.selectedDiTauMuUpCounter = process.selectedDiTauCounter.clone(src =  cms.InputTag("selectedDiTauMuUp"))

#######################################################################
getDiTauMassByLeptonPair(process, "muPtEtaIDIsoMuDown", "", "tauPtEtaIDAgMuAgElecIso", runOnMC, useMarkov, useRecoil, doSVFitReco, "MuDown")

process.selectedDiTauMuDown = process.selectedDiTau.clone(src = cms.InputTag("MergedDiTausMuDown:diTaus") )
process.selectedDiTauMuDownCounter = process.selectedDiTauCounter.clone(src =  cms.InputTag("selectedDiTauMuDown"))
########################################################################

process.tauPtEtaIDAgMuAgElec = cms.EDFilter( #apply AntiMuTight2
    "PATTauSelector",
    src = cms.InputTag("tauPtEtaIDAgMuAgElec"),
    cut = cms.string("tauID('againstMuonTight2')>0.5 "+
                     " && tauID('againstElectronLoose')>0.5 "
                     ),
    filter = cms.bool(False)
    )

process.tauPtEtaIDAgMuAgElecScaled = cms.EDProducer(
    "TauESCorrector",
    tauTag = cms.InputTag("tauPtEtaIDAgMuAgElec")
    #verbose         = cms.bool(True)
    )

process.tauPtEtaIDAgMuAgElecIso  = cms.EDFilter(
    "PATTauSelector",
    src = cms.InputTag("tauPtEtaIDAgMuAgElec"),
    cut = cms.string("pt>20 && abs(eta)<2.3"+
                     " && tauID('byLooseIsolationMVA2')>-0.5"
                     ),
    filter = cms.bool(False)
    )
process.tauPtEtaIDAgMuAgElecIsoPtRel  = cms.EDFilter(
    "PATTauSelector",
    src = cms.InputTag("tauPtEtaIDAgMuAgElec"),
    cut = cms.string("pt>19 && abs(eta)<2.3"+
                     " && tauID('byLooseIsolationMVA2')>-0.5"
                     ),
    filter = cms.bool(False)
    )
if applyTauESCorr:
    process.tauPtEtaIDAgMuAgElecIso.src = cms.InputTag("tauPtEtaIDAgMuAgElecScaled")
    process.tauPtEtaIDAgMuAgElecIsoPtRel.src = cms.InputTag("tauPtEtaIDAgMuAgElecScaled")
if runMoriond:
    process.tauPtEtaIDAgMuAgElec.cut = cms.string("(tauID('againstMuonTight2')>0.5 || tauID('againstMuonTight')>0.5)"+
                                                 " && tauID('againstElectronLoose')>0.5 ") 
    process.tauPtEtaIDAgMuAgElecIso.cut = cms.string("pt>20 && abs(eta)<2.3"+
                                                     " && (tauID('byLooseIsolationMVA2')>-0.5 || tauID('byLooseIsolationMVA')>-0.5)"
                                                     )
    process.tauPtEtaIDAgMuAgElecIsoPtRel.cut = cms.string("pt>19 && abs(eta)<2.3"+
                                                          " && (tauID('byLooseIsolationMVA2')>-0.5 || tauID('byLooseIsolationMVA')>-0.5)"
                                                          )
    
process.tauPtEtaIDAgMuAgElecIsoCounter = cms.EDFilter(
    "CandViewCountFilter",
    src = cms.InputTag("tauPtEtaIDAgMuAgElecIso"),
    minNumber = cms.uint32(1),
    maxNumber = cms.uint32(999),
    )
process.tauPtEtaIDAgMuAgElecIsoTauUp  =  process.tauPtEtaIDAgMuAgElecIso.clone(
    src = cms.InputTag("rescaledTaus", "U")
    )
process.tauPtEtaIDAgMuAgElecIsoTauUpCounter = process.tauPtEtaIDAgMuAgElecIsoCounter.clone(
    src = cms.InputTag("tauPtEtaIDAgMuAgElecIsoTauUp"),
    )
process.tauPtEtaIDAgMuAgElecIsoTauDown  =  process.tauPtEtaIDAgMuAgElecIso.clone(
    src = cms.InputTag("rescaledTaus", "D")
    )
process.tauPtEtaIDAgMuAgElecIsoTauDownCounter = process.tauPtEtaIDAgMuAgElecIsoCounter.clone(
    src = cms.InputTag("tauPtEtaIDAgMuAgElecIsoTauDown"),
    )


process.muPtEtaIDIso  = cms.EDFilter(
    "PATMuonSelector",
    src = cms.InputTag("muPtEtaID"),
    #MBcut = cms.string("userFloat('PFRelIsoDB04v2')<0.50 && pt>20 && abs(eta)<2.1"),
    cut = cms.string("userFloat('PFRelIsoDB04v2')<0.50 && pt>9 && abs(eta)<2.1"),
    filter = cms.bool(False)
    )
process.muPtEtaIDIsoPtRel  = cms.EDFilter(
    "PATMuonSelector",
    src = cms.InputTag("muPtEtaID"),
    #MBcut = cms.string("userFloat('PFRelIsoDB04v2')<0.50 && pt>19 && abs(eta)<2.1"),
    cut = cms.string("userFloat('PFRelIsoDB04v2')<0.50 && pt>8 && abs(eta)<2.1"),
    filter = cms.bool(False)
    )

process.muPtEtaIDIsoCounter = cms.EDFilter(
    "CandViewCountFilter",
    src = cms.InputTag("muPtEtaIDIso"),
    minNumber = cms.uint32(1),
    maxNumber = cms.uint32(999),
    )
process.muPtEtaIDIsoMuUp = process.muPtEtaIDIso.clone(
    src = cms.InputTag("rescaledMuons","U")
    )
process.muPtEtaIDIsoMuUpCounter = process.muPtEtaIDIsoCounter.clone(
    src = cms.InputTag("muPtEtaIDIsoMuUp"),
    )
process.muPtEtaIDIsoMuDown = process.muPtEtaIDIso.clone(
    src = cms.InputTag("rescaledMuons","D")
    )
process.muPtEtaIDIsoMuDownCounter = process.muPtEtaIDIsoCounter.clone(
    src = cms.InputTag("muPtEtaIDIsoMuDown"),
    )
process.muPtEtaRelID = process.muPtEtaIDIso.clone(
    src = cms.InputTag("muPtEtaRelID"),
    #MBcut = cms.string("pt>15")
    cut = cms.string("pt>7 && abs(userFloat('dxyWrtPV'))<0.045")
    )
process.muPtEtaRelIDMuUp   = process.muPtEtaRelID.clone(
    src = cms.InputTag("rescaledMuonsRel","U")
    )
process.muPtEtaRelIDMuDown = process.muPtEtaRelID.clone(
    src = cms.InputTag("rescaledMuonsRel","D")
    )
##Update 3rdLepVeto cuts##
##Loose MVA ID used for e-mu and 3d lepton veto selection
MVALoose = "((pt<=20 && abs(superCluster.eta)>=0.0 && abs(superCluster.eta)<0.8 && userFloat('mvaPOGNonTrig')>0.925) ||" + \
           "(pt<=20 && abs(superCluster.eta)>=0.8 && abs(superCluster.eta)<1.479 && userFloat('mvaPOGNonTrig')>0.915) ||" + \
           "(pt<=20 && abs(superCluster.eta)>=1.479 && abs(superCluster.eta)<2.5 && userFloat('mvaPOGNonTrig')>0.965) ||" + \
           "(pt>20  && abs(superCluster.eta)>=0.0 && abs(superCluster.eta)<0.8 && userFloat('mvaPOGNonTrig')>0.905) ||" + \
           "(pt>20  && abs(superCluster.eta)>=0.8 && abs(superCluster.eta)<1.479 && userFloat('mvaPOGNonTrig')>0.955) ||" + \
           "(pt>20  && abs(superCluster.eta)>=1.479 && abs(superCluster.eta)<2.5 && userFloat('mvaPOGNonTrig')>0.975))"
##Loose MVA ID: new e-Id, same FR as old one
MVALooseNew = "((pt<=20 && abs(superCluster.eta)>=0.0 && abs(superCluster.eta)<0.8 && userFloat('mvaPOGTrigNoIP')>-0.5375) ||" + \
              "(pt<=20 && abs(superCluster.eta)>=0.8 && abs(superCluster.eta)<1.479 && userFloat('mvaPOGTrigNoIP')>-0.375) ||" + \
              "(pt<=20 && abs(superCluster.eta)>=1.479 && abs(superCluster.eta)<2.5 && userFloat('mvaPOGTrigNoIP')>-0.025) ||" + \
              "(pt>20  && abs(superCluster.eta)>=0.0 && abs(superCluster.eta)<0.8 && userFloat('mvaPOGTrigNoIP')>0.325) ||" + \
              "(pt>20  && abs(superCluster.eta)>=0.8 && abs(superCluster.eta)<1.479 && userFloat('mvaPOGTrigNoIP')>0.775) ||" + \
              "(pt>20  && abs(superCluster.eta)>=1.479 && abs(superCluster.eta)<2.5 && userFloat('mvaPOGTrigNoIP')>0.775))"
process.electronsForVeto = cms.EDFilter(
    "PATElectronSelector",
    src = cms.InputTag("electronsForVeto"),
    cut = cms.string("userFloat('nHits')==0 && userInt('antiConv')>0.5"),
                     ##+" && "+MVALoose),#use MVAloose for back compability, can be changed in future
    filter = cms.bool(False)
    )
#########
process.filterSequence = cms.Sequence(
    (process.tauPtEtaIDAgMuAgElecIso       * process.tauPtEtaIDAgMuAgElecIsoCounter) +
    (process.tauPtEtaIDAgMuAgElecIsoTauUp  * process.tauPtEtaIDAgMuAgElecIsoTauUpCounter) +
    (process.tauPtEtaIDAgMuAgElecIsoTauDown* process.tauPtEtaIDAgMuAgElecIsoTauDownCounter) +
    (process.muPtEtaIDIso      * process.muPtEtaIDIsoCounter) +
    (process.muPtEtaIDIsoMuUp  * process.muPtEtaIDIsoMuUpCounter) +
    (process.muPtEtaIDIsoMuDown* process.muPtEtaIDIsoMuDownCounter) +
    (process.muPtEtaRelID+process.muPtEtaRelIDMuUp+process.muPtEtaRelIDMuDown)
    )

#######################################################################


#######################################################################
process.muTauStreamAnalyzer = cms.EDAnalyzer(
    "MuTauStreamAnalyzer",
    diTaus         = cms.InputTag("selectedDiTau"),
    jets           = cms.InputTag("selectedPatJets"),
    newJets        = cms.InputTag(""),
    met            = cms.InputTag("metRecoilCorrector00",  "N"),
    rawMet         = cms.InputTag("patMETsPFlow"),
    mvaMet         = cms.InputTag("patPFMetByMVA00"),
    metCov         = cms.InputTag("pfMEtMVACov00"),
    muons          = cms.InputTag("muPtEtaIDIso"),
    muonsRel       = cms.InputTag("muPtEtaRelID"),
    vertices       = cms.InputTag("selectedPrimaryVertices"),
    triggerResults = cms.InputTag("patTriggerEvent"),
    genParticles   = cms.InputTag("genParticles"),
    genTaus        = cms.InputTag("tauGenJetsSelectorAllHadrons"),
    isMC           = cms.bool(runOnMC),
    isRhEmb        = cms.untracked.bool(runOnEmbed and "RhEmbed" in embedType),
    deltaRLegJet   = cms.untracked.double(0.5),
    minCorrPt      = cms.untracked.double(15.),
    minJetID       = cms.untracked.double(0.5), # 1=loose,2=medium,3=tight
    verbose        = cms.untracked.bool( False ),
    doIsoMVAOrdering = cms.untracked.bool(False),
    doMuIsoMVA     = cms.untracked.bool( False ),
    )

if usePFMEtMVA:
    if useRecoil :
        process.muTauStreamAnalyzer.met = cms.InputTag("metRecoilCorrector00",  "N")
    else :
        process.muTauStreamAnalyzer.met = cms.InputTag("patPFMetByMVA00")


process.muTauStreamAnalyzerMuUp    = process.muTauStreamAnalyzer.clone(
    diTaus   =  cms.InputTag("selectedDiTauMuUp"),
    met      =  cms.InputTag("rescaledMETmuon","NNUNN"),
    muons    =  cms.InputTag("muPtEtaIDIsoMuUp"),
    muonsRel =  cms.InputTag("muPtEtaRelIDMuUp"),
    )
process.muTauStreamAnalyzerMuDown  = process.muTauStreamAnalyzer.clone(
    diTaus   =  cms.InputTag("selectedDiTauMuDown"),
    met      =  cms.InputTag("rescaledMETmuon","NNDNN"),
    muons    =  cms.InputTag("muPtEtaIDIsoMuDown"),
    muonsRel =  cms.InputTag("muPtEtaRelIDMuDown"),
    )

process.muTauStreamAnalyzerTauUp   = process.muTauStreamAnalyzer.clone(
    diTaus =  cms.InputTag("selectedDiTauTauUp"),
    #met    =  cms.InputTag("rescaledMETtau","NNNUN"),
    )
process.muTauStreamAnalyzerTauDown = process.muTauStreamAnalyzer.clone(
    diTaus =  cms.InputTag("selectedDiTauTauDown"),
    #met    =  cms.InputTag("rescaledMETtau","NNNDN")
    )
#######################################################################
# weights for embedding
process.kineWeightsForEmbed = cms.Sequence()
if runOnEmbed:
    if "RhEmbed" in embedType:
        process.load("TauAnalysis.MCEmbeddingTools.embeddingKineReweight_cff")
        if not runOnMC:
            process.kineWeightsForEmbed += process.embeddingKineReweightSequenceRECembedding
        else:
            process.kineWeightsForEmbed += process.embeddingKineReweightSequenceGENembedding
        if "MuTau" in embedType:
            if "LowPt" in embedType:
                process.embeddingKineReweightRECembedding.inputFileName = cms.FileInPath('TauAnalysis/MCEmbeddingTools/data/embeddingKineReweight_muPt7to25tauPtGt18_recEmbedded.root')
                process.embeddingKineReweightGENembedding.inputFileName = cms.FileInPath("TauAnalysis/MCEmbeddingTools/data/embeddingKineReweight_muPt7to25tauPtGt18_genEmbedded.root")
            if "HighPt" in embedType:
                process.embeddingKineReweightRECembedding.inputFileName = cms.FileInPath('TauAnalysis/MCEmbeddingTools/data/embeddingKineReweight_muPtGt16tauPtGt18_recEmbedded.root')
                process.embeddingKineReweightGENembedding.inputFileName = cms.FileInPath("TauAnalysis/MCEmbeddingTools/data/embeddingKineReweight_muPtGt16tauPtGt18_genEmbedded.root")
            if "FullRange" in embedType:
                process.embeddingKineReweightRECembedding.inputFileName = cms.FileInPath('TauAnalysis/MCEmbeddingTools/data/embeddingKineReweight_recEmbedding_mutau.root')
                process.embeddingKineReweightGENembedding.inputFileName = cms.FileInPath("TauAnalysis/MCEmbeddingTools/data/embeddingKineReweight_genEmbedding_mutau.root")
        if "EleTau" in embedType:
            if "LowPt" in embedType:
                process.embeddingKineReweightRECembedding.inputFileName = cms.FileInPath('TauAnalysis/MCEmbeddingTools/data/embeddingKineReweight_ePt9to30tauPtGt18_recEmbedded.root')
                process.embeddingKineReweightGENembedding.inputFileName = cms.FileInPath("TauAnalysis/MCEmbeddingTools/data/embeddingKineReweight_ePt9to30tauPtGt18_genEmbedded.root")
            if "HighPt" in embedType:
                process.embeddingKineReweightRECembedding.inputFileName = cms.FileInPath('TauAnalysis/MCEmbeddingTools/data/embeddingKineReweight_ePtGt20tauPtGt18_recEmbedded.root')
                process.embeddingKineReweightGENembedding.inputFileName = cms.FileInPath("TauAnalysis/MCEmbeddingTools/data/embeddingKineReweight_ePtGt20tauPtGt18_genEmbedded.root")
            if "FullRange" in embedType:
                process.embeddingKineReweightRECembedding.inputFileName = cms.FileInPath('TauAnalysis/MCEmbeddingTools/data/embeddingKineReweight_recEmbedding_etau.root')
                process.embeddingKineReweightGENembedding.inputFileName = cms.FileInPath("TauAnalysis/MCEmbeddingTools/data/embeddingKineReweight_genEmbedding_etau.root")
                
#######################################################################

process.seqNominal = cms.Sequence(
    process.allEventsFilter*
    process.runPatJets*
    (process.tauPtEtaIDAgMuAgElec*process.tauPtEtaIDAgMuAgElecScaled*
     process.tauPtEtaIDAgMuAgElecIso*process.tauPtEtaIDAgMuAgElecIsoCounter)*
    (process.muPtEtaIDIso *process.muPtEtaIDIsoCounter) *
    process.muPtEtaRelID *
    process.electronsForVeto *
    #(process.pfMEtMVAsequence*process.patPFMetByMVA)*    
    #(process.LeptonsForMVAMEt*process.puJetIdAndMvaMet)*
    process.puJetIdSequence *
    #process.produceType1corrPFMEt*
    #process.producePFMEtNoPileUp*
    process.produceCaloMEtNoHF*
    #process.metRecoilCorrector*
    #process.pfMEtMVACov*
    #process.diTau*process.selectedDiTau*process.selectedDiTauCounter*
    process.calibratedAK5PFJetsForPFMEtMVA*
    process.runMETByPairsSequence*
    process.selectedDiTau*process.selectedDiTauCounter*
    process.QuarkGluonTagger* #quark/gluon jets
    process.kineWeightsForEmbed*#IN
    process.muTauStreamAnalyzer
    )

process.seqMuUp = cms.Sequence(
    process.allEventsFilter*
    process.runPatJets*
    process.muPtEtaIDIsoPtRel *
    (process.tauPtEtaIDAgMuAgElec*process.tauPtEtaIDAgMuAgElecScaled*
     process.tauPtEtaIDAgMuAgElecIso*process.tauPtEtaIDAgMuAgElecIsoCounter)*
    #(process.pfMEtMVAsequence*process.patPFMetByMVA)*
    #(process.LeptonsForMVAMEt*process.puJetIdAndMvaMet)*
    process.puJetIdSequence *
    process.produceCaloMEtNoHF*
    #process.metRecoilCorrector*
    #(process.rescaledMETmuon+process.rescaledMuons+process.rescaledMuonsRel)*
    (process.rescaledMuons+process.rescaledMuonsRel)*
    (process.muPtEtaIDIsoMuUp*process.muPtEtaIDIsoMuUpCounter) *
    process.muPtEtaRelIDMuUp *
    process.electronsForVeto *
    #process.pfMEtMVACov*
    #process.diTauMuUp*process.selectedDiTauMuUp*process.selectedDiTauMuUpCounter*
    process.calibratedAK5PFJetsForPFMEtMVA*
    process.runMETByPairsSequenceMuUp*
    process.selectedDiTauMuUp*process.selectedDiTauMuUpCounter*
    process.QuarkGluonTagger* #quark/gluon jets
    process.kineWeightsForEmbed*#IN
    process.muTauStreamAnalyzerMuUp
    )
process.seqMuDown = cms.Sequence(
    process.allEventsFilter*
    process.runPatJets*
    process.muPtEtaIDIsoPtRel *
    (process.tauPtEtaIDAgMuAgElec*process.tauPtEtaIDAgMuAgElecScaled*
     process.tauPtEtaIDAgMuAgElecIso*process.tauPtEtaIDAgMuAgElecIsoCounter)*
    #(process.pfMEtMVAsequence*process.patPFMetByMVA)*
    #(process.LeptonsForMVAMEt*process.puJetIdAndMvaMet)*
    process.puJetIdSequence *
    process.produceCaloMEtNoHF*
    #process.metRecoilCorrector*
    #(process.rescaledMETmuon+process.rescaledMuons+process.rescaledMuonsRel)*
    (process.rescaledMuons+process.rescaledMuonsRel)* 
    (process.muPtEtaIDIsoMuDown*process.muPtEtaIDIsoMuDownCounter) *
    process.muPtEtaRelIDMuDown *
    process.electronsForVeto *
    #process.pfMEtMVACov*
    #process.diTauMuDown*process.selectedDiTauMuDown*process.selectedDiTauMuDownCounter*
    process.calibratedAK5PFJetsForPFMEtMVA*
    process.runMETByPairsSequenceMuDown*
    process.selectedDiTauMuDown*process.selectedDiTauMuDownCounter*
    process.QuarkGluonTagger* #quark/gluon jets
    process.kineWeightsForEmbed*#IN
    process.muTauStreamAnalyzerMuDown
    )

process.seqTauUp = cms.Sequence(
    process.allEventsFilter*
    process.runPatJets*
    (process.muPtEtaIDIso*process.muPtEtaIDIsoCounter) *
    process.tauPtEtaIDAgMuAgElec*process.tauPtEtaIDAgMuAgElecScaled*
    process.tauPtEtaIDAgMuAgElecIsoPtRel*
    process.muPtEtaRelID *
    process.electronsForVeto *
    #(process.pfMEtMVAsequence*process.patPFMetByMVA)*
    #(process.LeptonsForMVAMEt*process.puJetIdAndMvaMet)*
    process.puJetIdSequence *
    process.produceCaloMEtNoHF*
    #process.metRecoilCorrector*
    #(process.rescaledMETtau+process.rescaledTaus)*
    #(process.tauPtEtaIDAgMuAgElecIsoTauUp*process.tauPtEtaIDAgMuAgElecIsoTauUpCounter)*
    #process.pfMEtMVACov*
    #process.diTauTauUp*process.selectedDiTauTauUp*process.selectedDiTauTauUpCounter*
    (process.rescaledTaus)*
    (process.tauPtEtaIDAgMuAgElecIsoTauUp*process.tauPtEtaIDAgMuAgElecIsoTauUpCounter)*
    process.calibratedAK5PFJetsForPFMEtMVA*
    process.runMETByPairsSequenceTauUp*
    process.selectedDiTauTauUp*process.selectedDiTauTauUpCounter*
    process.QuarkGluonTagger* #quark/gluon jets
    process.kineWeightsForEmbed*#IN
    process.muTauStreamAnalyzerTauUp
    )
process.seqTauDown = cms.Sequence(
    process.allEventsFilter*
    process.runPatJets*
    (process.muPtEtaIDIso*process.muPtEtaIDIsoCounter) *
    process.tauPtEtaIDAgMuAgElec*process.tauPtEtaIDAgMuAgElecScaled*
    process.tauPtEtaIDAgMuAgElecIsoPtRel*
    process.muPtEtaRelID *
    process.electronsForVeto *
    #(process.pfMEtMVAsequence*process.patPFMetByMVA)*
    #(process.LeptonsForMVAMEt*process.puJetIdAndMvaMet)*
    process.puJetIdSequence *
    process.produceCaloMEtNoHF*
    #process.metRecoilCorrector*
    #(process.rescaledMETtau+process.rescaledTaus)*
    #(process.tauPtEtaIDAgMuAgElecIsoTauDown*process.tauPtEtaIDAgMuAgElecIsoTauDownCounter)*
    #process.pfMEtMVACov*
    #process.diTauTauDown*process.selectedDiTauTauDown*process.selectedDiTauTauDownCounter*
    (process.rescaledTaus)*
    (process.tauPtEtaIDAgMuAgElecIsoTauDown*process.tauPtEtaIDAgMuAgElecIsoTauDownCounter)*
    process.calibratedAK5PFJetsForPFMEtMVA*
    process.runMETByPairsSequenceTauDown*
    process.selectedDiTauTauDown*process.selectedDiTauTauDownCounter*
    process.QuarkGluonTagger* #quark/gluon jets
    process.kineWeightsForEmbed*#IN
    process.muTauStreamAnalyzerTauDown
    )


#######################################################################

if runOnMC:
    process.pNominal            = cms.Path( process.seqNominal )
    process.pTauUp              = cms.Path( process.seqTauUp)
    process.pTauDown            = cms.Path( process.seqTauDown )
    #process.pMuUp                  = cms.Path( process.seqMuUp)    #NOT INTERESTING FOR ANALYSIS
    #process.pMuDown                = cms.Path( process.seqMuDown)  #NOT INTERESTING FOR ANALYSIS
    ####

else:
    process.pNominal            = cms.Path( process.seqNominal )
    if runOnEmbed:
        process.pTauUp          = cms.Path( process.seqTauUp)
        process.pTauDown        = cms.Path( process.seqTauDown )
        #process.pMuUp                  = cms.Path( process.seqMuUp)    # NOT INTERESTING FOR ANALYSIS
        #process.pMuDown                = cms.Path( process.seqMuDown)  # NOT INTERESTING FOR ANALYSIS

#from PhysicsTools.PatAlgos.tools.helpers import massSearchReplaceAnyInputTag
#massSearchReplaceAnyInputTag(process.pNominalRaw,
#                             "patPFMetByMVA",
#                             "patMETsPFlow",
#                             verbose=True)
#massSearchReplaceAnyInputTag(process.pNominalRaw,
#                             "pfMEtMVACov",
#                             "",
#                             verbose=True)

#######################################################################

process.out = cms.OutputModule(
    "PoolOutputModule",
    outputCommands = cms.untracked.vstring( 'drop *',
                                            'keep *_metRecoilCorrector_*_*'),
    fileName = cms.untracked.string('patTuplesSkimmed_MuTauStream.root'),
    )

process.TFileService = cms.Service(
    "TFileService",
    fileName = cms.string("treeMuTauStream.root")
    )


## To work on Artur's skim
#from PhysicsTools.PatAlgos.tools.helpers import massSearchReplaceAnyInputTag
#massSearchReplaceAnyInputTag(process.pNominal,
#                             "muPtEtaID",
#                             "selectedPatMuonsTriggerMatch",
#                             verbose=False)
#massSearchReplaceAnyInputTag(process.pNominal,
#                             "muPtEtaRelID",
#                             "selectedPatMuonsTriggerMatch",
#                             verbose=False)
#massSearchReplaceAnyInputTag(process.pNominal,
#                             "selectedPrimaryVertices",
#                             "offlinePrimaryVertices",
#                             verbose=False)
#massSearchReplaceAnyInputTag(process.pNominal,
#                            "tauPtEtaIDAgMuAgElec",
#                             "selectedPatTausTriggerMatch",
#                             verbose=False)
#massSearchReplaceAnyInputTag(process.pNominal,
#                             "genParticles",
#                             "prunedGenParticles",
#                             verbose=False)
#massSearchReplaceAnyInputTag(process.pNominal,
#                             "tauGenJetsSelectorAllHadrons",
#                             "genTauDecaysToHadrons",
#                             verbose=False)

# before starting to process 1st event, print event content
#process.printEventContent = cms.EDAnalyzer("EventContentAnalyzer")
##process.filterFirstEvent = cms.EDFilter("EventCountFilter",
##    numEvents = cms.int32(1)
##)
##process.printFirstEventContentPath = cms.Path(process.filterFirstEvent + process.printEventContent)

process.outpath = cms.EndPath()

##
#processDumpFile = open('runMuTauStreamAnalyzer_Moriond2013_NewTauES.dump', 'w')
#print >> processDumpFile, process.dumpPython()
