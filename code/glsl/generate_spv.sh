#!/bin/bash
## -V: create SPIR-V binary
## -vn: save binary output as a uint32_t array named <name> in dedicated header file
## -o: output file
glslangValidator -V -o mapdraw_frag.h -S frag --vn mapdraw_frag_spv mapdraw_fp.glsl
glslangValidator -V -o mapdraw_vert.h -S vert --vn mapdraw_vert_spv mapdraw_vp.glsl


## -x: save binary output as text-based 32-bit hexadecimal numbers
