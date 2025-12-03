#!/usr/bin/env python3
"""
Benchmark script for comparing different path planning algorithms
"""

import numpy as np
import torch
import argparse
import logging
import json
from pathlib import Path
import matplotlib.pyplot as plt

from environment import SimplePathPlanningEnv, PathPlanningEnvironment
from planner_rl_train import ActorCritic

# Configure logging
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)


class BenchmarkSuite:
    """
    Suite for benchmarking path planning algorithms
    """
    
    def __init__(self, env_type='simple'):
        self.env_type = env_type
        if env_type == 'simple':
            self.env_factory = lambda: SimplePathPlanningEnv(width=20, height=20)
        else:
            self.env_factory = lambda: PathPlanningEnvironment(width=20, height=20, obstacle_ratio=0.2)
    
    def benchmark_algorithm(self, model_path, config_path, num_episodes=100):
        """
        Benchmark a trained algorithm
        """
        # Load configuration
        import yaml
        with open(config_path, 'r') as f:
            config = yaml.safe_load(f)
        
        # Initialize environment
        env = self.env_factory()
        
        # Initialize model
        model = ActorCritic(
            state_dim=config['state_dim'],
            action_dim=config['action_dim'],
            hidden_dim=config['hidden_dim'],
            num_layers=config.get('network_layers', 2),
            dropout_rate=config.get('dropout_rate', 0.0)
        )
        
        # Load trained weights
        model.load_state_dict(torch.load(model_path, map_location=torch.device('cpu')))
        model.eval()
        
        # Metrics
        total_rewards = []
        episode_lengths = []
        success_count = 0
        
        logger.info(f"Running benchmark with {num_episodes} episodes...")
        
        # Run benchmark
        for episode in range(num_episodes):
            state = env.reset()
            total_reward = 0
            steps = 0
            done = False
            
            while not done and steps < config.get('max_steps', 200):
                # Select action greedily
                with torch.no_grad():
                    state_tensor = torch.tensor(state, dtype=torch.float32)
                    policy, _ = model(state_tensor)
                    action = torch.argmax(policy).item()
                
                state, reward, done, _ = env.step(action)
                total_reward += reward
                steps += 1
            
            total_rewards.append(total_reward)
            episode_lengths.append(steps)
            
            if done:
                success_count += 1
            
            if episode % (num_episodes // 10) == 0 and episode > 0:
                logger.info(f"Benchmark progress: {episode}/{num_episodes} episodes")
        
        # Calculate metrics
        metrics = {
            'mean_reward': float(np.mean(total_rewards)),
            'std_reward': float(np.std(total_rewards)),
            'mean_length': float(np.mean(episode_lengths)),
            'std_length': float(np.std(episode_lengths)),
            'success_rate': float(success_count / num_episodes),
            'total_episodes': num_episodes
        }
        
        return metrics
    
    def compare_algorithms(self, models_configs, num_episodes=100):
        """
        Compare multiple algorithms
        """
        results = {}
        
        for name, (model_path, config_path) in models_configs.items():
            logger.info(f"Benchmarking {name}...")
            metrics = self.benchmark_algorithm(model_path, config_path, num_episodes)
            results[name] = metrics
            logger.info(f"{name} results: {metrics}")
        
        return results
    
    def plot_comparison(self, results, save_path=None):
        """
        Plot comparison results
        """
        algorithms = list(results.keys())
        metrics = ['mean_reward', 'mean_length', 'success_rate']
        
        fig, axes = plt.subplots(1, 3, figsize=(15, 5))
        fig.suptitle('Algorithm Comparison')
        
        for i, metric in enumerate(metrics):
            values = [results[alg][metric] for alg in algorithms]
            axes[i].bar(algorithms, values)
            axes[i].set_title(metric)
            axes[i].set_ylabel(metric)
            plt.setp(axes[i].get_xticklabels(), rotation=45, ha="right")
        
        plt.tight_layout()
        
        if save_path:
            plt.savefig(save_path, dpi=150, bbox_inches='tight')
            logger.info(f"Comparison plot saved to {save_path}")
        
        plt.show()


def main():
    parser = argparse.ArgumentParser(description='Benchmark path planning algorithms')
    parser.add_argument('--model-path', type=str, required=True,
                        help='Path to trained model weights')
    parser.add_argument('--config-path', type=str, default='config/advanced_ppo_config.yaml',
                        help='Path to model configuration')
    parser.add_argument('--episodes', type=int, default=100,
                        help='Number of episodes to run')
    parser.add_argument('--env-type', type=str, default='simple',
                        choices=['simple', 'complex'],
                        help='Type of environment for benchmarking')
    parser.add_argument('--output', type=str,
                        help='Path to save benchmark results')
    
    args = parser.parse_args()
    
    # Run benchmark
    benchmark_suite = BenchmarkSuite(env_type=args.env_type)
    metrics = benchmark_suite.benchmark_algorithm(
        args.model_path, args.config_path, args.episodes
    )
    
    # Print results
    print("\n=== BENCHMARK RESULTS ===")
    for key, value in metrics.items():
        print(f"{key}: {value}")
    
    # Save results
    if args.output:
        with open(args.output, 'w') as f:
            json.dump(metrics, f, indent=2)
        logger.info(f"Results saved to {args.output}")


if __name__ == "__main__":
    main()