// $Id$    --*- c++ -*--

// Copyright (C) 2003 Enrico Scholz <enrico.scholz@informatik.tu-chemnitz.de>
//  
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; version 2 of the License.
//  
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//  
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
//  

#include <stdint.h>

#define const
#include "../src/parser-const.h"
#undef const

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static void
initCharacterClassification(TokenTable chrs)
{
  int			c;
  unsigned int const	chrSYS = chrIFNAME | chrFILENAME;

    /*@-sizeoftype@*/
  memset(chrs, 0, sizeof(TokenTable));
    /*@=sizeoftype@*/

  /*@+charintliteral@*/
  for (c='0'; c<='9'; ++c) chrs[c] |= (chrDIGIT | chrNUMBER | chrIP |
				       chrSYS | chrUSERNAME);
  for (c='A'; c<='Z'; ++c) chrs[c] |= chrUPPERALPHA | chrSYS | chrUSERNAME;
  for (c='a'; c<='z'; ++c) chrs[c] |= chrLOWALPHA   | chrSYS | chrUSERNAME;
  chrs['\r'] |= chrNL;
  chrs['\n'] |= chrNL;
  
  chrs['\t'] |= chrBLANK;
  chrs[' ']  |= chrBLANK;
  chrs['.']  |= chrIP | chrFILENAME | chrUSERNAME | chrIFNAME;
  chrs['_']  |= chrSYS | chrUSERNAME;
  chrs['-']  |= chrSYS | chrUSERNAME;
  chrs[':']  |= chrSYS;
  chrs['/']  |= chrFILENAME;

  chrs['M']  |= chrUNIT;
  chrs['K']  |= chrUNIT;
  chrs['x']  |= chrBASEMOD;
  chrs['X']  |= chrBASEMOD;
  /*@-charintliteral@*/
}

int main()
{
  TokenTable	table;
  size_t	i;
  
  initCharacterClassification(table);

  for (i=0; i<(sizeof table)/(sizeof(table[0])); ++i) {
    if (i%8==0) printf("  ");

    printf("0x%04x,", table[i]);
    if (i%8 == 7) printf("\n");
    else	    printf(" ");
  }

  if (i%8!=7) printf("\n");

  return EXIT_SUCCESS;
}
