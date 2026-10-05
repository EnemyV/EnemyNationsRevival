static void ResetTrace()
{
    g_frame = 0;
    g_vehicleReleaseFrame = -1;
    g_visibleChangeFrame = -1;
    g_buildingVisibleFrame = -1;
    g_enumFrame = -1;
    g_removeFrame = -1;
    g_deleteFrame = -1;
    g_frameAdvanceCalls = 0;
    g_lastFrameAdvanceCall = -1;
    g_sideEffectBeforeKill = false;
}

static void RunCase(int animationFrames, bool isBuilding)
{
    ResetTrace();
    int deleted = 0;
    CUnit* target = isBuilding ? static_cast<CUnit*>(new CBuilding())
                               : static_cast<CUnit*>(new CVehicle());
    if (isBuilding) target->SetVisible(0);
    g_target = target;
    g_shooter = NULL;
    CExplosion* explosion = new CExplosion(animationFrames, target, &deleted);
    const int expectedDelete = animationFrames > enexpl::EXPL_KILLFRAME
                                   ? animationFrames : enexpl::EXPL_KILLFRAME;

    for (g_frame = 1; g_frame <= expectedDelete; ++g_frame)
    {
        explosion->Operate();
        if (g_frame < enexpl::EXPL_KILLFRAME) assert(deleted == 0);
        if (deleted) break;
    }

    assert(deleted == 1);
    assert(g_deleteFrame == expectedDelete);
    assert(!g_sideEffectBeforeKill);
    if (animationFrames > 0)
    {
        // Operate must advance the effect's own animation even when no Draw call
        // occurs; using only the global simulation frame would mask a stuck sprite.
        assert(g_frameAdvanceCalls == expectedDelete);
        assert(g_lastFrameAdvanceCall == expectedDelete);
    }
    else
    {
        // Missing art is considered finished, so it must not query/advance a view.
        assert(g_frameAdvanceCalls == 0);
        assert(g_lastFrameAdvanceCall == -1);
    }
    if (isBuilding)
    {
        assert(g_buildingVisibleFrame == enexpl::EXPL_KILLFRAME);
        assert(g_enumFrame == enexpl::EXPL_KILLFRAME);
        assert(g_visibleChangeFrame == enexpl::EXPL_KILLFRAME);
    }
    else
    {
        assert(g_vehicleReleaseFrame == enexpl::EXPL_KILLFRAME);
        assert(g_visibleChangeFrame == enexpl::EXPL_KILLFRAME);
        assert(g_enumFrame == -1);
    }

    printf("[data_expl_operate] art=%d target=%s release=%d delete=%d\n",
           animationFrames, isBuilding ? "building" : "vehicle",
           enexpl::EXPL_KILLFRAME, g_deleteFrame);
    delete target;
    g_target = NULL;
}

int main()
{
    // Four-frame and already-finished art must not delete before frame 5;
    // long art must release at frame 5 while staying alive until its one-shot ends.
    const int lengths[] = { 4, 40, 400, 0, 1 };
    for (size_t i = 0; i < sizeof(lengths) / sizeof(lengths[0]); ++i)
    {
        RunCase(lengths[i], false);
        RunCase(lengths[i], true);
    }
    puts("[data_expl_operate] production CExplosion::Operate lifecycle passed");
    return 0;
}
