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

#include "split.hh"

std::vector<BString>
regexx::split(const BString& _where, const BString& _str)
{
  BString temp;
  std::vector<BString> v;
  int32 lastpos = 0;
  int32 pos = _str.FindFirst(_where);
  while(pos != B_ERROR) {
    v.push_back(_str.CopyInto(temp,lastpos,pos-lastpos));
    lastpos = pos+_where.Length();
    pos = _str.FindFirst(_where,lastpos);
  }
  v.push_back(_str.CopyInto(temp,lastpos,_str.Length()));
  return v;
}

std::vector<BString>
regexx::splitex(const BString& _regex, const BString& _str)
{
  BString temp;
  std::vector<BString> v;
  Regexx rxx;
  rxx.expr(_regex);
  rxx.str(_str);
  v.reserve(rxx.exec());
  std::vector<RegexxMatch>::const_iterator i;
  int32 lastpos = 0;
  for(i = rxx.match.begin(); i != rxx.match.end(); i++) {
    v.push_back(_str.CopyInto(temp,lastpos,i->start()-lastpos));
    lastpos = i->start()+i->Length();
  }
  v.push_back(_str.CopyInto(temp,lastpos,i->start()));
  return v;
}
