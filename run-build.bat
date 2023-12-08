cd dependencies/glslang
python update_glslang_sources.py
cd ../..
cmake -Bbuild .
cmake --build build