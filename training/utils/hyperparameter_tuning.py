#!/usr/bin/env python3
"""
Hyperparameter tuning script for PPO path planning
Finds optimal hyperparameters to improve model accuracy
"""

import numpy as np
import torch
import yaml
import itertools
import argparse
import json
import os
from pathlib import Path
import sys

# Add parent directory to path to import environment
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from environment import SimplePathPlanningEnv
from planner_rl_train import PPOTrainer, ActorCritic


def train_with_params(config):
    """
    Train a model with given configuration and return performance metrics.
    """
    # Initialize environment
    env = SimplePathPlanningEnv(width=20, height=20)
    
    # Initialize trainer
    trainer = PPOTrainer(config)
    
    # Training metrics
    episode_rewards = []
    episode_lengths = []
    success_count = 0
    
    # Training loop
    for episode in range(config['episodes']):
        # Collect trajectory data
        states = []
        actions = []
        rewards = []
        log_probs = []
        values = []
        dones = []
        
        # Reset environment
        state = env.reset()
        episode_reward = 0
        episode_length = 0
        
        # Collect episode trajectory
        for step in range(config.get('max_steps', 200)):
            # Convert to tensor
            state_tensor = torch.tensor(np.array(state), dtype=torch.float32)
            
            # Get action probabilities and value
            logits, value = trainer.actor_critic(state_tensor)
            # Apply softmax to convert logits to probabilities for action selection
            policy = torch.softmax(logits, dim=-1)
            dist = torch.distributions.Categorical(policy)
            
            # Sample action
            action = dist.sample()
            log_prob = dist.log_prob(action)
            
            # Store data
            states.append(state.copy())
            actions.append(action.item())
            log_probs.append(log_prob.item())
            values.append(value.item())
            
            # Execute action in environment
            next_state, reward, done, info = env.step(action.item())
            state = next_state
            rewards.append(reward)
            dones.append(done)
            
            episode_reward += reward
            episode_length += 1
            
            if done:
                success_count += 1
                break
        
        # Update agent
        if len(states) > 0:
            trainer.update(states, actions, rewards, log_probs, values, dones)
        
        # Record episode metrics
        episode_rewards.append(episode_reward)
        episode_lengths.append(episode_length)
    
    # Calculate final metrics
    avg_reward = np.mean(episode_rewards[-100:]) if len(episode_rewards) >= 100 else np.mean(episode_rewards)
    success_rate = success_count / config['episodes']
    avg_length = np.mean(episode_lengths[-100:]) if len(episode_lengths) >= 100 else np.mean(episode_lengths)
    
    return {
        'avg_reward': float(avg_reward),
        'success_rate': float(success_rate),
        'avg_length': float(avg_length)
    }


def grid_search(base_config, param_grid, trials=3):
    """
    Perform grid search over parameter grid.
    """
    # Generate all parameter combinations
    param_names = list(param_grid.keys())
    param_values = list(param_grid.values())
    combinations = list(itertools.product(*param_values))
    
    best_config = None
    best_performance = -float('inf')
    results = []
    
    print(f"Testing {len(combinations)} parameter combinations...")
    
    for i, combination in enumerate(combinations):
        # Create config with current parameters
        config = base_config.copy()
        for name, value in zip(param_names, combination):
            config[name] = value
            
        print(f"\nTesting combination {i+1}/{len(combinations)}: {dict(zip(param_names, combination))}")
        
        # Run multiple trials for statistical significance
        trial_results = []
        for trial in range(trials):
            print(f"  Trial {trial+1}/{trials}")
            result = train_with_params(config)
            trial_results.append(result)
        
        # Average results across trials
        avg_result = {
            'avg_reward': np.mean([r['avg_reward'] for r in trial_results]),
            'success_rate': np.mean([r['success_rate'] for r in trial_results]),
            'avg_length': np.mean([r['avg_length'] for r in trial_results])
        }
        
        # Use success rate as primary metric, with average reward as tiebreaker
        performance_score = avg_result['success_rate'] * 1000 + avg_result['avg_reward']
        
        results.append({
            'config': dict(zip(param_names, combination)),
            'result': avg_result,
            'performance_score': performance_score
        })
        
        if performance_score > best_performance:
            best_performance = performance_score
            best_config = config.copy()
            print(f"  New best performance: {performance_score:.2f}")
        
        print(f"  Result: {avg_result}")
    
    return best_config, results


def main():
    parser = argparse.ArgumentParser(description='Hyperparameter tuning for PPO path planning')
    parser.add_argument('--output', type=str, default='tuning_results.json',
                        help='Path to save tuning results')
    parser.add_argument('--trials', type=int, default=2,
                        help='Number of trials per parameter combination')
    
    args = parser.parse_args()
    
    # Base configuration
    base_config = {
        'state_dim': 24,  # Updated to match specification
        'action_dim': 4,
        'hidden_dim': 64,
        'learning_rate': 0.0003,
        'gamma': 0.99,
        'lam': 0.95,
        'epsilon': 0.2,
        'epochs': 10,
        'batch_size': 64,
        'episodes': 500,  # Reduced for faster tuning
        'max_steps': 200,
        'entropy_coef': 0.01,
        'log_interval': 100
    }
    
    # Parameter grid for tuning
    param_grid = {
        'learning_rate': [0.0001, 0.0003, 0.001],
        'gamma': [0.9, 0.99, 0.999],
        'hidden_dim': [64, 128],
        'entropy_coef': [0.001, 0.01, 0.1]
    }
    
    print("Starting hyperparameter tuning...")
    best_config, results = grid_search(base_config, param_grid, trials=args.trials)
    
    # Sort results by performance
    results.sort(key=lambda x: x['performance_score'], reverse=True)
    
    # Save results
    output_data = {
        'base_config': base_config,
        'param_grid': param_grid,
        'best_config': best_config,
        'all_results': results
    }
    
    with open(args.output, 'w') as f:
        json.dump(output_data, f, indent=2)
    
    print(f"\nTuning complete!")
    print(f"Best configuration: {best_config}")
    print(f"Best performance score: {results[0]['performance_score']:.2f}")
    print(f"Results saved to {args.output}")
    
    # Show top 5 configurations
    print("\nTop 5 configurations:")
    for i, result in enumerate(results[:5]):
        print(f"{i+1}. Score: {result['performance_score']:.2f}")
        print(f"   Config: {result['config']}")
        print(f"   Result: {result['result']}")


if __name__ == "__main__":
    main()