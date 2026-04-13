#pragma once

#include <glm/glm.hpp>
#include <array>
#include <vector>

// Verlet mass-spring basketball net.
//
// Topology  (N_COLS = 16, N_ROWS = 8 free rows below anchor row 0):
//
//   Row 0 – anchor nodes fixed to the rim (16 nodes at rim radius)
//   Rows 1..N_ROWS – free nodes, each row tapering inward
//
//   Odd rows are angularly offset by half a column step, creating the
//   alternating V-shape / diamond pattern of a real net:
//
//     R   R   R   R   R   R  ...  (row 0 - even, at rim)
//      X   X   X   X   X   X      (cross-strands from rim to row 1)
//       V   V   V   V   V   V     (row 1 - odd, offset by pi/16)
//      X   X   X   X   X   X      (cross-strands from row 1 to row 2)
//     V   V   V   V   V   V   V   (row 2 - even, same angle as row 0)
//      ...
//
//   Radius tapers from rimRadius (top) down to ~37 % of that (bottom).

class Net {
public:
    static constexpr int N_COLS       = 16;
    static constexpr int N_ROWS       = 8;   // free rows below the anchor row
    static constexpr int N_TOTAL_ROWS = N_ROWS + 1;
    static constexpr int N_NODES      = N_COLS * N_TOTAL_ROWS;

    Net(glm::vec3 rimCenter, float rimRadius);

    void reset();

    // Step Verlet integration and respond to ball collision.
    void update(float dt, glm::vec3 ballPos, float ballRadius);

    struct Segment { glm::vec3 a, b; };
    const std::vector<Segment>& segments() const { return m_segs; }

private:
    glm::vec3 m_center;
    float     m_radius;

    struct Node {
        glm::vec3 pos  = {};
        glm::vec3 prev = {};
        bool      fixed = false;
    };

    std::array<Node, N_NODES> m_nodes;

    struct Constraint { int i, j; float rest; };
    std::vector<Constraint> m_constraints;
    std::vector<Segment>    m_segs;

    // Index of node at (col, row).
    int   at(int col, int row) const { return row * N_COLS + col; }

    // Rim radius for a given row (tapers toward bottom).
    float radiusAt(int row)          const;

    // Angle in the XZ plane for a node at (col, row).
    // Odd rows are shifted by π/N_COLS so strands alternate left–right.
    float angleAt (int col, int row) const;

    void initPositions();
    void buildConstraints();
    void stepVerlet(float dt);
    void solveConstraints();
    void handleBallCollision(glm::vec3 ballPos, float ballRadius);
    void buildSegments();
};
