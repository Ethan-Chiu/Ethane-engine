#!/bin/sh
python Setup.py "$@"

# Source VulkanSDK setup if available
if [ -d "$HOME/VulkanSDK" ]; then
    # Find the latest version directory
    VULKAN_VERSION=$(ls -1 "$HOME/VulkanSDK" | sort -V | tail -n 1)
    if [ -n "$VULKAN_VERSION" ] && [ -f "$HOME/VulkanSDK/$VULKAN_VERSION/setup-env.sh" ]; then
        echo "Sourcing VulkanSDK $VULKAN_VERSION"
        echo "run source \"$HOME/VulkanSDK/$VULKAN_VERSION/setup-env.sh\""
    else
        echo "Warning: VulkanSDK directory exists but no valid version found"
    fi
else
    echo "Warning: VulkanSDK not found in home directory"
fi

# bear -- make
