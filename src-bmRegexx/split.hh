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

#ifndef SPLIT_HH
#define SPLIT_HH

#include <vector>

#include "libregexx.h"
#include "BmString.h"
#include "regexx.hh"

namespace regexx {

  IMPEXPBMREGEXX vector<BmString> split(const BmString& _where, const BmString& _str);

  IMPEXPBMREGEXX void split(const BmString& _where, const BmString& _str, vector<BmString>& v);

  IMPEXPBMREGEXX vector<BmString> splitex(const BmString& _exp, const BmString& _str);

  IMPEXPBMREGEXX void splitex(const BmString& _regex, const BmString& _str, vector<BmString>& v);

}

#endif // SPLIT_HH
