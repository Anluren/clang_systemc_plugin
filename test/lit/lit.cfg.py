# pyright: reportUndefinedVariable=false, reportAttributeAccessIssue=false
# pylint: skip-file

import os
from types import SimpleNamespace

import lit.formats

# lit injects `config` when executing this file; define a fallback object so
# static analyzers do not flag unresolved names.
config = globals().get("config")
if config is None:
  config = SimpleNamespace(
      substitutions=[],
      plugin_obj_root="",
      clang_binary="clang++",
      plugin_path="",
      systemc_include_dir="",
      filecheck_binary="filecheck",
  )

config.name = "ScIntAssignCheckerLit"
config.test_format = lit.formats.ShTest(execute_external=True)
config.suffixes = [".cpp"]
config.excludes = ["lit.cfg.py", "lit.site.cfg.py"]

config.test_source_root = os.path.dirname(__file__)
config.test_exec_root = os.path.join(config.plugin_obj_root, "test", "lit")

config.substitutions.append(("%clang", config.clang_binary))
config.substitutions.append(("%plugin", config.plugin_path))
config.substitutions.append(("%systemc_inc", config.systemc_include_dir))
config.substitutions.append(("%filecheck", config.filecheck_binary))
