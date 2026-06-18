#include "Rendering/ParticleSystem.h"
#include "Parsers/EfpParser.h"
#include <cmath>
#include <iostream>

static int g_particleFailures = 0;

#define PARTICLE_TEST(cond) \
    do { \
        if (!(cond)) { \
            std::cerr << "FAIL: " #cond " at " << __LINE__ << "\n"; \
            ++g_particleFailures; \
        } \
    } while (0)

static EfpObjectDef MakeFireLoopObject() {
    EfpObjectDef obj;
    obj.LifeCommand = "NormalTimeLoop";
    obj.Emitter.MinParticles = 8;
    obj.Emitter.MaxParticles = 16;
    obj.Emitter.BurstRate = 1;
    obj.Emitter.SpawnRate = 4.0f;
    obj.Emitter.Valid = true;
    obj.Render.ViewMode = "ViewYBillboard";
    obj.Render.TexturePaths.push_back("fire001.ddj");
    obj.Render.TextureSlide.Left = Vector3(4.0f, 3.0f, 1.0f);
    obj.Render.TextureSlide.Valid = true;
    return obj;
}

static void TestParticleLoopStaysNearAnchor() {
    ParticleSystem system;
    const EfpObjectDef def = MakeFireLoopObject();
    const Vector3 anchor(100.0f, 50.0f, 200.0f);
    const Vector3 camera(anchor.x, anchor.y + 5.0f, anchor.z - 40.0f);
    const ParticleSystem::InstanceKey key = ParticleSystem::MakeKey("map/guild/red_orange_flame_01.efp", anchor, anchor);

    constexpr float dt = 0.05f;
    constexpr int totalSteps = 600;
    for (int step = 0; step < totalSteps; ++step) {
        system.UpdateInstance(key, def, anchor, 0.0f, dt, true, false, false, camera, nullptr);
    }

    const ParticleSystem::SimDebugInfo debug = system.GetSimDebug(key);
    PARTICLE_TEST(debug.ActiveCount > 0);
    PARTICLE_TEST(debug.MaxHeightAboveAnchor <= ParticleSystem::kMaxFireHeight + 1.0f);
    PARTICLE_TEST(debug.MaxHorizontalFromAnchor <= ParticleSystem::kMaxFireHorizontal + 1.0f);
}

static void TestParticleRespawnResetsHeight() {
    ParticleSystem system;
    EfpObjectDef def = MakeFireLoopObject();
    def.Emitter.MinParticles = 1;
    def.Emitter.MaxParticles = 1;
    def.Emitter.SpawnRate = 0.1f;

    const Vector3 anchor(0.0f, 10.0f, 0.0f);
    const Vector3 camera(0.0f, 12.0f, -30.0f);
    const ParticleSystem::InstanceKey key = ParticleSystem::MakeKey("test_fire.efp", anchor, anchor);

    system.UpdateInstance(key, def, anchor, 0.0f, 0.1f, true, false, false, camera, nullptr);
    for (int i = 0; i < 40; ++i) {
        system.UpdateInstance(key, def, anchor, 0.0f, 0.1f, true, false, false, camera, nullptr);
    }

    const ParticleSystem::SimDebugInfo afterLoops = system.GetSimDebug(key);
    PARTICLE_TEST(afterLoops.ActiveCount > 0);
    PARTICLE_TEST(afterLoops.MaxHeightAboveAnchor < ParticleSystem::kMaxFireHeight);
}

static void TestParticleLifetimeFade() {
    PARTICLE_TEST(ParticleSystem::ParticleLifetimeFade(0.0f) < 0.01f);
    PARTICLE_TEST(ParticleSystem::ParticleLifetimeFade(1.0f) < 0.01f);
    PARTICLE_TEST(ParticleSystem::ParticleLifetimeFade(0.5f) > 0.5f);
}

static void TestShouldUseMeshForItem() {
    EfpRenderItem meshItem;
    meshItem.Shape = "RenderMesh";
    meshItem.MeshPath = "effect/box.bms";
    meshItem.ViewMode = "ViewVBillboard";
    PARTICLE_TEST(EfpItemUsesMeshGeometry(meshItem));

    EfpRenderItem billboardItem;
    billboardItem.Shape = "RenderPlate";
    billboardItem.MeshPath = "effect/box.bms";
    billboardItem.ViewMode = "ViewVBillboard";
    PARTICLE_TEST(!EfpItemUsesMeshGeometry(billboardItem));

    EfpRenderItem viewNoneItem;
    viewNoneItem.ViewMode = "ViewNone";
    viewNoneItem.MeshPath = "effect/slab.bms";
    PARTICLE_TEST(EfpItemUsesMeshGeometry(viewNoneItem));
}

int RunParticleSystemTests() {
    g_particleFailures = 0;
    TestParticleLoopStaysNearAnchor();
    TestParticleRespawnResetsHeight();
    TestParticleLifetimeFade();
    TestShouldUseMeshForItem();
    if (g_particleFailures == 0) {
        std::cout << "ParticleSystemTests: all passed\n";
    } else {
        std::cerr << "ParticleSystemTests: " << g_particleFailures << " failure(s)\n";
    }
    return g_particleFailures;
}
