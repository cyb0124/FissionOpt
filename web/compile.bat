em++ --bind -s MODULARIZE=1 -s EXPORT_NAME=FissionOpt -o FissionOpt.js -std=c++17 -flto -O2 Bindings.cpp ../Fission.cpp ../OptMeta.cpp -I../../xtl/include -I../../xtensor/include
