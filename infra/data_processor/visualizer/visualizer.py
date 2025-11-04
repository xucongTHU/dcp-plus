# visualizer.py
"""
Visualization Module for Autonomous Driving Data Processing

This module provides visualization capabilities for the data processing pipeline,
including costmaps, semantic maps, paths, and data collection metrics.
"""

import numpy as np
import matplotlib.pyplot as plt
import matplotlib.patches as patches
import cv2
from typing import List, Tuple, Dict, Any, Optional
import json
from matplotlib.colors import LinearSegmentedColormap
from matplotlib.animation import FuncAnimation
import seaborn as sns

# Import data structures from other modules
# In a real implementation, these would be imported from their respective modules
class Point:
    def __init__(self, x: float = 0.0, y: float = 0.0):
        self.x = x
        self.y = y
    
    def __repr__(self):
        return f"Point({self.x}, {self.y})"

class SemanticType:
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

class SemanticObject:
    def __init__(self, semantic_type: int, position: Point, radius: float = 0.0, 
                 label: str = "", confidence: float = 0.0):
        self.semantic_type = semantic_type
        self.position = position
        self.radius = radius
        self.label = label
        self.confidence = confidence

class Path:
    def __init__(self, waypoints: List[Point] = None):
        self.waypoints = waypoints if waypoints is not None else []

class CostMap:
    def __init__(self, width: int, height: int, resolution: float):
        self.width = width
        self.height = height
        self.resolution = resolution
        self.costs = np.zeros((height, width))
        self.densities = np.zeros((height, width))

class Visualizer:
    """
    Main visualization class for the data processing pipeline
    """
    def __init__(self):
        # Define color schemes
        self.costmap_cmap = LinearSegmentedColormap.from_list(
            "costmap", ["green", "yellow", "red"], N=256
        )
        
        self.density_cmap = LinearSegmentedColormap.from_list(
            "density", ["blue", "cyan", "yellow", "red"], N=256
        )
        
        # Semantic object colors
        self.semantic_colors = {
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
        
        # Semantic labels
        self.semantic_labels = {
            SemanticType.ROAD: 'Road',
            SemanticType.LANE_MARKER: 'Lane Marker',
            SemanticType.TRAFFIC_SIGN: 'Traffic Sign',
            SemanticType.TRAFFIC_LIGHT: 'Traffic Light',
            SemanticType.PEDESTRIAN_CROSSING: 'Crosswalk',
            SemanticType.PARKING_SPOT: 'Parking Spot',
            SemanticType.BUILDING: 'Building',
            SemanticType.VEGETATION: 'Vegetation',
            SemanticType.DATA_COLLECTION_ZONE: 'Data Zone',
            SemanticType.UNKNOWN: 'Unknown'
        }
    
    def visualize_costmap(self, costmap: CostMap, title: str = "Costmap Visualization", 
                         show_densities: bool = False) -> plt.Figure:
        """
        Visualize costmap with optional density overlay
        
        Args:
            costmap: CostMap object to visualize
            title: Title for the plot
            show_densities: Whether to show data density information
        
        Returns:
            matplotlib figure object
        """
        fig, ax = plt.subplots(1, 1, figsize=(10, 8))
        
        if show_densities:
            # Show density map
            im = ax.imshow(costmap.densities, cmap=self.density_cmap, 
                          origin='lower', extent=[0, costmap.width * costmap.resolution,
                                                 0, costmap.height * costmap.resolution])
            cbar = plt.colorbar(im, ax=ax)
            cbar.set_label('Data Density')
        else:
            # Show cost map
            im = ax.imshow(costmap.costs, cmap=self.costmap_cmap, 
                          origin='lower', extent=[0, costmap.width * costmap.resolution,
                                                 0, costmap.height * costmap.resolution])
            cbar = plt.colorbar(im, ax=ax)
            cbar.set_label('Cost Value')
        
        ax.set_xlabel('X (meters)')
        ax.set_ylabel('Y (meters)')
        ax.set_title(title)
        ax.grid(True, alpha=0.3)
        
        return fig
    
    def visualize_semantic_map(self, semantic_objects: List[SemanticObject], 
                              map_size: Tuple[float, float] = (100.0, 100.0),
                              title: str = "Semantic Map") -> plt.Figure:
        """
        Visualize semantic objects on a 2D map
        
        Args:
            semantic_objects: List of SemanticObject instances
            map_size: Size of the map in meters (width, height)
            title: Title for the plot
        
        Returns:
            matplotlib figure object
        """
        fig, ax = plt.subplots(1, 1, figsize=(12, 10))
        
        # Plot semantic objects
        for obj in semantic_objects:
            color = self.semantic_colors.get(obj.semantic_type, 'black')
            label = self.semantic_labels.get(obj.semantic_type, 'Unknown')
            
            if obj.radius > 0:
                # Draw circle for objects with radius
                circle = patches.Circle((obj.position.x, obj.position.y), obj.radius,
                                      facecolor=color, alpha=0.6, edgecolor='black', linewidth=0.5)
                ax.add_patch(circle)
            else:
                # Draw point for objects without radius
                ax.scatter(obj.position.x, obj.position.y, c=color, s=50, alpha=0.8)
            
            # Add label
            ax.annotate(label, (obj.position.x, obj.position.y), 
                       xytext=(5, 5), textcoords='offset points',
                       fontsize=8, alpha=0.7)
        
        ax.set_xlim(0, map_size[0])
        ax.set_ylim(0, map_size[1])
        ax.set_xlabel('X (meters)')
        ax.set_ylabel('Y (meters)')
        ax.set_title(title)
        ax.grid(True, alpha=0.3)
        ax.set_aspect('equal')
        
        # Create legend
        unique_types = list(set([obj.semantic_type for obj in semantic_objects]))
        legend_elements = [patches.Patch(color=self.semantic_colors.get(t, 'black'), 
                                       label=self.semantic_labels.get(t, 'Unknown'))
                          for t in unique_types]
        ax.legend(handles=legend_elements, loc='upper right')
        
        return fig
    
    def visualize_path(self, path: Path, costmap: Optional[CostMap] = None,
                      semantic_objects: Optional[List[SemanticObject]] = None,
                      title: str = "Path Visualization") -> plt.Figure:
        """
        Visualize a path with optional costmap and semantic objects background
        
        Args:
            path: Path object to visualize
            costmap: Optional costmap to show as background
            semantic_objects: Optional semantic objects to show
            title: Title for the plot
        
        Returns:
            matplotlib figure object
        """
        fig, ax = plt.subplots(1, 1, figsize=(12, 10))
        
        # Show costmap background if provided
        if costmap is not None:
            im = ax.imshow(costmap.costs, cmap=self.costmap_cmap,
                          origin='lower', extent=[0, costmap.width * costmap.resolution,
                                                 0, costmap.height * costmap.resolution],
                          alpha=0.7)
            cbar = plt.colorbar(im, ax=ax)
            cbar.set_label('Cost Value')
        
        # Show semantic objects if provided
        if semantic_objects is not None:
            for obj in semantic_objects:
                color = self.semantic_colors.get(obj.semantic_type, 'black')
                if obj.radius > 0:
                    circle = patches.Circle((obj.position.x, obj.position.y), obj.radius,
                                          facecolor=color, alpha=0.4, edgecolor='black', linewidth=0.3)
                    ax.add_patch(circle)
                else:
                    ax.scatter(obj.position.x, obj.position.y, c=color, s=30, alpha=0.6)
        
        # Plot path
        if path.waypoints:
            x_coords = [p.x for p in path.waypoints]
            y_coords = [p.y for p in path.waypoints]
            
            # Plot path line
            ax.plot(x_coords, y_coords, 'b-', linewidth=2, alpha=0.8, label='Planned Path')
            
            # Plot waypoints
            ax.scatter(x_coords, y_coords, c='blue', s=20, alpha=0.9)
            
            # Mark start and end points
            ax.scatter(x_coords[0], y_coords[0], c='green', s=100, marker='o', 
                      label='Start', edgecolors='black', linewidth=1)
            ax.scatter(x_coords[-1], y_coords[-1], c='red', s=100, marker='s', 
                      label='Goal', edgecolors='black', linewidth=1)
        
        ax.set_xlabel('X (meters)')
        ax.set_ylabel('Y (meters)')
        ax.set_title(title)
        ax.grid(True, alpha=0.3)
        ax.legend()
        ax.set_aspect('equal')
        
        return fig
    
    def visualize_coverage_metrics(self, coverage_data: Dict[str, List[float]],
                                 title: str = "Coverage Metrics") -> plt.Figure:
        """
        Visualize coverage metrics over time
        
        Args:
            coverage_data: Dictionary with metric names as keys and lists of values
            title: Title for the plot
        
        Returns:
            matplotlib figure object
        """
        fig, ax = plt.subplots(1, 1, figsize=(12, 8))
        
        # Plot each metric
        for metric_name, values in coverage_data.items():
            epochs = list(range(len(values)))
            ax.plot(epochs, values, marker='o', linewidth=2, markersize=6, label=metric_name)
        
        ax.set_xlabel('Epoch/Mission')
        ax.set_ylabel('Coverage Ratio')
        ax.set_title(title)
        ax.legend()
        ax.grid(True, alpha=0.3)
        ax.set_ylim(0, 1.05)
        
        return fig
    
    def visualize_data_density_heatmap(self, density_data: np.ndarray,
                                     title: str = "Data Density Heatmap") -> plt.Figure:
        """
        Visualize data density as a heatmap
        
        Args:
            density_data: 2D numpy array with density values
            title: Title for the plot
        
        Returns:
            matplotlib figure object
        """
        fig, ax = plt.subplots(1, 1, figsize=(10, 8))
        
        # Create heatmap
        im = ax.imshow(density_data, cmap=self.density_cmap, origin='lower')
        cbar = plt.colorbar(im, ax=ax)
        cbar.set_label('Data Density')
        
        ax.set_xlabel('X Grid')
        ax.set_ylabel('Y Grid')
        ax.set_title(title)
        
        return fig
    
    def create_dashboard(self, costmap: CostMap, 
                        semantic_objects: List[SemanticObject],
                        path: Path,
                        coverage_metrics: Dict[str, List[float]]) -> plt.Figure:
        """
        Create a comprehensive dashboard with multiple visualizations
        
        Args:
            costmap: CostMap object
            semantic_objects: List of SemanticObject instances
            path: Path object
            coverage_metrics: Coverage metrics dictionary
        
        Returns:
            matplotlib figure object
        """
        fig = plt.figure(figsize=(16, 12))
        
        # Costmap subplot
        ax1 = plt.subplot(2, 2, 1)
        im1 = ax1.imshow(costmap.costs, cmap=self.costmap_cmap, origin='lower')
        plt.colorbar(im1, ax=ax1)
        ax1.set_title('Costmap')
        ax1.set_xlabel('X')
        ax1.set_ylabel('Y')
        
        # Semantic map subplot
        ax2 = plt.subplot(2, 2, 2)
        for obj in semantic_objects:
            color = self.semantic_colors.get(obj.semantic_type, 'black')
            if obj.radius > 0:
                circle = patches.Circle((obj.position.x, obj.position.y), obj.radius,
                                      facecolor=color, alpha=0.6)
                ax2.add_patch(circle)
            else:
                ax2.scatter(obj.position.x, obj.position.y, c=color, s=30)
        ax2.set_xlim(0, 100)
        ax2.set_ylim(0, 100)
        ax2.set_title('Semantic Map')
        ax2.set_xlabel('X (meters)')
        ax2.set_ylabel('Y (meters)')
        ax2.set_aspect('equal')
        
        # Path visualization subplot
        ax3 = plt.subplot(2, 2, 3)
        if path.waypoints:
            x_coords = [p.x for p in path.waypoints]
            y_coords = [p.y for p in path.waypoints]
            ax3.plot(x_coords, y_coords, 'b-', linewidth=2)
            ax3.scatter(x_coords, y_coords, c='blue', s=20)
            ax3.scatter(x_coords[0], y_coords[0], c='green', s=100, marker='o')
            ax3.scatter(x_coords[-1], y_coords[-1], c='red', s=100, marker='s')
        ax3.set_title('Planned Path')
        ax3.set_xlabel('X (meters)')
        ax3.set_ylabel('Y (meters)')
        ax3.set_aspect('equal')
        
        # Coverage metrics subplot
        ax4 = plt.subplot(2, 2, 4)
        for metric_name, values in coverage_metrics.items():
            epochs = list(range(len(values)))
            ax4.plot(epochs, values, marker='o', linewidth=2, label=metric_name)
        ax4.set_xlabel('Epoch')
        ax4.set_ylabel('Coverage Ratio')
        ax4.set_title('Coverage Metrics')
        ax4.legend()
        ax4.grid(True, alpha=0.3)
        ax4.set_ylim(0, 1.05)
        
        plt.tight_layout()
        return fig
    
    def save_figure(self, fig: plt.Figure, filepath: str, dpi: int = 300):
        """
        Save figure to file
        
        Args:
            fig: matplotlib figure object
            filepath: Path to save the figure
            dpi: Resolution in dots per inch
        """
        try:
            fig.savefig(filepath, dpi=dpi, bbox_inches='tight')
            print(f"Figure saved to {filepath}")
        except Exception as e:
            print(f"Error saving figure to {filepath}: {e}")
    
    def show_figure(self, fig: plt.Figure):
        """
        Display figure
        
        Args:
            fig: matplotlib figure object
        """
        plt.show()

def generate_sample_data():
    """
    Generate sample data for visualization testing
    """
    np.random.seed(42)
    
    # Generate sample costmap
    costmap = CostMap(100, 100, 1.0)
    costmap.costs = np.random.rand(100, 100) * 100
    costmap.densities = np.random.rand(100, 100)
    
    # Add some high-cost obstacles
    costmap.costs[30:40, 30:40] = 200
    costmap.costs[70:80, 70:80] = 150
    
    # Generate sample semantic objects
    semantic_objects = []
    
    # Add some buildings
    for i in range(5):
        x = np.random.uniform(10, 90)
        y = np.random.uniform(10, 90)
        radius = np.random.uniform(2, 5)
        obj = SemanticObject(SemanticType.BUILDING, Point(x, y), radius, "Building")
        semantic_objects.append(obj)
    
    # Add some data collection zones
    for i in range(3):
        x = np.random.uniform(20, 80)
        y = np.random.uniform(20, 80)
        radius = np.random.uniform(3, 6)
        obj = SemanticObject(SemanticType.DATA_COLLECTION_ZONE, Point(x, y), radius, "Data Zone")
        semantic_objects.append(obj)
    
    # Add some traffic signs
    for i in range(8):
        x = np.random.uniform(5, 95)
        y = np.random.uniform(5, 95)
        obj = SemanticObject(SemanticType.TRAFFIC_SIGN, Point(x, y), 0, "Traffic Sign")
        semantic_objects.append(obj)
    
    # Generate sample path
    waypoints = []
    for i in range(20):
        x = 10 + i * 4
        y = 50 + 10 * np.sin(i * 0.5)
        waypoints.append(Point(x, y))
    path = Path(waypoints)
    
    # Generate sample coverage metrics
    epochs = 20
    coverage_metrics = {
        'Overall Coverage': [0.1 + 0.03 * i + np.random.normal(0, 0.02) for i in range(epochs)],
        'Sparse Coverage': [0.05 + 0.04 * i + np.random.normal(0, 0.03) for i in range(epochs)],
        'Efficiency': [0.8 - 0.01 * i + np.random.normal(0, 0.05) for i in range(epochs)]
    }
    
    # Ensure values are within [0, 1]
    for metric_name in coverage_metrics:
        coverage_metrics[metric_name] = [max(0, min(1, val)) for val in coverage_metrics[metric_name]]
    
    return costmap, semantic_objects, path, coverage_metrics

def main():
    """
    Main function demonstrating visualization capabilities
    """
    print("Data Processor Visualizer for Autonomous Driving")
    print("=" * 45)
    
    # Create visualizer
    visualizer = Visualizer()
    
    # Generate sample data
    print("Generating sample data...")
    costmap, semantic_objects, path, coverage_metrics = generate_sample_data()
    
    print(f"Generated {len(semantic_objects)} semantic objects")
    print(f"Generated path with {len(path.waypoints)} waypoints")
    print(f"Generated coverage metrics for {len(coverage_metrics['Overall Coverage'])} epochs")
    
    # 1. Visualize costmap
    print("\n1. Visualizing costmap...")
    costmap_fig = visualizer.visualize_costmap(costmap, "Sample Costmap")
    visualizer.save_figure(costmap_fig, "sample_costmap.png")
    
    # 2. Visualize costmap with densities
    print("Visualizing costmap with densities...")
    density_fig = visualizer.visualize_costmap(costmap, "Sample Density Map", show_densities=True)
    visualizer.save_figure(density_fig, "sample_density_map.png")
    
    # 3. Visualize semantic map
    print("\n2. Visualizing semantic map...")
    semantic_fig = visualizer.visualize_semantic_map(semantic_objects, (100, 100), "Sample Semantic Map")
    visualizer.save_figure(semantic_fig, "sample_semantic_map.png")
    
    # 4. Visualize path
    print("\n3. Visualizing path...")
    path_fig = visualizer.visualize_path(path, costmap, semantic_objects, "Sample Path with Context")
    visualizer.save_figure(path_fig, "sample_path.png")
    
    # 5. Visualize coverage metrics
    print("\n4. Visualizing coverage metrics...")
    coverage_fig = visualizer.visualize_coverage_metrics(coverage_metrics, "Sample Coverage Metrics")
    visualizer.save_figure(coverage_fig, "sample_coverage_metrics.png")
    
    # 6. Visualize data density heatmap
    print("\n5. Visualizing data density heatmap...")
    heatmap_fig = visualizer.visualize_data_density_heatmap(costmap.densities, "Sample Data Density Heatmap")
    visualizer.save_figure(heatmap_fig, "sample_density_heatmap.png")
    
    # 7. Create dashboard
    print("\n6. Creating dashboard...")
    dashboard_fig = visualizer.create_dashboard(costmap, semantic_objects, path, coverage_metrics)
    visualizer.save_figure(dashboard_fig, "sample_dashboard.png")
    
    # Show one figure as example
    print("\nDisplaying semantic map...")
    visualizer.show_figure(semantic_fig)
    
    print("\nVisualization demonstration completed successfully!")
    print("All figures have been saved to the current directory.")

if __name__ == "__main__":
    main()