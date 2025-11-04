# utils.py
"""
Utility functions for the autonomous driving data processing pipeline

This module provides common helper functions for data manipulation, coordinate 
transformations, file I/O operations, and other frequently used operations 
in the data processing pipeline.
"""

import numpy as np
import json
import yaml
import csv
import os
import cv2
from typing import List, Tuple, Dict, Any, Optional, Union
from scipy.spatial.distance import cdist
from scipy.interpolate import interp1d
import matplotlib.pyplot as plt
from datetime import datetime

class Point:
    """
    Represents a 2D point
    """
    def __init__(self, x: float = 0.0, y: float = 0.0):
        self.x = x
        self.y = y
    
    def to_array(self) -> np.ndarray:
        return np.array([self.x, self.y])
    
    def distance_to(self, other: 'Point') -> float:
        """Calculate Euclidean distance to another point"""
        return np.sqrt((self.x - other.x)**2 + (self.y - other.y)**2)
    
    def __repr__(self):
        return f"Point({self.x}, {self.y})"
    
    def __eq__(self, other):
        if not isinstance(other, Point):
            return False
        return abs(self.x - other.x) < 1e-6 and abs(self.y - other.y) < 1e-6

class Pose:
    """
    Represents a 2D pose (position and orientation)
    """
    def __init__(self, position: Point = None, yaw: float = 0.0):
        self.position = position if position is not None else Point()
        self.yaw = yaw  # Orientation in radians
    
    def __repr__(self):
        return f"Pose(position={self.position}, yaw={self.yaw})"

class Path:
    """
    Represents a path as a sequence of waypoints
    """
    def __init__(self, waypoints: List[Point] = None):
        self.waypoints = waypoints if waypoints is not None else []
        self.length = self._calculate_length()
    
    def _calculate_length(self) -> float:
        """Calculate total path length"""
        if len(self.waypoints) < 2:
            return 0.0
        
        length = 0.0
        for i in range(1, len(self.waypoints)):
            length += self.waypoints[i-1].distance_to(self.waypoints[i])
        return length
    
    def add_point(self, point: Point):
        """Add a point to the path"""
        self.waypoints.append(point)
        self.length = self._calculate_length()
    
    def clear(self):
        """Clear all waypoints"""
        self.waypoints.clear()
        self.length = 0.0
    
    def resample(self, spacing: float) -> 'Path':
        """
        Resample path to have evenly spaced waypoints
        """
        if len(self.waypoints) < 2:
            return Path(self.waypoints.copy())
        
        # Calculate cumulative distances
        distances = [0.0]
        for i in range(1, len(self.waypoints)):
            distances.append(distances[-1] + self.waypoints[i-1].distance_to(self.waypoints[i]))
        
        # Create interpolation functions
        x_coords = [p.x for p in self.waypoints]
        y_coords = [p.y for p in self.waypoints]
        
        fx = interp1d(distances, x_coords, kind='linear')
        fy = interp1d(distances, y_coords, kind='linear')
        
        # Generate new waypoints
        new_distances = np.arange(0, distances[-1], spacing)
        if new_distances[-1] != distances[-1]:
            new_distances = np.append(new_distances, distances[-1])
        
        new_waypoints = [Point(float(fx(d)), float(fy(d))) for d in new_distances]
        return Path(new_waypoints)
    
    def smooth(self, smoothing_factor: float = 0.1) -> 'Path':
        """
        Smooth path using simple averaging
        """
        if len(self.waypoints) < 3:
            return Path(self.waypoints.copy())
        
        smoothed_waypoints = [self.waypoints[0]]  # Keep first point
        
        for i in range(1, len(self.waypoints) - 1):
            prev_point = self.waypoints[i-1]
            curr_point = self.waypoints[i]
            next_point = self.waypoints[i+1]
            
            # Simple moving average
            smoothed_x = (prev_point.x + curr_point.x * 2 + next_point.x) / 4
            smoothed_y = (prev_point.y + curr_point.y * 2 + next_point.y) / 4
            smoothed_waypoints.append(Point(smoothed_x, smoothed_y))
        
        smoothed_waypoints.append(self.waypoints[-1])  # Keep last point
        return Path(smoothed_waypoints)
    
    def __len__(self):
        return len(self.waypoints)
    
    def __repr__(self):
        return f"Path({len(self.waypoints)} waypoints, length={self.length:.2f})"

class DataProcessorUtils:
    """
    Main utility class for data processing operations
    """
    
    @staticmethod
    def euclidean_distance(p1: Point, p2: Point) -> float:
        """
        Calculate Euclidean distance between two points
        """
        return np.sqrt((p1.x - p2.x)**2 + (p1.y - p2.y)**2)
    
    @staticmethod
    def manhattan_distance(p1: Point, p2: Point) -> float:
        """
        Calculate Manhattan distance between two points
        """
        return abs(p1.x - p2.x) + abs(p1.y - p2.y)
    
    @staticmethod
    def point_to_segment_distance(point: Point, seg_start: Point, seg_end: Point) -> float:
        """
        Calculate distance from a point to a line segment
        """
        # Vector from seg_start to seg_end
        segment_vec = Point(seg_end.x - seg_start.x, seg_end.y - seg_start.y)
        segment_len_sq = segment_vec.x**2 + segment_vec.y**2
        
        if segment_len_sq == 0:
            return DataProcessorUtils.euclidean_distance(point, seg_start)
        
        # Project point onto segment
        t = max(0, min(1, ((point.x - seg_start.x) * segment_vec.x + 
                          (point.y - seg_start.y) * segment_vec.y) / segment_len_sq))
        
        # Find projection point
        projection = Point(seg_start.x + t * segment_vec.x,
                          seg_start.y + t * segment_vec.y)
        
        return DataProcessorUtils.euclidean_distance(point, projection)
    
    @staticmethod
    def world_to_grid(world_point: Point, resolution: float) -> Tuple[int, int]:
        """
        Convert world coordinates to grid coordinates
        """
        grid_x = int(np.floor(world_point.x / resolution))
        grid_y = int(np.floor(world_point.y / resolution))
        return (grid_x, grid_y)
    
    @staticmethod
    def grid_to_world(grid_x: int, grid_y: int, resolution: float) -> Point:
        """
        Convert grid coordinates to world coordinates
        """
        return Point((grid_x + 0.5) * resolution, (grid_y + 0.5) * resolution)
    
    @staticmethod
    def normalize_angle(angle: float) -> float:
        """
        Normalize angle to [-π, π]
        """
        while angle > np.pi:
            angle -= 2 * np.pi
        while angle < -np.pi:
            angle += 2 * np.pi
        return angle
    
    @staticmethod
    def clamp(value: float, min_val: float, max_val: float) -> float:
        """
        Clamp a value between min and max
        """
        return max(min_val, min(value, max_val))
    
    @staticmethod
    def calculate_heading(from_point: Point, to_point: Point) -> float:
        """
        Calculate heading from one point to another
        """
        dx = to_point.x - from_point.x
        dy = to_point.y - from_point.y
        return np.arctan2(dy, dx)
    
    @staticmethod
    def load_parameters_from_yaml(filepath: str) -> Dict[str, Any]:
        """
        Load parameters from YAML file
        """
        try:
            with open(filepath, 'r') as f:
                return yaml.safe_load(f)
        except Exception as e:
            print(f"Error loading YAML file {filepath}: {e}")
            return {}
    
    @staticmethod
    def save_parameters_to_yaml(filepath: str, parameters: Dict[str, Any]):
        """
        Save parameters to YAML file
        """
        try:
            with open(filepath, 'w') as f:
                yaml.dump(parameters, f, default_flow_style=False)
            print(f"Parameters saved to {filepath}")
        except Exception as e:
            print(f"Error saving parameters to {filepath}: {e}")
    
    @staticmethod
    def load_json_data(filepath: str) -> Any:
        """
        Load data from JSON file
        """
        try:
            with open(filepath, 'r') as f:
                return json.load(f)
        except Exception as e:
            print(f"Error loading JSON file {filepath}: {e}")
            return None
    
    @staticmethod
    def save_json_data(filepath: str, data: Any):
        """
        Save data to JSON file
        """
        try:
            with open(filepath, 'w') as f:
                json.dump(data, f, indent=2)
            print(f"Data saved to {filepath}")
        except Exception as e:
            print(f"Error saving data to {filepath}: {e}")
    
    @staticmethod
    def load_csv_data(filepath: str) -> List[Dict[str, Any]]:
        """
        Load data from CSV file
        """
        try:
            data = []
            with open(filepath, 'r') as f:
                reader = csv.DictReader(f)
                for row in reader:
                    data.append(row)
            return data
        except Exception as e:
            print(f"Error loading CSV file {filepath}: {e}")
            return []
    
    @staticmethod
    def save_csv_data(filepath: str, data: List[Dict[str, Any]], fieldnames: List[str] = None):
        """
        Save data to CSV file
        """
        try:
            if not data:
                print("No data to save")
                return
            
            if fieldnames is None:
                fieldnames = list(data[0].keys())
            
            with open(filepath, 'w', newline='') as f:
                writer = csv.DictWriter(f, fieldnames=fieldnames)
                writer.writeheader()
                writer.writerows(data)
            print(f"Data saved to {filepath}")
        except Exception as e:
            print(f"Error saving data to {filepath}: {e}")
    
    @staticmethod
    def file_exists(filepath: str) -> bool:
        """
        Check if file exists
        """
        return os.path.exists(filepath)
    
    @staticmethod
    def create_directory(dirpath: str) -> bool:
        """
        Create directory if it doesn't exist
        """
        try:
            os.makedirs(dirpath, exist_ok=True)
            return True
        except Exception as e:
            print(f"Error creating directory {dirpath}: {e}")
            return False
    
    @staticmethod
    def read_text_file(filepath: str) -> str:
        """
        Read content from text file
        """
        try:
            with open(filepath, 'r') as f:
                return f.read()
        except Exception as e:
            print(f"Error reading file {filepath}: {e}")
            return ""
    
    @staticmethod
    def write_text_file(filepath: str, content: str) -> bool:
        """
        Write content to text file
        """
        try:
            with open(filepath, 'w') as f:
                f.write(content)
            return True
        except Exception as e:
            print(f"Error writing to file {filepath}: {e}")
            return False
    
    @staticmethod
    def get_file_timestamp(filepath: str) -> Optional[datetime]:
        """
        Get file modification timestamp
        """
        try:
            timestamp = os.path.getmtime(filepath)
            return datetime.fromtimestamp(timestamp)
        except Exception as e:
            print(f"Error getting timestamp for {filepath}: {e}")
            return None

class Logger:
    """
    Simple logging utility
    """
    DEBUG = 0
    INFO = 1
    WARN = 2
    ERROR = 3
    
    def __init__(self, min_level: int = INFO):
        self.min_level = min_level
    
    def log(self, level: int, message: str):
        """
        Log message with level
        """
        if level < self.min_level:
            return
        
        level_names = {self.DEBUG: "DEBUG", self.INFO: "INFO", 
                      self.WARN: "WARN", self.ERROR: "ERROR"}
        level_name = level_names.get(level, "UNKNOWN")
        
        timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        print(f"[{timestamp}] [{level_name}] {message}")
    
    def debug(self, message: str):
        self.log(self.DEBUG, message)
    
    def info(self, message: str):
        self.log(self.INFO, message)
    
    def warn(self, message: str):
        self.log(self.WARN, message)
    
    def error(self, message: str):
        self.log(self.ERROR, message)
    
    def set_level(self, level: int):
        self.min_level = level

class DataAnalyzer:
    """
    Utility class for data analysis operations
    """
    
    @staticmethod
    def compute_statistics(data: np.ndarray) -> Dict[str, float]:
        """
        Compute basic statistics for data array
        """
        if len(data) == 0:
            return {}
        
        return {
            'mean': float(np.mean(data)),
            'std': float(np.std(data)),
            'min': float(np.min(data)),
            'max': float(np.max(data)),
            'median': float(np.median(data))
        }
    
    @staticmethod
    def compute_density_map(points: List[Point], grid_size: Tuple[int, int] = (100, 100)) -> np.ndarray:
        """
        Compute 2D density map from point data
        """
        if not points:
            return np.zeros(grid_size)
        
        # Create grid
        grid = np.zeros(grid_size)
        
        # Convert points to grid coordinates
        for point in points:
            x_idx = min(int(point.x * grid_size[0]), grid_size[0] - 1)
            y_idx = min(int(point.y * grid_size[1]), grid_size[1] - 1)
            if 0 <= x_idx < grid_size[0] and 0 <= y_idx < grid_size[1]:
                grid[y_idx, x_idx] += 1
        
        return grid
    
    @staticmethod
    def find_clusters(points: List[Point], distance_threshold: float = 1.0) -> List[List[Point]]:
        """
        Find clusters of points based on distance threshold
        """
        if not points:
            return []
        
        clusters = []
        visited = set()
        
        for i, point in enumerate(points):
            if i in visited:
                continue
            
            # Start new cluster
            cluster = [point]
            visited.add(i)
            
            # Find neighbors
            for j, other_point in enumerate(points):
                if j in visited:
                    continue
                
                if point.distance_to(other_point) <= distance_threshold:
                    cluster.append(other_point)
                    visited.add(j)
            
            clusters.append(cluster)
        
        return clusters

def generate_sample_data() -> List[Point]:
    """
    Generate sample point data for testing
    """
    np.random.seed(42)
    points = []
    
    # Generate some clustered points
    for _ in range(50):
        x = np.random.normal(10, 2)
        y = np.random.normal(10, 2)
        points.append(Point(x, y))
    
    # Generate some scattered points
    for _ in range(30):
        x = np.random.uniform(0, 20)
        y = np.random.uniform(0, 20)
        points.append(Point(x, y))
    
    return points

def visualize_points(points: List[Point], title: str = "Point Visualization"):
    """
    Visualize points in 2D
    """
    if not points:
        print("No points to visualize")
        return
    
    x_coords = [p.x for p in points]
    y_coords = [p.y for p in points]
    
    plt.figure(figsize=(8, 6))
    plt.scatter(x_coords, y_coords, alpha=0.7)
    plt.xlabel('X')
    plt.ylabel('Y')
    plt.title(title)
    plt.grid(True, alpha=0.3)
    plt.axis('equal')
    plt.show()

def main():
    """
    Main function demonstrating utility functions
    """
    print("Data Processor Utilities for Autonomous Driving")
    print("=" * 45)
    
    # Create logger
    logger = Logger(Logger.INFO)
    logger.info("Starting utility demonstration")
    
    # 1. Point and Path operations
    print("\n1. Point and Path operations:")
    p1 = Point(0, 0)
    p2 = Point(3, 4)
    distance = DataProcessorUtils.euclidean_distance(p1, p2)
    print(f"   Distance between {p1} and {p2}: {distance:.2f}")
    
    # Create a path
    path_points = [Point(0, 0), Point(1, 1), Point(2, 0), Point(3, -1), Point(4, 0)]
    path = Path(path_points)
    print(f"   Original path: {path}")
    
    # Resample path
    resampled_path = path.resample(0.5)
    print(f"   Resampled path: {resampled_path}")
    
    # Smooth path
    smoothed_path = path.smooth()
    print(f"   Smoothed path: {smoothed_path}")
    
    # 2. Coordinate transformations
    print("\n2. Coordinate transformations:")
    world_point = Point(5.7, 3.2)
    resolution = 1.0
    grid_coords = DataProcessorUtils.world_to_grid(world_point, resolution)
    print(f"   World point {world_point} -> Grid coordinates {grid_coords}")
    
    grid_point = DataProcessorUtils.grid_to_world(grid_coords[0], grid_coords[1], resolution)
    print(f"   Grid coordinates {grid_coords} -> World point {grid_point}")
    
    # 3. File I/O operations
    print("\n3. File I/O operations:")
    
    # Create test directory
    test_dir = "test_output"
    DataProcessorUtils.create_directory(test_dir)
    
    # Save sample parameters
    params = {
        'sparse_threshold': 0.2,
        'exploration_bonus': 0.5,
        'redundancy_penalty': 0.4,
        'grid_resolution': 1.0
    }
    yaml_file = os.path.join(test_dir, "test_params.yaml")
    DataProcessorUtils.save_parameters_to_yaml(yaml_file, params)
    
    # Load parameters
    loaded_params = DataProcessorUtils.load_parameters_from_yaml(yaml_file)
    print(f"   Loaded parameters: {loaded_params}")
    
    # Save JSON data
    json_data = {'points': [{'x': 1.0, 'y': 2.0}, {'x': 3.0, 'y': 4.0}]}
    json_file = os.path.join(test_dir, "test_data.json")
    DataProcessorUtils.save_json_data(json_file, json_data)
    
    # Load JSON data
    loaded_json = DataProcessorUtils.load_json_data(json_file)
    print(f"   Loaded JSON data: {loaded_json}")
    
    # 4. Data analysis
    print("\n4. Data analysis:")
    sample_points = generate_sample_data()
    print(f"   Generated {len(sample_points)} sample points")
    
    # Compute statistics on x-coordinates
    x_coords = np.array([p.x for p in sample_points])
    stats = DataAnalyzer.compute_statistics(x_coords)
    print(f"   X-coordinate statistics: {stats}")
    
    # Compute density map
    density_map = DataAnalyzer.compute_density_map(sample_points, (20, 20))
    print(f"   Density map shape: {density_map.shape}")
    print(f"   Density map max value: {np.max(density_map)}")
    
    # Find clusters
    clusters = DataAnalyzer.find_clusters(sample_points, distance_threshold=2.0)
    print(f"   Found {len(clusters)} clusters")
    
    # 5. Visualization
    print("\n5. Visualization:")
    visualize_points(sample_points[:20], "Sample Points Visualization")
    
    # 6. Logging
    print("\n6. Logging:")
    logger.debug("This is a debug message")
    logger.info("This is an info message")
    logger.warn("This is a warning message")
    logger.error("This is an error message")
    
    print("\nUtility demonstration completed successfully!")

if __name__ == "__main__":
    main()