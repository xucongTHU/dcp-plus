#!/usr/bin/env python3
"""
Reinforcement Learning Training Script for Navigation Planner
Implements PPO algorithm for path planning optimization
"""

import numpy as np
import torch
import torch.nn as nn
import torch.optim as optim
from torch.distributions import Categorical
import yaml
import argparse
import os
import logging

# Configure logging
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

class ActorCritic(nn.Module):
    def __init__(self, state_dim, action_dim, hidden_dim=64):
        super(ActorCritic, self).__init__()
        
        # Shared layers
        self.shared_layers = nn.Sequential(
            nn.Linear(state_dim, hidden_dim),
            nn.ReLU(),
            nn.Linear(hidden_dim, hidden_dim),
            nn.ReLU()
        )
        
        # Actor head (policy)
        self.actor_head = nn.Sequential(
            nn.Linear(hidden_dim, hidden_dim),
            nn.ReLU(),
            nn.Linear(hidden_dim, action_dim),
            nn.Softmax(dim=-1)
        )
        
        # Critic head (value)
        self.critic_head = nn.Sequential(
            nn.Linear(hidden_dim, hidden_dim),
            nn.ReLU(),
            nn.Linear(hidden_dim, 1)
        )
        
    def forward(self, state):
        shared_features = self.shared_layers(state)
        policy = self.actor_head(shared_features)
        value = self.critic_head(shared_features)
        return policy, value

class PPOTrainer:
    def __init__(self, config):
        self.config = config
        self.state_dim = config['state_dim']
        self.action_dim = config['action_dim']
        self.hidden_dim = config['hidden_dim']
        
        # Initialize networks
        self.actor_critic = ActorCritic(self.state_dim, self.action_dim, self.hidden_dim)
        self.optimizer = optim.Adam(self.actor_critic.parameters(), lr=config['learning_rate'])
        
        # Hyperparameters
        self.gamma = config['gamma']  # discount factor
        self.lam = config['lam']      # GAE lambda
        self.epsilon = config['epsilon']  # PPO clip parameter
        self.epochs = config['epochs']
        self.batch_size = config['batch_size']
        
    def compute_gae(self, rewards, values, dones):
        """
        Compute Generalized Advantage Estimation (GAE)
        """
        advantages = []
        gae = 0
        
        # Add a dummy value for next state
        values = values + [0]
        
        for i in reversed(range(len(rewards))):
            if dones[i]:
                delta = rewards[i] - values[i]
                gae = delta
            else:
                delta = rewards[i] + self.gamma * values[i+1] - values[i]
                gae = delta + self.gamma * self.lam * gae
                
            advantages.insert(0, gae)
            
        return advantages
    
    def update(self, states, actions, rewards, old_log_probs, values, dones):
        """
        Perform PPO update
        """
        # Compute advantages
        advantages = self.compute_gae(rewards, values, dones)
        advantages = torch.tensor(advantages, dtype=torch.float32)
        
        # Normalize advantages
        advantages = (advantages - advantages.mean()) / (advantages.std() + 1e-8)
        
        # Convert to tensors
        states = torch.tensor(states, dtype=torch.float32)
        actions = torch.tensor(actions, dtype=torch.int64)
        old_log_probs = torch.tensor(old_log_probs, dtype=torch.float32)
        values = torch.tensor(values, dtype=torch.float32)
        returns = advantages + values
        
        # PPO update
        for _ in range(self.epochs):
            # Sample mini-batches
            indices = np.random.permutation(len(states))
            
            for start in range(0, len(states), self.batch_size):
                end = start + self.batch_size
                batch_indices = indices[start:end]
                
                # Get batch data
                batch_states = states[batch_indices]
                batch_actions = actions[batch_indices]
                batch_old_log_probs = old_log_probs[batch_indices]
                batch_advantages = advantages[batch_indices]
                batch_returns = returns[batch_indices]
                
                # Forward pass
                policy, value = self.actor_critic(batch_states)
                dist = Categorical(policy)
                entropy = dist.entropy().mean()
                
                # New log probabilities
                new_log_probs = dist.log_prob(batch_actions)
                
                # Ratio
                ratio = torch.exp(new_log_probs - batch_old_log_probs)
                
                # Surrogate losses
                surr1 = ratio * batch_advantages
                surr2 = torch.clamp(ratio, 1 - self.epsilon, 1 + self.epsilon) * batch_advantages
                actor_loss = -torch.min(surr1, surr2).mean()
                
                # Critic loss
                critic_loss = nn.MSELoss()(value.squeeze(), batch_returns)
                
                # Total loss
                loss = actor_loss + 0.5 * critic_loss - 0.01 * entropy
                
                # Update
                self.optimizer.zero_grad()
                loss.backward()
                self.optimizer.step()
                
        logger.info(f"PPO update completed. Actor Loss: {actor_loss.item():.4f}, "
                   f"Critic Loss: {critic_loss.item():.4f}")

def load_training_config(config_path):
    """
    Load training configuration from YAML file
    """
    with open(config_path, 'r') as f:
        config = yaml.safe_load(f)
    return config

def main():
    parser = argparse.ArgumentParser(description='Train PPO agent for navigation planning')
    parser.add_argument('--config', type=str, default='config/ppo_train_config.yaml',
                        help='Path to training configuration file')
    parser.add_argument('--weights-path', type=str, default='models/ppo_weights.pth',
                        help='Path to save trained weights')
    parser.add_argument('--episodes', type=int, default=1000,
                        help='Number of training episodes')
    
    args = parser.parse_args()
    
    # Load configuration
    config = load_training_config(args.config)
    
    # Initialize trainer
    trainer = PPOTrainer(config)
    
    logger.info("Starting PPO training...")
    
    # Training loop (simplified)
    for episode in range(args.episodes):
        # In a real implementation, this would interact with a simulator or environment
        # Collect trajectory data
        states = []
        actions = []
        rewards = []
        log_probs = []
        values = []
        dones = []
        
        # Simulate a simple trajectory collection
        state = np.array([0.0, 0.0])  # Starting position
        goal = np.array([10.0, 10.0])  # Goal position
        
        for step in range(50):  # Max steps per episode
            # Convert to tensor
            state_tensor = torch.tensor(state, dtype=torch.float32)
            
            # Get action probabilities and value
            policy, value = trainer.actor_critic(state_tensor)
            dist = Categorical(policy)
            
            # Sample action
            action = dist.sample()
            log_prob = dist.log_prob(action)
            
            # Store data
            states.append(state.copy())
            actions.append(action.item())
            log_probs.append(log_prob.item())
            values.append(value.item())
            
            # Simulate environment step (simple grid movement)
            if action.item() == 0:  # Right
                state[0] += 1
            elif action.item() == 1:  # Up
                state[1] += 1
            elif action.item() == 2:  # Left
                state[0] -= 1
            elif action.item() == 3:  # Down
                state[1] -= 1
                
            # Compute reward (negative distance to goal)
            distance = np.linalg.norm(state - goal)
            reward = -distance
            rewards.append(reward)
            
            # Check if done
            done = distance < 0.5 or step == 49
            dones.append(done)
            
            if done:
                break
        
        # Update agent
        if len(states) > 0:
            trainer.update(states, actions, rewards, log_probs, values, dones)
        
        if episode % 100 == 0:
            avg_reward = np.mean(rewards)
            logger.info(f"Episode {episode}, Average Reward: {avg_reward:.2f}")
    
    # Save weights
    torch.save(trainer.actor_critic.state_dict(), args.weights_path)
    logger.info(f"Model weights saved to {args.weights_path}")

if __name__ == "__main__":
    main()