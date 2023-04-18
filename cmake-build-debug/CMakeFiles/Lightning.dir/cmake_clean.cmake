file(REMOVE_RECURSE
  "lib/libLightning.a"
  "lib/libLightning.pdb"
)

# Per-language clean rules from dependency scanning.
foreach(lang )
  include(CMakeFiles/Lightning.dir/cmake_clean_${lang}.cmake OPTIONAL)
endforeach()
