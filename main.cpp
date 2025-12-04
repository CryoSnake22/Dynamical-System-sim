#include "raylib.h"
#include <cmath>
#include <deque>
#include <vector>

const int MAX_TRAIL_LENGTH = 5000;
const float DT = 0.005f;
const int AGENT_COUNT = 50;
const float SPREAD = 0.1f;

struct State {
  float x, y, z;
};

State operator+(const State &a, const State &b) {
  return {a.x + b.x, a.y + b.y, a.z + b.z};
}
State operator*(const State &a, float s) { return {a.x * s, a.y * s, a.z * s}; }
//   float dy = -g * y + d * x * y;
// Volta prey predator model
// State dynamics(const State &s) {
//   float x = s.x, y = s.y, z = s.z;
//   float a = 1.0f;
//   float b = 1.0f;
//   float g = 1.0f;
//   float d = 1.0f;
//
//   float dx = a * x - x * y;
//   float dz = 0.0f;
//   return {dx, dy, dz};
// }

// State dynamics(const State &s) {
//   float x = s.x, y = s.y, z = s.z; // Alias for cleaner math
//
//   float dx = (z - 0.7f) * x - 3.5f * y;
//   float dy = 3.5f * x + (z - 0.7f) * y;
//   float dz = 0.6f + 0.95f * z - (z * z * z / 3.0f) -
//              (x * x + y * y) * (1.0f + 0.25f * z) + 0.1f * z * x * x * x;
//
//   return {dx, dy, dz};
// }

State dynamics(const State &s) {
  const float sigma = 10.0f;
  const float rho = 99.96f;
  const float beta = 8.0f / 3.0f;
  return {sigma * (s.y - s.x), s.x * (rho - s.z) - s.y, s.x * s.y - beta * s.z};
}

Vector3 to_screen(State s) { return (Vector3){s.x, s.z, s.y}; }

// Scaling small stuff.
// Vector3 to_screen(State s) {
//   float scale = 30.0f;
//   return {s.x * scale, s.z * scale, s.y * scale};
// }
State rk4_step(const State &current, float dt) {
  State k1 = dynamics(current);
  State k2 = dynamics(current + k1 * (0.5f * dt));
  State k3 = dynamics(current + k2 * (0.5f * dt));
  State k4 = dynamics(current + k3 * dt);
  return current + (k1 + k2 * 2.0f + k3 * 2.0f + k4) * (dt / 6.0f);
}

float cameraTheta = 0.0f;
float cameraPhi = 1.0f;
float cameraRadius = 300.0f;

Vector3 cameraTarget = {0.0f, 90.0f, 0.0f};

void HandleMouse(Camera &camera) {
  float mouseSensitivity = 0.005f;
  float zoomSensitivity = 5.0f;
  float panSensitivity = 0.5f;

  if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
    Vector2 delta = GetMouseDelta();
    cameraTheta += delta.x * mouseSensitivity;
    cameraPhi -= delta.y * mouseSensitivity;

    if (cameraPhi < 0.1f)
      cameraPhi = 0.1f;
    if (cameraPhi > 3.0f)
      cameraPhi = 3.0f;
  } else if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
    Vector2 delta = GetMouseDelta();
    cameraTarget.y += delta.y * panSensitivity;
  } else {
    cameraTheta += 0.001f;
  }

  cameraRadius -= GetMouseWheelMove() * zoomSensitivity;
  if (cameraRadius < 10.0f)
    cameraRadius = 10.0f;

  camera.position.x =
      cameraTarget.x + cameraRadius * sinf(cameraPhi) * cosf(cameraTheta);
  camera.position.y = cameraTarget.y + cameraRadius * cosf(cameraPhi);
  camera.position.z =
      cameraTarget.z + cameraRadius * sinf(cameraPhi) * sinf(cameraTheta);

  camera.target = cameraTarget;
}

struct Agent {
  State state;
  std::deque<Vector3> trail;
  Color color;
};

int main() {
  InitWindow(1200, 800, "Dynamical Systems");
  SetTargetFPS(60);

  Camera3D camera = {0};
  camera.up = (Vector3){0.0f, 1.0f, 0.0f};
  camera.fovy = 45.0f;
  camera.projection = CAMERA_PERSPECTIVE;

  // start_state matters since if you start outside the attractor bassin your
  // state might never converge.
  State start_state = {0.0f, 0.0f, 99.0f};

  std::vector<Agent> agents;
  for (int i = 0; i < AGENT_COUNT; i++) {
    float r1 = ((float)GetRandomValue(-100, 100) / 100.0f) * SPREAD;
    float r2 = ((float)GetRandomValue(-100, 100) / 100.0f) * SPREAD;
    float r3 = ((float)GetRandomValue(-100, 100) / 100.0f) * SPREAD;
    State s = {start_state.x + r1, start_state.y + r2, start_state.z + r3};

    // Cool colors
    // Color c = ColorFromHSV((float)i / AGENT_COUNT * 360.0f, 0.8f, 1.0f);
    // Color c = {100, 255, 255, 100}; // Light Blue
    // Color c = {252, 88, 134, 100}; // Lighter red
    // Color c = {252, 73, 100, 100}; // red
    // Color c = RAYWHITE;
    Color c = {177, 252, 73, 100};
    agents.push_back({s, {}, c});
  }

  while (!WindowShouldClose()) {
    // For each trail you essentially just do RK4 simulation 10 times per
    // timestep. then draw all the newfound states from trails
    for (int i = 0; i < 10; i++) {
      for (Agent &agent : agents) {
        agent.state = rk4_step(agent.state, DT);
        agent.trail.push_back(to_screen(agent.state));
        if (agent.trail.size() > MAX_TRAIL_LENGTH)
          agent.trail.pop_front();
      }
    }

    HandleMouse(camera);

    BeginDrawing();
    ClearBackground((Color){20, 20, 20, 255});

    BeginMode3D(camera);
    DrawGrid(20, 10.0f); // Floor at Y=0

    DrawLine3D({0, 0, 0}, {0, 200, 0}, DARKGRAY);

    BeginBlendMode(BLEND_ADDITIVE); // take low opacity trails and add them up
                                    // and you get a glow effect
    for (Agent &agent : agents) {
      if (agent.trail.size() > 1) {
        for (size_t i = 0; i < agent.trail.size() - 1; i++) {
          float t = (float)i / agent.trail.size();
          DrawLine3D(agent.trail[i], agent.trail[i + 1], agent.color);
        }
      }
      // DrawSphere(to_screen(agent.state), 0.1f, RAYWHITE); // if you want to
      // track trajectories
    }

    EndBlendMode();
    EndMode3D();
    DrawText("Raw Coordinates", 10, 10, 20, RAYWHITE);
    DrawText(TextFormat("Target Y: %.1f (Right Click to Pan)", cameraTarget.y),
             10, 35, 20, GRAY);
    DrawFPS(10, 770);
    EndDrawing();
  }

  CloseWindow();
  return 0;
}
