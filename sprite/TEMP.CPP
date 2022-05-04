//---------------------------------------------------------------------------
// CDestVehicleSprite::MakeSuperviews
//---------------------------------------------------------------------------
void
CDestVehicleSprite::MakeSuperviews(
	CSourceSprite const * psourcesprite )
{
	int	iZoomOffset 			= 0;
	int	iTotalCompressedSize = 0;
	int	iDir;
	int	iTilt;
	int	iDamage;
	int	iZoom;
	int	i;

	for ( iZoom = NUM_ZOOM_LEVELS - 1; iZoom >= xiFirstZoom; iZoom-- )
	{
		int	iCompressedZoomSize = 0;
		int	iUncompressedSuperviewSize = 0;

		m_appbySuperviews[ iZoom ] = new BYTE * [ m_nSuperviews ];

		for ( i = 0; i < m_nSuperviews; ++i )
			m_appbySuperviews[ iZoom ][i] = NULL;

		for ( iDir = 0; iDir < NUM_VEHICLE_DIRS; ++iDir )
			for ( iTilt = 0; iTilt < NUM_VEHICLE_TILTS; ++iTilt )
				for ( iDamage = 0; iDamage < NUM_VEHICLE_DAMAGES; ++iDamage )
				{
					int	iSpriteIndex =	m_aaaiViewIndices[ iDir ][ iTilt ][ iDamage ];

					if ( -1 != iSpriteIndex )
					{
						CSourceView * psourceview = &psourcesprite->m_pviews[iSpriteIndex];

						iUncompressedSuperviewSize += psourceview->GetDataSize( iZoom );
					}
				}

		BYTE	* pbyUncompressedSuperview = new BYTE [ iUncompressedSuperviewSize ];
		BYTE	* pby = pbyUncompressedSuperview;

		int	  aiDIBOffsetRez[ NUM_COLOR_REZ_BYTES ];

		for ( i = 0; i < NUM_COLOR_REZ_BYTES; ++i )
			aiDIBOffsetRez[i] = 0;

		for ( iDir = 0; iDir < NUM_VEHICLE_DIRS; ++iDir )
			for ( iTilt = 0; iTilt < NUM_VEHICLE_TILTS; ++iTilt )
				for ( iDamage = 0; iDamage < NUM_VEHICLE_DAMAGES; ++iDamage )
				{
					int	iSpriteIndex =	m_aaaiViewIndices[ iDir ][ iTilt ][ iDamage ];

					if ( -1 != iSpriteIndex )
					{
						CSourceView * psourceview = &psourcesprite->m_pviews[iSpriteIndex];
						CDestViewHdr * pdestview  = &m_pviewhdr[iSpriteIndex];

						pby += psourceview->GetData( iZoom, pby );

						pdestview->CalcBitmapOffsets( iZoom, aiDIBOffsetRez );
					}
				}

		int	iCompressedSuperviewSize = 0;

		m_appbySuperviews[iZoom][0] = CompressSuperview( pbyUncompressedSuperview, iUncompressedSuperviewSize, &iCompressedSuperviewSize );

		m_playoutinfozoomsSuperview[ 0 ].m_alayoutinfo[ iZoom ].m_blockinfoCompressed.m_iLength = iCompressedSuperviewSize;
		m_playoutinfozoomsSuperview[ 0 ].m_alayoutinfo[ iZoom ].m_blockinfoCompressed.m_iOffset = iCompressedZoomSize;

		iTotalCompressedSize += iCompressedSuperviewSize;
		iCompressedZoomSize  += iCompressedSuperviewSize;

		for ( i = 0; i < NUM_COLOR_REZ_BYTES; ++i )
			m_playoutinfozoomsSuperview[ 0 ].m_alayoutinfo[ iZoom ].m_aiDecompressedLength[i] = aiDIBOffsetRez[i];

		ASSERT( m_playoutinfozoomsSuperview[ 0 ].m_alayoutinfo[ iZoom ].m_aiDecompressedLength[((psourcesprite->m_iBitsPerPixel + 7 ) >>3)-1] == iUncompressedSuperviewSize );

		delete pbyUncompressedSuperview;

		pbyUncompressedSuperview = NULL;

		m_zoomblockinfo[ iZoom ].m_iOffset = iZoomOffset;
		m_zoomblockinfo[ iZoom ].m_iLength = iCompressedZoomSize;

		iZoomOffset += iCompressedZoomSize;
	}

	m_iDataLen = iTotalCompressedSize;
}


