#ifndef ENATIONS_ALTOUTPUT_H
#define ENATIONS_ALTOUTPUT_H

// Reusable "alternate-output toggle" system.
//
// Several buildings can be given a research-gated, per-building toggle that makes them
// ALSO produce a secondary resource while operating. The first instance shipped as the
// one-off BioFuel feature (#33: a farm that also makes oil); this file generalises that
// single pattern into one config-driven mechanism so the others (Coal Liquefaction,
// and the future Charcoal / Fracking) are just table entries -- one widget, one helper,
// no bespoke copies.
//
// The per-building ON/OFF state is the existing CUnit::alt_oil flag, which DOES persist
// across save/load (it rides m_unitFlags; CUnit::Serialize writes+reads it and
// CBuilding::Serialize chains through). This comment used to say the opposite -- see the
// note on the flag itself in unit.h; it was stale, not a spec.
// A def is "available" on a building when DefFor() matches its type AND the owner has
// researched the gating tech; only then does the generic toggle button appear and only
// then does Convert() do anything.
//
// Three production modes cover the family:
//   ePctAdditive   -- output = pct% of the per-call production amount; the input is NOT
//                     consumed (the building's primary output is unchanged). pct comes
//                     from a per-tier player accessor. This is BioFuel (oil = % of food).
//   eRatioConsume  -- consume m_iRatioIn units of the input material from the building's
//                     store and credit 1 unit of output, scaled by the per-call amount.
//                     This is Coal Liquefaction (2 coal -> 1 oil) and Charcoal.
//   eFlatTrickle   -- credit a FLAT per-minute amount (m_pfnFlat, a per-tier player
//                     accessor returning units/min), independent of any primary output and
//                     consuming no input. The caller passes the game-OPERS elapsed this
//                     call as iAmount; Convert() scales the rate by opers/minute. This is
//                     Fracking: an otherwise-stopped (exhausted) oil well trickles oil.
//   eGlobalConsume -- like eRatioConsume but the input is a GLOBAL player resource (food),
//                     not the building's own store: consume m_iRatioIn units of the owner's
//                     global food per 1 output unit and credit the output (oil), scaled by
//                     the per-call amount. This is BioFuel: a refinery STOPS making gas and
//                     instead burns the player's global food into oil. The gas-suppression
//                     itself lives in the production hook (CBuilding::BuildMaterials); this
//                     mode just does the food->oil accounting.
//
// Both credit output the same way BioFuel always has: AddToStore + IncMaterialMade +
// IncMaterialHave, with a runtime fractional accumulator so sub-unit yields aren't lost.

class CBuilding;
class CPlayer;

namespace AltOutput
{
    enum EMode
    {
        ePctAdditive,    // output = m_pfnPct(owner)% of amount; input not consumed
        eRatioConsume,   // consume m_iRatioIn input from store -> 1 output (Coal Liquefaction)
        eFlatTrickle,    // credit m_pfnFlat(owner) units/min, scaled by opers elapsed (Fracking)
        eGlobalConsume,  // consume m_iRatioIn GLOBAL player food from owner -> 1 output (BioFuel)
        eMultiTrickle,   // credit SEVERAL flat units/min (m_aMulti), no input (Desperate/Scrounging)
        eModifier        // a def that produces NOTHING and only carries a per-building toggle plus
                         // modifier fields (m_iWorkforceAdd / m_iPowerMultAdd, and whatever the
                         // feature's own production hook reads). Convert() early-returns for it.
    };

    // What DRIVES a conversion def's production. This governs PRODUCTION ONLY -- skip the fuel
    // burn, run off elapsed time, gate on a whole batch of the def's own input. It says NOTHING
    // about whether the host still feeds the power grid; that is the derived StopsPower() below.
    //   eFuelDriven -- today's shape: the host burns its own fuel and the amount burned this
    //                  batch is what gets fed to Convert().
    //   eTimeDriven -- no fuel is burned; the conversion runs off the production accumulator and
    //                  consumes the def's input material a whole batch at a time.
    // MUST be an enum, never a bool: AltOutputDef is initialised POSITIONALLY throughout s_aDefs
    // (trailing fields simply omitted), so a bool appended next to another bool would silently
    // mis-bind if the tail were ever reordered, while a distinct type fails to compile instead.
    // eFuelDriven is FIRST so a def that omits the trailing initializer stays fuel-driven.
    enum EDrive { eFuelDriven, eTimeDriven };

    // eMultiTrickle: one flat per-minute output line (a material id + units/min).
    struct AltMat { int m_iMat; int m_iPerMin; };
    const int kMaxMulti = 4;

    struct AltOutputDef
    {
        const char* m_szLabel;                   // UI button / info-window label
        const char* m_szDesc;                    // one-line effect blurb for the (i) tooltip (#36)
        bool        (*m_pfnMatches)(CBuilding*); // does this def apply to this building's type?
        bool        (*m_pfnHasTech)(CPlayer*);   // has the owner researched the gating tech?
        int         m_iInputMat;                 // CMaterialTypes input  (consumed only in eRatioConsume)
        int         m_iOutputMat;                // CMaterialTypes output
        EMode       m_eMode;
        int         (*m_pfnPct)(CPlayer*);       // ePctAdditive: output percent of amount (else nullptr)
        int         (*m_pfnFlat)(CPlayer*);      // eFlatTrickle: flat output units/min      (else nullptr)
        int         m_iRatioIn;                  // eRatioConsume: input units per 1 output (else 0)
        float       m_fEnergyMult;               // optional output multiplier (1.0f = none)
        int         m_iWorkforceAdd;             // #2: extra workforce (ABSOLUTE people) drawn while this
                                                 // mode is ON (0 = none). Absolute, NOT a percent of base
                                                 // GetPeople() -- a lumber mill's base people is ~0, so a
                                                 // percent added nothing. Applied in CBuilding::Operate.
        int         m_iPowerMultAdd;             // extra power drawn while ON, as ADDITIONAL copies of the
                                                 // building's base GetPower() (0 = none, 1 = double). Applied
                                                 // in CBuilding::Operate next to m_iWorkforceAdd; base power
                                                 // is already added by the per-type tick, so this is the delta.
        AltMat      m_aMulti[kMaxMulti];         // eMultiTrickle: the output lines (else all-zero)
        int         m_nMulti;                    // eMultiTrickle: number of active m_aMulti lines (else 0)
        int         (*m_pfnRatioIn)(CPlayer*);   // eRatioConsume: optional PER-TIER input ratio override
                                                 // (units of input per 1 output). nullptr = use m_iRatioIn.
                                                 // Coal Liquefaction uses this to drop 3:1 -> 2:1 at tier 2.
        EDrive      m_eDrive;                    // PRODUCTION driver (see EDrive). Every entry in
                                                 // s_aDefs omits it, so every def is eFuelDriven and
                                                 // the shipped fuel-burn path is byte-identical.
    };

    // The matching def for this building's type, or nullptr. Type-only -- does NOT check the
    // tech gate (use Available() for "show the toggle / would convert" decisions).
    const AltOutputDef* DefFor( CBuilding* pBldg );

    // The matching def when one applies AND the owner is valid AND has researched its tech,
    // else nullptr. Gates both the generic toggle button and Convert().
    const AltOutputDef* Available( CBuilding* pBldg );

    // TRUE iff this building's alt-output mode suppresses its power generation. DERIVED from the
    // def + the host type, so it needs no per-def field and cannot collide with one: a store-
    // consuming conversion def (eRatioConsume) hosted on a UTpower plant. That reproduces exactly
    // the bCoalLiq test CPowerBuilding::BuildPower used to spell out inline -- the only UTpower
    // def today is Coal Liquefaction. Governs the power suppression and the 2-power conversion
    // draw ONLY; it says nothing about how production is driven (that is m_eDrive).
    bool StopsPower( CBuilding* pBldg, const AltOutputDef* pDef );

    // The def's effective input units per 1 output unit for this building's owner: the per-tier
    // override (m_pfnRatioIn) when the def supplies one, else the fixed m_iRatioIn. 0 when there
    // is no def, no owner, or the def consumes no input.
    int InputRatio( CBuilding* pBldg, const AltOutputDef* pDef );

    // The shared mode-change operation behind BOTH toggle sites (the info-window checkbox and the
    // QA harness). Flips CUnit::alt_oil and then, for a def that changes what must be DELIVERED
    // (eRatioConsume / eGlobalConsume), tells the human router this building's role changed.
    // Without that, a stocked plant switched into a mode whose input store is 0 returns at
    // BuildPower's first gate before any notification path, so it never asks for its first load
    // and the toggle appears to do nothing. Fires on BOTH ON and OFF (OFF has the mirror problem:
    // the plant wants its fuel again). No-op when the flag already matches. The trickle modes
    // (eFlatTrickle / eMultiTrickle) and eModifier consume no input, so they are deliberately NOT
    // notified -- there is no first delivery to schedule, and four shipped features must not change.
    void SetToggle( CBuilding* pBldg, bool bOn );

    // Shared production helper. Call from a building's production loop with the amount of
    // primary production this call represents (food harvested for a farm, input burned for
    // a power plant, etc.; or the game-OPERS elapsed this call for an eFlatTrickle def
    // like Fracking). If the building's alt-output toggle is ON and a def is
    // Available(), produce the secondary output per the def's mode and credit it. No-op
    // otherwise. fAccum is a per-building runtime fractional accumulator (carries the
    // leftover sub-unit yield between calls); pass the building's own accumulator field.
    void Convert( CBuilding* pBldg, int iAmount, float& fAccum );

    // eMultiTrickle helper (Desperate Measures / Scrounging). Like Convert's eFlatTrickle
    // branch but for SEVERAL outputs at once: each active m_aMulti line credits its own
    // material at its own units/min, each carried by its own accumulator in afAccum[kMaxMulti].
    // No-op unless the toggle is ON and a multi-trickle def is Available(). iAmount = opers
    // elapsed this call. food/gas credit the global pools; other materials the building store.
    void ConvertMulti( CBuilding* pBldg, int iAmount, float* afAccum );

    // Low-level multi-trickle crediting, shared by ConvertMulti (Scrounging AltOutput toggle) and
    // the Desperate Measures rocket EDICT. Each of nMats lines accrues m_iPerMin/min into afAccum[i]
    // and emits whole units (food/gas -> global pools, else the building's store). iAmount = opers
    // elapsed. No toggle/Available gate here -- the caller decides when to credit.
    void CreditTrickle( CBuilding* pBldg, int iAmount, float* afAccum, const AltMat* pMats, int nMats );
}

#endif // ENATIONS_ALTOUTPUT_H
