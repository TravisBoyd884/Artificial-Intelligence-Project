import os
import socket
import struct
import subprocess
import time

import gymnasium as gym
import numpy as np
from gymnasium import spaces

OBS_W, OBS_H, OBS_C = 84, 84, 3
OBS_BYTES = OBS_W * OBS_H * OBS_C  # 21168
MSG_BYTES = OBS_BYTES + 4 + 1  # + f32 reward + u8 done

SOCKET_PATH = "/tmp/glitter_env.sock"

# Action bounds; action space is normalised to [-1, 1].
# yaw is now a *offset* from the spawn→hoop direction (camera yaw).
# plus or minus 25deg comfortably covers all random spawn positions.
YAW_LO, YAW_HI = -25.0, 25.0
PITCH_LO, PITCH_HI = 5.0, 60.0
POWER_LO, POWER_HI = 5.0, 25.0


class GlitterEnv(gym.Env):
    """
    Gym wrapper around the Glitter basketball environment.

    Parameters:
    config : str
        Scene config name passed to the C++ binary ("low", "medium", "high").
    binary_path : str or None
        Path to the compiled Glitter binary. When provided the env launches
        and manages the process automatically. When None the binary must
        already be running before reset() is called.
    socket_path : str
        Unix socket path that the C++ server listens on.
    render : bool
        Pass --render to the binary (real time meaning v sync is on). Only relevant when
        binary_path is set.
    """

    metadata = {"render_modes": []}

    def __init__(
        self,
        config="low",
        binary_path=None,
        socket_path=SOCKET_PATH,
        render=False,
        fixed_spawn=False,
    ):
        super().__init__()

        self.observation_space = spaces.Box(
            low=0, high=255, shape=(OBS_H, OBS_W, OBS_C), dtype=np.uint8
        )
        self.action_space = spaces.Box(low=-1.0, high=1.0, shape=(3,), dtype=np.float32)

        self.config = config
        self.binary_path = binary_path
        self.socket_path = socket_path
        self.render_flag = render
        self.fixed_spawn = fixed_spawn

        self._proc = None
        self._sock = None

    # Gym API
    def reset(self, *, seed=None, options=None):
        super().reset(seed=seed)
        if self._sock is None:
            self._launch()
            self._connect()
            obs, _, _ = self._recv_obs()  # C++ sends the first obs on connect
        else:
            self._send(struct.pack("<B", 0))
            obs, _, _ = self._recv_obs()
        return obs, {}

    def step(self, action):
        yaw, pitch, power = self._denorm(action)
        self._send(struct.pack("<Bfff", 1, yaw, pitch, power))
        obs, reward, done = self._recv_obs()
        return obs, reward, done, False, {}

    def close(self):
        if self._sock is not None:
            try:
                self._sock.close()
            except OSError:
                pass
            self._sock = None
        if self._proc is not None:
            self._proc.terminate()
            self._proc.wait()
            self._proc = None

    # helpers

    def _denorm(self, action):
        def lerp(lo, hi, t):
            return lo + (float(t) + 1.0) * 0.5 * (hi - lo)

        return (
            lerp(YAW_LO, YAW_HI, action[0]),
            lerp(PITCH_LO, PITCH_HI, action[1]),
            lerp(POWER_LO, POWER_HI, action[2]),
        )

    def _recv_obs(self):
        raw = self._recvn(MSG_BYTES)
        pixels = (
            np.frombuffer(raw[:OBS_BYTES], dtype=np.uint8)
            .reshape(OBS_H, OBS_W, OBS_C)[::-1]  # flip: OpenGL origin is bottom-left
            .copy()
        )
        (reward,) = struct.unpack_from("<f", raw, OBS_BYTES)
        done = raw[OBS_BYTES + 4] != 0
        return pixels, float(reward), bool(done)

    def _recvn(self, n):
        buf = bytearray()
        while len(buf) < n:
            chunk = self._sock.recv(n - len(buf))
            if not chunk:
                raise RuntimeError("GlitterEnv: socket closed unexpectedly")
            buf.extend(chunk)
        return bytes(buf)

    def _send(self, data):
        self._sock.sendall(data)

    def _launch(self):
        if self.binary_path is None:
            return
        if self._proc is not None and self._proc.poll() is None:
            self._proc.terminate()
            self._proc.wait()
        if os.path.exists(self.socket_path):
            os.unlink(self.socket_path)
        cmd = [self.binary_path, "--rl", self.config]
        if self.render_flag:
            cmd.append("--render")
        if self.fixed_spawn:
            cmd.append("--fixed-spawn")
        self._proc = subprocess.Popen(
            cmd,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
            env={**os.environ},  # forward DISPLAY etc.
        )

    def _connect(self):
        self._sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        deadline = time.monotonic() + 10.0
        while True:
            try:
                self._sock.connect(self.socket_path)
                return
            except (ConnectionRefusedError, FileNotFoundError):
                if time.monotonic() > deadline:
                    raise RuntimeError(
                        f"GlitterEnv: could not connect to {self.socket_path} "
                        f"after 10 s"
                    )
                time.sleep(0.1)
