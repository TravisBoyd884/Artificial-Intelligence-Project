#include "Net/net.hpp"

#include <cmath>

namespace {
    constexpr float kPi         = 3.14159265358979f;
    constexpr float kGravity    = -9.8f;
    constexpr float kDamping    = 0.015f;   // velocity damping per step
    constexpr float kCordRadius = 0.004f;   // half-thickness for ball collision
    constexpr float kNetHeight  = 0.43f;    // total hang length (metres)
    // Bottom of the net tapers to this fraction of the rim radius.
    constexpr float kBottomFrac = 0.37f;    // ~85 mm at bottom vs 230 mm at top
    constexpr int   kSolverIters = 10;
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

float Net::radiusAt(int row) const {
    float t = static_cast<float>(row) / static_cast<float>(N_ROWS);
    return m_radius * (1.0f - (1.0f - kBottomFrac) * t);
}

float Net::angleAt(int col, int row) const {
    float base   = (2.0f * kPi / N_COLS) * static_cast<float>(col);
    float offset = (row % 2 != 0) ? (kPi / N_COLS) : 0.0f;
    return base + offset;
}

// ---------------------------------------------------------------------------
// Construction / reset
// ---------------------------------------------------------------------------

Net::Net(glm::vec3 rimCenter, float rimRadius)
    : m_center(rimCenter), m_radius(rimRadius) {
    initPositions();
    buildConstraints();
    buildSegments();
}

void Net::reset() {
    initPositions();
    buildSegments();
}

void Net::initPositions() {
    const float vStep = kNetHeight / static_cast<float>(N_ROWS);

    for (int row = 0; row < N_TOTAL_ROWS; ++row) {
        float r = radiusAt(row);
        float y = m_center.y - row * vStep;

        for (int col = 0; col < N_COLS; ++col) {
            float   theta = angleAt(col, row);
            Node&   n     = m_nodes[at(col, row)];
            n.pos   = glm::vec3(m_center.x + r * std::cos(theta),
                                y,
                                m_center.z + r * std::sin(theta));
            n.prev  = n.pos;
            n.fixed = (row == 0);
        }
    }
}

// ---------------------------------------------------------------------------
// Constraints
// ---------------------------------------------------------------------------

void Net::buildConstraints() {
    m_constraints.clear();

    for (int row = 0; row < N_TOTAL_ROWS; ++row) {
        for (int col = 0; col < N_COLS; ++col) {

            // ---- Horizontal ring at this row ----------------------------------
            // Skip row 0 (anchor) — the rim mesh is already visible there.
            if (row > 0) {
                int nc = (col + 1) % N_COLS;
                m_constraints.push_back({
                    at(col, row), at(nc, row),
                    glm::length(m_nodes[at(col,row)].pos - m_nodes[at(nc,row)].pos)
                });
            }

            // ---- Diagonal strands down to the next row ------------------------
            // Even rows (angle = base):
            //   node k connects to offset-row nodes k  (right of it) and k-1 (left).
            // Odd rows (angle = base + half-step):
            //   node k connects to even-row nodes k (left of it) and k+1 (right).
            if (row < N_ROWS) {
                int colA = col;
                int colB = (row % 2 == 0)
                             ? (col - 1 + N_COLS) % N_COLS   // left neighbour in odd row
                             : (col + 1) % N_COLS;            // right neighbour in even row

                m_constraints.push_back({
                    at(col, row), at(colA, row + 1),
                    glm::length(m_nodes[at(col,row)].pos - m_nodes[at(colA,row+1)].pos)
                });
                m_constraints.push_back({
                    at(col, row), at(colB, row + 1),
                    glm::length(m_nodes[at(col,row)].pos - m_nodes[at(colB,row+1)].pos)
                });
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Simulation
// ---------------------------------------------------------------------------

void Net::update(float dt, glm::vec3 ballPos, float ballRadius) {
    stepVerlet(dt);
    for (int i = 0; i < kSolverIters; ++i)
        solveConstraints();
    handleBallCollision(ballPos, ballRadius);
    buildSegments();
}

void Net::stepVerlet(float dt) {
    const glm::vec3 gravity(0.0f, kGravity, 0.0f);
    for (auto& n : m_nodes) {
        if (n.fixed) continue;
        glm::vec3 vel = (n.pos - n.prev) * (1.0f - kDamping);
        n.prev = n.pos;
        n.pos += vel + gravity * (dt * dt);
    }
}

void Net::solveConstraints() {
    for (auto& c : m_constraints) {
        Node& a  = m_nodes[c.i];
        Node& b  = m_nodes[c.j];
        glm::vec3 d    = b.pos - a.pos;
        float     dist = glm::length(d);
        if (dist < 1e-6f) continue;
        float     corr = (dist - c.rest) / dist;
        glm::vec3 half = 0.5f * corr * d;
        if (!a.fixed) a.pos += half;
        if (!b.fixed) b.pos -= half;
    }
}

void Net::handleBallCollision(glm::vec3 ballPos, float ballRadius) {
    const float threshold = ballRadius + kCordRadius;
    for (auto& n : m_nodes) {
        if (n.fixed) continue;
        glm::vec3 diff = n.pos - ballPos;
        float     dist = glm::length(diff);
        if (dist < threshold && dist > 1e-6f)
            n.pos = ballPos + (diff / dist) * threshold;
    }
}

// ---------------------------------------------------------------------------
// Rendering data
// ---------------------------------------------------------------------------

void Net::buildSegments() {
    // Each constraint is exactly one rendered line segment.
    m_segs.clear();
    m_segs.reserve(m_constraints.size());
    for (const auto& c : m_constraints)
        m_segs.push_back({ m_nodes[c.i].pos, m_nodes[c.j].pos });
}
