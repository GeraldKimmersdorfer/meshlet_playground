# Attribute Compression for meshlets
The repository for my bachelor thesis about attribute compression for skinned meshlets.

## Setup
 - ```git clone https://github.com/GeraldKimmersdorfer/compressed_meshlet_skinning && cd compressed_meshlet_skinning```
 - ```git submodule update --init --recursive --remote```
 - ```cmake -S meshoptimizer/ -B meshoptimizer/build```

### Further Setup Notes:
 - Make sure that auto_vk_toolkit is set on development branch
 - Make sure that the working directory of the compressed_meshlet_skinning project is set to *$(OutputPath)*