/*************************************************************************/
/*                                                                       */
/*  Regexx - Regular Expressions C++ solution.                           */
/*                                                                       */
/*  http://projects.nn.com.br/                                           */
/*                                                                       */
/*  Copyright (C) 2000 Gustavo Niemeyer <gustavo@nn.com.br>              */
/*                                                                       */
/*  This library is free software; you can redistribute it and/or        */
/*  modify it under the terms of the GNU Library General Public          */
/*  License as published by the Free Software Foundation; either         */
/*  version 2 of the License, or (at your option) any later version.     */
/*                                                                       */
/*  This library is distributed in the hope that it will be useful,      */
/*  but WITHOUT ANY WARRANTY; without even the implied warranty of       */
/*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU    */
/*  Library General Public License for more details.                     */
/*                                                                       */
/*  You should have received a copy of the GNU Library General Public    */
/*  License along with this library; if not, write to the                */
/*  Free Software Foundation, Inc., 59 Temple Place - Suite 330,         */
/*  Boston, MA  02111-1307, USA.                                         */
/*                                                                       */
/*************************************************************************/

// $Revision$
// $Date$

#include "regexx.hh"
#include "pcre.h"

const unsigned int&
regexx::Regexx::exec(int _flags)
  throw(CompileException)
{
  if(!m_compiled) {
    const char *errptr;
    int erroffset;
    int cflags =
      ((_flags&nocase)?PCRE_CASELESS:0)
      | ((_flags&newline)?PCRE_MULTILINE:0);
    m_preg = pcre_compile(m_expr.String(),cflags,&errptr,&erroffset,0);
    if(m_preg == NULL) {
      throw CompileException(errptr);
    }
    pcre_fullinfo(m_preg, NULL, PCRE_INFO_CAPTURECOUNT, (void*)&m_capturecount);
    m_compiled = true;
  }

  if(!m_study && (_flags&study)) {
    const char *errptr;
    m_extra = pcre_study(m_preg, 0, &errptr);
    if(errptr != NULL)
      throw CompileException(errptr);
    m_study = true;
  }

  match.clear();

  int eflags = ((_flags&notbol)?PCRE_NOTBOL:0) | ((_flags&noteol)?PCRE_NOTEOL:0);

  int ssv[33];
  int ssc;
  m_matches = 0;

  ssc = pcre_exec(m_preg,m_extra,m_str.String(),m_str.Length(),0,eflags,ssv,33);
  bool ret = (ssc > 0);

  if(_flags&global) {
      while(ret) {
	m_matches++;
	int matchLen = ssv[1]-ssv[0];
	if(!(_flags&nomatch)) {
	  match.push_back(RegexxMatch(m_str,ssv[0],matchLen));
	  if(!(_flags&noatom)) {
	    match.back().atom.reserve(m_capturecount);
	    for(int i = 1; i < ssc; i++) {
	      if (ssv[i*2] != -1)
	        match.back().atom.push_back(RegexxMatchAtom(m_str,ssv[i*2],ssv[(i*2)+1]-ssv[i*2]));
	      else
	        match.back().atom.push_back(RegexxMatchAtom(m_str,0,0));
	    }
	  }
	}
	int lastPos = matchLen ? ssv[1] : ssv[1]+1;
	ret = (pcre_exec(m_preg,m_extra,m_str.String(),m_str.Length(),lastPos,eflags,ssv,33) > 0);
      }
  }
  else {
    if(_flags&nomatch) {
      if(ret)
	m_matches=1;
    }
    else if(_flags&noatom) {
      if(ret) {
	m_matches=1;
	match.push_back(RegexxMatch(m_str,ssv[0],ssv[1]-ssv[0]));
      }
    }
    else {
      if(ret) {
	m_matches=1;
	match.push_back(RegexxMatch(m_str,ssv[0],ssv[1]-ssv[0]));
	match.back().atom.reserve(m_capturecount);
	for(int i = 1; i < ssc; i++) {
	  if (ssv[i*2] != -1)
	    match.back().atom.push_back(RegexxMatchAtom(m_str,ssv[i*2],ssv[(i*2)+1]-ssv[i*2]));
	  else
	    match.back().atom.push_back(RegexxMatchAtom(m_str,0,0));
	}
//	ret = (pcre_exec(m_preg,m_extra,m_str.String(),m_str.Length(),ssv[1],eflags,ssv,33) > 0);
      }
    }
  }
  return m_matches;
}

const BString&
regexx::Regexx::replace(const BString& _repstr, int _flags)
  throw(CompileException)
{
  exec(_flags&~nomatch);
  std::vector< std::pair<unsigned int,int32> > v;
  v.reserve(m_capturecount);
  int32 pos = _repstr.FindFirst("$");
  while(pos != B_ERROR) {
    if((pos==0 || _repstr[pos-1] != '\\')
       && _repstr[pos+1] >= '1'
       && _repstr[pos+1] <= '9') {
      v.push_back(std::pair<unsigned int,int32>(_repstr[pos+1]-'1',pos));
    }
    pos = _repstr.FindFirst("$",pos+1);
  }
  BString tmprep;
  m_replaced = "";
  int32 destSize = m_str.Length()+10;
  char* destBuf = m_replaced.LockBuffer( destSize);
  int32 destLen = 0;
  int32 lastPos = 0;
  std::vector<RegexxMatch>::const_iterator m;
  std::vector< std::pair<unsigned int,int32> >::const_iterator i;
  for(m = match.begin(); m != match.end(); m++) {
    if (lastPos < m->start()) {
    	// copy chars between last and current match:
		int32 copyLen = m->start()-lastPos;
    	if (destLen+copyLen >= destSize) {
    	  m_replaced.UnlockBuffer( destLen);
    	  destSize *= 2;
		  destBuf = m_replaced.LockBuffer( destSize);
    	}
    	memcpy( destBuf+destLen, m_str.String()+lastPos, copyLen);
		destLen += copyLen;
    }
    tmprep = _repstr;
	 int offset = 0;
    for(i = v.begin(); i != v.end(); i++) {
    	int32 pos=i->second+offset;
      if(i->first < m->atom.size()) {
			tmprep.Remove( pos, 2);
			tmprep.Insert( m->atom[i->first], pos);
	    	offset += (m->atom[i->first].Length()-2);
      } else {
			tmprep.Remove( pos, 2);
	    	offset -= 2;
      }
    }
    if (destLen+tmprep.Length() >= destSize) {
      m_replaced.UnlockBuffer( destLen);
      destSize *= 2;
      destBuf = m_replaced.LockBuffer( destSize);
  	 }
  	 strcpy( destBuf+destLen, tmprep.String());
    destLen += tmprep.Length();

	 lastPos = m->start() + m->Length();
  }
  if (lastPos < m_str.Length()) {
  	// copy chars following final match:
  	int32 copyLen = m_str.Length() - lastPos;
    if (destLen+copyLen >= destSize) {
      m_replaced.UnlockBuffer( destLen);
      destSize += copyLen+1;
      destBuf = m_replaced.LockBuffer( destSize);
    }
    strcpy( destBuf+destLen, m_str.String()+lastPos);
    destLen += copyLen;
  }
  m_replaced.UnlockBuffer( destLen);
  return m_replaced;
}
