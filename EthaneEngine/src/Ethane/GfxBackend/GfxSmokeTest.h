//
//  GfxSmokeTest.h
//  EthaneEngine
//
//  Debug-only smoke test for the graphics backend: creates one of each GPU resource type
//  through GfxBackendAPI, uploads data, then releases everything. Run it with the validation
//  layer on to catch backend bugs before a real renderer exists.
//
//  Enabled by the environment variable ETH_GFX_SMOKE_TEST (Debug builds only):
//    ETH_GFX_SMOKE_TEST=1     run the test at startup, then continue normally
//    ETH_GFX_SMOKE_TEST=exit  run the test, then close the app (full startup -> shutdown check)
//
#pragma once

namespace Ethane {

class GfxBackendAPI;

namespace GfxSmokeTest {

    // Returns true when the test is enabled for this run
    bool IsEnabled();
    // Returns true when the app should close right after the test (ETH_GFX_SMOKE_TEST=exit)
    bool ExitAfterRun();

    // Creates/uploads/releases a vertex buffer, index buffer, shader, render pass and pipeline.
    // All resources are released before this returns. Logs "[GfxSmokeTest] PASSED" on success.
    void Run(GfxBackendAPI& backend);

}
}
