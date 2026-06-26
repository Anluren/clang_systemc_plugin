

config.plugin_obj_root = "/home/dzheng/opensource/clang_systemc_plugin/build_llvm19"
config.clang_binary = "/usr/lib/llvm-19/bin/clang++"
config.plugin_path = "/home/dzheng/opensource/clang_systemc_plugin/build_llvm19/libScIntAssignChecker.so"
config.systemc_include_dir = "/home/dzheng/opensource/systemc/install/include"
config.filecheck_binary = "/home/dzheng/.local/bin/filecheck"

lit_config.load_config(config, "/home/dzheng/opensource/clang_systemc_plugin/test/lit/lit.cfg.py")
