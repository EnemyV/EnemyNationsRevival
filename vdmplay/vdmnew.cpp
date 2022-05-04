// advdlg.cpp : implementation file
//
#include "afxwin.h"
#include "afxdlgs.h"


#include "stdafx.h"
#include "vdmplay.h"
#include "resource.h"
#include "advdlg.h"
#include "base.h"
#include "help.h"



void* operator new(size_t s)
{
	void*  p = malloc(s);

	memset(p, 0, s);

	return p;
}

