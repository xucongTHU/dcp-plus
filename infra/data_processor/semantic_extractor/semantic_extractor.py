# semantic_extractor.py
"""
Semantic Extractor Module for Autonomous Driving Data Processing

This module provides functionality to extract semantic information from sensor data
such as images, point clouds, and other sensor modalities to support navigation
planning and data collection in the closed-loop system.
"""

import numpy as np
import cv2
from typing import List, Tuple, Dict, Any, Optional
import json
from dataclasses import dataclass
from enum import Enum
import matplotlib.pyplot as plt
from scipy.spatial import KDTree

class SemanticType(Enum):
    """
    Enum for different semantic types
    """
    ROAD = 0
    LANE_MARKER = 1
    TRAFFIC_SIGN = 2
    TRAFFIC_LIGHT = 3
    PEDESTRIAN_CROSSING = 4
    PARKING_SPOT = 5
    BUILDING = 6
    VEGETATION = 7
    DATA_COLLECTION_ZONE = 8
    UNKNOWN = 9

@dataclass
class Point3D:
    """
    Represents a 3D point
    """
    x: float = 0.0
    y: float = 0.0
    z: float = 0.0
    
    def to_array(self) -> np.ndarray:
        return np.array([self.x, self.y, self.z])

@dataclass
class BoundingBox2D:
    """
    2D Bounding box representation
    """
    x_min: int = 0
    y_min: int = 0
    x_max: int = 0
    y_max: int = 0
    
    @property
    def width(self) -> int:
        return self.x_max - self.x_min
    
    @property
    def height(self) -> int:
        return self.y_max - self.y_min
    
    @property
    def area(self) -> int:
        return self.width * self.height

@dataclass
class SemanticObject:
    """
    Represents a semantic object with its properties
    """
    semantic_type: SemanticType
    position: Point3D
    bounding_box: Optional[BoundingBox2D] = None
    radius: float = 0.0  # Bounding radius for 3D objects
    label: str = ""
    confidence: float = 0.0
    sensor_id: str = ""
    timestamp: float = 0.0
    
    def __repr__(self):
        return (f"SemanticObject(type={self.semantic_type.name}, "
                f"position=({self.position.x:.2f}, {self.position.y:.2f}, {self.position.z:.2f}), "
                f"confidence={self.confidence:.2f})")

class SemanticExtractor:
    """
    Main class for extracting semantic information from sensor data
    """
    def __init__(self):
        # Predefined color mappings for visualization
        self.color_map = {
            SemanticType.ROAD: (128, 64, 128),           # Purple
            SemanticType.LANE_MARKER: (255, 255, 255),   # White
            SemanticType.TRAFFIC_SIGN: (220, 220, 0),    # Yellow
            SemanticType.TRAFFIC_LIGHT: (0, 0, 255),     # Red
            SemanticType.PEDESTRIAN_CROSSING: (153, 153, 153),  # Gray
            SemanticType.PARKING_SPOT: (0, 255, 0),      # Green
            SemanticType.BUILDING: (70, 70, 70),         # Dark gray
            SemanticType.VEGETATION: (107, 142, 35),     # Olive
            SemanticType.DATA_COLLECTION_ZONE: (0, 0, 142),  # Blue
            SemanticType.UNKNOWN: (0, 0, 0)              # Black
        }
        
        # Semantic labels mapping
        self.label_map = {
            0: "road",
            1: "lane_marker",
            2: "traffic_sign",
            3: "traffic_light",
            4: "pedestrian_crossing",
            5: "parking_spot",
            6: "building",
            7: "vegetation",
            8: "data_collection_zone",
            9: "unknown"
        }
    
    def extract_from_image(self, image: np.ndarray, 
                          detections: List[Dict[str, Any]]) -> List[SemanticObject]:
        """
        Extract semantic objects from 2D image detections
        
        Args:
            image: Input image (H, W, 3)
            detections: List of detection dictionaries with keys:
                       - 'bbox': [x_min, y_min, x_max, y_max]
                       - 'class_id': integer class ID
                       - 'confidence': float confidence score
                       - 'label': string label (optional)
        
        Returns:
            List of SemanticObject instances
        """
        semantic_objects = []
        height, width = image.shape[:2]
        
        for detection in detections:
            # Extract bounding box
            bbox_coords = detection['bbox']
            bbox = BoundingBox2D(
                x_min=int(bbox_coords[0]),
                y_min=int(bbox_coords[1]),
                x_max=int(bbox_coords[2]),
                y_max=int(bbox_coords[3])
            )
            
            # Calculate center position (normalized to [0,1])
            center_x = (bbox.x_min + bbox.x_max) / 2 / width
            center_y = (bbox.y_min + bbox.y_max) / 2 / height
            
            # Create semantic object
            semantic_type = SemanticType(detection['class_id'])
            label = detection.get('label', self.label_map.get(detection['class_id'], 'unknown'))
            
            obj = SemanticObject(
                semantic_type=semantic_type,
                position=Point3D(x=center_x, y=center_y, z=0.0),
                bounding_box=bbox,
                label=label,
                confidence=detection['confidence'],
                sensor_id="camera"
            )
            
            semantic_objects.append(obj)
        
        return semantic_objects
    
    def extract_from_point_cloud(self, points: np.ndarray, 
                               labels: np.ndarray) -> List[SemanticObject]:
        """
        Extract semantic objects from 3D point cloud with semantic labels
        
        Args:
            points: 3D points array (N, 3) with [x, y, z] coordinates
            labels: Semantic labels for each point (N,)
        
        Returns:
            List of SemanticObject instances
        """
        semantic_objects = []
        
        # Group points by semantic label
        unique_labels = np.unique(labels)
        
        for label in unique_labels:
            # Skip unknown labels
            if label not in [t.value for t in SemanticType]:
                continue
            
            # Get points with this label
            mask = labels == label
            labeled_points = points[mask]
            
            if len(labeled_points) == 0:
                continue
            
            # Calculate centroid
            centroid = np.mean(labeled_points, axis=0)
            
            # Calculate bounding radius
            distances = np.linalg.norm(labeled_points - centroid, axis=1)
            radius = np.max(distances)
            
            # Create semantic object
            semantic_type = SemanticType(label)
            
            obj = SemanticObject(
                semantic_type=semantic_type,
                position=Point3D(x=centroid[0], y=centroid[1], z=centroid[2]),
                radius=radius,
                label=self.label_map.get(label, 'unknown'),
                confidence=1.0,  # Point cloud segmentation usually has high confidence
                sensor_id="lidar"
            )
            
            semantic_objects.append(obj)
        
        return semantic_objects
    
    def extract_data_collection_zones(self, costmap_data: np.ndarray, 
                                    threshold: float = 0.2) -> List[SemanticObject]:
        """
        Extract data collection zones from costmap data based on density
        
        Args:
            costmap_data: 2D array representing data density
            threshold: Density threshold for identifying sparse areas
        
        Returns:
            List of SemanticObject instances representing data collection zones
        """
        semantic_objects = []
        
        # Identify sparse regions (below threshold)
        sparse_mask = costmap_data < threshold
        
        # Find connected components
        num_labels, labels, stats, centroids = cv2.connectedComponentsWithStats(
            sparse_mask.astype(np.uint8), connectivity=8)
        
        # Create semantic objects for each connected component
        for i in range(1, num_labels):  # Skip background (label 0)
            # Get component properties
            area = stats[i, cv2.CC_STAT_AREA]
            centroid_x = centroids[i, 0]
            centroid_y = centroids[i, 1]
            
            # Calculate bounding box
            x_min = stats[i, cv2.CC_STAT_LEFT]
            y_min = stats[i, cv2.CC_STAT_TOP]
            x_max = x_min + stats[i, cv2.CC_STAT_WIDTH]
            y_max = y_min + stats[i, cv2.CC_STAT_HEIGHT]
            
            # Only consider significant regions
            if area > 10:  # Minimum area threshold
                obj = SemanticObject(
                    semantic_type=SemanticType.DATA_COLLECTION_ZONE,
                    position=Point3D(x=centroid_x, y=centroid_y, z=0.0),
                    bounding_box=BoundingBox2D(x_min, y_min, x_max, y_max),
                    radius=np.sqrt(area / np.pi),  # Equivalent radius
                    label="data_collection_zone",
                    confidence=1.0 - (np.mean(costmap_data[labels == i])),  # Lower density = higher priority
                    sensor_id="costmap"
                )
                
                semantic_objects.append(obj)
        
        return semantic_objects
    
    def filter_objects_by_confidence(self, objects: List[SemanticObject], 
                                   min_confidence: float = 0.5) -> List[SemanticObject]:
        """
        Filter semantic objects by confidence threshold
        """
        return [obj for obj in objects if obj.confidence >= min_confidence]
    
    def filter_objects_by_type(self, objects: List[SemanticObject], 
                              semantic_types: List[SemanticType]) -> List[SemanticObject]:
        """
        Filter semantic objects by type
        """
        return [obj for obj in objects if obj.semantic_type in semantic_types]
    
    def get_objects_in_radius(self, objects: List[SemanticObject], 
                            center: Point3D, radius: float) -> List[SemanticObject]:
        """
        Get semantic objects within a radius of a center point
        """
        result = []
        for obj in objects:
            distance = np.linalg.norm(
                np.array([obj.position.x, obj.position.y, obj.position.z]) - 
                np.array([center.x, center.y, center.z])
            )
            if distance <= radius:
                result.append(obj)
        return result
    
    def visualize_semantic_objects(self, image: np.ndarray, 
                                 objects: List[SemanticObject]) -> np.ndarray:
        """
        Visualize semantic objects on an image
        """
        vis_image = image.copy()
        
        for obj in objects:
            color = self.color_map.get(obj.semantic_type, (0, 0, 0))
            
            if obj.bounding_box is not None:
                # Draw bounding box
                bbox = obj.bounding_box
                cv2.rectangle(vis_image, 
                            (bbox.x_min, bbox.y_min), 
                            (bbox.x_max, bbox.y_max), 
                            color, 2)
                
                # Draw label
                label = f"{obj.label}: {obj.confidence:.2f}"
                cv2.putText(vis_image, label, 
                          (bbox.x_min, bbox.y_min - 10), 
                          cv2.FONT_HERSHEY_SIMPLEX, 0.5, color, 1)
            
            # Draw center point
            center_x = int(obj.position.x)
            center_y = int(obj.position.y)
            cv2.circle(vis_image, (center_x, center_y), 5, color, -1)
        
        return vis_image
    
    def save_semantic_objects(self, objects: List[SemanticObject], filepath: str):
        """
        Save semantic objects to JSON file
        """
        data = []
        for obj in objects:
            obj_data = {
                'semantic_type': obj.semantic_type.value,
                'position': {
                    'x': obj.position.x,
                    'y': obj.position.y,
                    'z': obj.position.z
                },
                'label': obj.label,
                'confidence': obj.confidence,
                'sensor_id': obj.sensor_id,
                'timestamp': obj.timestamp
            }
            
            if obj.bounding_box is not None:
                obj_data['bounding_box'] = {
                    'x_min': obj.bounding_box.x_min,
                    'y_min': obj.bounding_box.y_min,
                    'x_max': obj.bounding_box.x_max,
                    'y_max': obj.bounding_box.y_max
                }
            
            if obj.radius > 0:
                obj_data['radius'] = obj.radius
                
            data.append(obj_data)
        
        with open(filepath, 'w') as f:
            json.dump(data, f, indent=2)
        
        print(f"Saved {len(objects)} semantic objects to {filepath}")
    
    def load_semantic_objects(self, filepath: str) -> List[SemanticObject]:
        """
        Load semantic objects from JSON file
        """
        with open(filepath, 'r') as f:
            data = json.load(f)
        
        objects = []
        for obj_data in data:
            semantic_type = SemanticType(obj_data['semantic_type'])
            position = Point3D(
                obj_data['position']['x'],
                obj_data['position']['y'],
                obj_data['position']['z']
            )
            
            bbox = None
            if 'bounding_box' in obj_data:
                bbox_data = obj_data['bounding_box']
                bbox = BoundingBox2D(
                    bbox_data['x_min'],
                    bbox_data['y_min'],
                    bbox_data['x_max'],
                    bbox_data['y_max']
                )
            
            obj = SemanticObject(
                semantic_type=semantic_type,
                position=position,
                bounding_box=bbox,
                radius=obj_data.get('radius', 0.0),
                label=obj_data.get('label', ''),
                confidence=obj_data.get('confidence', 0.0),
                sensor_id=obj_data.get('sensor_id', ''),
                timestamp=obj_data.get('timestamp', 0.0)
            )
            
            objects.append(obj)
        
        print(f"Loaded {len(objects)} semantic objects from {filepath}")
        return objects

def generate_sample_image_detections() -> Tuple[np.ndarray, List[Dict[str, Any]]]:
    """
    Generate sample image and detections for testing
    """
    # Create sample image
    image = np.random.randint(0, 255, (480, 640, 3), dtype=np.uint8)
    
    # Create sample detections
    detections = [
        {
            'bbox': [100, 100, 200, 200],
            'class_id': SemanticType.TRAFFIC_SIGN.value,
            'confidence': 0.95,
            'label': 'traffic_sign'
        },
        {
            'bbox': [300, 150, 400, 250],
            'class_id': SemanticType.BUILDING.value,
            'confidence': 0.87,
            'label': 'building'
        },
        {
            'bbox': [50, 300, 550, 400],
            'class_id': SemanticType.ROAD.value,
            'confidence': 0.99,
            'label': 'road'
        }
    ]
    
    return image, detections

def generate_sample_point_cloud() -> Tuple[np.ndarray, np.ndarray]:
    """
    Generate sample point cloud data with semantic labels
    """
    np.random.seed(42)
    
    # Generate sample points
    points = np.random.randn(1000, 3) * 10
    
    # Generate semantic labels
    labels = np.random.choice([t.value for t in SemanticType], 1000)
    
    return points, labels

def generate_sample_costmap() -> np.ndarray:
    """
    Generate sample costmap data
    """
    # Create sample costmap with some sparse regions
    costmap = np.random.rand(100, 100)
    
    # Add some sparse regions
    costmap[20:30, 20:30] = 0.1  # Very sparse area
    costmap[70:80, 70:80] = 0.05  # Very sparse area
    
    return costmap

def visualize_semantic_map(objects: List[SemanticObject]):
    """
    Visualize semantic objects in 2D
    """
    fig, ax = plt.subplots(1, 1, figsize=(10, 8))
    
    # Color map for visualization
    color_map = {
        SemanticType.ROAD: 'purple',
        SemanticType.LANE_MARKER: 'white',
        SemanticType.TRAFFIC_SIGN: 'yellow',
        SemanticType.TRAFFIC_LIGHT: 'red',
        SemanticType.PEDESTRIAN_CROSSING: 'gray',
        SemanticType.PARKING_SPOT: 'green',
        SemanticType.BUILDING: 'darkgray',
        SemanticType.VEGETATION: 'olive',
        SemanticType.DATA_COLLECTION_ZONE: 'blue',
        SemanticType.UNKNOWN: 'black'
    }
    
    for obj in objects:
        color = color_map.get(obj.semantic_type, 'black')
        marker = 'o' if obj.radius > 0 else 's'  # Circle for 3D objects, square for 2D
        
        ax.scatter(obj.position.x, obj.position.y, 
                  c=color, marker=marker, s=max(50, obj.radius*100), 
                  alpha=0.7, label=obj.semantic_type.name)
        
        # Add label
        ax.annotate(obj.label, (obj.position.x, obj.position.y), 
                   xytext=(5, 5), textcoords='offset points', 
                   fontsize=8, alpha=0.8)
    
    ax.set_xlabel('X Position')
    ax.set_ylabel('Y Position')
    ax.set_title('Semantic Map')
    ax.grid(True, alpha=0.3)
    
    # Create legend with unique labels
    handles, labels = ax.get_legend_handles_labels()
    unique_labels = list(set(labels))
    unique_handles = [handles[labels.index(label)] for label in unique_labels]
    ax.legend(unique_handles, unique_labels, loc='upper right')
    
    plt.tight_layout()
    plt.show()

def main():
    """
    Main function demonstrating semantic extraction
    """
    print("Semantic Extractor for Autonomous Driving Data Processing")
    print("=" * 55)
    
    # Create semantic extractor
    extractor = SemanticExtractor()
    
    # 1. Extract from image detections
    print("\n1. Extracting semantic objects from image...")
    image, detections = generate_sample_image_detections()
    image_objects = extractor.extract_from_image(image, detections)
    print(f"   Extracted {len(image_objects)} objects from image")
    
    # Visualize image results
    vis_image = extractor.visualize_semantic_objects(image, image_objects)
    print("   Image visualization created")
    
    # 2. Extract from point cloud
    print("\n2. Extracting semantic objects from point cloud...")
    points, labels = generate_sample_point_cloud()
    point_objects = extractor.extract_from_point_cloud(points, labels)
    print(f"   Extracted {len(point_objects)} objects from point cloud")
    
    # 3. Extract data collection zones from costmap
    print("\n3. Extracting data collection zones from costmap...")
    costmap = generate_sample_costmap()
    zone_objects = extractor.extract_data_collection_zones(costmap)
    print(f"   Extracted {len(zone_objects)} data collection zones")
    
    # Combine all objects
    all_objects = image_objects + point_objects + zone_objects
    print(f"\nTotal semantic objects extracted: {len(all_objects)}")
    
    # Filter by confidence
    high_conf_objects = extractor.filter_objects_by_confidence(all_objects, 0.8)
    print(f"Objects with high confidence (>0.8): {len(high_conf_objects)}")
    
    # Filter by type
    road_objects = extractor.filter_objects_by_type(all_objects, [SemanticType.ROAD])
    print(f"Road objects: {len(road_objects)}")
    
    # Find objects near a point
    center_point = Point3D(0, 0, 0)
    nearby_objects = extractor.get_objects_in_radius(all_objects, center_point, 15.0)
    print(f"Objects within 15m of origin: {len(nearby_objects)}")
    
    # Save to file
    print("\n4. Saving semantic objects to file...")
    extractor.save_semantic_objects(all_objects, 'semantic_objects.json')
    
    # Load from file
    print("\n5. Loading semantic objects from file...")
    loaded_objects = extractor.load_semantic_objects('semantic_objects.json')
    print(f"   Loaded {len(loaded_objects)} objects")
    
    # Visualize semantic map
    print("\n6. Visualizing semantic map...")
    visualize_semantic_map(all_objects)
    
    print("\nSemantic extraction completed successfully!")

if __name__ == "__main__":
    main()