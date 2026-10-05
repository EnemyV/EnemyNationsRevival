int main()
{
    CProjBase explosionOnlyA(CProjBase::explosion);
    CProjBase explosionHead(CProjBase::explosion);
    CProjBase projectileA(CProjBase::projectile);
    CProjBase projectileB(CProjBase::projectile);
    CProjBase projectileOnly(CProjBase::projectile);
    CProjBase explosionOnlyB(CProjBase::explosion);
    explosionHead.Link(&projectileA);
    projectileA.Link(&projectileB);

    CProjMap map;
    map.SetAt(10, &explosionOnlyA);
    map.SetAt(11, &explosionHead);
    map.SetAt(12, &projectileOnly);
    map.SetAt(13, &explosionOnlyB);

    // Preserve occupied-hex semantics: multiple projectiles in one bucket count
    // once, and explosion-only buckets do not consume a firing slot.
    assert(map.GetCount() == 4);
    assert(map.GetProjectileHexCount() == 2);

    // Exercise the production firing threshold: a projectile at each distinct
    // hex consumes a slot, while an explosion-only hex does not.
    CProjBase projectiles[24] = {
        CProjBase(CProjBase::projectile), CProjBase(CProjBase::projectile),
        CProjBase(CProjBase::projectile), CProjBase(CProjBase::projectile),
        CProjBase(CProjBase::projectile), CProjBase(CProjBase::projectile),
        CProjBase(CProjBase::projectile), CProjBase(CProjBase::projectile),
        CProjBase(CProjBase::projectile), CProjBase(CProjBase::projectile),
        CProjBase(CProjBase::projectile), CProjBase(CProjBase::projectile),
        CProjBase(CProjBase::projectile), CProjBase(CProjBase::projectile),
        CProjBase(CProjBase::projectile), CProjBase(CProjBase::projectile),
        CProjBase(CProjBase::projectile), CProjBase(CProjBase::projectile),
        CProjBase(CProjBase::projectile), CProjBase(CProjBase::projectile),
        CProjBase(CProjBase::projectile), CProjBase(CProjBase::projectile),
        CProjBase(CProjBase::projectile), CProjBase(CProjBase::projectile)
    };
    CProjMap threshold;
    for (DWORD i = 0; i < 23; ++i) threshold.SetAt(i, &projectiles[i]);
    threshold.SetAt(100, &explosionOnlyA);
    assert(threshold.GetProjectileHexCount() == 23);
    assert(threshold.GetProjectileHexCount() < 24);
    threshold.SetAt(23, &projectiles[23]);
    assert(threshold.GetProjectileHexCount() == 24);
    assert(!(threshold.GetProjectileHexCount() < 24));
    return 0;
}
