# Use a lightweight Ubuntu base image
FROM ubuntu:22.04

# Set environment variables to prevent interactive prompts during apt installs
ENV DEBIAN_FRONTEND=noninteractive

# Update package lists and install necessary tools:
# git: for cloning repositories
# build-essential: includes g++ and make, essential for C/C++ compilation
# ca-certificates: often needed for HTTPS connections (e.g., for git clone)
RUN apt-get update && \
    apt-get install -y \
    git \
    build-essential \
    ca-certificates && \
    rm -rf /var/lib/apt/lists/*

# Verify installations (optional, but good for debugging Dockerfile)
RUN git --version && g++ --version && make --version

# Set the working directory for subsequent instructions.
# GitLab CI will clone your repository into its own working directory,
# so this WORKDIR is primarily for any manual commands you might run inside the container.
# The actual build will happen in CI_PROJECT_DIR.
WORKDIR /app

# Default command if running the container directly (optional)
# CMD ["bash"]
