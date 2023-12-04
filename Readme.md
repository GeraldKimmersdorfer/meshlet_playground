# Attribute Compression for meshlets
The repository for my bachelor thesis about attribute compression for skinned meshlets. It is a work in progress but at some point will contain state-of-the-art compression methods for vertex attribute compression.

## Setup
 - ```git clone https://github.com/GeraldKimmersdorfer/compressed_meshlet_skinning && cd compressed_meshlet_skinning```
 - ```git submodule update --init --recursive --remote```
 - ```cmake -S meshoptimizer/ -B meshoptimizer/build```

### Further Setup Notes:
 - Make sure that the working directory of the compressed_meshlet_skinning project is set to *$(OutputPath)* (for all configurations!!)