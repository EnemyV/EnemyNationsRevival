#ifndef ECONPROD_H
#define ECONPROD_H

//---------------------------------------------------------------------------
//
//  Production-display predicates (bug #13).
//
//  Header-only and dependency-free ON PURPOSE: new_unit.cpp compiles it into
//  the game and tests/econ/test_mat_prod_bar.cpp compiles the same text with
//  cl.exe alone, so the fixture exercises the shipped predicate rather than a
//  copy of it.
//
//  DISPLAY ONLY. Nothing here may be called from the simulation: it answers
//  "what should the player SEE", and the sim's own gates (CBuilding::Build-
//  Materials) stay exactly as they are.
//
//---------------------------------------------------------------------------

namespace enecon {

//  TRUE when a materials converter cannot fund even ONE output batch, i.e. the
//  batch it is nominally working on will produce NOTHING.
//
//  Why "< one batch" and not "empty": CBuilding::BuildMaterials computes the
//  batches it can afford as an integer divide, iNum = GetStore( i ) / GetInput( i ),
//  so anything short of a full batch's worth of ANY required input truncates to
//  zero batches -- the accumulated progress is dropped (m_iBuildDone = 0) and no
//  output at all is emitted. A store of 4 iron against a 10-iron recipe is just
//  as unproductive as a store of 0.
//
//  fnInput( i ) = units of material i consumed per batch (0 = not an input)
//  fnStore( i ) = units of material i this building has on hand
template <class TInput, class TStore>
inline bool MatBarStalled( int nTypes, TInput fnInput, TStore fnStore )
{
    for ( int iIn = 0; iIn < nTypes; ++iIn )
    {
        int iNeed = fnInput( iIn );
        if ( iNeed > 0 && fnStore( iIn ) < iNeed )
            return ( true );
    }
    return ( false );
}

}  // namespace enecon

#endif  // ECONPROD_H
