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

std::vector<BmString>
regexx::split(const BmString& _where, const BmString& _str)
{
  std::vector<BmString> v;
  split( _where, _str, v);
  return v;
}

void
regexx::split(const BmString& _where, const BmString& _str,
				  std::vector<BmString>& v)
{
  BmString temp;
  v.clear();
  int32 lastpos = 0;
  int32 pos = _str.FindFirst(_where);
  while(pos != B_ERROR) {
    v.push_back(_str.CopyInto(temp,lastpos,pos-lastpos));
    lastpos = pos+_where.Length();
    pos = _str.FindFirst(_where,lastpos);
  }
  if (lastpos<_str.Length())
    v.push_back(_str.CopyInto(temp,lastpos,_str.Length()-lastpos));
}

std::vector<BmString>
regexx::splitex(const BmString& _regex, const BmString& _str)
{
  std::vector<BmString> v;
  splitex( _regex, _str, v);
  return v;
}

void
regexx::splitex(const BmString& _regex, const BmString& _str,
					 std::vector<BmString>& v)
{
  BmString temp;
  Regexx rxx;
  rxx.expr(_regex);
  rxx.str(_str);
  v.clear();
  v.reserve(rxx.exec());
  std::vector<RegexxMatch>::const_iterator i;
  int32 lastpos = 0;
  for(i = rxx.match.begin(); i != rxx.match.end(); i++) {
    v.push_back(_str.CopyInto(temp,lastpos,i->start()-lastpos));
    lastpos = i->start()+i->Length();
  }
  if (lastpos<_str.Length())
    v.push_back(_str.CopyInto(temp,lastpos,_str.Length()-lastpos));
}
