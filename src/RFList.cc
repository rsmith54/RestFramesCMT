/////////////////////////////////////////////////////////////////////////
//   RestFrames: particle physics event analysis library
//   --------------------------------------------------------------------
//   Copyright (c) 2014-2015, Christopher Rogan
/////////////////////////////////////////////////////////////////////////
///
///  \file   RFList.cc
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

#include "RestFrames/RFList.hh"
#include "RestFrames/RFrame.hh"
#include "RestFrames/Jigsaw.hh"
#include "RestFrames/Group.hh"
#include "RestFrames/InvisibleState.hh"
#include "RestFrames/CombinatoricState.hh"

using namespace std;

namespace RestFrames {

   RFKey g_Key(-1);

  template <typename T>
  T& RFList<T>::m_Empty = RFList<T>::m_EmptyHandler.GetEmpty();
  template <typename T>
  RFListEmpty<T> RFList<T>::m_EmptyHandler(g_Key);

  ///////////////////////////////////////////////
  // RFListEmpty class methods
  ///////////////////////////////////////////////
  template <class T>
  RFListEmpty<T>::RFListEmpty(const RFKey& key){ 
    m_Empty = new T(); 
    m_Empty->SetKey(key); 
  }

  template <class T>
  RFListEmpty<T>::~RFListEmpty(){ 
    delete m_Empty; 
  }

  template <class T>
  T& RFListEmpty<T>::GetEmpty() const { 
    return *m_Empty; 
  } 

  ///////////////////////////////////////////////
  // RFList class methods
  ///////////////////////////////////////////////
  template <class T>
  RFList<T>::RFList(){ }
  template <class T>
  RFList<T>::~RFList<T>(){
    Clear();
  }

  template <class T>
  void RFList<T>::Clear(){
    m_Objs.clear();
  }

  template <class T>
  T& RFList<T>::Get(const RFKey& key) const{
    int N = GetN();
    for(int i = 0; i < N; i++)
      if(m_Objs[i]->IsSame(key)) return *m_Objs[i];
    
    return m_Empty;
  }

  template <class T>
  T& RFList<T>::Get(int i) const { 
    if(i < 0 || i >= GetN()) return m_Empty;
    return *m_Objs[i];
  }

  template <class T>
  bool RFList<T>::Add(T& obj){
    if(obj.IsSame(g_Key)) return false;
    int N = GetN();
    for(int i = 0; i < N; i++){
      if(m_Objs[i]->IsSame(obj)) return false;
    }
    m_Objs.push_back(&obj);
    return true;
  }

  template <class T>
  bool RFList<T>::Add(const RFList<T>& objs){
    int N = objs.GetN();
    double ret = true;
    for(int i = 0; i < N; i++) 
      if(!Add(objs.Get(i))) ret = false;
    return ret;
  }

  template <class T>
  int RFList<T>::Remove(const T& obj){
    int N = GetN();
    for(int i = 0; i < N; i++){
      if(m_Objs[i]->IsSame(obj)){
	m_Objs.erase(m_Objs.begin()+i);
	return i;
      }
    }
    return -1;
  }

  template <class T>
  void RFList<T>::Remove(const RFList<T>& objs){
    int N = objs.GetN();
    for(int i = 0; i < N; i++) Remove(objs.Get(i));
  }

  template <class T>
  bool RFList<T>::Contains(const RFKey& key) const {
    int N = GetN();
    for(int i = 0; i < N; i++){
      if(m_Objs[i]->IsSame(key)) return true;
    }
    return false;
  }

  template <class T>
  bool RFList<T>::Contains(const T& obj) const {
    int N = GetN();
    for(int i = 0; i < N; i++){
      if(m_Objs[i]->IsSame(obj)) return true;
    }
    return false;
  }

  template <class T>
  bool RFList<T>::Contains(const RFList<T>& objs) const {
    int N = objs.GetN();
    for(int i = 0; i < N; i++){
      if(!Contains(objs.Get(i))) return false;
    }
    return true;
  }

  template <class T>
  bool RFList<T>::IsSame(const RFList<T>& objs) const {
    return SizeUnion(objs) == SizeIntersection(objs);
  }

  template <class T>
  int RFList<T>::GetIndex(const RFKey& key) const {
    int N = GetN();
    for(int i = 0; i < N; i++){
      if(m_Objs[i]->IsSame(key)) return i;
    }
    return -1;
  }

  template <class T>
  int RFList<T>::GetIndex(const T& obj) const {
    int N = GetN();
    for(int i = 0; i < N; i++){
      if(m_Objs[i]->IsSame(obj)) return i;
    }
    return -1;
  }

  template <class T>
  RFList<T> RFList<T>::Copy() const {
    RFList<T> objs;
    int N = GetN();
    for(int i = 0; i < N; i++) objs.Add(*m_Objs[i]);
    return objs;
  }

  template <class T>
  RFList<T> RFList<T>::Union(const RFList<T>& objs) const {
    RFList<T> objs_this = Copy();
    objs_this.Add(objs);
    return objs_this;
  }

  template <class T>
  RFList<T> RFList<T>::Intersection(const RFList<T>& objs) const {
    RFList<T> inter; 
    int N = objs.GetN();
    for(int i = 0; i < N; i++){
      if(Contains(objs.Get(i))) inter.Add(objs.Get(i));
    }
    return inter;
  }

  template <class T>
  RFList<T> RFList<T>::Complement(const RFList<T>& objs) const {
    RFList<T> comp = Copy();
    int N = objs.GetN();
    for(int i = 0; i < N; i++)
      if(comp.Contains(objs.Get(i))) 
	comp.Remove(objs.Get(i));
    
    return comp;
  }

  template <class T>
  int RFList<T>::SizeUnion(const RFList<T>& objs) const {
    int Nthis = GetN();
    int Nthat = objs.GetN();
    for(int i = 0; i < Nthat; i++)
      if(!Contains(objs.Get(i))) Nthis++;
    
    return Nthis;
  }

  template <class T>
  int RFList<T>::SizeIntersection(const RFList<T>& objs) const {
    int Nthis = 0;
    int Nthat = objs.GetN();
    for(int i = 0; i < Nthat; i++)
      if(Contains(objs.Get(i))) Nthis++;
    
    return Nthis;
  }

  template <class T>
  int RFList<T>::SizeComplement(const RFList<T>& objs) const {
    int Nthis = GetN();
    int Nthat = objs.GetN();
    for(int i = 0; i < Nthat; i++)
      if(Contains(objs.Get(i))) Nthis--;
    
    return Nthis;
  }

  template class RFList<RFBase>;
  template class RFList<RestFrame>; 
  template class RFList<RFrame>; 
  template class RFList<Jigsaw>;
  template class RFList<Group>;
  template class RFList<State>;
  template class RFList<InvisibleState>;
  template class RFList<CombinatoricState>;

}
