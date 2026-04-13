import numpy as np
from scipy.ndimage import convolve


def sobel_edge_density(frame: np.ndarray) -> float:
    """Mean Sobel edge magnitude for a single HxWxC uint8 frame, in [0, 1]."""
    gray = frame.mean(axis=2).astype(np.float32) / 255.0
    kx = np.array([[-1, 0, 1], [-2, 0, 2], [-1, 0, 1]], dtype=np.float32)
    gx = convolve(gray, kx)
    gy = convolve(gray, kx.T)
    return float(np.sqrt(gx ** 2 + gy ** 2).mean())


def pixel_entropy(frame: np.ndarray) -> float:
    """Shannon entropy of the grayscale pixel distribution (bits, max 8)."""
    gray   = frame.mean(axis=2).astype(np.uint8)
    counts = np.bincount(gray.flatten(), minlength=256).astype(np.float64)
    probs  = counts / counts.sum()
    probs  = probs[probs > 0]
    return float(-np.sum(probs * np.log2(probs)))


def compute_complexity(frames: list) -> dict:
    """
    Aggregate complexity metrics across a list of (H, W, C) uint8 frames.

    Returns
    -------
    dict with keys:
        edge_density  – mean Sobel edge magnitude (higher = more edges)
        entropy       – mean pixel entropy in bits (max 8)
        combined      – normalised average of both, in [0, 1]
    """
    edges     = [sobel_edge_density(f) for f in frames]
    entropies = [pixel_entropy(f)      for f in frames]

    mean_edge = float(np.mean(edges))
    mean_ent  = float(np.mean(entropies))

    # Rough normalisation: edge density ~0–0.15 for typical OpenGL scenes;
    # entropy 0–8 bits.
    norm_edge = min(mean_edge / 0.15, 1.0)
    norm_ent  = mean_ent / 8.0

    return {
        "edge_density": mean_edge,
        "entropy":      mean_ent,
        "combined":     (norm_edge + norm_ent) / 2.0,
    }


def measure_config(env, n_frames: int = 32) -> dict:
    """
    Collect n_frames initial observations from env and compute complexity.
    All frames come from the reset state (fixed camera, no shot taken).
    """
    frames = []
    for _ in range(n_frames):
        obs, _ = env.reset()
        frames.append(obs)
    return compute_complexity(frames)
