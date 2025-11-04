# feature_alignment.py
"""
Feature Alignment Module for Autonomous Driving Data Processing

This module provides functionality to align features from different sensor 
modalities or temporal frames to improve data consistency in the closed-loop system.
"""

import numpy as np
import cv2
from typing import List, Tuple, Dict, Any, Optional
import json
from scipy.spatial.transform import Rotation as R
from scipy.optimize import minimize
from scipy.spatial.distance import cdist
import matplotlib.pyplot as plt

class Point3D:
    """
    Represents a 3D point
    """
    def __init__(self, x: float = 0.0, y: float = 0.0, z: float = 0.0):
        self.x = x
        self.y = y
        self.z = z
    
    def to_array(self) -> np.ndarray:
        return np.array([self.x, self.y, self.z])
    
    def __repr__(self):
        return f"Point3D({self.x}, {self.y}, {self.z})"

class Feature:
    """
    Represents a feature with position and descriptor
    """
    def __init__(self, position: Point3D, descriptor: np.ndarray = None, 
                 timestamp: float = 0.0, sensor_id: str = ""):
        self.position = position
        self.descriptor = descriptor if descriptor is not None else np.array([])
        self.timestamp = timestamp
        self.sensor_id = sensor_id
    
    def __repr__(self):
        return f"Feature(pos={self.position}, sensor={self.sensor_id}, time={self.timestamp})"

class Transformation:
    """
    Represents a 3D transformation (rotation + translation)
    """
    def __init__(self, rotation: np.ndarray = None, translation: np.ndarray = None):
        self.rotation = rotation if rotation is not None else np.eye(3)
        self.translation = translation if translation is not None else np.zeros(3)
    
    def transform_point(self, point: Point3D) -> Point3D:
        """
        Apply transformation to a 3D point
        """
        point_array = point.to_array()
        transformed = self.rotation @ point_array + self.translation
        return Point3D(transformed[0], transformed[1], transformed[2])
    
    def to_matrix(self) -> np.ndarray:
        """
        Convert to 4x4 transformation matrix
        """
        matrix = np.eye(4)
        matrix[:3, :3] = self.rotation
        matrix[:3, 3] = self.translation
        return matrix
    
    @classmethod
    def from_matrix(cls, matrix: np.ndarray):
        """
        Create transformation from 4x4 matrix
        """
        rotation = matrix[:3, :3]
        translation = matrix[:3, 3]
        return cls(rotation, translation)
    
    def inverse(self) -> 'Transformation':
        """
        Compute inverse transformation
        """
        inv_rotation = self.rotation.T
        inv_translation = -inv_rotation @ self.translation
        return Transformation(inv_rotation, inv_translation)
    
    def __repr__(self):
        return f"Transformation(R={self.rotation}, T={self.translation})"

class FeatureMatcher:
    """
    Matches features between different sensor data or time frames
    """
    def __init__(self, distance_threshold: float = 0.7, max_matches: int = 1000):
        self.distance_threshold = distance_threshold
        self.max_matches = max_matches
    
    def match_features(self, features1: List[Feature], features2: List[Feature]) -> List[Tuple[int, int]]:
        """
        Match features between two sets using descriptor distance
        """
        if not features1 or not features2:
            return []
        
        # Extract descriptors
        desc1 = np.array([f.descriptor for f in features1 if f.descriptor.size > 0])
        desc2 = np.array([f.descriptor for f in features2 if f.descriptor.size > 0])
        
        if desc1.size == 0 or desc2.size == 0:
            return []
        
        # Compute distance matrix
        distances = cdist(desc1, desc2, metric='euclidean')
        
        # Find matches
        matches = []
        for i in range(len(desc1)):
            if len(desc2) > 0:
                # Find closest and second closest matches
                sorted_indices = np.argsort(distances[i])
                if len(sorted_indices) >= 2:
                    best_dist = distances[i, sorted_indices[0]]
                    second_dist = distances[i, sorted_indices[1]]
                    
                    # Apply distance ratio test
                    if best_dist < self.distance_threshold * second_dist:
                        matches.append((i, sorted_indices[0]))
                elif len(sorted_indices) == 1:
                    if distances[i, sorted_indices[0]] < self.distance_threshold:
                        matches.append((i, sorted_indices[0]))
        
        return matches[:self.max_matches]
    
    def match_spatially(self, features1: List[Feature], features2: List[Feature], 
                       max_distance: float = 5.0) -> List[Tuple[int, int]]:
        """
        Match features based on spatial proximity
        """
        matches = []
        for i, f1 in enumerate(features1):
            best_j = -1
            best_distance = float('inf')
            
            for j, f2 in enumerate(features2):
                dist = np.linalg.norm(f1.position.to_array() - f2.position.to_array())
                if dist < best_distance and dist < max_distance:
                    best_distance = dist
                    best_j = j
            
            if best_j >= 0:
                matches.append((i, best_j))
        
        return matches

class ICPAligner:
    """
    Iterative Closest Point algorithm for point cloud alignment
    """
    def __init__(self, max_iterations: int = 50, tolerance: float = 1e-6):
        self.max_iterations = max_iterations
        self.tolerance = tolerance
    
    def align_point_clouds(self, source: List[Point3D], target: List[Point3D]) -> Transformation:
        """
        Align two point clouds using ICP algorithm
        """
        # Convert to numpy arrays
        source_points = np.array([p.to_array() for p in source]).T
        target_points = np.array([p.to_array() for p in target]).T
        
        # Initialize transformation
        transformation = Transformation()
        
        prev_error = float('inf')
        
        for iteration in range(self.max_iterations):
            # Find correspondences
            correspondences = self._find_correspondences(source_points, target_points)
            
            # Compute transformation
            transform = self._compute_transformation(source_points, target_points, correspondences)
            
            # Apply transformation
            source_points = transform.rotation @ source_points + transform.translation.reshape(3, 1)
            
            # Update cumulative transformation
            transformation.rotation = transform.rotation @ transformation.rotation
            transformation.translation = transform.rotation @ transformation.translation + transform.translation
            
            # Check convergence
            error = self._compute_alignment_error(source_points, target_points, correspondences)
            if abs(prev_error - error) < self.tolerance:
                break
            prev_error = error
        
        return transformation
    
    def _find_correspondences(self, source: np.ndarray, target: np.ndarray) -> np.ndarray:
        """
        Find nearest neighbor correspondences between source and target points
        """
        correspondences = []
        for i in range(source.shape[1]):
            src_point = source[:, i]
            distances = np.linalg.norm(target - src_point.reshape(3, 1), axis=0)
            closest_idx = np.argmin(distances)
            correspondences.append((i, closest_idx))
        return np.array(correspondences)
    
    def _compute_transformation(self, source: np.ndarray, target: np.ndarray, 
                              correspondences: np.ndarray) -> Transformation:
        """
        Compute the optimal transformation given correspondences
        """
        # Extract corresponding points
        src_corr = source[:, correspondences[:, 0]]
        tgt_corr = target[:, correspondences[:, 1]]
        
        # Compute centroids
        src_centroid = np.mean(src_corr, axis=1)
        tgt_centroid = np.mean(tgt_corr, axis=1)
        
        # Center the points
        src_centered = src_corr - src_centroid.reshape(3, 1)
        tgt_centered = tgt_corr - tgt_centroid.reshape(3, 1)
        
        # Compute covariance matrix
        cov_matrix = src_centered @ tgt_centered.T
        
        # Compute SVD
        U, _, Vt = np.linalg.svd(cov_matrix)
        
        # Compute rotation matrix
        rotation = Vt.T @ U.T
        
        # Ensure proper rotation (det = 1)
        if np.linalg.det(rotation) < 0:
            Vt[2, :] *= -1
            rotation = Vt.T @ U.T
        
        # Compute translation
        translation = tgt_centroid - rotation @ src_centroid
        
        return Transformation(rotation, translation)
    
    def _compute_alignment_error(self, source: np.ndarray, target: np.ndarray, 
                               correspondences: np.ndarray) -> float:
        """
        Compute alignment error
        """
        error = 0.0
        for i, j in correspondences:
            error += np.linalg.norm(source[:, i] - target[:, j]) ** 2
        return error / len(correspondences) if len(correspondences) > 0 else 0.0

class FeatureAligner:
    """
    Main class for feature alignment in the data processing pipeline
    """
    def __init__(self):
        self.matcher = FeatureMatcher()
        self.icp_aligner = ICPAligner()
        self.transformations = {}
    
    def align_sensor_data(self, sensor_data: Dict[str, List[Feature]]) -> Dict[str, Transformation]:
        """
        Align features from multiple sensors to a common reference frame
        """
        if not sensor_data:
            return {}
        
        # Select reference sensor (first one)
        reference_sensor = list(sensor_data.keys())[0]
        reference_features = sensor_data[reference_sensor]
        
        # Align all other sensors to reference
        transformations = {reference_sensor: Transformation()}
        
        for sensor_id, features in sensor_data.items():
            if sensor_id == reference_sensor:
                continue
            
            # Match features
            matches = self.matcher.match_features(reference_features, features)
            
            if len(matches) < 3:  # Need at least 3 points for transformation estimation
                print(f"Warning: Not enough matches for {sensor_id}, using identity transformation")
                transformations[sensor_id] = Transformation()
                continue
            
            # Extract matched points
            ref_points = [reference_features[i].position for i, _ in matches]
            sensor_points = [features[j].position for _, j in matches]
            
            # Align using ICP
            try:
                transform = self.icp_aligner.align_point_clouds(sensor_points, ref_points)
                transformations[sensor_id] = transform
            except Exception as e:
                print(f"Error aligning {sensor_id}: {e}")
                transformations[sensor_id] = Transformation()
        
        self.transformations = transformations
        return transformations
    
    def transform_features(self, features: List[Feature], transformation: Transformation) -> List[Feature]:
        """
        Apply transformation to a list of features
        """
        transformed_features = []
        for feature in features:
            transformed_position = transformation.transform_point(feature.position)
            new_feature = Feature(
                position=transformed_position,
                descriptor=feature.descriptor,
                timestamp=feature.timestamp,
                sensor_id=feature.sensor_id
            )
            transformed_features.append(new_feature)
        return transformed_features
    
    def fuse_aligned_features(self, sensor_data: Dict[str, List[Feature]]) -> List[Feature]:
        """
        Fuse features from multiple sensors after alignment
        """
        if not self.transformations:
            # Perform alignment if not already done
            self.align_sensor_data(sensor_data)
        
        fused_features = []
        
        for sensor_id, features in sensor_data.items():
            if sensor_id in self.transformations:
                # Transform features to common reference frame
                transformed = self.transform_features(features, self.transformations[sensor_id])
                fused_features.extend(transformed)
            else:
                # No transformation available, use as is
                fused_features.extend(features)
        
        return fused_features
    
    def evaluate_alignment_quality(self, sensor_data: Dict[str, List[Feature]], 
                                 sample_size: int = 100) -> Dict[str, float]:
        """
        Evaluate the quality of alignment using reprojection errors
        """
        if not self.transformations:
            return {}
        
        quality_metrics = {}
        
        for sensor_id, features in sensor_data.items():
            if sensor_id not in self.transformations or len(features) == 0:
                quality_metrics[sensor_id] = float('inf')
                continue
            
            # Sample features for evaluation
            indices = np.random.choice(len(features), min(sample_size, len(features)), replace=False)
            sampled_features = [features[i] for i in indices]
            
            # Transform features
            transformed = self.transform_features(sampled_features, self.transformations[sensor_id])
            
            # Find matches with reference sensor
            reference_sensor = list(sensor_data.keys())[0]
            reference_features = sensor_data[reference_sensor]
            
            matches = self.matcher.match_spatially(transformed, reference_features, max_distance=2.0)
            
            if len(matches) == 0:
                quality_metrics[sensor_id] = float('inf')
                continue
            
            # Compute average reprojection error
            total_error = 0.0
            for i, j in matches:
                error = np.linalg.norm(
                    transformed[i].position.to_array() - 
                    reference_features[j].position.to_array()
                )
                total_error += error
            
            quality_metrics[sensor_id] = total_error / len(matches)
        
        return quality_metrics
    
    def save_transformation(self, sensor_id: str, transformation: Transformation, filepath: str):
        """
        Save transformation to file
        """
        data = {
            'sensor_id': sensor_id,
            'rotation': transformation.rotation.tolist(),
            'translation': transformation.translation.tolist()
        }
        
        with open(filepath, 'w') as f:
            json.dump(data, f, indent=2)
        
        print(f"Transformation for {sensor_id} saved to {filepath}")
    
    def load_transformation(self, filepath: str) -> Tuple[str, Transformation]:
        """
        Load transformation from file
        """
        with open(filepath, 'r') as f:
            data = json.load(f)
        
        sensor_id = data['sensor_id']
        rotation = np.array(data['rotation'])
        translation = np.array(data['translation'])
        
        transformation = Transformation(rotation, translation)
        return sensor_id, transformation

def generate_sample_data() -> Dict[str, List[Feature]]:
    """
    Generate sample sensor data for testing
    """
    np.random.seed(42)
    
    # Camera features
    camera_features = []
    for i in range(50):
        x = np.random.uniform(-10, 10)
        y = np.random.uniform(-5, 5)
        z = np.random.uniform(0, 20)
        descriptor = np.random.rand(128)  # SIFT-like descriptor
        feature = Feature(Point3D(x, y, z), descriptor, i * 0.1, "camera")
        camera_features.append(feature)
    
    # LiDAR features with some offset (simulating calibration error)
    lidar_features = []
    for i in range(40):
        x = np.random.uniform(-10, 10) + 0.5  # 0.5m offset
        y = np.random.uniform(-5, 5) - 0.3   # -0.3m offset
        z = np.random.uniform(0, 20) + 0.1   # 0.1m offset
        descriptor = np.random.rand(128)
        feature = Feature(Point3D(x, y, z), descriptor, i * 0.1, "lidar")
        lidar_features.append(feature)
    
    # Radar features
    radar_features = []
    for i in range(30):
        x = np.random.uniform(-15, 15)
        y = np.random.uniform(-8, 8)
        z = np.random.uniform(0, 15)
        descriptor = np.random.rand(64)
        feature = Feature(Point3D(x, y, z), descriptor, i * 0.1, "radar")
        radar_features.append(feature)
    
    return {
        "camera": camera_features,
        "lidar": lidar_features,
        "radar": radar_features
    }

def visualize_alignment(original_data: Dict[str, List[Feature]], 
                       aligned_data: List[Feature]):
    """
    Visualize the alignment results
    """
    fig = plt.figure(figsize=(12, 8))
    
    # 3D plot
    ax = fig.add_subplot(111, projection='3d')
    
    # Plot original data
    colors = ['red', 'blue', 'green']
    sensors = list(original_data.keys())
    
    for i, (sensor_id, features) in enumerate(original_data.items()):
        xs = [f.position.x for f in features]
        ys = [f.position.y for f in features]
        zs = [f.position.z for f in features]
        ax.scatter(xs, ys, zs, c=colors[i], marker='o', label=f'{sensor_id} (original)', alpha=0.6)
    
    # Plot aligned data
    xs = [f.position.x for f in aligned_data]
    ys = [f.position.y for f in aligned_data]
    zs = [f.position.z for f in aligned_data]
    ax.scatter(xs, ys, zs, c='black', marker='x', label='Fused', alpha=0.8)
    
    ax.set_xlabel('X (m)')
    ax.set_ylabel('Y (m)')
    ax.set_zlabel('Z (m)')
    ax.set_title('Feature Alignment Results')
    ax.legend()
    
    plt.tight_layout()
    plt.show()

def main():
    """
    Main function demonstrating feature alignment
    """
    print("Feature Alignment for Autonomous Driving Data Processing")
    print("=" * 50)
    
    # Generate sample data
    print("Generating sample sensor data...")
    sensor_data = generate_sample_data()
    
    for sensor_id, features in sensor_data.items():
        print(f"  {sensor_id}: {len(features)} features")
    
    # Create feature aligner
    print("\nCreating feature aligner...")
    aligner = FeatureAligner()
    
    # Align sensor data
    print("Aligning sensor data...")
    transformations = aligner.align_sensor_data(sensor_data)
    
    for sensor_id, transform in transformations.items():
        print(f"  {sensor_id}:")
        print(f"    Rotation:\n{transform.rotation}")
        print(f"    Translation: {transform.translation}")
    
    # Fuse aligned features
    print("\nFusing aligned features...")
    fused_features = aligner.fuse_aligned_features(sensor_data)
    print(f"Fused {len(fused_features)} features from all sensors")
    
    # Evaluate alignment quality
    print("\nEvaluating alignment quality...")
    quality_metrics = aligner.evaluate_alignment_quality(sensor_data)
    for sensor_id, error in quality_metrics.items():
        print(f"  {sensor_id}: avg error = {error:.4f}m")
    
    # Save a transformation
    print("\nSaving transformation for LiDAR...")
    if 'lidar' in transformations:
        aligner.save_transformation('lidar', transformations['lidar'], 'lidar_transformation.json')
    
    # Visualize results
    print("\nVisualizing alignment results...")
    visualize_alignment(sensor_data, fused_features)
    
    print("\nFeature alignment completed successfully!")

if __name__ == "__main__":
    main()