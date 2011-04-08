//
// TigerTree.h
//
// Copyright (c) Shareaza Development Team, 2002-2006.
// This file is part of SHAREAZA (www.shareaza.com)
//
// Shareaza is free software; you can redistribute it
// and/or modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2 of
// the License, or (at your option) any later version.
//
// Shareaza is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Shareaza; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//

#ifndef TIGERTREE_H_INCLUDED
#define TIGERTREE_H_INCLUDED

#pragma once
//#include <Windows.h>
#include <memory.h>
#include <stdlib.h>

typedef unsigned long DWORD;
typedef int BOOL;
typedef unsigned char BYTE;
typedef const void *LPCVOID;
typedef BYTE *LPBYTE;

#ifndef max
#  define max(a, b) ((a > b)?(a):(b))
#endif

#ifndef min
#  define min(a, b) ((a < b)?(a):(b))
#endif

#ifndef FALSE
#  define FALSE 0
#endif

#ifndef TRUE
#  define TRUE 1
#endif

#ifndef _ASSERT
#  define _ASSERT assert
#endif

#ifndef CopyMemory
#  define CopyMemory(Destination,Source,Length) memcpy((Destination),(Source),(Length))
#endif

typedef unsigned __int64 uint64;
typedef unsigned int uint32;
namespace std {
  typedef unsigned int size_t;
}

class CTigerNode;

class CTigerTree
{
// Construction
public:
	CTigerTree();
	virtual ~CTigerTree();

// Operations
public:
	void	SetupAndAllocate(DWORD nHeight, uint64 nLength);
	void	SetupParameters(uint64 nLength);
	void	Clear();
	//void	Serialize(CArchive& ar);
	DWORD	GetSerialSize() const;
public:
	BOOL	GetRoot(unsigned char* md) const;
	void	Assume(CTigerTree* pSource);
public:
	void	BeginFile(DWORD nHeight, uint64 nLength);
	void	AddToFile(const void* pInput, DWORD nLength);
	BOOL	FinishFile();
public:
	void	BeginBlockTest();
	void	AddToTest(const void* pInput, DWORD nLength);
	BOOL	FinishBlockTest(DWORD nBlock);
public:
	BOOL	ToBytes(BYTE** pOutput, DWORD* pnOutput, DWORD nHeight = 0);
	BOOL	FromBytes(BYTE* pOutput, DWORD nOutput, DWORD nHeight, uint64 nLength);
	BOOL	CheckIntegrity();
	void	Dump();

// Inlines
public:
	BOOL	IsAvailable() const { return m_pNode != NULL; }
	DWORD	GetHeight() const { return m_nHeight; }
	DWORD	GetBlockLength() const { return 1024 * m_nBlockCount; }
	DWORD	GetBlockCount() const { return m_nBaseUsed; }

// Attributes
private:
	DWORD		m_nHeight;
	CTigerNode*	m_pNode;
	DWORD		m_nNodeCount;

// Processing Data
private:
	DWORD		m_nNodeBase;
	DWORD		m_nNodePos;
	DWORD		m_nBaseUsed;
	DWORD		m_nBlockCount;
	DWORD		m_nBlockPos;
	CTigerNode*	m_pStackBase;
	CTigerNode*	m_pStackTop;

// Implementation
private:
	void	Collapse();
	void	BlocksToNode();
	void	Tiger(LPCVOID pInput, uint64 nInput, uint64* pOutput, uint64* pInput1 = NULL, uint64* pInput2 = NULL);
};

class CTigerNode
{
// Construction
public:
	CTigerNode() : bValid( false ) { memset(value, 0, sizeof(value)); }

// Attributes
public:
	uint64	value[3];
	bool bValid;
};

#endif // #ifndef TIGERTREE_H_INCLUDED
