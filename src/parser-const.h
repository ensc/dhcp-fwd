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

#ifndef H_DHCP_FORWARDER_SRC_PARSER_CONST_H
#define H_DHCP_FORWARDER_SRC_PARSER_CONST_H

#include <stdint.h>

#define chrDIGIT	0x01u
#define chrLOWALPHA	0x02u
#define chrUPPERALPHA	0x04u
#define chrALPHA        (chrUPPERALPHA | chrLOWALPHA)
#define chrBLANK	0x08u
#define chrIP		0x10u
#define chrNUMBER	0x20u
#define chrUNIT		0x40u
#define chrNL		0x80u
#define chrEOF		0x100u
#define chrUSERNAME	0x200u
#define chrIFNAME	0x400u
#define chrFILENAME	0x800u
#define chrBASEMOD	0x1000u

typedef uint16_t const	TokenType;
typedef TokenType	TokenTable[257];

#endif	//  H_DHCP_FORWARDER_SRC_PARSER_CONST_H
