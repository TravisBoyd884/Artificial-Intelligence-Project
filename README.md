# Travis Boyd Artificial Intelligence Project

This is a basketball simulator used to see how visual scene complexity affects reinforcement learning. A PPO agent learns to shoot from random positions by reading an 84x84 RBG image of the scene. There are low medium and high configurations for the scene which map to complexity of the scene.

## Dependencies

**C++**

- CMake, a C++17 compiler

### These are already in the project as submodules

I used glitter which is an open source repo to get these
libraries: https://github.com/Polytonic/Glitter.git

- GLFW, glad, GLM ()
- Bullet3 (bundled)

**Python**

```
pip install -r rl/requirements.txt
```

## Build

```bash
./build.sh
```

## Run

**Interactive mode** This mode lets you just play the simulator

```bash
./Build/Glitter/Glitter low
```

**Train an agent:**

run the script and specify the complexity and steps

```bash
./train.sh low 500000
```

**Watch the agent:**

```bash
./demo.sh low
```

**Run the full experiment sweep:**
this runs all the complexity leves

```bash
./experiment.sh 500000
```

**Plot results:**
get the finally plotted results

```bash
./plot.sh
```
