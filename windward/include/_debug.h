#ifndef ___DEBUG_H__
#define ___DEBUG_H__


// priorities
const int ASSERT_PRI_ALWAYS = 0;
const int ASSERT_PRI_CRITICAL = 1;
const int ASSERT_PRI_USEFUL = 2;
const int ASSERT_PRI_ANAL = 3;

// windward.lib sections
const int ASSERT_SEC_MMIO =		0x00000001;
const int ASSERT_SEC_MUSIC =	0x00000002;
const int ASSERT_SEC_DIB =		0x00000004;
const int ASSERT_SEC_WND =		0x00000008;
const int ASSERT_SEC_MISC =		0x00000010;
const int ASSERT_RESERVED =		0x00000020;


#ifdef _DEBUG

extern int __iAssertPriority, __iAssertSection;

#define	xASSERT(iPriority,iSection,bTrue) do { if (((iPriority) <= __iAssertPriority) && ((iSection) & __iAssertSection)) if (!(bTrue) && AfxAssertFailedLine(THIS_FILE, __LINE__)) AfxDebugBreak(); } while (0)
#define	xASSERT_VALID(iPriority,iSection,pObject) ((((iPriority) > __iAssertPriority) || (! ((iSection) & __iAssertSection))) ? 0 : ASSERT_VALID (pObject))
#define	xASSERT_VALID_OR_NULL(iPriority,iSection,pObject) ((((iPriority) > __iAssertPriority) || (! ((iSection) & __iAssertSection)) || ((pObject) == NULL)) ? 0 : ASSERT_VALID (pObject))
#define	xASSERT_VALID_ADDR(iPriority,iSection,pMem,iLen) xASSERT (iPriority, iSection, AfxIsValidAddress ((pMem), (iLen)))
#define	xASSERT_VALID_ADDR_OR_NULL(iPriority,iSection,pMem,iLen) xASSERT (iPriority, iSection, (pMem == NULL) || AfxIsValidAddress ((pMem), (iLen)))

#else

#define	xASSERT(iPriority,iSection,bTrue) 
#define	xASSERT_VALID(iPriority,iSection,pObject)
#define	xASSERT_VALID_OR_NULL(iPriority,iSection,pObject)
#define	xASSERT_VALID_ADDR(iPriority,iSection,pMem,iLen)
#define	xASSERT_VALID_ADDR_OR_NULL(iPriority,iSection,pMem,iLen)

#endif


#endif
