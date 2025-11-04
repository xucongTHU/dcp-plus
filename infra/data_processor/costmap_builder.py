#!/usr/bin/env python3
# costmap_builder.py

import numpy as np
import yaml
import json
import os
from typing import List, Tuple, Dict, Any
import matplotlib.pyplot as plt
from scipy.ndimage import gaussian_filter

class Point:
    """
    Represents a 2D point in space
    """
    def __init__(self, x: float = 0.0, y: float = 0.0):
        self.x = x
        self.y = y
    
    def __repr__(self):
        return f"Point({self.x}, {self.y})"

class Cell:
    """
    Represents a cell in the costmap grid
    """
    def __init__(self, x: int = 0, y: int = 0, cost: float = 0.0, data_density: float = 0.0):
        self.x = x
        self.y = y
        self.cost = cost
        self.data_density = data_density
    
    def __repr__(self):
        return f"Cell({self.x}, {self.y}, cost={self.cost}, density={self.data_density})"

class CostMap:
    """
    CostMap implementation in Python that mirrors the C++ version
    """
    def __init__(self, width: int, height: int, resolution: float):
        self.width = width
        self.height = height
        self.resolution = resolution
        self.sparse_threshold = 0.2
        self.exploration_bonus = 0.5
        self.redundancy_penalty = 0.4
        
        # Initialize cells
        self.cells = []
        for y in range(height):
            row = []
            for x in range(width):
                row.append(Cell(x, y, 0.0, 0.0))
            self.cells.append(row)
    
    def set_parameters(self, sparse_threshold: float, exploration_bonus: float, redundancy_penalty: float):
        """
        Set costmap parameters
        """
        self.sparse_threshold = sparse_threshold
        self.exploration_bonus = exploration_bonus
        self.redundancy_penalty = redundancy_penalty
    
    def update_with_data_statistics(self, data_points: List[Point]):
        """
        Update costmap with collected data statistics
        """
        # Reset data density
        for y in range(self.height):
            for x in range(self.width):
                self.cells[y][x].data_density = 0.0
        
        # Calculate density based on data points
        for point in data_points:
            cell_x = int(point.x / self.resolution)
            cell_y = int(point.y / self.resolution)
            
            if 0 <= cell_x < self.width and 0 <= cell_y < self.height:
                self.cells[cell_y][cell_x].data_density += 1.0
        
        # Normalize densities
        max_density = 0.0
        for y in range(self.height):
            for x in range(self.width):
                max_density = max(max_density, self.cells[y][x].data_density)
        
        if max_density > 0.0:
            for y in range(self.height):
                for x in range(self.width):
                    self.cells[y][x].data_density /= max_density
    
    def adjust_costs_based_on_density(self):
        """
        Adjust costs based on data density to encourage exploration of sparse areas
        """
        for y in range(self.height):
            for x in range(self.width):
                # Adjust cost based on data density
                if self.cells[y][x].data_density < self.sparse_threshold:
                    # Decrease cost for sparse areas (encourage exploration)
                    self.cells[y][x].cost -= self.exploration_bonus
                else:
                    # Increase cost for dense areas (discourage redundancy)
                    self.cells[y][x].cost += self.redundancy_penalty
    
    def get_data_density(self, x: int, y: int) -> float:
        """
        Get data density at a specific cell
        """
        if 0 <= x < self.width and 0 <= y < self.height:
            return self.cells[y][x].data_density
        return 0.0
    
    def set_cell_cost(self, x: int, y: int, cost: float):
        """
        Set cost for a specific cell
        """
        if 0 <= x < self.width and 0 <= y < self.height:
            self.cells[y][x].cost = cost
    
    def get_cell_cost(self, x: int, y: int) -> float:
        """
        Get cost for a specific cell
        """
        if 0 <= x < self.width and 0 <= y < self.height:
            return self.cells[y][x].cost
        return 0.0
    
    def is_valid_cell(self, x: int, y: int) -> bool:
        """
        Check if cell coordinates are valid
        """
        return 0 <= x < self.width and 0 <= y < self.height

class DataStats:
    """
    Data statistics for costmap building
    """
    def __init__(self, width: int, height: int):
        self.width = width
        self.height = height
        self.density_map = [[0.0 for _ in range(width)] for _ in range(height)]
    
    def data_density(self, position: Point) -> float:
        """
        Get data density at a position
        """
        x = int(position.x)
        y = int(position.y)
        
        if 0 <= x < self.width and 0 <= y < self.height:
            return self.density_map[y][x]
        return 0.0

class MapData:
    """
    Map data container
    """
    def __init__(self, width: int, height: int, resolution: float):
        self.costmap = CostMap(width, height, resolution)
    
    def to_costmap(self) -> CostMap:
        """
        Get costmap
        """
        return self.costmap

class CostMapBuilder:
    """
    Main class for building costmaps from collected data
    """
    def __init__(self, config_file: str = "/workspaces/ad_data_closed_loop/infra/navigation_planner/config/planner_weights.yaml"):
        self.config_file = config_file
        self.parameters = {}
        self.load_configuration()
    
    def load_configuration(self):
        """
        Load configuration from YAML file
        """
        try:
            with open(self.config_file, 'r') as f:
                self.parameters = yaml.safe_load(f)
            print(f"Loaded configuration from {self.config_file}")
        except Exception as e:
            print(f"Failed to load configuration: {e}")
            # Use default parameters
            self.parameters = {
                'sparse_threshold': 0.2,
                'exploration_bonus': 0.5,
                'redundancy_penalty': 0.4,
                'grid_resolution': 1.0
            }
    
    def build_costmap_from_data(self, data_points: List[Point], width: int = 100, height: int = 100) -> CostMap:
        """
        Build costmap from collected data points
        """
        resolution = self.parameters.get('grid_resolution', 1.0)
        costmap = CostMap(width, height, resolution)
        
        # Set parameters from config
        costmap.set_parameters(
            self.parameters.get('sparse_threshold', 0.2),
            self.parameters.get('exploration_bonus', 0.5),
            self.parameters.get('redundancy_penalty', 0.4)
        )
        
        # Update with data statistics
        costmap.update_with_data_statistics(data_points)
        
        # Adjust costs based on density
        costmap.adjust_costs_based_on_density()
        
        return costmap
    
    def detect_sparse_regions(self, costmap: CostMap) -> List[Tuple[int, int]]:
        """
        Detect sparse regions in the costmap
        """
        sparse_regions = []
        threshold = self.parameters.get('sparse_threshold', 0.2)
        
        for y in range(costmap.height):
            for x in range(costmap.width):
                if costmap.get_data_density(x, y) < threshold:
                    sparse_regions.append((x, y))
        
        return sparse_regions
    
    def generate_density_heatmap(self, data_points: List[Point], width: int = 100, height: int = 100) -> np.ndarray:
        """
        Generate a density heatmap from data points
        """
        heatmap = np.zeros((height, width))
        
        for point in data_points:
            x = int(point.x)
            y = int(point.y)
            
            if 0 <= x < width and 0 <= y < height:
                heatmap[y, x] += 1
        
        # Apply Gaussian filter to smooth the heatmap
        heatmap = gaussian_filter(heatmap, sigma=1.0)
        
        return heatmap
    
    def save_costmap_to_file(self, costmap: CostMap, filename: str):
        """
        Save costmap to a JSON file
        """
        data = {
            'width': costmap.width,
            'height': costmap.height,
            'resolution': costmap.resolution,
            'sparse_threshold': costmap.sparse_threshold,
            'exploration_bonus': costmap.exploration_bonus,
            'redundancy_penalty': costmap.redundancy_penalty,
            'cells': []
        }
        
        for y in range(costmap.height):
            for x in range(costmap.width):
                cell = costmap.cells[y][x]
                data['cells'].append({
                    'x': cell.x,
                    'y': cell.y,
                    'cost': cell.cost,
                    'data_density': cell.data_density
                })
        
        with open(filename, 'w') as f:
            json.dump(data, f, indent=2)
        
        print(f"Costmap saved to {filename}")
    
    def visualize_costmap(self, costmap: CostMap, title: str = "Costmap Visualization"):
        """
        Visualize the costmap using matplotlib
        """
        # Create cost matrix
        cost_matrix = np.zeros((costmap.height, costmap.width))
        density_matrix = np.zeros((costmap.height, costmap.width))
        
        for y in range(costmap.height):
            for x in range(costmap.width):
                cost_matrix[y, x] = costmap.get_cell_cost(x, y)
                density_matrix[y, x] = costmap.get_data_density(x, y)
        
        # Plot
        fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(12, 5))
        
        # Plot costmap
        im1 = ax1.imshow(cost_matrix, cmap='viridis', origin='lower')
        ax1.set_title(f'{title} - Costs')
        ax1.set_xlabel('X')
        ax1.set_ylabel('Y')
        plt.colorbar(im1, ax=ax1)
        
        # Plot density map
        im2 = ax2.imshow(density_matrix, cmap='plasma', origin='lower')
        ax2.set_title(f'{title} - Data Density')
        ax2.set_xlabel('X')
        ax2.set_ylabel('Y')
        plt.colorbar(im2, ax=ax2)
        
        plt.tight_layout()
        plt.show()
    
    def update_planner_weights(self, data_points: List[Point], output_file: str = None):
        """
        Update planner weights based on collected data
        """
        if output_file is None:
            output_file = self.config_file
        
        # Build costmap
        costmap = self.build_costmap_from_data(data_points)
        
        # Detect sparse regions
        sparse_regions = self.detect_sparse_regions(costmap)
        
        # Adjust weights based on coverage
        sparse_ratio = len(sparse_regions) / (costmap.width * costmap.height)
        
        # Update parameters
        if sparse_ratio > 0.5:  # More than 50% sparse
            self.parameters['exploration_bonus'] = min(1.0, self.parameters.get('exploration_bonus', 0.5) * 1.2)
        elif sparse_ratio < 0.2:  # Less than 20% sparse
            self.parameters['exploration_bonus'] = max(0.1, self.parameters.get('exploration_bonus', 0.5) * 0.8)
        
        # Save updated parameters
        try:
            with open(output_file, 'w') as f:
                yaml.dump(self.parameters, f, default_flow_style=False)
            print(f"Updated planner weights saved to {output_file}")
        except Exception as e:
            print(f"Failed to save updated weights: {e}")

def main():
    """
    Main function demonstrating the costmap builder
    """
    # Create costmap builder
    builder = CostMapBuilder()
    
    # Generate some sample data points
    np.random.seed(42)
    data_points = []
    
    # Add some dense clusters
    for _ in range(100):
        x = np.random.normal(30, 5)
        y = np.random.normal(30, 5)
        data_points.append(Point(x, y))
    
    # Add some sparse points
    for _ in range(50):
        x = np.random.uniform(0, 100)
        y = np.random.uniform(0, 100)
        data_points.append(Point(x, y))
    
    print(f"Generated {len(data_points)} sample data points")
    
    # Build costmap
    costmap = builder.build_costmap_from_data(data_points, 100, 100)
    
    # Detect sparse regions
    sparse_regions = builder.detect_sparse_regions(costmap)
    print(f"Detected {len(sparse_regions)} sparse regions")
    
    # Save costmap
    builder.save_costmap_to_file(costmap, 'output_costmap.json')
    
    # Visualize costmap
    builder.visualize_costmap(costmap, "Sample Costmap")
    
    # Update planner weights
    builder.update_planner_weights(data_points, 'updated_planner_weights.yaml')
    
    print("Costmap building completed successfully")

if __name__ == "__main__":
    main()