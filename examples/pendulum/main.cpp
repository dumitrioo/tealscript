#include <tealscript_runtime.hpp>

#include "bullet/btBulletDynamicsCommon.h"
#include "bullet/btBulletCollisionCommon.h"

#include <raylib.h>
#include <rlgl.h>

template<typename T, typename D = std::default_delete<T>>
using uniqp_t = std::unique_ptr<T, D>;

int main(int argc, char **argv) {
    if(argc < 2) {
        return 0;
    }

    teal::runtime teal_rt{};
    teal_rt.load_file(argv[1]);
    teal_rt.loading_complete();
    teal_rt.run_cycle();

    // 1. INIT BULLET PHYSICS
    uniqp_t<btDefaultCollisionConfiguration> collisionConfiguration{std::make_unique<btDefaultCollisionConfiguration>()};
    uniqp_t<btCollisionDispatcher> dispatcher{std::make_unique<btCollisionDispatcher>(collisionConfiguration.get())};
    uniqp_t<btBroadphaseInterface> overlappingPairCache{std::make_unique<btDbvtBroadphase>()};
    uniqp_t<btSequentialImpulseConstraintSolver> solver{std::make_unique<btSequentialImpulseConstraintSolver>()};
    uniqp_t<btDiscreteDynamicsWorld> dynamicsWorld{std::make_unique<btDiscreteDynamicsWorld>(
        dispatcher.get(), overlappingPairCache.get(), solver.get(), collisionConfiguration.get())
    };

    dynamicsWorld->setGravity(btVector3(0, -9.81, 0));

    // 2. Ground
    uniqp_t<btCollisionShape> groundShape{std::make_unique<btStaticPlaneShape>(btVector3(0, 1, 0), 0)};
    uniqp_t<btDefaultMotionState> groundMotionState{std::make_unique<btDefaultMotionState>(
        btTransform(btQuaternion(0, 0, 0, 1), btVector3(0, 0, 0)))
    };
    uniqp_t<btCollisionObject> groundCollisionObj{std::make_unique<btCollisionObject>()};
    groundCollisionObj->setCollisionShape(groundShape.get());
    groundCollisionObj->setFriction(teal_rt.get_output("friction_force").cast_to_double());
    btRigidBody::btRigidBodyConstructionInfo groundRigidBodyCI(
        0, groundMotionState.get(), groundShape.get(), btVector3(0,0,0)
    );
    uniqp_t<btRigidBody> groundBody{std::make_unique<btRigidBody>(groundRigidBodyCI)};
    dynamicsWorld->addRigidBody(groundBody.get());

    // 3. Cart
    uniqp_t<btCollisionShape> cartShape{std::make_unique<btBoxShape>(btVector3(0.5, 0.15, 0.3))};
    uniqp_t<btCollisionObject> cartCollisionObj{std::make_unique<btCollisionObject>()};
    cartCollisionObj->setCollisionShape(cartShape.get());
    cartCollisionObj->setFriction(teal_rt.get_output("friction_force").cast_to_double());
    btScalar cartMass = teal_rt.get_output("cart_mass").cast_to_double();
    btVector3 cartInertia(0,0,0);
    cartShape->calculateLocalInertia(cartMass, cartInertia);
    uniqp_t<btDefaultMotionState> cartMotionState{std::make_unique<btDefaultMotionState>(
        btTransform(btQuaternion(0,0,0,1), btVector3(0, 0.15, 0)))
    };
    btRigidBody::btRigidBodyConstructionInfo cartRigidBodyCI(
        cartMass, cartMotionState.get(), cartShape.get(), cartInertia
    );
    uniqp_t<btRigidBody> cartBody{std::make_unique<btRigidBody>(cartRigidBodyCI)};
    dynamicsWorld->addRigidBody(cartBody.get());

    // 4. Pendulum
    uniqp_t<btCollisionShape> poleShape{std::make_unique<btBoxShape>(btVector3(0.005, 1.0, 0.005))};
    btScalar poleMass = teal_rt.get_output("pend_mass").cast_to_double();
    btVector3 poleInertia(0,0,0);
    poleShape->calculateLocalInertia(poleMass, poleInertia);
    uniqp_t<btDefaultMotionState> poleMotionState{std::make_unique<btDefaultMotionState>(
        btTransform(btQuaternion(0,0,0,1), btVector3(0, 1.3, 0)))
    };
    btRigidBody::btRigidBodyConstructionInfo poleRigidBodyCI(
        poleMass, poleMotionState.get(), poleShape.get(), poleInertia
    );
    uniqp_t<btRigidBody> poleBody{std::make_unique<btRigidBody>(poleRigidBodyCI)};
    dynamicsWorld->addRigidBody(poleBody.get());

    // 5. Connection
    uniqp_t<btHingeConstraint> poleHinge{std::make_unique<btHingeConstraint>(
        *poleBody,
        *cartBody,
        btVector3(0, -1.0, 0),
        btVector3(0, 0.15, 0),
        btVector3(0, 0, 1),
        btVector3(0, 0, 1),
        true
    )};
    dynamicsWorld->addConstraint(poleHinge.get(), true);

    // Cart X-axis Slider
    uniqp_t<btSliderConstraint> cartSlider{std::make_unique<btSliderConstraint>(
        *cartBody, btTransform::getIdentity(), true
    )};
    cartSlider->setLowerLinLimit(-1000000000.0);
    cartSlider->setUpperLinLimit(1000000000.0);
    cartSlider->setLowerAngLimit(0.0);
    cartSlider->setUpperAngLimit(0.0);
    dynamicsWorld->addConstraint(cartSlider.get(), true);
    double start_force{teal_rt.get_output("start_force_impulse").cast_to_double()};
    if(start_force != 0) {
        poleBody->applyCentralImpulse(btVector3(start_force, 0, 0));
    }

    // 6. Simulation (Sense -> Compute -> Act)
    double sim_cycle_time = 1.0 / 100.0;
    double prev_ang{0};
    long double prev_time{teal::steady_time_sec()};
    double angle = 0;
    double ang_vel = 0;
    double force = 0;
    double cart_x = 0;
    double cart_vel = 0;
    bool term{false};
    bool volatile should_close{false};

    // Visualization
    std::thread rlt{
        [&]() {
            SetConfigFlags(FLAG_MSAA_4X_HINT);
            InitWindow(1280, 720, "TealScript Physics Demo");
            SetTargetFPS(60);
            Camera3D camera = {
                { 8.0f, 6.0f, 8.0f },
                { 0.0f, 0.5f, 0.0f },
                { 0.0f, 1.0f, 0.0f },
                45.0f,
                0
            };

            while(!term && !WindowShouldClose()) {
                BeginDrawing();
                ClearBackground(RAYWHITE);
                BeginMode3D(camera);
                DrawGrid(20, 1.0f);
                // Get Matrices from Bullet
                btTransform tCart; cartBody->getMotionState()->getWorldTransform(tCart);
                btTransform tPole; poleBody->getMotionState()->getWorldTransform(tPole);
                // Draw Cart
                Vector3 cPos = {
                    (float)tCart.getOrigin().x(),
                    (float)tCart.getOrigin().y(),
                    (float)tCart.getOrigin().z()
                };
                DrawCubeV(cPos, (Vector3){1.0f, 0.3f, 0.6f}, BLUE);
                // Draw Pendulum
                {
                    btTransform tPole;
                    poleBody->getMotionState()->getWorldTransform(tPole);
                    btVector3 pPos = tPole.getOrigin();
                    btMatrix3x3 pBasis = tPole.getBasis();
                    // 1. Assembling matrix 4x4 for Raylib (column-major)
                    Matrix matTransform = {
                        (float)pBasis[0].x(), (float)pBasis[1].x(), (float)pBasis[2].x(), 0.0f,
                        (float)pBasis[0].y(), (float)pBasis[1].y(), (float)pBasis[2].y(), 0.0f,
                        (float)pBasis[0].z(), (float)pBasis[1].z(), (float)pBasis[2].z(), 0.0f,
                        (float)pPos.x(),      (float)pPos.y(),      (float)pPos.z(),      1.0f
                    };
                    // 2. Apply matrix to the Raylib rendering pipeline
                    rlPushMatrix();
                    rlMultMatrixf(&matTransform.m0);
                    DrawCubeV(Vector3{ 0.0f, 0.0f, 0.0f }, Vector3{0.1f, 2.0f, 0.1f}, GRAY);
                    rlPopMatrix();
                }
                // Force vector (Arrow)
                Vector3 forceStart = { cPos.x, cPos.y, cPos.z + 0.5f };
                Vector3 forceEnd = { cPos.x + (float)force, cPos.y, cPos.z + 0.5f };
                DrawCylinderEx(forceStart, forceEnd, 0.02, 0.02, 10, MAROON);
                Vector3 arrowEnd{forceEnd};
                arrowEnd.x += teal::math::sign(force) * 0.2;
                DrawCylinderEx(forceEnd, arrowEnd, 0.05, 0.0, 10, MAROON);
                // Soft walls
                DrawCube((Vector3){4.5f, 0.5f, 0.0f}, 0.1f, 1.0f, 1.0f, Fade(VIOLET, 0.3f));
                DrawCube((Vector3){-4.5f, 0.5f, 0.0f}, 0.1f, 1.0f, 1.0f, Fade(RED, 0.3f));
                EndMode3D();
                // 2D overlay
                int y_draw_pos{10};
                DrawText(TextFormat("Angle: %0.3f rad", angle), 10, y_draw_pos, 20, DARKGRAY);
                y_draw_pos += 25;
                DrawText(TextFormat("Angular speed: %0.3f rad/sec", ang_vel), 10, y_draw_pos, 20, DARKGRAY);
                y_draw_pos += 25;
                DrawText(TextFormat("Force: %0.3f N", force), 10, y_draw_pos, 20, DARKGRAY);
                y_draw_pos += 25;
                DrawText(TextFormat("Cart Mass: %0.3f kg", cartMass), 10, y_draw_pos, 20, DARKGRAY);
                y_draw_pos += 25;
                DrawText(TextFormat("Pendulum Mass: %0.3f kg", poleMass), 10, y_draw_pos, 20, DARKGRAY);
                y_draw_pos += 25;
                DrawText(TextFormat("Cart Offset: %0.3f m", cart_x), 10, y_draw_pos, 20, DARKGRAY);
                y_draw_pos += 25;
                DrawText(TextFormat("Cart Velocity: %0.3f m/s", cart_vel), 10, y_draw_pos, 20, DARKGRAY);
                y_draw_pos += 25;
                DrawText(TextFormat("Cart Friction Coeff: %0.3f",
                    teal_rt.get_output("friction_force").cast_to_double()), 10,
                    y_draw_pos, 20, DARKGRAY
                );
                y_draw_pos += 25;
                EndDrawing();
            }
            CloseWindow();
            should_close = true;
        }
    };

    for(int64_t i = 0; !should_close; ++i) {
        std::this_thread::sleep_for(std::chrono::duration<double, std::ratio<1, 1>>{sim_cycle_time});
        long double curr_time{teal::steady_time_sec()};
        double dt = curr_time - prev_time;
        prev_time = curr_time;
        dynamicsWorld->stepSimulation(dt, 10, dt);
        dynamicsWorld->clearForces();

        angle = poleHinge->getHingeAngle();
        ang_vel = (angle - prev_ang) / dt;
        prev_ang = angle;

        btVector3 cart_pos = cartBody->getCenterOfMassPosition();
        cart_x = cart_pos.getX();

        btVector3 cart_vel_vec = cartBody->getLinearVelocity();
        cart_vel = cart_vel_vec.getX();

        // --- COMPUTE (TealScript) ---
        teal_rt.set_input("dt", dt);
        teal_rt.set_input("ang", angle);
        teal_rt.set_input("cart_pos", cart_x);
        teal_rt.set_input("cart_vel", cart_vel);
        teal_rt.run_cycle();
        force = teal_rt.get_output("motor_force").cast_to_double();

        // --- ACT ---
        cartBody->activate();
        poleBody->activate();
        // Apply force X
        cartBody->applyCentralForce(btVector3(force, 0, 0));
    }

    term = true;
    if(rlt.joinable()) { rlt.join(); }

    return 0;
}
