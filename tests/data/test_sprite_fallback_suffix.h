int main()
{
    CSprite invalidDummy(-1);
    CSprite zeroViews(0);
    CSprite validA(1);
    CSprite validB(3);
    Ptr<CSprite> sprites[5];
    sprites[0].Set(&invalidDummy);
    sprites[1].Set(&zeroViews);
    sprites[2].Set(&validA);
    sprites[3].Set(&validB);
    sprites[4].Set(NULL);
    CSpriteCollectionInfo info(0);
    CSpriteCollection collection(sprites, 5, &info);

    // The extracted production inline accessor safely handles both a null
    // sprite header and a present header that declares zero views.
    assert(invalidDummy.GetNumViews() == 0);
    assert(zeroViews.GetNumViews() == 0);
    assert(validA.GetNumViews() == 1);
    assert(validB.GetNumViews() == 3);

    // Nonstrict lookup preserves a chosen drawable sprite.
    info.SetIndex(3);
    assert(collection.GetSprite(42, 7, FALSE) == &validB);
    assert(info.LastID() == 42 && info.LastOffset() == 7 && info.LastStrict() == FALSE);
    assert(collection.GetSprite(42, 7, TRUE) == &validB);
    assert(info.LastID() == 42 && info.LastOffset() == 7 && info.LastStrict() == TRUE);

    // A registered len=-1 dummy is null in strict mode and falls back to the
    // first loaded drawable sprite in ordinary mode. A zero-view header is
    // treated the same way.
    info.SetIndex(0);
    assert(collection.GetSprite(42, 0, TRUE) == NULL);
    assert(collection.GetSprite(42, 0, FALSE) == &validA);
    info.SetIndex(1);
    assert(collection.GetSprite(42, 0, TRUE) == NULL);
    assert(collection.GetSprite(42, 0, FALSE) == &validA);

    // A null collection slot also triggers the same nonstrict fallback.
    info.SetIndex(4);
    assert(collection.GetSprite(42, 0, FALSE) == &validA);

    // Preserve the collection-info miss behavior: no index means no fallback.
    info.SetIndex(-1);
    assert(collection.GetSprite(42, 0, FALSE) == NULL);

    Ptr<CSprite> noDrawable[3];
    noDrawable[0].Set(&invalidDummy);
    noDrawable[1].Set(&zeroViews);
    noDrawable[2].Set(NULL);
    info.SetIndex(0);
    CSpriteCollection emptyCollection(noDrawable, 3, &info);
    assert(emptyCollection.GetSprite(42, 0, FALSE) == NULL);
    return 0;
}
