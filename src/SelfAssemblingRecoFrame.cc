/////////////////////////////////////////////////////////////////////////
//   RestFrames: particle physics event analysis library
//   --------------------------------------------------------------------
//   Copyright (c) 2014-2015, Christopher Rogan
/////////////////////////////////////////////////////////////////////////
///
///  \file   SelfAssemblingRecoFrame.cc
///
///  \author Christopher Rogan
///          (crogan@cern.ch)
///
///  \date   2015 Jan
///
//   This file is part of RestFrames.
//
//   RestFrames is free software; you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation; either version 2 of the License, or
//   (at your option) any later version.
// 
//   RestFrames is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
// 
//   You should have received a copy of the GNU General Public License
//   along with RestFrames. If not, see <http://www.gnu.org/licenses/>.
/////////////////////////////////////////////////////////////////////////

#include <sstream>
#include "RestFrames/SelfAssemblingRecoFrame.hh"
#include "RestFrames/VisibleRecoFrame.hh"
#include "RestFrames/CombinatoricState.hh"

using namespace std;

namespace RestFrames {

  ///////////////////////////////////////////////
  // SelfAssemblingRecoFrame class
  ///////////////////////////////////////////////
  SelfAssemblingRecoFrame::SelfAssemblingRecoFrame(const string& sname, const string& stitle)
    : DecayRecoFrame(sname,stitle)
  {
    Init();
  }
  
  SelfAssemblingRecoFrame::~SelfAssemblingRecoFrame(){ }

  void SelfAssemblingRecoFrame::Init(){
    m_RType = RDSelfAssembling;
    m_IsAssembled = false;
    m_IsBackedUp = false;
    m_Body_UnAssembled = false;
    m_Mind_UnAssembled = false;
    m_Nvisible = 0;
    m_Ndecay = 0;
  }

  void SelfAssemblingRecoFrame::Clear(){
    m_VisibleFrames.Clear();
    m_DecayFrames.Clear();
    ReconstructionFrame::Clear();
  }

  void SelfAssemblingRecoFrame::ClearEventRecursive(){
    SetSpirit(false);
   
    if(!IsSoundBody() || !IsSoundMind()) return;
   
    Disassemble();
    ReconstructionFrame::ClearEventRecursive();
  }

  void SelfAssemblingRecoFrame::Disassemble(){
    if(!m_IsAssembled) return;

    m_Nvisible = 0;
    m_Ndecay = 0;
  
    // replace frames with unassembled ones
    RemoveChildren();
    ClearNewFrames();
    int Ncf = m_ChildFrames_UnAssembled.GetN();
    for(int i = 0; i < Ncf; i++)
      AddChildFrame(m_ChildFrames_UnAssembled.Get(i));
    
    if(!IsSoundBodyRecursive()){
      m_Log << LogWarning;
      m_Log << "Problem with recursive tree after disassembly";
      m_Log << m_End;
      SetBody(false);
    } else 
      SetBody(true);

    // group list
    RFList<Group> groups = GetListGroups();

    // state list
    RFList<State> states;

    m_ChildStates.clear();
    int Ncs = m_ChildStates_UnAssembled.size();
    for(int i = 0; i < Ncs; i++)
      states.Add(m_ChildStates_UnAssembled[i]);

    if(!InitializeStatesRecursive(states, groups)){
      m_Log << LogWarning;
      m_Log << "Problem connecting states after disassembly";
      m_Log << m_End;
      SetMind(false);
    } else 
      SetMind(true);

    SetSpirit(false);
    m_IsAssembled = false;
  }

  void SelfAssemblingRecoFrame::Assemble(){
    if(m_IsAssembled) Disassemble();
    if(!IsSoundBody() || !IsSoundMind()){
      m_Log << LogWarning << "Unable to assemble frame" << m_End;
      return;
    }

    RFList<Group> groups = GetListGroups();

    // new States
    RFList<State> states;
    // new Frames associated with States
    vector<RestFrame*> frames;
    // States' four-vector
    vector<TLorentzVector> Ps; 

    // clear unassembled lists
    m_ChildFrames_UnAssembled.Clear();
    m_ChildStates_UnAssembled.clear();
    int N = GetNChildren();
    for(int i = 0; i < N; i++){
      RestFrame& frame = GetChildFrame(i);
      // fill unassembled lists
      m_ChildFrames_UnAssembled.Add(frame);
      m_ChildStates_UnAssembled.push_back(m_ChildStates[i]);
      
      bool expand = false; 
      
      // if there is only one State associated with child, 
      // check for CombinatoricState to expand
      if(m_ChildStates[i].GetN() == 1){ 
	CombinatoricState* statePtr = dynamic_cast<CombinatoricState*>(&m_ChildStates[i].Get(0));
	if(statePtr){ // is CombinatoricState
	  RFList<State> elements = statePtr->GetElements();
	  int Nelement = elements.GetN();
	  for(int e = 0; e < Nelement; e++){
	    State& element = elements.Get(e);
	    // Get a new visible frame for each individual state
	    RestFrame& new_frame = GetNewVisibleFrame(frame.GetName(),frame.GetTitle());
	    element.ClearFrames();
	    element.AddFrame(new_frame);
	    frames.push_back(&new_frame);
	    TLorentzVector V = element.GetFourVector();
	    if(V.M() < 0.) V.SetVectM(V.Vect(),0.);
	    Ps.push_back(V);
	    states.Add(element);
	  }
	  expand = true;
	}
      }
      if(!expand){
	frames.push_back(&frame);
	TLorentzVector V = m_ChildStates[i].GetFourVector();
	if(V.M() < 0.) V.SetVectM(V.Vect(),0.);
	Ps.push_back(V);
	states.Add(m_ChildStates[i]);
      }
    }

    RemoveChildren();
    m_ChildStates.clear();

    AssembleRecursive(*this, frames, Ps); 

    if(!IsSoundBodyRecursive()){
      m_Log << LogWarning;
      m_Log << "Problem with recursive tree after assembly";
      m_Log << m_End;
      SetBody(false);
    }

    SetMind(true);
    if(!InitializeStatesRecursive(states, groups)){
      m_Log << LogWarning;
      m_Log << "Problem connecting states after assembly";
      m_Log << m_End;
      SetMind(false);
    }

    // for(int i = 0; i < m_Ndecay; i++)
    //   if(!m_DecayFrames.Get(i).InitializeStates(states, groups)){
    // 	m_Log << LogWarning;
    // 	m_Log << "Problem connecting states after assembly in frame:";
    // 	m_Log << Log(m_DecayFrames.Get(i));
    // 	SetMind(false);
    //   }
    
    // for(int i = 0; i < m_Nvisible; i++)
    //   if(!m_VisibleFrames.Get(i).InitializeStates(states, groups)){
    // 	m_Log << LogWarning;
    // 	m_Log << "Problem connecting states after assembly in frame:";
    // 	m_Log << Log(m_VisibleFrames.Get(i));
    // 	SetMind(false);
    //   }
   
    m_IsAssembled = true;
  }

  void SelfAssemblingRecoFrame::AssembleRecursive(RestFrame& frame, vector<RestFrame*>& frames, vector<TLorentzVector>& Ps){
    int Ninput = frames.size();
    if(Ninput <= 1){
      for(int i = 0; i < Ninput; i++) frame.AddChildFrame(*frames[i]);
      return;
    }

    TLorentzVector TOT(0.,0.,0.,0.);
    for(int i = 0; i < Ninput; i++) TOT += Ps[i];
    TVector3 boost = TOT.BoostVector();
    for(int i = 0; i < Ninput; i++){
      Ps[i].Boost(-boost);
    }

    int ip_max[2];
    int jp_max[2];
    for(int i = 0; i < 2; i++) ip_max[i] = -1;
    for(int i = 0; i < 2; i++) jp_max[i] = -1;
    double val_max = -1.;
    // Loop over all 2-jet seed probes
    int ip[2], jp[2];
    for(ip[0] = 0; ip[0] < Ninput-1; ip[0]++){
      for(ip[1] = ip[0]+1; ip[1] < Ninput; ip[1]++){
	TVector3 nRef = Ps[ip[0]].Vect().Cross(Ps[ip[1]].Vect());
	int Nhem[2];
	TLorentzVector hem[2];
	for(int i = 0; i < 2; i++){
	  Nhem[i] = 0;
	  hem[i].SetPxPyPzE(0.,0.,0.,0.);
	}
	// Loop over all jets
	for(int i = 0; i < Ninput; i++){
	  if((i == ip[0]) || (i ==ip[1])) continue;
	  int ihem = int(Ps[i].Vect().Dot(nRef) > 0.);
	  Nhem[ihem]++;
	  hem[ihem] += Ps[i];
	}
	// assign 2 probes
	for(jp[0] = 0; jp[0] < 2; jp[0]++){
	  for(jp[1] = 0; jp[1] < 2; jp[1]++){
	    if(jp[0] == jp[1] && Nhem[(jp[0]+1)%2] == 0) continue;
	    TLorentzVector hem_probes[2];
	    for(int i = 0; i < 2; i++) hem_probes[i] = hem[i];
	    for(int i = 0; i < 2; i++) hem_probes[jp[i]] += Ps[ip[i]];
	    double val = hem_probes[0].P() + hem_probes[1].P();
	    if(val > val_max){
	      val_max = val;
	      for(int i = 0; i < 2; i++) ip_max[i] = ip[i];
	      for(int i = 0; i < 2; i++) jp_max[i] = jp[i];
	    }
	  }
	}
      }
    }
    vector<RestFrame*> child_frames[2];
    vector<TLorentzVector> child_Ps[2];
    TLorentzVector hem[2];
    for(int i = 0; i < 2; i++){
      hem[i].SetPxPyPzE(0.,0.,0.,0.);
    }
    for(int i = 0; i < 2; i++){
      child_frames[jp_max[i]].push_back(frames[ip_max[i]]);
      child_Ps[jp_max[i]].push_back(Ps[ip_max[i]]);
      hem[jp_max[i]] += Ps[ip_max[i]];
    }
    TVector3 nRef = Ps[ip_max[0]].Vect().Cross(Ps[ip_max[1]].Vect());
    for(int i = 0; i < Ninput; i++){
      if((i == ip_max[0]) || (i == ip_max[1])) continue;
      int ihem = int(Ps[i].Vect().Dot(nRef) > 0.);
      child_frames[ihem].push_back(frames[i]);
      child_Ps[ihem].push_back(Ps[i]);
      hem[ihem] += Ps[i];
    }

    int flip = int(hem[1].M() > hem[0].M());
    for(int i = 0; i < 2; i++){
      int j = (i+flip)%2;
      if(child_frames[j].size() == 1){
	frame.AddChildFrame(*child_frames[j][0]);
      } else {
	RestFrame& new_frame = GetNewDecayFrame(GetName(),GetTitle());
	frame.AddChildFrame(new_frame);
	AssembleRecursive(new_frame, child_frames[j], child_Ps[j]);
      }
    }
  }

  bool SelfAssemblingRecoFrame::AnalyzeEventRecursive(){
    // Disassemble Frame tree if it assembled
    if(m_IsAssembled) Disassemble();
    if(!ReconstructionFrame::AnalyzeEventRecursive()){
      m_Log << LogWarning;
      m_Log << "Unable to recursively analyze event with ";
      m_Log << "disassembled SelfAssemblingRecoFrame" << m_End;
      SetSpirit(false);
      return false;
    }
    // Assemble Frame tree
    Assemble();
    if(!ReconstructionFrame::AnalyzeEventRecursive()){
      m_Log << LogWarning;
      m_Log << "Unable to recursively analyze event with ";
      m_Log << "assembled SelfAssemblingRecoFrame" << m_End;
      SetSpirit(false);
      return false;
    }
    SetSpirit(true);
    return true;
  }

  void SelfAssemblingRecoFrame::ClearNewFrames(){
    int N = m_DecayFrames.GetN();
    for(int i = 0; i < N; i++) m_DecayFrames.Get(i).Clear();
    N = m_VisibleFrames.GetN();
    for(int i = 0; i < N; i++) m_VisibleFrames.Get(i).Clear();
  }

  ReconstructionFrame& SelfAssemblingRecoFrame::GetNewDecayFrame(const string& sname, const string& stitle){
    if(m_Ndecay < m_DecayFrames.GetN()){
      m_Ndecay++;
      return m_DecayFrames.Get(m_Ndecay-1);
    }
    ostringstream name;
    name << sname << "_" << m_Ndecay+1;
    ostringstream title; 
    title << "#left(" << stitle << "#right)_{" << m_Ndecay+1 << "}";
    DecayRecoFrame* framePtr = new DecayRecoFrame(name.str(),title.str());
    
    m_DecayFrames.Add(*framePtr);
    AddDependent(framePtr);
    m_Ndecay++;
    return *framePtr;
  }

  ReconstructionFrame& SelfAssemblingRecoFrame::GetNewVisibleFrame(const string& sname, const string& stitle){
    if(m_Nvisible < m_VisibleFrames.GetN()){
      m_Nvisible++;
      return m_VisibleFrames.Get(m_Nvisible-1);
    }
    ostringstream name;
    name << sname << "_" << m_Nvisible+1;
    ostringstream title; 
    title << "#left(" << stitle << "#right)_{" << m_Nvisible+1 << "}";
    VisibleRecoFrame* framePtr = new VisibleRecoFrame(name.str(),title.str());
    
    m_VisibleFrames.Add(*framePtr);
    AddDependent(framePtr);
    m_Nvisible++;
    return *framePtr;
  }

  RestFrame const& SelfAssemblingRecoFrame::GetFrame(const RFKey& key) const {
    if(!m_IsAssembled) return g_RestFrame;

    for(int i = 0; i < m_ChildStates.size(); i++){
      int index = m_ChildStates[i].GetIndex(key);
      if(index >= 0) return m_ChildStates[i].Get(index).GetFrame();
    }
    return g_RestFrame;
  }

}