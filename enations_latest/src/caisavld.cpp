////////////////////////////////////////////////////////////////////////////
//
//  CAISavLd.cpp : Implements class for the Save/Load class.
//
//  Last update:    09/13/96
//
//  Copyright (c) 1995, 1996. Windward Studios, Inc. - All Rights Reserved
//
////////////////////////////////////////////////////////////////////////////

#include "STDAFX.H"
#include "CAI.h"
#include "CAISavLd.hpp"

////////////////////////////////////////////////////////////////////////////
//
//  CAISavLd CLASS MEMBER FUNCTIONS
//
////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNAMIC( CAISavLd, CObject );

#define new DEBUG_NEW
#define DEBUG_SAVELOAD	0

extern CAIGoalList *plGoalList;	// standard goals
extern CAITaskList *plTaskList;	// standard tasks
//extern CWordArray *pwaRCIP;		// racial characteristics and initial pos
extern CWordArray *pwaIG;		// initial goals

CAISavLd::CAISavLd( CWnd *pParent )
{
    m_pParent = pParent;
}

void CAISavLd::SaveBinary( void )
{
    CFile *pFile;
    //CFileException* theException = new CFileException;
	
   	try
   	{
       	pFile = OpenForDump( "stdgta.dat" );
       	ASSERT( pFile != NULL );
            
       	DumpBinaryData( pFile );
   	}
   	catch( CFileException theException )
   	{
   		if( m_pParent != NULL )
       		m_pParent->MessageBox( "Save Binary Error", "stdgta.dat", 
       		MB_ICONEXCLAMATION | MB_SYSTEMMODAL | MB_OK );
       	
    	pFile->Close();
    	delete pFile;
		return;
       	//throw(ERR_CAI_BAD_FILE);
   	}

    pFile->Close();
    delete pFile;
        
	//theException->Delete();
}

void CAISavLd::Save( void )
{
    CFile *pFile;
    CFileException* theException = new CFileException;
    CString sStdExt, sHead, sSave;
    sStdExt = ".std";

    for( int i=TASK_DATA; i<=GOAL_DATA; ++i )
    {
    	switch( i )
    	{
    		case GOAL_DATA:
    			sHead = "goals";
    			break;
    		case TASK_DATA:
    			sHead = "tasks";
    			break;
    		default:
    			return;
    	}
		sSave = sHead + sStdExt;

		CString sMsg;
    	// save the list of objects
    	TRY
    	{
        	pFile = OpenForDump( sSave );
        	ASSERT( pFile != NULL );
            
        	DumpData( pFile, i );
    	}
    	CATCH( CFileException, theException )
    	{
    		// "Save Error"
    		sMsg = "Save Error";
    		if( m_pParent != NULL )
        	m_pParent->MessageBox( sMsg, sSave, 
        		MB_ICONEXCLAMATION | MB_SYSTEMMODAL | MB_OK );
        	sMsg.Empty();

        	//throw(ERR_CAI_BAD_FILE);
    	}
    	AND_CATCH( CArchiveException, theException )
    	{
        	pFile->Close();
        	delete pFile;
        	// "Archive Error"
        	sMsg = "Archive Error";
        	if( m_pParent != NULL )
        	m_pParent->MessageBox( sMsg, sSave, 
        		MB_ICONEXCLAMATION | MB_SYSTEMMODAL | MB_OK );
        	sMsg.Empty();

        	//throw(ERR_CAI_BAD_FILE);
    	}
		END_CATCH

        pFile->Close();
        delete pFile;
        
        sSave.Empty();
        sHead.Empty();
    }
    //delete theException;
	theException->Delete();
    sStdExt.Empty();
}

void CAISavLd::LoadBinary( void )
{
    CFile *pFile = NULL;
    //CFileException* theException = new CFileException;

   	try
   	{
		pFile = theDataFile.OpenAsFile( "stdgta.dat" );
#ifdef BUGBUG
       	pFile = OpenForLoad( "stdgta.dat" );
#endif
       	ASSERT( pFile != NULL );
            
       	LoadBinaryData( pFile );
   	}
   	catch( CFileException theException )
   	{
   		if( m_pParent != NULL )
       	m_pParent->MessageBox( "Load Binary Error", "stdgta.dat", 
       		MB_ICONEXCLAMATION | MB_SYSTEMMODAL | MB_OK );
       	
		if( pFile != NULL )
		{
    		pFile->Close();
    		delete pFile;
		}
       	
       	throw(ERR_CAI_BAD_FILE);
   	}

    pFile->Close();
    delete pFile;
        
	//theException->Delete();
}

void CAISavLd::Load( void )
{
    CFile *pFile;
    //CFileException* theException = new CFileException;
    CString sStdExt, sHead, sFile, sMsg;
    sStdExt = ".std";

    for( int i=TASK_DATA; i<=GOAL_DATA; ++i )
    {
    	switch( i )
    	{
    		case GOAL_DATA:
    			sHead = "goals";
    			break;
    		case TASK_DATA:
    			sHead = "tasks";
    			break;
    		default:
    			return;
    	}
		sFile = sHead + sStdExt;

    	try
    	{
        	pFile = OpenForLoad( sFile );
        	ASSERT( pFile != NULL );
            
        	LoadData( pFile, i );
    	}
    	catch( CFileException theException )
    	{
    		// "LOAD File Error"
    		sMsg = "LOAD File Error";
    		if( m_pParent != NULL )
        	m_pParent->MessageBox( sMsg, sFile, 
        		MB_ICONEXCLAMATION | MB_SYSTEMMODAL | MB_OK );
        	sMsg.Empty();

        	throw(ERR_CAI_BAD_FILE);
    	}
    	catch( CArchiveException theException )
    	{
    		if( theException.m_cause !=
    			CArchiveException::endOfFile )
    		{
        	pFile->Close();
        	delete pFile;
        	// "LOAD Archive Error"
        	sMsg = "LOAD Archive Error";
        	if( m_pParent != NULL )
        	m_pParent->MessageBox( sMsg, sFile, 
        		MB_ICONEXCLAMATION | MB_SYSTEMMODAL | MB_OK );
        	sMsg.Empty();

        	throw(ERR_CAI_BAD_FILE);
        	}
    	}

    	pFile->Close();
    	delete pFile;
    	
    	sFile.Empty();
    	sHead.Empty();    	
    }
    //delete theException;
	//theException->Delete();
    sStdExt.Empty();
}

CWordArray *CAISavLd::LoadRCIP( int iSmart )
{
	CString sHead;
	sHead = "drcip"; // name of serialized word array of values
	return( LoadArrays( iSmart, sHead ) );
}

CWordArray *CAISavLd::LoadIG( int iSmart )
{
	CString sHead;
	sHead = "goals"; // name of serialized word array of values
	return( LoadArrays( iSmart, sHead ) );
}

//
// load the word arrays that contain the racial characteristics
// and initial position of the AI player based on difficulty
// value previously set in GameData
//
CWordArray *CAISavLd::LoadArrays( int iSmart, CString& sHead )
{
	CWordArray *pwaLoadRCIP = NULL;
    CFile *pFile;
    //CFileException* theException = new CFileException;
    CString sExt, sFile, sMsg;
    
    
    wsprintf( sExt.GetBuffer( 3 ), ".%d", iSmart );
    sExt.ReleaseBuffer();
    sFile = sHead + sExt;

   	try
   	{
       	pFile = OpenForLoad( sFile );
       	ASSERT( pFile != NULL );
            
       	pwaLoadRCIP = LoadData( pFile );
   	}
   	catch( CFileException theException )
   	{
   		// "LOAD File Error"
   		sMsg = "LOAD File Error";
   		if( m_pParent != NULL )
       	m_pParent->MessageBox( sMsg, sFile, 
       		MB_ICONEXCLAMATION | MB_SYSTEMMODAL | MB_OK );
       	sMsg.Empty();

       	throw(ERR_CAI_BAD_FILE);
   	}
   	catch( CArchiveException theException )
   	{
   		if( theException.m_cause !=
   			CArchiveException::endOfFile )
   		{
        	pFile->Close();
        	delete pFile;
        	// "LOAD Archive Error"
        	sMsg = "LOAD Archive Error";
        	if( m_pParent != NULL )
        	m_pParent->MessageBox( sMsg, sFile, 
        		MB_ICONEXCLAMATION | MB_SYSTEMMODAL | MB_OK );
        	sMsg.Empty();

        	throw(ERR_CAI_BAD_FILE);
       	}
   	}

   	pFile->Close();
   	delete pFile;
    //delete theException;
	//theException->Delete();
    sExt.Empty();
   	sFile.Empty();
   	sHead.Empty();
   	
   	return( pwaLoadRCIP );
}

void CAISavLd::SaveIG( int iSmart, CWordArray *pwaData )
{
	CString sHead;
	sHead = "goals"; // name of serialized word array of values
	SaveArrays( iSmart, pwaData, sHead );
}

void CAISavLd::SaveRCIP( int iSmart, CWordArray *pwaData )
{
	CString sHead;
	sHead = "drcip"; // name of serialized word array of values
	SaveArrays( iSmart, pwaData, sHead );
}

void CAISavLd::SaveArrays( int iSmart, CWordArray *pwaData, CString& sHead )
{
    CFile *pFile;
    CFileException* theException = new CFileException;
    CString sExt, sFile, sMsg;
    
    
    wsprintf( sExt.GetBuffer( 3 ), ".%d", iSmart );
    sExt.ReleaseBuffer();
    sFile = sHead + sExt;

   	TRY
   	{
       	pFile = OpenForDump( sFile );
       	ASSERT( pFile != NULL );
            
       	DumpData( pFile, pwaData );
   	}
   	CATCH( CFileException, theException )
   	{
    	// "Save Error"
    	sMsg = "Save Error";
   		if( m_pParent != NULL )
       	m_pParent->MessageBox( sMsg, sFile, 
       		MB_ICONEXCLAMATION | MB_SYSTEMMODAL | MB_OK );
       	sMsg.Empty();
       	return;
   	}
   	AND_CATCH( CArchiveException, theException )
   	{
   		if( theException->m_cause !=
   			CArchiveException::endOfFile )
   		{
        	pFile->Close();
        	delete pFile;
        	// "SAVE Archive Error"
        	sMsg = "SAVE Archive Error";
        	if( m_pParent != NULL )
        	m_pParent->MessageBox( sMsg, sFile, 
        		MB_ICONEXCLAMATION | MB_SYSTEMMODAL | MB_OK );
        	sMsg.Empty();
        	return;
       	}
   	}
   	END_CATCH

   	pFile->Close();
   	delete pFile;
    //delete theException;
	theException->Delete();
    sExt.Empty();
   	sFile.Empty();
   	sHead.Empty();
}

void CAISavLd::LoadBinaryData( CFile *pFile )
{
	GoalBuff goalbuffer;
	TaskBuff taskbuffer;
	//CFileException* theException = new CFileException;

	if( pFile == NULL )
		throw(ERR_CAI_BAD_FILE);

	// first, make sure goal list is empty
	if( plGoalList != NULL )
		plGoalList->DeleteList();

	int iCnt;
	try
	{
		pFile->Read( (void*)&iCnt, sizeof(int) );
	}
    catch( CFileException theException )
    {
        throw(ERR_CAI_BAD_FILE);
    }


	// if there are goals to read
	if( iCnt )
	{
		int iBytes;
		for( int i=0; i<iCnt; ++i )
		{
			// read file data into buffer
			try
			{
				iBytes = 
					pFile->Read( 
					(void*)&goalbuffer, sizeof(GoalBuff) );
			}
    		catch( CFileException theException )
    		{
        		throw(ERR_CAI_BAD_FILE);
    		}

			if( iBytes != sizeof(GoalBuff) )
				return;

			// count actual tasks for this goal
			for( int j=0; j<NUM_INITIAL_GOALS; ++j )
			{
				if( !goalbuffer.iTasks[j] )
					break;
			}
			// j contains number of actual tasks

			CWordArray *pwaTasks = NULL;
			CAIGoal *pGoal = NULL;
			try
			{
				// create array for tasks
				pwaTasks = new CWordArray();
				pwaTasks->SetSize(j);
				for( j=0; j<pwaTasks->GetSize(); ++j )
					pwaTasks->SetAt( j, goalbuffer.iTasks[j] );

				//CAIGoal( WORD wID, BYTE cType, CWordArray *pwaTasks );
				pGoal = new CAIGoal( (WORD)goalbuffer.iID,
					(BYTE)goalbuffer.iType, pwaTasks );
				plGoalList->AddTail( (CObject *)pGoal );
			}
			catch( CException e )
			{
				if( pwaTasks != NULL )
				{
				// clean up array
				pwaTasks->RemoveAll();
				delete pwaTasks;
				}
				if( pGoal != NULL )
					delete pGoal;

				throw(ERR_CAI_BAD_NEW);
			}

			// clean up array
			if( pwaTasks != NULL )
			{
				pwaTasks->RemoveAll();
				delete pwaTasks;
			}
		}
	}

	// make sure task list is empty
	if( plTaskList != NULL )
		plTaskList->DeleteList();

	// now, read count of tasks
	try
	{
		pFile->Read( (void*)&iCnt, sizeof(int) );
	}
    catch( CFileException theException )
    {
        throw(ERR_CAI_BAD_FILE);
    }
	
	if( iCnt )
	{
		int iBytes;
		for( int i=0; i<iCnt; ++i )
		{
			// read file data into buffer
			try
			{
				iBytes = 
					pFile->Read( 
					(void*)&taskbuffer, sizeof(TaskBuff) );
			}
    		catch( CFileException theException )
    		{
        		throw(ERR_CAI_BAD_FILE);
    		}


			if( iBytes != sizeof(TaskBuff) )
				return;

			CWordArray *pwaParams = NULL;
			CAITask *pTask = NULL;
			try
			{
				// create array for tasks params
				pwaParams = new CWordArray();
				pwaParams->SetSize(MAX_TASKPARAMS);
				for( int j=0; j<pwaParams->GetSize(); ++j )
					pwaParams->SetAt( j, taskbuffer.iParams[j] );

				//	CAITask( WORD wID, BYTE cType, BYTE cPriority, 
				//		WORD wOrderID, CWordArray *pwaParams );
				pTask = new CAITask( (WORD)taskbuffer.iID,
					(BYTE)taskbuffer.iType,(BYTE)taskbuffer.iPriority,
					(WORD)taskbuffer.iOrderID, pwaParams );

				plTaskList->AddTail( (CObject *)pTask );
			}
			catch( CException e )
			{
				if( pwaParams != NULL )
				{
				// clean up array
				pwaParams->RemoveAll();
				delete pwaParams;
				}
				if( pTask != NULL )
					delete pTask;

				throw(ERR_CAI_BAD_NEW);
			}
			if( pwaParams != NULL )
			{
				// clean up array
				pwaParams->RemoveAll();
				delete pwaParams;
			}
		}
	}


	/*
	// now do racial charactistics and inital position
	if( pwaRCIP == NULL )
	{
		try
		{
			int iSizeOne = NUM_RACE_CHAR + NUM_INIT_SPLYS;
			// calculate size all difficulty levels
			int iSize = iSizeOne * NUM_DIFFICUTY_LEVELS;
			pwaRCIP = new CWordArray();
			pwaRCIP->SetSize(iSize);
			for( int i=0; i<pwaRCIP->GetSize(); ++i )
				pwaRCIP->SetAt(i,0);
		}
		catch( CException e )
		{
			if( pwaRCIP != NULL )
			{
				pwaRCIP->RemoveAll();
				delete pwaRCIP;
				pwaRCIP = NULL;
			}
			throw(ERR_CAI_BAD_NEW);
		}
	}

	for( int i=0; i<pwaRCIP->GetSize(); ++i )
	{
		try
		{
			pFile->Read( (void*)&iCnt, sizeof(int) );
		}
    	catch( CFileException theException )
    	{
       		throw(ERR_CAI_BAD_FILE);
    	}

		pwaRCIP->SetAt(i,iCnt);
	}
	*/

	// and now the initial goals
	if( pwaIG == NULL )
	{
		try
		{
			// calculate size all difficulty levels
			int iSize = NUM_INITIAL_GOALS * NUM_DIFFICUTY_LEVELS;
			pwaIG = new CWordArray();
			pwaIG->SetSize(iSize);
			for( int i=0; i<pwaIG->GetSize(); ++i )
				pwaIG->SetAt(i,0);
		}
		catch( CException e )
		{
			if( pwaIG != NULL )
			{
				pwaIG->RemoveAll();
				delete pwaIG;
				pwaIG = NULL;
			}
			throw(ERR_CAI_BAD_NEW);
		}

	}

	for( int i=0; i<pwaIG->GetSize(); ++i )
	{
		try
		{
			pFile->Read( (void*)&iCnt, sizeof(int) );
		}
    	catch( CFileException theException )
    	{
       		throw(ERR_CAI_BAD_FILE);
    	}

		pwaIG->SetAt(i,iCnt);
	}

	//theException->Delete();
}

//
// this function loads the racial characteristics/initial
// position data that has been serialized in pFile
//
CWordArray *CAISavLd::LoadData( CFile *pFile )
{
    CFileException* theException = new CFileException;
    ASSERT( theException != NULL );
    
    CArchive archive( pFile, CArchive::load );
    CObject *pObj;
    
    TRY
    {
        archive >> (CObject *)pObj;
    }
    CATCH( CArchiveException, theException )
    {
        archive.Close();
        THROW_LAST();
    }
    END_CATCH
    archive.Close();
    //delete theException;
	theException->Delete();
    
    return( (CWordArray *)pObj );
}

void CAISavLd::LoadData( CFile *pFile, UINT uWhich )
{
    CFileException* theException = new CFileException;
    ASSERT( theException != NULL );
    
    CArchive archive( pFile, CArchive::load );
    CObject *pObj;
    
    TRY
    {
        archive >> (CObject *)pObj;
    }
    CATCH( CArchiveException, theException )
    {
        archive.Close();
        THROW_LAST();
    }
    END_CATCH
    archive.Close();
    //delete theException;
	theException->Delete();
    
    if( uWhich == GOAL_DATA )
    {
        plGoalList = (CAIGoalList *)pObj;
    }
    else if( uWhich == TASK_DATA )
    {
        plTaskList = (CAITaskList *)pObj;
    }
}

void CAISavLd::DumpBinaryData( CFile *pFile )
{
	GoalBuff goalbuffer;
	TaskBuff taskbuffer;
	//CFileException* theException = new CFileException;

	// first, write count of goals
	int iCnt = plGoalList->GetCount();
	try
	{
		pFile->Write( (const void*)&iCnt, sizeof(int) );
	}
    catch( CFileException theException )
    {
        throw(ERR_CAI_BAD_FILE);
    }

	// if there are goals to write
	if( iCnt )
	{
    	POSITION pos = plGoalList->GetHeadPosition();
    	while( pos != NULL )
    	{   
        	CAIGoal *pGoal = (CAIGoal *)plGoalList->GetNext( pos );
        	if( pGoal != NULL )
        	{
        		ASSERT_VALID( pGoal );

				// move data to buffer
				goalbuffer.iID = (int)pGoal->GetID();
				goalbuffer.iType = (int)pGoal->GetType();

				for( int i=0; i<NUM_INITIAL_GOALS; ++i )
					goalbuffer.iTasks[i] = 0;
				
				i = 0;
				while( pGoal->GetTaskAt(i) )
				{
					goalbuffer.iTasks[i] = (int)pGoal->GetTaskAt(i);
					++i;
				}

				// now write buffer out
				try
				{
					pFile->Write( 
						(const void*)&goalbuffer, sizeof(GoalBuff) );
				}
    			catch( CFileException theException )
    			{
        			throw(ERR_CAI_BAD_FILE);
    			}
        	}
    	}
	}

	// now, write count of tasks
	iCnt = plTaskList->GetCount();
	try
	{
		pFile->Write( (const void*)&iCnt, sizeof(int) );
	}
    catch( CFileException theException )
    {
        throw(ERR_CAI_BAD_FILE);
    }

	// if there are tasks to write
	if( iCnt )
	{
    	POSITION pos = plTaskList->GetHeadPosition();
    	while( pos != NULL )
    	{   
        	CAITask *pTask = (CAITask *)plTaskList->GetNext( pos );
        	if( pTask != NULL )
        	{
        		ASSERT_VALID( pTask );

				// move data to buffer
				taskbuffer.iID = (int)pTask->GetID();
				taskbuffer.iGoal = 0;
				taskbuffer.iType = (int)pTask->GetType();
				taskbuffer.iPriority = (int)pTask->GetPriority();
				taskbuffer.iOrderID = (int)pTask->GetOrderID();

				for( int i=0; i<MAX_TASKPARAMS; ++i )
					taskbuffer.iParams[i] = 0;
				for( i=0; i<MAX_TASKPARAMS; ++i )
					taskbuffer.iParams[i] = (int)pTask->GetTaskParam(i);
				
				// now write buffer out
				try
				{
					pFile->Write( 
						(const void*)&taskbuffer, sizeof(TaskBuff) );
				}
    			catch( CFileException theException )
    			{
        			throw(ERR_CAI_BAD_FILE);
    			}
        	}
    	}
	}


	/*
	// now do racial charactistics and inital position
	if( pwaRCIP != NULL )
	{
		for( int i=0; i<pwaRCIP->GetSize(); ++i )
		{
			iCnt = (int)pwaRCIP->GetAt(i);
			try
			{
				pFile->Write( (const void*)&iCnt, sizeof(int) );
			}
    		catch( CFileException theException )
    		{
        		throw(ERR_CAI_BAD_FILE);
    		}
		}
	}
	*/

	// and now the initial goals
	if( pwaIG != NULL )
	{
		for( int i=0; i<pwaIG->GetSize(); ++i )
		{
			iCnt = (int)pwaIG->GetAt(i);
			try
			{
				pFile->Write( (const void*)&iCnt, sizeof(int) );
			}
    		catch( CFileException theException )
    		{
        		throw(ERR_CAI_BAD_FILE);
    		}
		}
	}

	//theException->Delete();
}

void CAISavLd::DumpData( CFile *pFile, CWordArray *pwaData )
{
    CArchive archive( pFile, CArchive::store );
    CFileException* theException = new CFileException;
    ASSERT( theException != NULL );
    CObject *pObj = NULL;
    pObj = (CObject *)pwaData;

    TRY
    {
    	if( pObj != NULL )
        	archive << (CObject *)pObj;
    }
    CATCH( CArchiveException, theException )
    {
        archive.Close();
        THROW_LAST();
    }
    END_CATCH
    archive.Close();
    //delete theException;
	theException->Delete();
}

void CAISavLd::DumpData( CFile *pFile, UINT uWhich )
{
    CArchive archive( pFile, CArchive::store );
    CFileException* theException = new CFileException;
    ASSERT( theException != NULL );
    CObject *pObj = NULL;
    

    if( uWhich == GOAL_DATA )
    {
        pObj = (CObject *)plGoalList;
    }
    else if( uWhich == TASK_DATA )
    {
    	pObj = (CObject *)plTaskList;
    }

    TRY
    {
    	if( pObj != NULL )
        	archive << (CObject *)pObj;
    }
    CATCH( CArchiveException, theException )
    {
        archive.Close();
        THROW_LAST();
    }
    END_CATCH
    archive.Close();
    //delete theException;
	theException->Delete();
}

//
// OpenForLoad - open a file for reading
//
CFile* CAISavLd::OpenForLoad( const CString& rFileName )
{
    CFile* pFile = new CFile;
    CFileException* theException = new CFileException;
    if ( !pFile->Open( rFileName, CFile::modeRead, theException ) )
    {
        delete pFile;
        throw(ERR_CAI_BAD_FILE);
    }
    //delete theException;
	theException->Delete();

    // Exit here if no exceptions
    return pFile;
}

//
// OpenForDump - open a file for writing
//
CFile* CAISavLd::OpenForDump( const CString& rFileName )
{
    CFile* pFile = new CFile;
    CFileStatus status;
    UINT nAccess = CFile::modeWrite;

    // GetStatus will return TRUE if file exists,
    // or FALSE if it doesn't exist
    // if ( !CFile::GetStatus( rFileName, status ) )
        nAccess |= CFile::modeCreate;
    
    CFileException* theException = new CFileException;
    if ( !pFile->Open( rFileName, nAccess, theException ) )
    {
        delete pFile;
        throw(ERR_CAI_BAD_FILE);
    }
    //delete theException;
	theException->Delete();
    
    // Exit here if no errors or exceptions
    return pFile;
}

// end of CAISavLd.cpp
