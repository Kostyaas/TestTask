file(REMOVE_RECURSE
  "libLogger.dylib"
  "libLogger.pdb"
)

# Per-language clean rules from dependency scanning.
foreach(lang CXX)
  include(CMakeFiles/Logger_shared.dir/cmake_clean_${lang}.cmake OPTIONAL)
endforeach()
